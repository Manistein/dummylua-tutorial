#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastate.h"
#include "../vm/luafunc.h"
#include "../common/luamem.h"
#include "../common/luastring.h"
#include "../vm/luado.h"
#include "luacode.h"
#include "../vm/luaopcodes.h"
#include "../common/luatable.h"

static void suffixedexp(struct lua_State* L, LexState* ls, FuncState* fs, expdesc* e);
static void expr(FuncState* fs, expdesc* e);
static void checknext(struct lua_State* L, LexState* ls, int token);
static int check(struct lua_State* L, LexState* ls, int token);
static void yindex(struct lua_State* L, FuncState* fs, expdesc* e);
static int testnext(LexState* ls, int token);

#define eqstr(a, b) ((a) == (b))
#define vkisvar(v) (v->k >= VLOCAL && v->k <= VINDEXED)
#define check_condition(ls, c, s) if(!(c)) luaX_syntaxerror(ls->L, ls, s)
#define hasmulret(v) ((v)->k == VCALL)

static void init_exp(expdesc* e, expkind k, int i) {
	e->k = k;
	e->u.info = i;
	e->t = NO_JUMP;
	e->f = NO_JUMP;
}

static int newupvalues(FuncState* fs, expdesc* e, TString* n) {
	struct lua_State* L = fs->ls->L;
	Proto* p = fs->p;
	luaM_growvector(L, p->upvalues, fs->nups, p->sizeupvalues, Upvaldesc, MAXUPVAL);
	
	for (int i = fs->nups; i < p->sizeupvalues; i++) {
		p->upvalues[i].idx = 0;
		p->upvalues[i].in_stack = 0;
		p->upvalues[i].name = NULL;
	}

	p->upvalues[fs->nups].idx = e->u.info;
	p->upvalues[fs->nups].in_stack = e->k == VLOCAL;
	p->upvalues[fs->nups].name = n;

	return fs->nups++;
}

static void open_func(LexState* ls, FuncState* fs) {
	fs->firstlocal = 0;
	fs->freereg = 0;
	fs->ls = ls;
	fs->nactvars = 0;
	fs->nlocvars = 0;
	fs->nk = 0;
	fs->np = 0;
	fs->nups = 0;
	fs->prev = ls->fs;
	fs->jpc = NO_JUMP;
	ls->fs = fs;
	ls->lookahead.token = TK_EOS;
	
	fs->pc = 0;
}

static bool test_token(struct lua_State* L, LexState* ls, int token) {
	if (ls->t.token != token) {
		return false;
	}
	return true;
}

static LocVar* getlocvar(FuncState* fs, int n) {
	LexState* ls = fs->ls;
	int idx = ls->dyd->actvar.arr[fs->firstlocal + n];
	lua_assert(idx < fs->nlocvars);
	return &fs->p->locvars[idx];
}

// search local var for current function
static int searchvar(FuncState* fs, TString* name) {
	for (int i = fs->nactvars - 1; i >= 0; i--) {
		if eqstr(getlocvar(fs, i)->varname, name) {
			return i;
		}
	}

	return -1;
}

static int searchupvalues(FuncState* fs, expdesc* e, TString* n) {
	Proto* p = fs->p;
	for (int i = 0; i < fs->nups; i++) {
		if (eqstr(p->upvalues[i].name, n)) {
			init_exp(e, VUPVAL, i);
			return i;
		}
	}

	return -1;
}

static int singlevaraux(FuncState* fs, expdesc* e, TString* n) {
	// can not find n in local variables, try to find it in global
	if (fs == NULL) {
		init_exp(e, VVOID, 0);
		return -1;
	}

	int reg = searchvar(fs, n);
	if (reg >= 0) {
		init_exp(e, VLOCAL, reg);
	}
	else { 
		// can not find it in local variables, then try upvalues
		reg = searchupvalues(fs, e, n);

		// try to find in parent function
		if (reg < 0) {
			singlevaraux(fs->prev, e, n);
			if (e->k != VVOID) {
				reg = newupvalues(fs, e, n);
				init_exp(e, VUPVAL, reg);
			}
		}
	}
	return reg;
}

static void codestring(FuncState* fs, TString* n, expdesc* e) {
	init_exp(e, VK, luaK_stringK(fs, n));
}

// First, search local variable, if it is not exist, then it will try to search it's upvalues
// if it also not exist, then try to search it in _ENV
static void singlevar(FuncState* fs, expdesc* e, TString* n) {
	singlevaraux(fs, e, n);
	if (e->k == VVOID) { // variable is in global
		expdesc k;
		singlevaraux(fs, e, fs->ls->env);
		lua_assert(e->k == VUPVAL);
		codestring(fs, n, &k);
		luaK_indexed(fs, e, &k);
	}
}

static void primaryexp(struct lua_State* L, LexState* ls, FuncState* fs, expdesc* e) {
	switch (ls->t.token) {
	case TK_NAME: {
		singlevar(fs, e, ls->t.seminfo.s);
	} break;
	case '(': {
		luaX_next(L, ls);
		expr(fs, e);
		luaK_exp2nextreg(fs, e);
		check(L, ls, ')');
	} break;
	default: {
		luaX_syntaxerror(L, ls, "unsupport syntax");
	} break;
	}
}

// This structure is used for recording information in 
// brace
struct ConsControl {
	expdesc v;		// element in the table
	expdesc* t;     // represent the table 
	int na;			// total elements in array
	int nh;			// total elements in hash table
	int tostore;    // elements are ready to store
};

static void listfield(FuncState* fs, struct ConsControl* cc) {
	expr(fs, &cc->v);

	cc->na++;
	cc->tostore++;
}

static void closelistfield(FuncState* fs, struct ConsControl* cc) {
	if (cc->v.k == VVOID) return;
	luaK_exp2nextreg(fs, &cc->v);
	cc->v.k = VVOID;

	if (cc->tostore == LFIELD_PER_FLUSH) {
		luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);
		cc->tostore = 0;
	}
}

static void recfield(FuncState* fs, struct ConsControl* cc) {
	expdesc k, v;
	init_exp(&k, VVOID, 0);
	init_exp(&v, VVOID, 0);

	int reg = fs->freereg;
	switch (fs->ls->t.token) {
	case TK_NAME: {
		init_exp(&k, VK, luaK_stringK(fs, fs->ls->t.seminfo.s));
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case '[': {
		yindex(fs->ls->L, fs, &k);
	} break;
	default: luaX_syntaxerror(fs->ls->L, fs->ls, "unknow token for recfield");
	}

	checknext(fs->ls->L, fs->ls, '=');
	int rk = luaK_exp2RK(fs, &k);
	expr(fs, &v);
	luaK_codeABC(fs, OP_SETTABLE, cc->t->u.info, rk, luaK_exp2RK(fs, &v));

	fs->freereg = reg;
	cc->nh++;
}

static void field(FuncState* fs, struct ConsControl* cc) {
	switch (fs->ls->t.token) {
	case TK_NAME: {
		if (luaX_lookahead(fs->ls->L, fs->ls) != '=') {
			listfield(fs, cc);
		}
		else {
			recfield(fs, cc);
		}
	} break;
	case '[': {
		recfield(fs, cc);
	} break;
	default: listfield(fs, cc); break;
	}
}

static void lastlistfield(FuncState* fs, struct ConsControl* cc) {
	if (cc->tostore == 0) 
		return;

	if (hasmulret(&cc->v)) {
		cc->na--;
		luaK_setlist(fs, cc->t->u.info, LUA_MULRET, cc->na);
	}
	else {
		if (cc->v.k != VVOID) luaK_exp2nextreg(fs, &cc->v);
		luaK_setlist(fs, cc->t->u.info, cc->tostore, cc->na);
	}
}

static void constructor(struct lua_State* L, FuncState* fs, expdesc* t) {
	struct ConsControl cc;
	cc.na = cc.nh = cc.tostore = 0;
	cc.t = t;
	init_exp(&cc.v, VVOID, 0);
	int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
	init_exp(cc.t, VRELOCATE, pc);
	luaK_exp2nextreg(fs, cc.t);

	checknext(L, fs->ls, '{');
	do {
		lua_assert(cc.v.k == VVOID || cc.tostore > 0);
		if (fs->ls->t.token == '}') break;
		closelistfield(fs, &cc);
		field(fs, &cc);
	} while (testnext(fs->ls, ',') || testnext(fs->ls, ';'));
	checknext(L, fs->ls, '}');
	lastlistfield(fs, &cc);

	SET_ARG_B(fs->p->code[pc], cc.na);
	SET_ARG_C(fs->p->code[pc], cc.nh);
}

static void simpleexp(FuncState* fs, expdesc* e) {
	LexState* ls = fs->ls;
	switch (ls->t.token) {
	case TK_STRING: {
		codestring(fs, ls->t.seminfo.s, e);
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case TK_INT: {
		e->k = VINT;
		e->u.i = ls->t.seminfo.i;
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case TK_FLOAT: {
		e->k = VFLT;
		e->u.r = ls->t.seminfo.r;
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case TK_TRUE: {
		init_exp(e, VTRUE, 0);
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case TK_FALSE: {
		init_exp(e, VFALSE, 0);
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case TK_NIL: {
		init_exp(e, VNIL, 0);
		luaX_next(fs->ls->L, fs->ls);
	} break;
	case '{': {
		constructor(fs->ls->L, fs, e);
	} break;
	case TK_FUNCTION:
		luaX_syntaxerror(ls->L, ls, "unsupport syntax");
		break;
	default:
		suffixedexp(fs->ls->L, fs->ls, fs, e);
		break;
	}
}

static UnOpr getunopr(LexState* ls) {
	switch (ls->t.token) {
	case '-': return UNOPR_MINUS;
	case TK_NOT: return UNOPR_NOT;
	case '#': return UNOPR_LEN;
	case '~': return UNOPR_BNOT;
	default: return NOUNOPR;
	}
}

static BinOpr getbinopr(LexState* ls) {
	switch (ls->t.token) {
	case '+': return BINOPR_ADD;
	case '-': return BINOPR_SUB;
	case '*': return BINOPR_MUL;
	case '/': return BINOPR_DIV;
	case TK_MOD: return BINOPR_MOD;
	case '^': return BINOPR_POW;
	case '&': return BINOPR_BAND;
	case '|': return BINOPR_BOR;
	case '~': return BINOPR_BXOR;
	case TK_SHL: return BINOPR_SHL;
	case TK_SHR: return BINOPR_SHR;
	case TK_CONCAT: return BINOPR_CONCAT;
	case '>': return BINOPR_GREATER;
	case '<': return BINOPR_LESS;
	case TK_GREATEREQUAL: return BINOPR_GREATEQ;
	case TK_LESSEQUAL: return BINOPR_LESSEQ;
	case TK_EQUAL: return BINOPR_EQ;
	case TK_NOTEQUAL: return BINOPR_NOTEQ;
	case TK_AND: return BINOPR_AND;
	case TK_OR: return BINOPR_OR;
	default:return NOBINOPR;
	}
}

static const struct {
	lu_byte left;   // left priority for each binary operator
	lu_byte right;  // right priority
} priority[] = {
	{10,10}, {10,10},		   // '+' and '-'
	{11,11}, {11,11}, {11,11}, {11, 11}, // '*', '/', '//' and '%'
	{14,13},				   // '^' right associative
	{6,6}, {4,4}, {5,5},	   // '&', '|' and '~'
	{7,7}, {7,7},			   // '<<' and '>>'
	{9,8},					   // '..' right associative
	{3,3}, {3,3}, {3,3}, {3,3}, {3,3}, {3,3}, // '>', '<', '>=', '<=', '==', '~=',
	{2,2}, {1,1},			   // 'and' and 'or'
};

static int subexpr(FuncState* fs, expdesc* e, int limit) {
	LexState* ls = fs->ls;
	int unopr = getunopr(ls);

	if (unopr != NOUNOPR) {
		luaX_next(fs->ls->L, fs->ls);
		subexpr(fs, e, UNOPR_PRIORITY);
		luaK_prefix(fs, unopr, e);
	}
	else simpleexp(fs, e);

	int binopr = getbinopr(ls);
	while (binopr != NOBINOPR && priority[binopr].left > limit) {
		expdesc e2;
		init_exp(&e2, VVOID, 0);

		luaX_next(ls->L, ls);
		luaK_infix(fs, binopr, e);
		int nextop = subexpr(fs, &e2, priority[binopr].right);
		luaK_posfix(fs, binopr, e, &e2);

		binopr = nextop;
	}

	return binopr;
}

static void expr(FuncState* fs, expdesc* e) {
	subexpr(fs, e, 0);
}

static void checknext(struct lua_State* L, LexState* ls, int token) {
	if (!test_token(L, ls, token)) {
		luaX_syntaxerror(L, ls, "unsupport syntax");
	}

	luaX_next(L, ls);
}

static int testnext(LexState* ls, int token) {
	if (!test_token(ls->L, ls, token))
		return 0;
	luaX_next(ls->L, ls);
	return 1;
}

static int explist(FuncState* fs, expdesc* e) {
	int var = 1;
	expr(fs, e);

	for (;;) {
		if (test_token(fs->ls->L, fs->ls, ',')) {
			luaX_next(fs->ls->L, fs->ls);
			luaK_exp2nextreg(fs, e);

			expr(fs, e);
			var++;
		}
		else {
			break;
		}
	}

	return var;
}

static void funcargs(FuncState* fs, expdesc* e) {
	luaX_next(fs->ls->L, fs->ls);

	expdesc args;
	int narg = 0;
	if (fs->ls->t.token == ')') {
		init_exp(&args, VVOID, 0);
	}
	else {
		narg = explist(fs, &args);
	}

	if (args.k != VVOID) {
		luaK_exp2nextreg(fs, &args);
	}

	lua_assert(e->k == VNONRELOC);
	int base = e->u.info;
	int nparams = fs->freereg - (base + 1);
	init_exp(e, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 1));
	fs->freereg = base + 1;

	checknext(fs->ls->L, fs->ls, ')');
}

static int check(struct lua_State* L, LexState* ls, int token) {
	if (ls->t.token != token) {
		luaX_syntaxerror(L, ls, "token is not match");
	}

	return 1;
}

static void checkname(struct lua_State* L, LexState* ls, expdesc* e) {
	check(L, ls, TK_NAME);
	codestring(ls->fs, ls->t.seminfo.s, e);
	luaX_next(L, ls);
}

static void fieldsel(struct lua_State* L, LexState* ls, FuncState* fs, expdesc* e) {
	luaX_next(L, ls);
	luaK_exp2anyregup(fs, e);

	check(L, ls, TK_NAME);

	expdesc k;
	codestring(fs, ls->t.seminfo.s, &k);

	luaK_indexed(fs, e, &k);
	luaX_next(L, ls);
}

static void yindex(struct lua_State* L, FuncState* fs, expdesc* e) {
	luaX_next(L, fs->ls);
	
	if (fs->ls->t.token == TK_NAME) {
		luaX_syntaxerror(L, fs->ls, "yindex:there is a TK_NAME in square brackets");
	}

	expr(fs, e);
	luaK_exp2val(fs, e);
	checknext(L, fs->ls, ']');
}

static void suffixedexp(struct lua_State* L, LexState* ls, FuncState* fs, expdesc* e) {
	primaryexp(L, ls, fs, e);
	luaX_next(L, ls);

	for (;;) {
		switch (ls->t.token) {
		case '.': {
			fieldsel(L, ls, fs, e);
		} break;
		case ':': {
			expdesc key;
			luaX_next(L, ls);
			checkname(L, ls, &key);
			luaK_self(fs, e, &key);
			funcargs(fs, e);
		} break;
		case '[': {
			luaK_exp2anyregup(fs, e);
			expdesc key;
			yindex(L, fs, &key);
			luaK_indexed(fs, e, &key);
		} break;
		case '(': {
			luaK_exp2nextreg(fs, e);
			funcargs(fs, e);
		} break;
		default: return;
		}
	}
}

static void adjust_assign(FuncState* fs, int nvars, int nexps, expdesc* e) {
	int extra = nvars - nexps;

	if (e->k == VCALL) {
		extra++;
		if (extra < 0) extra = 0;
		luaK_setreturns(fs, e, extra);
		if (extra > 1) luaK_reserveregs(fs, extra - 1);
	}
	else {
		if (e->k != VVOID) luaK_exp2nextreg(fs, e);
		if (extra > 0) {
			luaK_reserveregs(fs, extra);
			luaK_nil(fs, fs->freereg - extra, fs->freereg - 1);
		}
	}

	if (nexps > nvars) {
		fs->freereg -= nexps - nvars;
	}
}

static void assignment(FuncState* fs, LH_assign* v, int nvars) {
	expdesc* var = &v->v;
	check_condition(fs->ls, vkisvar(var), "not var");

	LH_assign lh;
	lh.prev = v;

	expdesc e;
	init_exp(&e, VVOID, 0);
	if (fs->ls->t.token == ',') {
		luaX_next(fs->ls->L, fs->ls);
		suffixedexp(fs->ls->L, fs->ls, fs, &lh.v);

		assignment(fs, &lh, nvars + 1);
	}
	else if (fs->ls->t.token == '=') {
		luaX_next(fs->ls->L, fs->ls);
		int nexps = explist(fs, &e);
		adjust_assign(fs, nvars, nexps, &e);
	}
	else {
		luaX_syntaxerror(fs->ls->L, fs->ls, "syntax error");
	}

	init_exp(&e, VNONRELOC, fs->freereg - 1);
	luaK_storevar(fs, &v->v, &e);
}

static void exprstat(struct lua_State* L, LexState* ls, FuncState* fs) {
	LH_assign lh;
	suffixedexp(L, ls, fs, &lh.v);
	
	if (ls->t.token == '=' || ls->t.token == ',') {
		assignment(fs, &lh, 1);
	}
	else {
		check_condition(ls, lh.v.k == VCALL, "exp type error");
	}
}

static void statement(struct lua_State* L, LexState* ls, FuncState* fs) {
	switch (ls->t.token) {
	case TK_NAME: {
		exprstat(L, ls, fs);
	} break;
	default:
		luaX_syntaxerror(L, ls, "unsupport syntax");
		break;
	}
}

static bool block_follow(struct lua_State* L, LexState* ls) {
	switch (ls->t.token) {
	case TK_EOS: case TK_END: {
		return true;
	} break;
	default: return false;
	}

	return false;
}

static void statlist(struct lua_State* L, LexState* ls, FuncState* fs) {
	while (!block_follow(L, ls)) {
		statement(L, ls, fs);
	}
}

static void close_func(struct lua_State* L, FuncState* fs) {
	luaK_ret(fs, 0, 0);
}

static void mainfunc(struct lua_State* L, LexState* ls, FuncState* fs) {
	expdesc e;
	init_exp(&e, VLOCAL, 0);

	open_func(ls, fs);
	newupvalues(fs, &e, fs->ls->env);

	luaX_next(L, ls);
	statlist(L, ls, fs);
	close_func(L, fs);
}

static void test_lexer(struct lua_State* L, LexState* ls) {
	int token = luaX_next(L, ls);
	while (token != TK_EOS) {
		if (token <= TK_FUNCTION && token >= FIRST_REVERSED) {
			printf("REVERSED: %s \n", luaX_tokens[token - FIRST_REVERSED]);
		}
		else {
			switch (token)
			{
			case TK_STRING: {
				printf("TK_STRING %s \n", getstr(ls->t.seminfo.s));
			} break;
			case TK_NAME: {
				printf("TK_NAME %s \n", getstr(ls->t.seminfo.s));
			} break;
			case TK_FLOAT: {
				printf("TK_FLOAT %f \n", (float)ls->t.seminfo.r);
			} break;
			case TK_INT: {
				printf("TK_INT %d \n", (int)ls->t.seminfo.i);
			} break;
			case TK_NOTEQUAL: {
				printf("TK_NOEQUAL ~= \n");
			} break;
			case TK_EQUAL: {
				printf("TK_EQUAL == \n");
			} break;
			case TK_GREATEREQUAL: {
				printf("TK_GREATEREQUAL >= \n");
			} break;
			case TK_LESSEQUAL: {
				printf("TK_LESSEQUAL <= \n");
			} break;
			case TK_SHL: {
				printf("TK_SHL << \n");
			} break;
			case TK_SHR: {
				printf("TK_SHR >> \n");
			} break;
			case TK_MOD: {
				printf("TK_MOD %% \n");
			} break;
			case TK_DOT: {
				printf("TK_DOT . \n");
			} break;
			case TK_CONCAT: {
				printf("TK_CONCAT .. \n");
			} break;
			case TK_VARARG: {
				printf("TK_VARARG ... \n");
			} break;
			default:{
				printf("%c \n", ls->t.token);
			} break;
			}
		}
		token = luaX_next(L, ls);
	}
	printf("total linenumber = %d", ls->linenumber);
}

LClosure* luaY_parser(struct lua_State* L, Zio* zio, MBuffer* buffer, Dyndata* dyd, const char* name) {
	FuncState fs;
	LexState ls;
	luaX_setinput(L, &ls, zio, buffer, dyd, luaS_newliteral(L, name), luaS_newliteral(L, LUA_ENV));
	ls.current = zget(ls.zio);
	
	LClosure* closure = luaF_newLclosure(L, 1);
	closure->p = fs.p = luaF_newproto(L);

	setlclvalue(L->top, closure);
	increase_top(L);
	ptrdiff_t save_top = savestack(L, L->top);

	ls.h = luaH_new(L);
	setgco(L->top, obj2gco(ls.h));
	increase_top(L);

	mainfunc(L, &ls, &fs);
	L->top = restorestack(L, save_top);

	return closure;
}
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

static void suffixexp(struct lua_State* L, LexState* ls, FuncState* fs, expdesc* e);

#define eqstr(a, b) ((a) == (b))

static void init_exp(expdesc* e, expkind k, int i) {
	e->k = k;
	e->u.info = i;
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
	ls->fs = fs;
	
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
	default: {
		luaX_syntaxerror(L, ls, "unsupport syntax");
	} break;
	}
}

static void expr(FuncState* fs, expdesc* e) {
	LexState* ls = fs->ls;
	switch (ls->t.token) {
	case TK_NAME: {
		suffixexp(fs->ls->L, fs->ls, fs, e);
	} break;
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
	default:
		luaX_syntaxerror(ls->L, ls, "unsupport syntax");
		break;
	}
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
		else if (test_token(fs->ls->L, fs->ls, ')')) {
			break;
		}
		else {
			luaX_syntaxerror(fs->ls->L, fs->ls, "unsupport syntax");
		}
	}

	return var;
}

static void funcargs(FuncState* fs, expdesc* e) {
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
	init_exp(e, VCALL, luaK_codeABC(fs, OP_CALL, base, narg + 1, 1));
	fs->freereg = base + 1;

	if (!test_token(fs->ls->L, fs->ls, ')')) {
		luaX_syntaxerror(fs->ls->L, fs->ls, "unsupport syntax");
	}

	luaX_next(fs->ls->L, fs->ls);
}

static void suffixexp(struct lua_State* L, LexState* ls, FuncState* fs, expdesc* e) {
	primaryexp(L, ls, fs, e);
	luaX_next(L, ls);
	switch (ls->t.token) {
	case '(': {
		luaX_next(L, ls);

		luaK_exp2nextreg(fs, e);
		funcargs(fs, e);
	} break;
	case ',': break;
	default: 
		luaX_syntaxerror(L, ls, "unsupport syntax");
		break;
	}
}

static void exprstat(struct lua_State* L, LexState* ls, FuncState* fs) {
	expdesc e;
	suffixexp(L, ls, fs, &e);
}

static void statement(struct lua_State* L, LexState* ls, FuncState* fs) {
	switch (ls->t.token)
	{
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
			printf("%s \n", luaX_tokens[token - FIRST_REVERSED]);
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
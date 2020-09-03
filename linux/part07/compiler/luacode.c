#include "luacode.h"
#include "../common/luaobject.h"
#include "../common/luastate.h"
#include "../vm/luagc.h"
#include "../common/luatable.h"
#include "../vm/luavm.h"
#include "../common/luamem.h"
#include "../vm/luaopcodes.h"
#include "../common/luastring.h"
#include "../vm/luado.h"

#define MAININDEX 255
#define hasjump(e) (e->t != e->f)
#define testTMode(t) (luaP_opmodes[GET_OPCODE(t)] & (1 << 7))

static void freereg(FuncState* fs, int reg);
static void freeexp(FuncState* fs, expdesc* e);
static void discharge2anyreg(FuncState* fs, expdesc* e);
static int exp2reg(FuncState* fs, expdesc* e, int reg);
static void patchlistaux(FuncState* fs, int list, int dtarget, int reg, int vtarget);

static int addk(FuncState* fs, TValue* v) {
	LexState* ls = fs->ls;
	Proto* p = fs->p;
	TValue* idx = luaH_set(ls->L, ls->h, v);
	if (!ttisnil(idx)) {
		int k = (int)idx->value_.i;
		if (k < fs->nk && luaV_eqobject(ls->L, &p->k[k], v)) {
			return k;
		}
	}

	int k = fs->nk;
	luaM_growvector(ls->L, p->k, fs->nk, p->sizek, TValue, INT_MAX);
	setobj(&p->k[k], v);
	setivalue(idx, k);

	fs->nk++;
	return k;
}

static int tonumeral(expdesc* e, TValue* v) {
	int ret = 0;

	switch (e->k) {
	case VINT: {
		if (v) {
			v->tt_ = LUA_NUMINT;
			v->value_.i = e->u.i;
		}
		ret = 1;
	} break;
	case VFLT: {
		if (v) {
			v->tt_ = LUA_NUMFLT;
			v->value_.n = e->u.r;
		}
		ret = 1;
	} break;
	default:break;
	}

	return ret;
}

static int validop(FuncState* fs, const TValue* v1, const TValue* v2, int op) {
	switch (op) {
	case LUA_OPT_BAND: case LUA_OPT_BOR: case LUA_OPT_BXOR: case LUA_OPT_BNOT:
	case LUA_OPT_IDIV: case LUA_OPT_SHL: case LUA_OPT_SHR: case LUA_OPT_MOD: {
		if (!ttisinteger(v1) || !ttisinteger(v2))
			return 0;

		if (op == LUA_OPT_IDIV && v2->value_.i == 0) {
			return 0;
		}
	} return 1;
	case LUA_OPT_UMN: case LUA_OPT_DIV: case LUA_OPT_ADD: case LUA_OPT_SUB: 
	case LUA_OPT_MUL: case LUA_OPT_POW: {
		lua_State* L = fs->ls->L;
		lua_Number n1, n2;
		if (!luaV_tonumber(L, v1, &n1) || !luaV_tonumber(L, v2, &n2)) {
			return 0;
		}

		if (op == LUA_OPT_DIV && n2 == 0.0f) {
			return 0;
		}
	} return 1;
	default:return 1;
	}
}

static int constfolding(FuncState* fs, int op, expdesc* e1, expdesc* e2) {
	TValue v1, v2;
	if (!(tonumeral(e1, &v1) && tonumeral(e2, &v2) && validop(fs, &v1, &v2, op))) {
		return 0;
	}

	luaO_arith(fs->ls->L, op, &v1, &v2);
	if (v1.tt_ == LUA_NUMINT) {
		e1->k = VINT;
		e1->u.i = v1.value_.i;
	}
	else {
		e1->k = VFLT;
		e1->u.r = v1.value_.n;
	}

	return 1;
}

static void codeunexpval(FuncState* fs, int op, expdesc* e) {
	luaK_exp2anyreg(fs, e);
	freeexp(fs, e);

	int opcode;

	switch (op) {
	case UNOPR_MINUS: {
		opcode = OP_UNM;
	} break;
	case UNOPR_BNOT: {
		opcode = OP_BNOT;
	} break;
	case UNOPR_LEN: {
		opcode = OP_LEN;
	} break;
	default:lua_assert(0);
	}

	e->u.info = luaK_codeABC(fs, opcode, 0, e->u.info, 0);
	e->k = VRELOCATE;
}

static void codenot(FuncState* fs, expdesc* e) {
	switch (e->k) {
	case VFALSE: case VNIL: {
		e->k = VTRUE;
	} return;
	case VTRUE: case VINT: case VFLT: {
		e->k = VFALSE;
	} return;
	default:break;
	}

	discharge2anyreg(fs, e);
	freeexp(fs, e);
	lua_assert(e->k == VNONRELOC);
	e->u.info = luaK_codeABC(fs, OP_NOT, 0, e->u.info, 0);
	e->k = VRELOCATE;
}

static void codebinexp(FuncState* fs, int op, expdesc* e1, expdesc* e2) {
	luaK_exp2anyreg(fs, e1);
	luaK_exp2anyreg(fs, e2);
	freeexp(fs, e2);
	freeexp(fs, e1);

	luaK_reserveregs(fs, 1);

	luaK_codeABC(fs, op, fs->freereg - 1, e1->u.info, e2->u.info);
	e1->k = VNONRELOC;
	e1->u.info = fs->freereg - 1;
}

void luaK_prefix(FuncState* fs, int op, expdesc* e) {
	expdesc ef;
	ef.k = VINT; ef.u.info = 0; ef.t = ef.f = NO_JUMP;

	switch (op) {
	case UNOPR_MINUS: case UNOPR_BNOT: {
		if (constfolding(fs, LUA_OPT_UMN + op, e, &ef)) {
			break;
		}
	}
	case UNOPR_LEN: {
		codeunexpval(fs, op, e);
	} break;
	case UNOPR_NOT: {
		codenot(fs, e);
	} break;
	default:lua_assert(0);
	}
}

void luaK_infix(FuncState* fs, int op, expdesc* e) {
	switch (op) {
	case BINOPR_AND: {
		luaK_goiftrue(fs, e);
	} break;
	case BINOPR_OR: {
		luaK_goiffalse(fs, e);
	} break;
	case BINOPR_CONCAT: {
		luaK_exp2nextreg(fs, e);
	} break;
	case BINOPR_ADD: case BINOPR_SUB: case BINOPR_MUL: case BINOPR_DIV:
	case BINOPR_IDIV: case BINOPR_MOD: case BINOPR_POW: case BINOPR_BAND:
	case BINOPR_BOR: case BINOPR_BXOR: case BINOPR_SHL: case BINOPR_SHR: {
		if (!tonumeral(e, NULL)) {
			luaK_exp2RK(fs, e);
		}
	} break;
	case BINOPR_LESS: case BINOPR_GREATER: case BINOPR_LESSEQ: case BINOPR_GREATEQ:
	case BINOPR_EQ: case BINOPR_NOTEQ: {
		luaK_exp2RK(fs, e);
	} break;
	default: {
		printf("luaK_infix:unknow binopr:%d \n", op);
		luaD_throw(fs->ls->L, LUA_ERRPARSER);
	} break;
	}
}

static void codecmp(FuncState* fs, int op, expdesc* e1, expdesc* e2) {
	discharge2anyreg(fs, e1);
	discharge2anyreg(fs, e2);
	freeexp(fs, e2);
	freeexp(fs, e1);

	switch (op) {
	case BINOPR_EQ: {
		luaK_codeABC(fs, OP_EQ, 1, e1->u.info, e2->u.info);
	} break;
	case BINOPR_NOTEQ: {
		luaK_codeABC(fs, OP_EQ, 0, e1->u.info, e2->u.info);
	} break;
	case BINOPR_GREATER: {
		luaK_codeABC(fs, OP_LT, 0, e1->u.info, e2->u.info);
	} break;
	case BINOPR_LESS: {
		luaK_codeABC(fs, OP_LT, 1, e1->u.info, e2->u.info);
	} break;
	case BINOPR_GREATEQ: {
		luaK_codeABC(fs, OP_LE, 0, e1->u.info, e2->u.info);
	} break;
	case BINOPR_LESSEQ: {
		luaK_codeABC(fs, OP_LE, 1, e1->u.info, e2->u.info);
	} break;
	default: {
		printf("unknow compare operator:%d \n", op);
		luaD_throw(fs->ls->L, LUA_ERRPARSER);
	} break;
	}

	e1->u.info = luaK_jump(fs, e1);
	e1->k = VJMP;
}

void luaK_posfix(FuncState* fs, int op, expdesc* e1, expdesc* e2) {
	switch (op) {
	case BINOPR_AND: case BINOPR_OR: {
		luaK_concat(fs, &e2->t, e1->t);
		luaK_concat(fs, &e2->f, e1->f);
		*e1 = *e2;
	} break;
	case BINOPR_CONCAT: {
		luaK_exp2nextreg(fs, e2);
		fs->freereg -= 2;
		luaK_codeABC(fs, OP_CONCAT, fs->freereg, e1->u.info, e2->u.info);
		luaK_reserveregs(fs, 1);
		e1->k = VNONRELOC;
		e1->u.info = fs->freereg - 1;
	} break;
	case BINOPR_ADD: case BINOPR_SUB: case BINOPR_MUL: case BINOPR_DIV:
	case BINOPR_IDIV: case BINOPR_MOD: case BINOPR_POW: case BINOPR_BAND:
	case BINOPR_BOR: case BINOPR_BXOR: case BINOPR_SHL: case BINOPR_SHR: {
		if (!constfolding(fs, LUA_OPT_ADD + op, e1, e2)) {
			codebinexp(fs, OP_ADD + op, e1, e2);
		}
	} break;
	case BINOPR_LESS: case BINOPR_GREATER: case BINOPR_LESSEQ: case BINOPR_GREATEQ:
	case BINOPR_EQ: case BINOPR_NOTEQ: {
		codecmp(fs, op, e1, e2);
	} break;
	default: {
		printf("luaK_posfix:unknow binopr:%d \n", op);
		luaD_throw(fs->ls->L, LUA_ERRPARSER);
	} break;
	}
}

void luaK_setreturns(FuncState* fs, expdesc* e, int nret) {
	lua_assert(e->k == VCALL);
	Instruction* i = &get_instruction(fs, e);
	SET_ARG_C(*i, (nret + 1));
}

void luaK_setoneret(FuncState* fs, expdesc* e) {

}

void luaK_storevar(FuncState* fs, expdesc* var, expdesc* ex) {
	switch (var->k) {
	case VLOCAL: {
		freeexp(fs, ex);
		exp2reg(fs, ex, var->u.info);
	} break;
	case VUPVAL: {
		int e = luaK_exp2anyreg(fs, ex);
		luaK_codeABC(fs, OP_SETUPVAL, var->u.info, e, 0);
	} break;
	case VINDEXED: {
		int e = luaK_exp2RK(fs, ex);
		luaK_codeABC(fs, OP_SETTABUP, var->u.ind.t, var->u.ind.idx, e);
	} break;
	default:break;
	}

	freeexp(fs, ex);
}

void luaK_exp2val(FuncState* fs, expdesc* e) {
	if (hasjump(e)) {
		luaK_exp2anyreg(fs, e);
	}
	else {
		luaK_dischargevars(fs, e);
	}
}

int luaK_exp2RK(FuncState* fs, expdesc* e) {
	luaK_exp2val(fs, e);
	switch (e->k) {
	case VFLT: e->u.info = luaK_floatK(fs, e->u.r); goto vk;
	case VINT: e->u.info = luaK_intK(fs, e->u.i); goto vk;
	case VTRUE: case VFALSE: e->u.info = luaK_boolK(fs, e->k == VTRUE); goto vk;
	case VK: {
	vk:
		e->k = VK;
		if (e->u.info <= MAININDEX) {
			return RKMASK(e->u.info);
		}
	} break;
	default:break;
	}

	return luaK_exp2anyreg(fs, e);
}

int luaK_stringK(FuncState* fs, TString* key) {
	TValue value;
	setgco(&value, obj2gco(key));
	return addk(fs, &value);
}

int luaK_nilK(FuncState* fs) {
	TString* nil = luaS_newliteral(fs->ls->L, "nil");
	TValue value;
	setgco(&value, obj2gco(nil));
	return addk(fs, &value);
}

int luaK_floatK(FuncState* fs, lua_Number r) {
	TValue value;
	setfltvalue(&value, r);
	return addk(fs, &value);
}

int luaK_intK(FuncState* fs, lua_Integer i) {
	TValue value;
	setivalue(&value, i);
	return addk(fs, &value);
}

int luaK_boolK(FuncState* fs, int v) {
	TValue value;
	setbvalue(&value, v != 0);
	return addk(fs, &value);
}

int luaK_ret(FuncState* fs, int first, int nret) {
	luaK_codeABC(fs, OP_RETURN, first, nret + 1, 0);
	return LUA_OK;
}

void luaK_self(FuncState* fs, expdesc* e, expdesc* key) {
	int ereg = 0;
	luaK_exp2anyreg(fs, e);
	ereg = e->u.info;

	freeexp(fs, e);
	e->u.info = fs->freereg;
	e->k = VNONRELOC;

	luaK_reserveregs(fs, 2);
	luaK_codeABC(fs, OP_SELF, e->u.info, ereg, luaK_exp2RK(fs, key));
	freeexp(fs, key);
}

static int condjump(FuncState* fs, expdesc* e, int b) {
	luaK_codeABC(fs, OP_TESTSET, 0, e->u.info, b);
	return luaK_jump(fs, e);
}

int luaK_jump(FuncState* fs, expdesc* e) {
	return luaK_codeAsBx(fs, OP_JUMP, 0, NO_JUMP);
}

static int jumponcond(FuncState* fs, expdesc* e, int b) {
	discharge2anyreg(fs, e);
	freeexp(fs, e);

	return condjump(fs, e, b);
}

// get next jump instruction if it is exist
static int get_next_jmp(FuncState* fs, int pc) {
	Instruction jmp = fs->p->code[pc];
	int offset = GET_ARG_sBx(jmp);
	if (offset == NO_JUMP) {
		return NO_JUMP;
	}
	else {
		return (pc + 1) + offset;
	}
}

static void fixjmp(FuncState* fs, int list, int l2) {
	int offset = l2 - list - 1;
	if (offset > MAXARG_sBx) {
		luaX_syntaxerror(fs->ls->L, fs->ls, "offset overflow");
	}
	SET_ARG_sBx(fs->p->code[list], offset);
}

void luaK_concat(FuncState* fs, int* l1, int l2) {
	if (l2 == NO_JUMP) {
		return;
	}
	else if (*l1 == NO_JUMP) {
		*l1 = l2;
	}
	else {
		int list = *l1;
		int next = 0;
		while ((next = get_next_jmp(fs, list)) != NO_JUMP)
			list = next;
		fixjmp(fs, list, l2);
	}
}

void luaK_patchtohere(FuncState* fs, int pc) {
	luaK_concat(fs, &fs->jpc, pc);
}

static Instruction* getjumpcontrol(FuncState* fs, int pc) {
	Instruction* i = &fs->p->code[pc];
	int jop = GET_OPCODE(*i);
	int prev_op = GET_OPCODE(*(i - 1));
	if (pc >= 1 && testTMode(*(i - 1))) {
		return i - 1;
	}
	else {
		return i;
	}
}

static void negatecondition(FuncState* fs, int pc) {
	Instruction* i = getjumpcontrol(fs, pc);
	lua_assert(GET_OPCODE(*i) != OP_TESTSET && GET_OPCODE(*i) != OP_TEST);
	SET_ARG_A(*i, !GET_ARG_A(*i));
}

void luaK_goiftrue(FuncState* fs, expdesc* e) {
	int pc;

	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VJMP: {
		negatecondition(fs, e->u.info);
		pc = e->u.info;
	} break;
	case VK: case VFLT: case VINT: case VTRUE: {
		pc = NO_JUMP;
	} break;
	default: {
		pc = jumponcond(fs, e, 0);
	} break;
	}

	luaK_concat(fs, &e->f, pc);
	luaK_patchtohere(fs, e->t);
	e->t = NO_JUMP;
}

void luaK_goiffalse(FuncState* fs, expdesc* e) {
	int pc;

	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VJMP: {
		pc = e->u.info;
	} break;
	case VNIL: case VFALSE: {
		pc = NO_JUMP;
	} break;
	default: {
		pc = jumponcond(fs, e, 1);
	} break;
	}

	luaK_concat(fs, &e->t, pc);
	luaK_patchtohere(fs, e->f);
	e->f = NO_JUMP;
}

void luaK_indexed(FuncState* fs, expdesc* e, expdesc* key) {
	e->u.ind.t = e->u.info;
	e->u.ind.idx = luaK_exp2RK(fs, key);
	e->u.ind.vt = e->k == VNONRELOC ? VLOCAL : VUPVAL;
	e->k = VINDEXED;
}

void luaK_nil(FuncState* fs, int from, int to) {
	luaK_codeABC(fs, OP_LOADNIL, from, to, 0);
}

static void dischargejpc(FuncState* fs) {
	patchlistaux(fs, fs->jpc, fs->pc, fs->freereg - 1, fs->pc);
	fs->last_target = fs->pc;
	fs->jpc = NO_JUMP;
}

int luaK_codeABC(FuncState* fs, int opcode, int a, int b, int c) {
	dischargejpc(fs);

	luaM_growvector(fs->ls->L, fs->p->code, fs->pc, fs->p->sizecode, Instruction, INT_MAX);
	Instruction i = (b << POS_B) | (c << POS_C) | (a << POS_A) | opcode;
	fs->p->code[fs->pc] = i;

	return fs->pc++;
}

int luaK_codeABx(FuncState* fs, int opcode, int a, int bx) {
	dischargejpc(fs);

	luaM_growvector(fs->ls->L, fs->p->code, fs->pc, fs->p->sizecode, Instruction, INT_MAX);
	Instruction i = (bx << POS_C) | (a << POS_A) | opcode;
	fs->p->code[fs->pc] = i;

	return fs->pc++;
}

void luaK_dischargevars(FuncState* fs, expdesc* e) {
	switch (e->k) {
	case VLOCAL: {
		e->k = VNONRELOC;
	} break;
	case VUPVAL: {
		e->u.info = luaK_codeABC(fs, OP_GETUPVAL, 0, e->u.info, 0);
		e->k = VRELOCATE;
	} break;
	case VINDEXED: {
		freereg(fs, e->u.ind.idx);
		if (e->u.ind.vt == VLOCAL) {
			freereg(fs, e->u.ind.t);
			e->u.info = luaK_codeABC(fs, OP_GETTABLE, 0, e->u.ind.t, e->u.ind.idx);
		}
		else { // VUPVAL
			e->u.info = luaK_codeABC(fs, OP_GETTABUP, 0, e->u.ind.t, e->u.ind.idx);
		}

		e->k = VRELOCATE;
	} break;
	case VCALL: {
		e->k = VNONRELOC;
		e->u.info = GET_ARG_A(get_instruction(fs, e));
	} break;
	default:break;
	}
}

static void freereg(FuncState* fs, int reg) {
	if (!ISK(reg) && reg >= fs->nactvars) {
		fs->freereg--;
		lua_assert(fs->freereg == reg);
	}
}

static void freeexp(FuncState* fs, expdesc* e) {
	if (e->k == VNONRELOC) {
		freereg(fs, e->u.info);
	}
}

static void checkstack(FuncState* fs, int n) {
	if (fs->freereg + n > fs->p->maxstacksize) {
		fs->p->maxstacksize = fs->freereg + n;
	}
}

void luaK_reserveregs(FuncState* fs, int n) {
	checkstack(fs, n);
	fs->freereg += n;
}

void luaK_setlist(FuncState* fs, int base, int nelemts, int tostore) {
	int c = (nelemts - 1) / LFIELD_PER_FLUSH + 1;
	int b = tostore == LUA_MULRET ? 0 : tostore;
	if (c <= MAXARG_Bx)
		luaK_codeABC(fs, OP_SETLIST, base, b, c);
	else
		luaX_syntaxerror(fs->ls->L, fs->ls, "constructor is too long");
	fs->freereg = base + 1;
}

static void discharge2reg(FuncState* fs, expdesc* e, int reg) {
	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VNIL: {
		luaK_codeABC(fs, OP_LOADNIL, reg, reg, 0);
	} break;
	case VFLT: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_floatK(fs, e->u.r));
	} break;
	case VINT: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_intK(fs, e->u.i));
	} break;
	case VTRUE: case VFALSE: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_boolK(fs, e->k == VTRUE));
	} break;
	case VK: {
		luaK_codeABx(fs, OP_LOADK, reg, e->u.info);
	} break;
	case VRELOCATE: {
		SET_ARG_A(fs->p->code[e->u.info], reg);
	} break;
	case VNONRELOC: {
		if (e->u.info != reg) {
			luaK_codeABC(fs, OP_MOVE, reg, e->u.info, 0);
		}
	} break;
	default:break;
	}
}

static void discharge2anyreg(FuncState* fs, expdesc* e) {
	if (e->k != VNONRELOC) {
		luaK_reserveregs(fs, 1);
		discharge2reg(fs, e, fs->freereg - 1);

		e->k = VNONRELOC;
		e->u.info = fs->freereg - 1;
	}
}

static int patchtestreg(FuncState* fs, int pc, int reg) {
	Instruction* i = getjumpcontrol(fs, pc);
	if (GET_OPCODE(*i) != OP_TESTSET)
		return 0;

	if (reg != NO_REG && reg != GET_ARG_A(*i)) {
		SET_ARG_A(*i, reg);
	}

	return 1;
}

static void patchlistaux(FuncState* fs, int list, int dtarget, int reg, int vtarget) {
	while (list != NO_JUMP) {
		int next = get_next_jmp(fs, list);

		if (patchtestreg(fs, list, reg))
			fixjmp(fs, list, dtarget);
		else
			fixjmp(fs, list, vtarget);

		list = next;
	}
}

static int need_value(FuncState* fs, int list) {
	for (; list != NO_JUMP; list = get_next_jmp(fs, list)) {
		Instruction* i = getjumpcontrol(fs, list);
		int op = GET_OPCODE(*i);
		if (op != OP_TESTSET && op != OP_TEST) {
			return 1;
		}
	}

	return 0;
}

static int exp2reg(FuncState* fs, expdesc* e, int reg) {
	discharge2reg(fs, e, reg);

	if (e->k == VJMP) {
		luaK_concat(fs, &e->t, e->u.info);
	}

	if (hasjump(e)) {
		int p_f = NO_JUMP;
		int p_t = NO_JUMP;

		if (need_value(fs, e->t) || need_value(fs, e->f)) {
			int jf = e->k == VJMP ? NO_JUMP : luaK_jump(fs, e);
			p_f = luaK_codeABC(fs, OP_LOADBOOL, reg, 0, 1);
			p_t = luaK_codeABC(fs, OP_LOADBOOL, reg, 1, 0);
			luaK_patchtohere(fs, jf);
		}

		int final = fs->pc;
		patchlistaux(fs, e->f, final, reg, p_f);
		patchlistaux(fs, e->t, final, reg, p_t);
	}

	e->k = VNONRELOC;
	e->u.info = reg;
	e->t = NO_JUMP;
	e->f = NO_JUMP;

	return e->u.info;
}

int luaK_exp2nextreg(FuncState* fs, expdesc* e) {
	luaK_dischargevars(fs, e);
	freeexp(fs, e);
	luaK_reserveregs(fs, 1);
	return exp2reg(fs, e, fs->freereg - 1);
}

int luaK_exp2anyreg(FuncState* fs, expdesc * e) {
	luaK_dischargevars(fs, e);
	if (e->k == VNONRELOC) {
		return e->u.info;
	}
	return luaK_exp2nextreg(fs, e);
}

void luaK_exp2anyregup(FuncState* fs, expdesc* e) {
	if (e->k != VUPVAL) {
		luaK_exp2anyreg(fs, e);
	}
}
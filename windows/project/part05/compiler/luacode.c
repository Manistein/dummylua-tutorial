#include "luacode.h"
#include "../common/luaobject.h"
#include "../common/luastate.h"
#include "../vm/luagc.h"
#include "../common/luatable.h"
#include "../vm/luavm.h"
#include "../common/luamem.h"
#include "../vm/luaopcodes.h"
#include "../common/luastring.h"

#define MAININDEX 255

static int addk(FuncState* fs, TValue* v) {
	LexState* ls = fs->ls;
	Proto* p = fs->p;
	TValue* idx = luaH_set(ls->L, ls->h, v);
	if (!ttisnil(idx)) {
		int k = idx->value_.i;
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

int luaK_exp2RK(FuncState* fs, expdesc* e) {
	switch (e->k) {
	case VNIL: e->u.info = luaK_nilK(fs); goto vk;
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
	setfltvalue(&value, (float)r);
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

void luaK_indexed(FuncState* fs, expdesc* e, expdesc* key) {
	e->u.ind.t = e->u.info;
	e->u.ind.idx = luaK_exp2RK(fs, key);
	e->u.ind.vt = e->k == VLOCAL ? VLOCAL : VUPVAL;
	e->k = VINDEXED;
}

int luaK_codeABC(FuncState* fs, int opcode, int a, int b, int c) {
	luaM_growvector(fs->ls->L, fs->p->code, fs->pc, fs->p->sizecode, Instruction, INT_MAX);
	Instruction i = (b << POS_B) | (c << POS_C) | (a << POS_A) | opcode;
	fs->p->code[fs->pc] = i;

	return fs->pc++;
}

int luaK_codeABx(FuncState* fs, int opcode, int a, int bx) {
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
	case VINDEXED: {
		if (e->u.ind.vt == VLOCAL) {
			e->u.info = luaK_codeABC(fs, OP_GETTABLE, 0, e->u.ind.t, e->u.ind.idx);
		}
		else { // VUPVAL
			e->u.info = luaK_codeABC(fs, OP_GETTABUP, 0, e->u.ind.t, e->u.ind.idx);
		}

		e->k = VRELOCATE;
	} break;
	case VCALL: {
		e->k = VRELOCATE;
		e->u.info = GET_ARG_A(fs->p->code[e->u.info]);
	} break;
	default:break;
	}
}

static void freereg(FuncState* fs, int reg) {
	if (!ISK(reg) && reg > fs->nactvars) {
		fs->freereg--;
		lua_assert(fs->freereg == reg);
	}
}

static void free_exp(FuncState* fs, expdesc* e) {
	if (e->k == VNONRELOC) {
		freereg(fs, e->u.info);
	}
}

static void checkstack(FuncState* fs, int n) {
	if (fs->freereg + n > fs->p->maxstacksize) {
		fs->p->maxstacksize = fs->freereg + n;
	}
}

static void reserve_reg(FuncState* fs, expdesc* e, int n) {
	checkstack(fs, n);
	fs->freereg += n;
}

static int exp2reg(FuncState* fs, expdesc* e, int reg) {
	switch (e->k) {
	case VNIL: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_nilK(fs));
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
	default:break;
	}

	e->k = VNONRELOC;
	e->u.info = reg;

	return e->u.info;
}

int luaK_exp2nextreg(FuncState* fs, expdesc* e) {
	luaK_dischargevars(fs, e);
	free_exp(fs, e);
	reserve_reg(fs, e, 1);
	return exp2reg(fs, e, fs->freereg - 1);
}

int luaK_exp2anyreg(FuncState* fs, expdesc * e) {
	luaK_dischargevars(fs, e);
	if (e->k == VNONRELOC) {
		return e->u.info;
	}
	return luaK_exp2nextreg(fs, e);
}
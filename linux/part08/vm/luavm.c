/* Copyright (c) 2018 Manistein,https://manistein.github.io/blog/  

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.*/

#include "luavm.h"
#include "../common/luastring.h"
#include "luado.h"
#include "luagc.h"
#include "luaopcodes.h"
#include "luafunc.h"
#include "../common/luaobject.h"

// slot == NULL意味着t不是table类型
void luaV_finishget(struct lua_State* L, struct Table* t, StkId val, const TValue* slot) {
    if (slot == NULL) {
        luaD_throw(L, LUA_ERRRUN);
    }
    else {
        setnilvalue(val);
    }
}

void luaV_finishset(struct lua_State* L, struct Table* t, const TValue* key, StkId val, const TValue* slot) {
    if (slot == NULL) {
        luaD_throw(L, LUA_ERRRUN);
    }

    if (slot == luaO_nilobject) {
        slot = luaH_newkey(L, t, key);
    }

    setobj(cast(TValue*, slot), val);
    luaC_barrierback(L, t, slot);
}

int luaV_eqobject(struct lua_State* L, const TValue* a, const TValue* b) {
    if ((ttisfloat(a) && lua_numisnan(a->value_.n)) || 
            (ttisfloat(b) && lua_numisnan(b->value_.n))) {
        return 0;
    }

    if (a->tt_ != b->tt_) {
        // 只有数值，在类型不同的情况下可能相等
        if (novariant(a) == LUA_TNUMBER && novariant(b) == LUA_TNUMBER) {
            double fa = ttisinteger(a) ? a->value_.i : a->value_.n;
            double fb = ttisinteger(b) ? b->value_.i : b->value_.n;
            return fa == fb;
        }
        else {
            return 0;
        }
    }    

    switch(a->tt_) {
        case LUA_TNIL: return 1;
        case LUA_NUMFLT: return a->value_.n == b->value_.n;
        case LUA_NUMINT: return a->value_.i == b->value_.i;
        case LUA_SHRSTR: return luaS_eqshrstr(L, gco2ts(gcvalue(a)), gco2ts(gcvalue(b)));
        case LUA_LNGSTR: return luaS_eqlngstr(L, gco2ts(gcvalue(a)), gco2ts(gcvalue(b)));
        case LUA_TBOOLEAN: return a->value_.b == b->value_.b; 
        case LUA_TLIGHTUSERDATA: return a->value_.p == b->value_.p; 
        case LUA_TLCF: return a->value_.f == b->value_.f;
        default: {
            return gcvalue(a) == gcvalue(b);
        } break;
    } 

    return 0;
}

// implement of instructions
static void newframe(struct lua_State* L);
static void op_move(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int reg_b = GET_ARG_B(i);
	StkId o = L->ci->l.base + reg_b;
	setobj(ra, o);
}

static void op_loadk(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int bx = GET_ARG_Bx(i);
	setobj(ra, &cl->p->k[bx]);
}

static void op_getupval(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	UpVal* upval = cl->upvals[GET_ARG_B(i)];
	if (upval) {
		setobj(ra, upval->v);
	}
	else {
		printf("op_getupval:upval is not exist");
		luaD_throw(L, LUA_ERRRUN);
	}
}

static void op_gettabup(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	TValue* upval = cl->upvals[b]->v;
	struct Table* t = gco2tbl(gcvalue(upval));
	int arg_c = GET_ARG_C(i);
	if (ISK(arg_c)) {
		int index = arg_c - MAININDEXRK - 1;
		TValue* key = &cl->p->k[index];
		TValue* value = (TValue*)luaH_get(L, t, key);
		setobj(ra, value);
	}
	else {
		TValue* value = L->ci->l.base + arg_c;
		setobj(ra, value);
	}
}

static void op_call(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int narg = GET_ARG_B(i);
	int nresult = GET_ARG_C(i) - 1;
	if (narg > 0) {
		L->top = ra + narg;
	}

	if (luaD_precall(L, ra, nresult)) { // c function
		L->top = L->ci->top;
	}
	else {
		newframe(L);
	}
}

static void op_return(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	luaD_poscall(L, ra, b ? (b - 1) : (int)(L->top - ra));
	if (L->ci->callstatus & CIST_LUA) {
		//if (b) {
		//	L->top = L->ci->top;
		//}
		lua_assert(GET_OPCODE(*(L->ci->savedpc - 1)) == OP_CALL);
	}
}

static void op_gettable(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	int c = GET_ARG_C(i);
	TValue* vt = L->ci->l.base + b;
	if (!ttistable(vt)) {
		// TODO thow exception
		printf("OP_GETTABLE, RA(%d) RB(%d) RC(%d); RB is not index to a table", ra - L->ci->l.base, b, c);
		luaD_throw(L, LUA_ERRRUN);
	}

	struct Table* t = gco2tbl(gcvalue(vt));
	TValue* v;
	if (ISK(c)) {
		int k = c - MAININDEXRK - 1;
		v = (TValue*)luaH_get(L, t, (const TValue*)&cl->p->k[k]);
	}
	else {
		v = (TValue*)luaH_get(L, t, (const TValue*)(L->ci->l.base + c));
	}
	setobj(ra, v);
}

static void op_self(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	int c = GET_ARG_C(i);

	TValue* vt = L->ci->l.base + b;
	if (!ttistable(vt)) {
		// TODO thow exception
		printf("OP_SELF, RA(%d) RB(%d) RC(%d); RB is not index to a table", ra - L->ci->l.base, b, c);
		luaD_throw(L, LUA_ERRRUN);
	}
	struct Table* t = gco2tbl(gcvalue(vt));

	StkId ra1 = ra + 1;
	StkId f;
	setobj(ra1, vt);
	if (ISK(c)) {
		int idx = c - MAININDEXRK - 1;
		f = (StkId)luaH_get(L, t, &cl->p->k[idx]);
	}
	else {
		f = (StkId)luaH_get(L, t, L->ci->l.base + c);
	}
	setobj(ra, f);
}

static void op_test(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int c = GET_ARG_C(i);

	int cond = 0;
	switch (ra->tt_) {
	case LUA_TNIL: cond = 0; break;
	case LUA_TBOOLEAN: cond = ra->value_.b; break;
	default: cond = 1; break;
	}

	if (!(cond == c)) {
		L->ci->l.savedpc++;
	}
}

static void op_testset(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	int c = GET_ARG_C(i);

	StkId rb = L->ci->l.base + b;
	int cond = 0;
	switch (rb->tt_) {
	case LUA_TNIL: cond = 0; break;
	case LUA_TBOOLEAN: cond = rb->value_.b; break;
	default: cond = 1; break;
	}

	if (cond == c) {
		setobj(ra, rb);
	}
	else {
		L->ci->l.savedpc++;
	}
}

static void op_jump(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int sbx = GET_ARG_Bx(i);
	sbx -= LUA_IBIAS;
	L->ci->l.savedpc += sbx;
}

static void op_unm(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	StkId rb = L->ci->l.base + b;
	TValue o;
	setobj(&o, rb);
	if (!luaO_arith(L, LUA_OPT_UMN, &o, rb)) {
		// TODO throw exception
		printf("op_unm: rb's type is incorrect");
		luaD_throw(L, LUA_ERRRUN);
	}
	setobj(ra, &o);
}

static void op_len(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	StkId rb = L->ci->l.base + b;
	if (ttistable(rb)) {
		struct Table* t = gco2tbl(gcvalue(rb));
		setivalue(ra, t->arraysize);
	}
	else if (ttisshrstr(rb) || ttislngstr(rb)) {
		struct TString* ts = gco2ts(gcvalue(rb));
		int len = ttisshrstr(rb) ? ts->shrlen : ts->u.lnglen;
		setivalue(ra, len);
	}
	else {
		// TODO throw exception
		printf("op_len: rb's type is incorrect");
		luaD_throw(L, LUA_ERRRUN);
	}
}

static void op_bnot(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId rb = L->ci->l.base + GET_ARG_B(i);
	TValue o;
	if (!luaO_arith(L, LUA_OPT_BNOT, &o, rb)) {
		// TODO throw exception
		printf("op_len: rb's type is incorrect");
		luaD_throw(L, LUA_ERRRUN);
	}
	setobj(ra, &o);
}

static void op_not(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId rb = L->ci->l.base + GET_ARG_B(i);
	switch (novariant(rb)) {
	case LUA_TNUMBER: {
		float v = ttisfloat(rb) ? (float)rb->value_.n : (float)rb->value_.i;
		setivalue(ra, !((int)v));
	} break;
	case LUA_TBOOLEAN: {
		setbvalue(ra, !rb->value_.b);
	} break;
	case LUA_TLCF: {
		setivalue(ra, !((int)rb->value_.f));
	} break;
	case LUA_TLIGHTUSERDATA: {
		setivalue(ra, !((int)rb->value_.p));
	} break;
	default: {
		setivalue(ra, !((int)gcvalue(rb)));
	} break;
	}
}

static void binop(struct lua_State* L, LClosure* cl, StkId ra, Instruction i, int op) {
	int b = GET_ARG_B(i);
	int c = GET_ARG_C(i);
	StkId rb, rc;
	if (ISK(b)) {
		rb = &cl->p->k[b - MAININDEXRK - 1];
	}
	else {
		rb = L->ci->l.base + b;
	}

	if (ISK(c)) {
		rc = &cl->p->k[c - MAININDEXRK - 1];
	}
	else {
		rc = L->ci->l.base + c;
	}

	TValue o;
	setobj(&o, rb);
	if (!luaO_arith(L, op, &o, rc)) {
		// TODO throw exception
		printf("binop: rb's type is incorrect. type is %d", op);
		luaD_throw(L, LUA_ERRRUN);
	}
	setobj(ra, &o);
}

static void op_add(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_ADD);
}

static void op_sub(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_SUB);
}

static void op_mul(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_MUL);
}

static void op_div(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_DIV);
}

static void op_idiv(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_IDIV);
}

static void op_mod(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_MOD);
}

static void op_pow(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_POW);
}

static void op_band(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_BAND);
}

static void op_bor(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_BOR);
}

static void op_bxor(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_BXOR);
}

static void op_shl(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_SHL);
}

static void op_shr(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	binop(L, cl, ra, i, LUA_OPT_SHR);
}

static void op_concat(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	// TODO
}

static void op_eq(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId rb = RK(L, cl, GET_ARG_B(i));
	StkId rc = RK(L, cl, GET_ARG_C(i));

	if (luaV_eqobject(L, rb, rc) != GET_ARG_A(i))
		L->ci->l.savedpc++;
}

static void op_lt(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId rb = RK(L, cl, GET_ARG_B(i));
	StkId rc = RK(L, cl, GET_ARG_C(i));
	lua_Number nb, nc;
	if (!luaV_tonumber(L, rb, &nb) || !luaV_tonumber(L, rc, &nc)) {
		// TODO throw exception
		printf("op_lt: rb or rc is not number");
		luaD_throw(L, LUA_ERRRUN);
	}

	if ((nb < nc) != GET_ARG_A(i)) {
		L->ci->l.savedpc++;
	}
}

static void op_le(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId rb = RK(L, cl, GET_ARG_B(i));
	StkId rc = RK(L, cl, GET_ARG_C(i));
	lua_Number nb, nc;
	if (!luaV_tonumber(L, rb, &nb) || !luaV_tonumber(L, rc, &nc)) {
		// TODO throw exception
		printf("op_le: rb or rc is not number");
		luaD_throw(L, LUA_ERRRUN);
	}

	if ((nb <= nc) != GET_ARG_A(i)) {
		L->ci->l.savedpc++;
	}
}

static void op_loadbool(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	setbvalue(ra, GET_ARG_B(i));
	if (GET_ARG_C(i))
		L->ci->l.savedpc++;
}

static void op_loadnil(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int a = GET_ARG_A(i);
	int b = GET_ARG_B(i);
	for (int i = a; i <= b; i++)
		setnilvalue(L->ci->l.base + i);
}

static void op_setupval(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	setobj(cl->upvals[GET_ARG_B(i)]->v, ra);
}

static void op_settabup(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	TValue* upval = cl->upvals[GET_ARG_A(i)]->v;
	if (!ttistable(upval)) {
		// TODO throw exception
		printf("op_settabup: upval is not table");
		luaD_throw(L, LUA_ERRRUN);
	}
	struct Table* t = gco2tbl(gcvalue(upval));
	TValue* v = luaH_set(L, t, RK(L, cl, GET_ARG_B(i)));
	setobj(v, RK(L, cl, GET_ARG_C(i)));
}

static void op_newtable(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	struct Table* t = luaH_new(L);
	luaH_resize(L, t, GET_ARG_B(i), GET_ARG_C(i));
	ra->value_.gc = obj2gco(t);
	ra->tt_ = LUA_TTABLE;
}

static void op_setlist(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	if (!ttistable(ra)) {
		// TODO throw exception
		printf("op_setlist: ra is not table type");
		luaD_throw(L, LUA_ERRRUN);
	}

	struct Table* t = gco2tbl(gcvalue(ra));
	int b = GET_ARG_B(i);
	int c = GET_ARG_C(i);

	int count = 0;
	if (b == 0) {
		count = L->top - ra - 1;
	}
	else {
		count = b;
	}

	int base = (c - 1) * LFIELD_PER_FLUSH;
	for (int i = 1; i <= count; i++) {
		luaH_setint(L, t, base + i, ra + i);
	}
}

static void op_settable(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	TValue* tv = L->ci->l.base + GET_ARG_A(i);
	if (tv->tt_ != LUA_TTABLE) {
		// TODO throw exception
		printf("op_settable: ra is not table type");
		luaD_throw(L, LUA_ERRRUN);
	}

	struct Table* t = gco2tbl(gcvalue(tv));
	StkId rb, rc;
	int b = GET_ARG_B(i);
	int c = GET_ARG_C(i);
	if (ISK(b)) {
		rb = &cl->p->k[b - MAININDEXRK - 1];
	}
	else {
		rb = L->ci->l.base + b;
	}

	if (ISK(c)) {
		rc = &cl->p->k[c - MAININDEXRK - 1];
	}
	else {
		rc = L->ci->l.base + c;
	}

	TValue* val = luaH_set(L, t, rb);
	setobj(val, rc);
}

static void op_forprep(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId step_ptr = ra + 2;
	if (!ttisinteger(step_ptr) && !ttisfloat(step_ptr)) {
		printf("op_forprep:step type error");
		luaD_throw(L, LUA_ERRRUN);
	}
	
	int arith_ret = luaO_arith(L, LUA_OPT_SUB, ra, step_ptr);
	if (!arith_ret) {
		printf("op_forprep:arith error");
		luaD_throw(L, LUA_ERRRUN);
	}

	int sbx = GET_ARG_sBx(i);
	L->ci->l.savedpc += sbx;
}

static void op_forloop(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId limit = ra + 1;
	StkId step_ptr = ra + 2;
	StkId local_var = ra + 3;

	lua_assert(ttisinteger(limit) || ttisfloat(limit));
	lua_assert(ttisinteger(step_ptr) || ttisfloat(step_ptr));

	int arith_ret = luaO_arith(L, LUA_OPT_ADD, ra, step_ptr);
	if (!arith_ret) {
		printf("op_forloop:arith error");
		luaD_throw(L, LUA_ERRRUN);
	}

	lua_Number v1, v2, v3;
	luaV_tonumber(L, ra, &v1);
	luaV_tonumber(L, limit, &v2);
	luaV_tonumber(L, step_ptr, &v3);
	int sbx = GET_ARG_sBx(i);

	if (v3 >= 0) {
		if (v1 <= v2) {
			L->ci->l.savedpc += sbx;
			setobj(local_var, ra);
		}
	}
	else {
		if (v1 >= v2) {
			L->ci->l.savedpc += sbx;
			setobj(local_var, ra);
		}
	}
}

static void op_tforcall(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	StkId temp_ra3 = ra + 3;
	StkId temp_ra4 = ra + 4;
	StkId temp_ra5 = ra + 5;

	setobj(temp_ra3, ra);
	setobj(temp_ra4, ra + 1);
	setobj(temp_ra5, ra + 2);
	L->top = ra + 6;

	if (luaD_precall(L, temp_ra3, 2)) { // is c call?
		L->top = L->ci->top;
	}
	else {
		newframe(L);
	}
}

static void op_tforloop(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int sbx = GET_ARG_sBx(i);
	if (!ttisnil((ra + 1))) {
		setobj(ra, ra + 1);
		L->ci->l.savedpc += sbx;
	}
}

static void op_closure(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	Proto* proto = cl->p->p[GET_ARG_Bx(i)];
	LClosure* new_cl = luaF_newLclosure(L, proto->sizeupvalues);
	new_cl->p = proto;
	setgco(ra, obj2gco(new_cl));

	// TODO we just ignore upvalues in this version
	new_cl->upvals[0] = cl->upvals[0];
}

int luaV_tonumber(struct lua_State* L, const TValue* v, lua_Number* n) {
	int result = 0;
	if (ttisinteger(v)) {
		*n = (lua_Number)v->value_.i;
		result = 1;
	}
	else if (ttisfloat(v)) {
		*n = v->value_.n;
		result = 1;
	}
	return result;
}

int luaV_tointeger(struct lua_State* L, const TValue* v, lua_Integer* i) {
	int result = 0;
	if (ttisinteger(v)) {
		*i = v->value_.i;
		result = 1;
	}
	else if (ttisfloat(v)) {
		*i = (lua_Integer)v->value_.n;
		result = 1;
	}
	return result;
}

void luaV_execute(struct lua_State* L) {
	L->ci->callstatus |= CIST_FRESH;
	newframe(L);
}

static Instruction vmfetch(struct lua_State* L) {
	Instruction i = *(L->ci->l.savedpc++);
	return i;
}

static StkId vmdecode(struct lua_State* L, Instruction i) {
	StkId ra = L->ci->l.base + GET_ARG_A(i);
	return ra;
}

static bool vmexecute(struct lua_State* L, StkId ra, Instruction i) {
	bool is_loop = true;
	struct GCObject* gco = gcvalue(L->ci->func);
	LClosure* cl = gco2lclosure(gco);

	switch (GET_OPCODE(i)) {
	case OP_MOVE: {
		op_move(L, cl, ra, i);
	} break;
	case OP_GETUPVAL: {
		op_getupval(L, cl, ra, i);
	} break;
	case OP_GETTABLE: {
		op_gettable(L, cl, ra, i);
	} break;
	case OP_GETTABUP: {
		op_gettabup(L, cl, ra, i);
	} break;
	case OP_LOADK: {
		op_loadk(L, cl, ra, i);
	} break;
	case OP_CALL: {
		op_call(L, cl, ra, i);
	} break;
	case OP_SELF: {
		op_self(L, cl, ra, i);
	} break;
	case OP_TEST: {
		op_test(L, cl, ra, i);
	} break;
	case OP_TESTSET: {
		op_testset(L, cl, ra, i);
	} break;
	case OP_JUMP: {
		op_jump(L, cl, ra, i);
	} break;
	case OP_UNM: {
		op_unm(L, cl, ra, i);
	} break;
	case OP_LEN: {
		op_len(L, cl, ra, i);
	} break;
	case OP_BNOT: {
		op_bnot(L, cl, ra, i);
	} break;
	case OP_NOT: {
		op_not(L, cl, ra, i);
	} break;
	case OP_ADD: {
		op_add(L, cl, ra, i);
	} break;
	case OP_SUB: {
		op_sub(L, cl, ra, i);
	} break;
	case OP_MUL: {
		op_mul(L, cl, ra, i);
	} break;
	case OP_DIV: {
		op_div(L, cl, ra, i);
	} break;
	case OP_IDIV: {
		op_idiv(L, cl, ra, i);
	} break;
	case OP_MOD: {
		op_mod(L, cl, ra, i);
	} break;
	case OP_POW: {
		op_pow(L, cl, ra, i);
	} break;
	case OP_BAND: {
		op_band(L, cl, ra, i);
	} break;
	case OP_BOR: {
		op_bor(L, cl, ra, i);
	} break;
	case OP_BXOR: {
		op_bxor(L, cl, ra, i);
	} break;
	case OP_SHL: {
		op_shl(L, cl, ra, i);
	} break;
	case OP_SHR: {
		op_shr(L, cl, ra, i);
	} break;
	case OP_CONCAT: {
		op_concat(L, cl, ra, i);
	} break;
	case OP_EQ: {
		op_eq(L, cl, ra, i);
	} break;
	case OP_LT: {
		op_lt(L, cl, ra, i);
	} break;
	case OP_LE: {
		op_le(L, cl, ra, i);
	} break;
	case OP_LOADBOOL: {
		op_loadbool(L, cl, ra, i);
	} break;
	case OP_LOADNIL: {
		op_loadnil(L, cl, ra, i);
	} break;
	case OP_SETUPVAL: {
		op_setupval(L, cl, ra, i);
	} break;
	case OP_SETTABUP: {
		op_settabup(L, cl, ra, i);
	} break;
	case OP_NEWTABLE: {
		op_newtable(L, cl, ra, i);
	} break;
	case OP_SETLIST: {
		op_setlist(L, cl, ra, i);
	} break;
	case OP_SETTABLE: {
		op_settable(L, cl, ra, i);
	} break;
	case OP_FORPREP: {
		op_forprep(L, cl, ra, i);
	} break;
	case OP_FORLOOP: {
		op_forloop(L, cl, ra, i);
	} break;
	case OP_TFORCALL: {
		op_tforcall(L, cl, ra, i);
	} break;
	case OP_TFORLOOP: {
		op_tforloop(L, cl, ra, i);
	} break;
	case OP_CLOSURE: {
		op_closure(L, cl, ra, i);
	} break;
	case OP_RETURN: {
		op_return(L, cl, ra, i);
		is_loop = false;
	} break;
	default:break;
	}

	return is_loop;
}

static void print_TValue(const TValue* v) {
	switch (v->tt_)
	{
	case LUA_NUMINT: {
		printf("%lld ", v->value_.i);
	} break;
	case LUA_NUMFLT: {
		printf("%.14g ", v->value_.n);
	} break;
	case LUA_SHRSTR: case LUA_LNGSTR: {
		TString* ts = gco2ts(gcvalue(v));
		printf("%s ", getstr(ts));
	} break;
	case LUA_TBOOLEAN: {
		printf("%s ", v->value_.b ? "true" : "false");
	} break;
	default:
		break;
	}
}

static char* code2name[] = {
	"OP_MOVE",
	"OP_LOADK",
	"OP_GETUPVAL",
	"OP_CALL",
	"OP_RETURN",
	"OP_GETTABUP",
	"OP_GETTABLE",
	"OP_SELF",
	"OP_TEST",
	"OP_TESTSET",
	"OP_JUMP",
	"OP_UNM",
	"OP_LEN",
	"OP_BNOT",
	"OP_NOT",
	"OP_ADD",
	"OP_SUB",
	"OP_MUL",
	"OP_DIV",
	"OP_IDIV",
	"OP_MOD",
	"OP_POW",
	"OP_BAND",
	"OP_BOR",
	"OP_BXOR",
	"OP_SHL",
	"OP_SHR",
	"OP_CONCAT",
	"OP_EQ",
	"OP_LT",
	"OP_LE",
	"OP_LOADBOOL",
	"OP_LOADNIL",
	"OP_SETUPVAL",
	"OP_SETTABUP",
	"OP_NEWTABLE",
	"OP_SETLIST",
	"OP_SETTABLE",
	"OP_FORPREP",
	"OP_FORLOOP",
	"OP_TFORCALL",
	"OP_TFORLOOP",
	"OP_CLOSURE"
};

static void print_Instruction(int idx, Instruction i) {
	switch (luaP_opmodes[GET_OPCODE(i)] & 0x03) {
	case iABC: {
		printf("[%d] opcode(%s) ra(%d) rb(%d) rc(%d) \n", idx, code2name[GET_OPCODE(i)], GET_ARG_A(i), GET_ARG_B(i), GET_ARG_C(i));
	} break;
	case iABx: {
		printf("[%d] opcode(%s) ra(%d) bx(%d) \n", idx, code2name[GET_OPCODE(i)], GET_ARG_A(i), GET_ARG_Bx(i));
	} break;
	case iAsBx: {
		printf("[%d] opcode(%s) ra(%d) sbx(%d) \n", idx, code2name[GET_OPCODE(i)], GET_ARG_A(i), GET_ARG_sBx(i));
	} break;
	default: break;
	}
}

static void newframe(struct lua_State* L) {
	// print table k
	struct GCObject* gco = gcvalue(L->ci->func);
	LClosure* cl = gco2lclosure(gco);
	printf("k=>");
	for (int i = 0; i < cl->p->sizek; i++) {
		printf("[%d]:", i);
		print_TValue(&cl->p->k[i]);
	}
	printf("\n");

	for (int i = 0; i < cl->p->sizecode; i++) {
		print_Instruction(i, cl->p->code[i]);
		if (GET_OPCODE(cl->p->code[i]) == OP_RETURN) {
			break;
		}
	}
	printf("\n");

	bool is_loop = true;
	while (is_loop) {
		Instruction i = vmfetch(L);
		StkId ra = vmdecode(L, i);
		is_loop = vmexecute(L, ra, i);
	}
}
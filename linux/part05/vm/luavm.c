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
    if ((ttisnumber(a) && lua_numisnan(a->value_.n)) || 
            (ttisnumber(b) && lua_numisnan(b->value_.n))) {
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
static void op_loadk(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int bx = GET_ARG_Bx(i);
	setobj(ra, &cl->p->k[bx]);
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
		if (nresult >= 0) {
			L->top = L->ci->top;
		}
	}
	else {
		newframe(L);
	}
}

static void op_return(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	luaD_poscall(L, ra, b ? (b - 1) : (int)(L->top - ra));
	if (L->ci->callstatus & CIST_LUA) {
		L->top = L->ci->top;
		lua_assert(GET_OPCODE(*(L->ci->savedpc - 1)) == OP_CALL);
	}
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
	case OP_GETTABUP: {
		op_gettabup(L, cl, ra, i);
	} break;
	case OP_LOADK: {
		op_loadk(L, cl, ra, i);
	} break;
	case OP_CALL: {
		op_call(L, cl, ra, i);
	} break;
	case OP_RETURN: {
		op_return(L, cl, ra, i);
		is_loop = false;
	} break;
	default:break;
	}

	return is_loop;
}

static void newframe(struct lua_State* L) {
	bool is_loop = true;
	while (is_loop) {
		Instruction i = vmfetch(L);
		StkId ra = vmdecode(L, i);
		is_loop = vmexecute(L, ra, i);
	}
}
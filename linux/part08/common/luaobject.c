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

#include "luaobject.h"
#include "../vm/luavm.h"

#define arithint(op, v1, v2) (v1->value_.i = (v1->value_.i op v2->value_.i))
#define arithnum(op, v1, v2) (v1->value_.n = (v1->value_.n op v2->value_.n))

const TValue luaO_nilobject_ = { {NULL}, LUA_TNIL };

int luaO_ceillog2(lua_Integer value) {  
	for (int n = 0; n < MAXABITS; n++) {
		if (value <= (lua_Integer)pow(2, n))
			return n;
	}

    return -1;
}

static void intarith(int op, TValue* v1, TValue* v2) {
	switch (op) {
	case LUA_OPT_BAND:	arithint(&, v1, v2); break;
	case LUA_OPT_BOR:	arithint(|, v1, v2); break;
	case LUA_OPT_BXOR:	arithint(^, v1, v2); break;
	case LUA_OPT_BNOT:	v1->value_.i = ~v1->value_.i; break;
	case LUA_OPT_IDIV:  arithint(/, v1, v2); break;
	case LUA_OPT_SHL:	arithint(<<, v1, v2); break;
	case LUA_OPT_SHR:	arithint(>>, v1, v2); break;
	default:printf("intarith:unknow int op %c \n", op); break;
	}
}

static void numarith(int op, TValue* v1, TValue* v2) {
	switch (op) {
	case LUA_OPT_UMN: v1->value_.n = -v1->value_.n; break;
	case LUA_OPT_DIV: arithnum(/, v1, v2); break;
	case LUA_OPT_ADD: arithnum(+, v1, v2); break;
	case LUA_OPT_SUB: arithnum(-, v1, v2); break;
	case LUA_OPT_MUL: arithnum(*, v1, v2); break;
	case LUA_OPT_MOD: v1->value_.n = fmod(v1->value_.n, v2->value_.n); break;
	case LUA_OPT_POW: v1->value_.n = pow(v1->value_.n, v2->value_.n); break;
	default:printf("intarith:unknow int op %c \n", op); break;
	}
}

int luaO_arith(struct lua_State* L, int op, TValue* v1, TValue* v2) {
	switch (op) {
	case LUA_OPT_BAND: case LUA_OPT_BOR: case LUA_OPT_BXOR: case LUA_OPT_BNOT:
	case LUA_OPT_IDIV: case LUA_OPT_SHL: case LUA_OPT_SHR: {
		lua_Integer i1, i2;
		if (!luaV_tointeger(L, v1, &i1) || !luaV_tointeger(L, v2, &i2))
			return 0;

		setivalue(v1, i1);
		setivalue(v2, i2);

		intarith(op, v1, v2);
	} return 1;
	case LUA_OPT_UMN: case LUA_OPT_DIV: case LUA_OPT_ADD: case LUA_OPT_SUB:
	case LUA_OPT_MUL: case LUA_OPT_POW: case LUA_OPT_MOD: {
		lua_Number n1, n2;
		if (!luaV_tonumber(L, v1, &n1) || !luaV_tonumber(L, v2, &n2)) {
			return 0;
		}

		TValue nv1, nv2;
		setfltvalue(&nv1, n1);
		setfltvalue(&nv2, n2);

		numarith(op, &nv1, &nv2);

		setobj(v1, &nv1);
	} return 1;
	default: lua_assert(0);
	}

	return 0;
}
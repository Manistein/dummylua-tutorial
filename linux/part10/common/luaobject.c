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
#include "luastring.h"
#include "../vm/luagc.h"
#include "../vm/luado.h"
#include "luadebug.h"

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

static void intarith(struct lua_State* L, int op, TValue* v1, TValue* v2) {
	switch (op) {
	case LUA_OPT_BAND:	arithint(&, v1, v2); break;
	case LUA_OPT_BOR:	arithint(|, v1, v2); break;
	case LUA_OPT_BXOR:	arithint(^, v1, v2); break;
	case LUA_OPT_BNOT:	v1->value_.i = ~v1->value_.i; break;
	case LUA_OPT_IDIV:  arithint(/, v1, v2); break;
	case LUA_OPT_SHL:	arithint(<<, v1, v2); break;
	case LUA_OPT_SHR:	arithint(>>, v1, v2); break;
	default:luaG_runerror(L, "intarith:unknow int op %c \n", cast(char, op)); break;
	}
}

static void numarith(struct lua_State* L, int op, TValue* v1, TValue* v2) {
	switch (op) {
	case LUA_OPT_UMN: v1->value_.n = -v1->value_.n; break;
	case LUA_OPT_DIV: arithnum(/, v1, v2); break;
	case LUA_OPT_ADD: arithnum(+, v1, v2); break;
	case LUA_OPT_SUB: arithnum(-, v1, v2); break;
	case LUA_OPT_MUL: arithnum(*, v1, v2); break;
	case LUA_OPT_MOD: v1->value_.n = fmod(v1->value_.n, v2->value_.n); break;
	case LUA_OPT_POW: v1->value_.n = pow(v1->value_.n, v2->value_.n); break;
	default:luaG_runerror(L, "intarith:unknow int op %c \n", cast(char, op)); break;
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

		intarith(L, op, v1, v2);
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

		numarith(L, op, &nv1, &nv2);

		setobj(v1, &nv1);
	} return 1;
	default: lua_assert(0);
	}

	return 0;
}

int luaO_concat(struct lua_State* L, TValue* arg1, TValue* arg2, TValue* target) {
	if (!ttisshrstr(arg1) && !ttislngstr(arg1)) {
		return 0;
	}

	if (!ttisshrstr(arg2) && !ttislngstr(arg2)) {
		return 0;
	}

	TString* o1 = gco2ts(gcvalue(arg1));
	TString* o2 = gco2ts(gcvalue(arg2));

	char* s1 = getstr(o1);
	char* s2 = getstr(o2);
	int s1_sz = strlen(s1);
	int s2_sz = strlen(s2);

	char* new_s = (char*)malloc(s1_sz + s2_sz + 1);
	memcpy(new_s, s1, s1_sz);
	memcpy(new_s + s1_sz, s2, s2_sz);
	new_s[s1_sz + s2_sz] = '\0';
	TString* ts = luaS_new(L, new_s, s1_sz + s2_sz + 1);
	free(new_s);

	setgco(target, obj2gco(ts));

	return 1;
}

static void pushstr(struct lua_State* L, const char* str, unsigned int l) {
	TString* ts = luaS_newlstr(L, str, l);
	setgco(L->top, obj2gco(ts));
	increase_top(L);
}

TString* luaO_pushfvstring(struct lua_State* L, const char* fmt, va_list argp) {
	char buff[BUFSIZ];

	int n = 0;
	for (;;) {
		char* e = strchr(fmt, '%');
		if (e == NULL) break;
		pushstr(L, fmt, e - fmt);

		switch (*(e + 1)) {
		case 'd': {
			int intval = va_arg(argp, int);
			l_sprintf(buff, sizeof(buff), "%d", intval);
			pushstr(L, buff, strlen(buff));
		}break;
		case 'I': {
			lua_Integer lintval = va_arg(argp, lua_Integer);
			l_sprintf(buff, sizeof(buff), "%lld", lintval);
			pushstr(L, buff, strlen(buff));
		}break;
		case 's': {
			const char* str = va_arg(argp, const char*);
			pushstr(L, str, strlen(str));
		}break;
		case 'f':break;
			lua_Number fvalue = va_arg(argp, lua_Number);
			l_sprintf(buff, sizeof(buff), "%f", fvalue);
			pushstr(L, buff, strlen(buff));
		default:luaG_runerror(L, "unknow format type %c", cast(char, *(e + 1))); break;
		}

		n += 2;
		fmt = e + 2;
	}

	pushstr(L, fmt, strlen(fmt));
	n++;

	pushstr(L, "\n", 1);
	n++;

	if (n > 1) {
		for (int i = 1; i < n; i++) {
			luaO_concat(L, L->top - 2, L->top - 1, L->top - 2);
			L->top--;
		}
	}

	return gco2ts(gcvalue(L->top - 1));
}

TString* luaO_pushfstring(struct lua_State* L, const char* fmt, ...) {
	TString* msg = NULL;

	va_list argp;
	va_start(argp, fmt);
	msg = luaO_pushfvstring(L, fmt, argp);
	va_end(argp);

	luaC_checkgc(L);

	return msg;
}
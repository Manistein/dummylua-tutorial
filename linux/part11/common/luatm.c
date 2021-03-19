#include "luatm.h"
#include "luastate.h"
#include "luastring.h"
#include "luatable.h"
#include "luaobject.h"
#include "luadebug.h"
#include "../vm/luado.h"

static const char* s_tm[] = {
	"__lt", "__gt", "__le", "__ge", "__eq", "__concat",
	"__add", "__sub", "__mul", "__div", "__idiv", "__pow", "__band", "__bor", "__xor", "__shl", "__shr", "__mod",
	"__index", "__newindex", "__gc"
};

void luaT_init(struct lua_State* L) {
	for (int i = 0; i < LUA_NUMS; i++) {
		G(L)->mt[i] = NULL;
	}

	for (int i = 0; i < sizeof(s_tm) / sizeof(s_tm[0]); i++) {
		TString* ts = luaS_newliteral(L, s_tm[i]);
		G(L)->tmnames[i] = ts;
		luaC_fix(L, obj2gco(ts));
	}
}

void luaT_trycallbinTM(struct lua_State* L, TValue* lhs, TValue* rhs, TMS tms) {
	TValue* tm = luaT_gettmbyobj(L, lhs, tms);
	if (!tm || !ttisfunction(tm)) { // is tag method of lhs exist?
		tm = luaT_gettmbyobj(L, rhs, tms);
		if (!tm || !ttisfunction(tm)) {
			luaG_runerror(L, "rhs has not tag method %s", s_tm[(int)tms]);
		}
	}
	
	StkId func = L->top;
	setobj(func, tm);
	setobj(func + 1, lhs);
	setobj(func + 2, rhs);
	L->top += 3;

	int ret = 0;
	if (is_lua(func)) {
		ret = luaD_call(L, func, 1);
	}
	else {
		ret = luaD_callnoyield(L, func, 1);
	}

	if (ret != LUA_OK)
		luaG_runerror(L, "fail to call meta method %s", G(L)->tmnames[tms]);
}

TValue* luaT_gettmbyobj(struct lua_State* L, TValue* o, TMS event) {
	switch (novariant(o)) {
	case LUA_TTABLE: {
		if (hvalue(o)->metatable == NULL) {
			return NULL;
		}

		TValue* tm = (TValue*)luaH_getstr(L, hvalue(o)->metatable, luaS_newliteral(L, s_tm[(int)event]));
		return ttisnil(tm) ? NULL : tm;
	} break;
	case LUA_TUSERDATA: {
		if (uvalue(o)->metatable == NULL) {
			return NULL;
		}

		TValue* tm = (TValue*)luaH_getstr(L, uvalue(o)->metatable, luaS_newliteral(L, s_tm[(int)event]));
		return ttisnil(tm) ? NULL : tm;
	} break;
	default: {
		if (!G(L)->mt[novariant(o)]) {
			return NULL;
		}

		TValue* tm = (TValue*)luaH_getstr(L, G(L)->mt[novariant(o)], luaS_newliteral(L, s_tm[(int)event]));
		return ttisnil(tm) ? NULL : tm;
	} break;
	}

	return NULL;
}

void luaT_callTM(struct lua_State* L, TValue* f, TValue* p1, TValue* p2, TValue* p3, int hasres) {
	ptrdiff_t result = savestack(L, p3);

	StkId func = L->top;
	setobj(func, f);
	setobj(func + 1, p1);
	setobj(func + 2, p2);
	L->top += 3;

	if (!hasres) {
		setobj(func + 3, p3);
		L->top++;
	}

	if (is_lua(func)) {
		luaD_call(L, func, hasres);
	}
	else {
		luaD_callnoyield(L, func, hasres);
	}

	if (hasres) {
		p3 = restorestack(L, result);
		setobj(p3, L->top - 1);
		L->top--;
	}
}
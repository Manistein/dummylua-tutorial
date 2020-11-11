#include "luabase.h"
#include "../clib/luaaux.h"
#include "luastring.h"
#include "../vm/luagc.h"
#include "lua.h"
#include "../vm/luado.h"
#include "luatable.h"

#define MAX_NUMBER_STR_SIZE 64

static int lprint(struct lua_State* L) {
	int num_arg = lua_gettop(L);
	for (int i = 0; i < num_arg; i ++) {
		lua_getglobal(L, "tostring");
		lua_pushvalue(L, i + 1);
		luaL_pcall(L, 1, 1);
		char* s = lua_tostring(L, lua_gettop(L));
		lua_writestring(s);
		if (i < num_arg - 1) {
			lua_writestring(" ");
		}
	}
	lua_writeline();

	return 0;
}

static int ltostring(struct lua_State* L) {
	TValue* o = index2addr(L, lua_gettop(L));
	char max_number_buffer[MAX_NUMBER_STR_SIZE];
	memset(max_number_buffer, '\0', sizeof(max_number_buffer));

	switch (o->tt_)
	{
	case LUA_NUMINT: {
		l_sprintf(max_number_buffer, MAX_NUMBER_STR_SIZE - 1, LUA_INTEGER_FORMAT, o->value_.i);
		luaL_pushstring(L, max_number_buffer);
	} break;
	case LUA_NUMFLT: {
		l_sprintf(max_number_buffer, MAX_NUMBER_STR_SIZE, LUA_NUMBER_FORMAT, o->value_.n);
		luaL_pushstring(L, max_number_buffer);
	} break;
	case LUA_LNGSTR:
	case LUA_SHRSTR: {
		struct GCObject* gco = gcvalue(o);
		TString* s = gco2ts(gco);
		luaL_pushstring(L, getstr(s));
	} break;
	case LUA_TNIL: {
		luaL_pushstring(L, "nil");
	} break;
	case LUA_TBOOLEAN: {
		if (o->value_.b) {
			luaL_pushstring(L, "true");
		}
		else {
			luaL_pushstring(L, "false");
		}
	} break;
	case LUA_TLIGHTUSERDATA: {
		lua_Integer addr = (lua_Integer)o->value_.p;
		l_sprintf(max_number_buffer, MAX_NUMBER_STR_SIZE - 1, "%x", (unsigned int)addr);
		luaL_pushstring(L, max_number_buffer);
	} break;
	case LUA_TNONE: {
		luaL_pushstring(L, "(none)");
	} break;
	case LUA_TLCF: {
		lua_Integer addr = (lua_Integer)o->value_.f;
		l_sprintf(max_number_buffer, MAX_NUMBER_STR_SIZE - 1, "%x", (unsigned int)addr);
		luaL_pushstring(L, max_number_buffer);
	} break;
	default: {
		struct GCObject* gco = gcvalue(o);
		lua_Integer addr = (lua_Integer)gco;
		l_sprintf(max_number_buffer, MAX_NUMBER_STR_SIZE - 1, "%x", (unsigned int)addr);
		luaL_pushstring(L, max_number_buffer);
	} break;
	}

	return 1;
}

static int ipairs(struct lua_State* L) {
	StkId t = L->top - 2;
	if (!ttistable(t)) {
		printf("ipairs:target is not a table");
		luaD_throw(L, LUA_ERRRUN);
	}
	
	StkId key = L->top - 1;
	TValue* v = (TValue*)luaH_getint(L, gco2tbl(gcvalue(t)), key->value_.i + 1);
	if (ttisnil(v)) {
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else {
		lua_pushinteger(L, key->value_.i + 1);
		setobj(L->top, v);
		increase_top(L);
	}

	return 2;
}

static int luaB_ipairs(struct lua_State* L) {
	lua_pushvalue(L, -1); 
	setfvalue(L->top - 2, ipairs);
	lua_pushinteger(L, 0);

	return 3;
}

static int pairs(struct lua_State* L) {
	StkId t = L->top - 2;
	if (!ttistable(t)) {
		printf("pairs:target is not a table");
		luaD_throw(L, LUA_ERRRUN);
	}

	StkId key = L->top - 1;
	if (luaH_next(L, gco2tbl(gcvalue(t)), key) == LUA_OK) {
		increase_top(L);
		return 2;
	}
	else {
		setnilvalue(L->top - 1);
		return 1;
	}
}

static int luaB_pairs(struct lua_State* L) {
	lua_pushvalue(L, -1);
	setfvalue(L->top - 2, pairs);
	lua_pushnil(L);

	return 3;
}

const lua_Reg base_reg[] = {
	{ "print", lprint },
	{ "tostring", ltostring },
	{ "ipairs", luaB_ipairs },
	{ "pairs", luaB_pairs },
	{ NULL, NULL },
};

int luaB_openbase(struct lua_State* L) {
	lua_pushglobaltable(L);

	for (int i = 0; i < (sizeof(base_reg) / sizeof(base_reg[0])); i++) {
		if (base_reg[i].name && base_reg[i].func) {
			lua_pushstring(L, base_reg[i].name);
			lua_pushCclosure(L, base_reg[i].func, 1);
			lua_settable(L, -3);
		}
	}

	return 1;
}
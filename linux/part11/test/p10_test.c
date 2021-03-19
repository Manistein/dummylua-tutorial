#include "p10_test.h"
#include "../common/luastring.h"
#include "../vm/luagc.h"
#include "../common/luatable.h"

typedef struct Vector3 {
	float x;
	float y;
	float z;
} Vector3;

int gcfunc(struct lua_State* L) {
	Udata* u = lua_touserdata(L, -1);
	Vector3* v3 = (Vector3*)getudatamem(u);
	printf("total_size:%d x:%f, y:%f, z:%f", u->len, v3->x, v3->y, v3->z);
	return 0;
}

void test_create_object(struct lua_State* L) {
	Udata* u = luaS_newuserdata(L, sizeof(Vector3));

	Vector3* v3 = (Vector3*)getudatamem(u);
	v3->x = 10.0f;
	v3->y = 10.0f;
	v3->z = 10.0f;

	L->top->tt_ = LUA_TUSERDATA;
	L->top->value_.gc = obj2gco(u);
	increase_top(L);

	struct Table* t = luaH_new(L);
	struct GCObject* gco = obj2gco(t);
	TValue tv;
	tv.tt_ = LUA_TTABLE;
	tv.value_.gc = gco;
	setobj(L->top, &tv);
	increase_top(L);

	lua_pushCclosure(L, gcfunc, 0);
	lua_setfield(L, -2, "__gc");

	lua_setmetatable(L, -2);
	L->top--;

	return;
}

void p10_test_main() {
	struct lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	test_create_object(L);
	luaC_fullgc(L);

	luaL_close(L);
}
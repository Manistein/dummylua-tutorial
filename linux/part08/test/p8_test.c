#include "p8_test.h"
#include "../clib/luaaux.h"

void p8_test_main() {
	struct lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	int ok = luaL_loadfile(L, "../part08/scripts/part08_test.lua");
	if (ok == LUA_OK) {
		luaL_pcall(L, 0, 0);
	}

	luaL_close(L);
}
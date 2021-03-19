#include "p5_test.h"
#include "../common/lua.h"
#include "../clib/luaaux.h"

void p5_test_main() {
	struct lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	int ok = luaL_loadfile(L, "../dummylua/scripts/part05_test.lua");
	if (ok == LUA_OK) {
		luaL_pcall(L, 0, 0);
	}

	luaL_close(L);
}
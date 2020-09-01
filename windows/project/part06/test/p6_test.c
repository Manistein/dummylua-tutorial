#include "p6_test.h"
#include "../clib/luaaux.h"

void p6_test_main() {
	struct lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, "../part06/scripts/part06_test.lua");
}
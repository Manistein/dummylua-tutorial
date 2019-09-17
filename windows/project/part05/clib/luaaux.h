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

#ifndef luaaux_h
#define luaaux_h 

#include "../common/luastate.h"

struct lua_State* luaL_newstate();
void luaL_close(struct lua_State* L);

void luaL_pushinteger(struct lua_State* L, int integer);
void luaL_pushnumber(struct lua_State* L, float number);
void luaL_pushlightuserdata(struct lua_State* L, void* userdata);
void luaL_pushnil(struct lua_State* L);
void luaL_pushcfunction(struct lua_State* L, lua_CFunction f);
void luaL_pushboolean(struct lua_State* L, bool boolean);
void luaL_pushstring(struct lua_State* L, const char* str);
int luaL_pcall(struct lua_State* L, int narg, int nresult);

bool luaL_checkinteger(struct lua_State* L, int idx);
lua_Integer luaL_tointeger(struct lua_State* L, int idx);
lua_Number luaL_tonumber(struct lua_State* L, int idx);
void* luaL_touserdata(struct lua_State* L, int idx);
bool luaL_toboolean(struct lua_State* L, int idx);
char* luaL_tostring(struct lua_State* L, int index);
int luaL_isnil(struct lua_State* L, int idx);
TValue* luaL_index2addr(struct lua_State* L, int idx);

int luaL_createtable(struct lua_State* L);
int luaL_settable(struct lua_State* L, int idx);
int luaL_gettable(struct lua_State* L, int idx);
int luaL_getglobal(struct lua_State* L);

void luaL_pop(struct lua_State* L);
int luaL_stacksize(struct lua_State* L);

// load source and compile, if load success, then it returns 1, otherwise it returns 0 
int luaL_loadfile(struct lua_State* L, const char* filename);
#endif

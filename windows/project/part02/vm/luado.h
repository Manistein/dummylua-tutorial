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

#ifndef luado_h
#define luado_h 

#include "../common/luastate.h"

typedef int (*Pfunc)(struct lua_State* L, void* ud);

void seterrobj(struct lua_State* L, int error);
void luaD_checkstack(struct lua_State* L, int need);
void luaD_growstack(struct lua_State* L, int size);
void luaD_throw(struct lua_State* L, int error);

int luaD_rawrunprotected(struct lua_State* L, Pfunc f, void* ud);
int luaD_precall(struct lua_State* L, StkId func, int nresult);
int luaD_poscall(struct lua_State* L, StkId first_result, int nresult);
int luaD_call(struct lua_State* L, StkId func, int nresult);
int luaD_pcall(struct lua_State* L, Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef);

#endif 

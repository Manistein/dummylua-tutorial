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

#ifndef luafunc_h
#define luafunc_h

#include "../common/luaobject.h"

#define sizeofLClosure(n) (sizeof(LClosure) + sizeof(TValue*) * ((n) - 1))
#define sizeofCClosure(n) (sizeof(CClosure) + sizeof(TValue*) * ((n) - 1))
#define upisopen(up) (up->v != &up->u.value)

struct UpVal {
	TValue* v;  // point to stack or its own value (when open)
	int refcount;
	union {
		struct UpVal* next; // next open upvalue
		TValue value;		// its value (when closed)
	} u;
};

Proto* luaF_newproto(struct lua_State* L);
void luaF_freeproto(struct lua_State* L, Proto* f);
lu_mem luaF_sizeproto(struct lua_State* L, Proto* f);

LClosure* luaF_newLclosure(struct lua_State* L, int nup);
void luaF_freeLclosure(struct lua_State* L, LClosure* cl);

CClosure* luaF_newCclosure(struct lua_State* L, lua_CFunction func, int nup);
void luaF_freeCclosure(struct lua_State* L, CClosure* cc);

void luaF_initupvals(struct lua_State* L, LClosure* cl);

#endif
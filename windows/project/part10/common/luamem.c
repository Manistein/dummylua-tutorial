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

#include "luamem.h"
#include "../vm/luado.h"

#define MINARRAYSIZE 4

void* luaM_growaux_(struct lua_State* L, void* ptr, int* size, int element, int limit)
{
	void* newblock = NULL;
	int block_size = *size;
	if (block_size > limit / 2) {
		if (block_size > limit) {
			luaO_pushfstring(L, "%s", "luaM_growaux_ size too big");
			luaD_throw(L, LUA_ERRMEM);
		}

		block_size = limit;
	}
	else {
		block_size *= 2;
	}

	if (block_size <= MINARRAYSIZE) {
		block_size = MINARRAYSIZE;
	}

	newblock = luaM_realloc(L, ptr, *size * element, block_size * element);
	*size = block_size;
	return newblock;
}

void* luaM_realloc(struct lua_State* L, void* ptr, size_t osize, size_t nsize) {
    struct global_State* g = G(L);
    int oldsize = ptr ? osize : 0;

    void* ret = (*g->frealloc)(g->ud, ptr, oldsize, nsize);
    if (ret == NULL && nsize > 0) {
		luaO_pushfstring(L, "%s", "not enough memory");
        luaD_throw(L, LUA_ERRMEM);
    }

    g->GCdebt = g->GCdebt - oldsize + nsize;
    return ret;
}

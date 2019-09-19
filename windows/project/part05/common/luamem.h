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

#ifndef luamem_h
#define luamem_h 

#include "luastate.h"

#define luaM_free(L, ptr, osize) luaM_realloc(L, ptr, osize, 0)
#define luaM_reallocvector(L, ptr, o, n, t) ((ptr) = (t*)luaM_realloc(L, ptr, o * sizeof(t), n * sizeof(t)))  
#define luaM_newvector(L, n, t) luaM_realloc(L, NULL, 0, n * sizeof(t))
#define luaM_growvector(L, ptr, s, t, l) \
	if (s + 1 < l) { ptr = luaM_growaux_(L, ptr, &s, sizeof(t), l); } 

void* luaM_growaux_(struct lua_State* L, void* ptr, int* size, int element, int limit);
void* luaM_realloc(struct lua_State* L, void* ptr, size_t osize, size_t nsize);

#endif 

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

#ifndef luastring_h
#define luastring_h

#include "luastate.h"
#include "../vm/luagc.h"

#define sizelstring(l) (sizeof(TString) + (l + 1) * sizeof(char))
#define getstr(ts) (ts->data)
#define luaS_newliteral(L, s) luaS_newlstr(L, s, strlen(s))

void luaS_init(struct lua_State* L);
int luaS_resize(struct lua_State* L, unsigned int nsize); // only for short string
struct TString* luaS_newlstr(struct lua_State* L, const char* str, unsigned int l);
struct TString* luaS_new(struct lua_State* L, const char* str, unsigned int l);
void luaS_remove(struct lua_State* L, struct TString* ts); // remove TString from stringtable, only for short string

void luaS_clearcache(struct lua_State* L);
int luaS_eqshrstr(struct lua_State* L, struct TString* a, struct TString* b);
int luaS_eqlngstr(struct lua_State* L, struct TString* a, struct TString* b);

unsigned int luaS_hash(struct lua_State* L, const char* str, unsigned int l, unsigned int h);
unsigned int luaS_hashlongstr(struct lua_State* L, struct TString* ts);
struct TString* luaS_createlongstr(struct lua_State* L, const char* str, size_t l);

Udata* luaS_newuserdata(struct lua_State* L, int size);

#endif 

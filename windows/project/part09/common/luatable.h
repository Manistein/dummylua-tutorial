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

#ifndef luatable_h
#define luatable_h

#include "luastate.h"

#define isdummy(t) ((t)->lastfree == NULL)
#define getkey(n) (cast(const TValue*, &n->key.tvk)) 
#define getwkey(n) (cast(TValue*, &n->key.tvk))
#define getval(n) (cast(TValue*, &n->value))
#define getnode(t, i) (&(t)->node[i])

#define hashint(key, t) getnode(t, lmod(key, twoto(t->lsizenode))) 
#define hashstr(ts, t) getnode(t, lmod(ts->hash, twoto(t->lsizenode)))
#define hashboolean(key, t) getnode(t, lmod(key, twoto(t->lsizenode)))
#define hashpointer(p, t) getnode(t, lmod(point2uint(p), twoto(t->lsizenode)))

struct Table* luaH_new(struct lua_State* L);
void luaH_free(struct lua_State* L, struct Table* t);
// if luaH_getint return luaO_nilobject, that means key is not exist
const TValue* luaH_getint(struct lua_State* L, struct Table* t, lua_Integer key);
int luaH_setint(struct lua_State* L, struct Table* t, int key, const TValue* value);

const TValue* luaH_getshrstr(struct lua_State* L, struct Table* t, struct TString* key);
const TValue* luaH_getstr(struct lua_State* L, struct Table* t, struct TString* key);

const TValue* luaH_get(struct lua_State* L, struct Table* t, const TValue* key);
TValue* luaH_set(struct lua_State* L, struct Table* t, const TValue* key);

int luaH_resize(struct lua_State* L, struct Table* t, unsigned int asize, unsigned int hsize);
TValue* luaH_newkey(struct lua_State* L, struct Table* t, const TValue* key);

// 传入一个key，获得下一个key的key值和对应的value值，并且入栈
int luaH_next(struct lua_State* L, struct Table* t, TValue* key);
int luaH_getn(struct lua_State* L, struct Table* t);
#endif 

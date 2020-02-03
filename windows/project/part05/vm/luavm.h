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

#ifndef luavm_h
#define luavm_h

#include "../common/luastate.h"
#include "../common/luatable.h"

#define luaV_fastget(L, t, k, get, slot) \
    (!ttistable(t) ? (slot = NULL, 0) : \
     (slot = get(L, t, k), !ttisnil(slot)))

#define luaV_gettable(L, t, k, v) { const TValue* slot = NULL; \
    if (luaV_fastget(L, t, k, luaH_get, slot)) { setobj(v, cast(TValue*, slot));  } \
    else luaV_finishget(L, t, v, slot); }

#define luaV_fastset(L, t, k, v, get, slot) \
    (!ttistable(t) ? (slot = NULL, 0) : \
     (slot = cast(TValue*, get(L, t, k)), \
        (ttisnil(slot) ? (0) : \
            (setobj(slot, v), \
            luaC_barrierback(L, t, slot), 1))))

#define luaV_settable(L, t, k, v) { TValue* slot = NULL; \
    if (!luaV_fastset(L, t, k, v, luaH_get, slot)) \
        luaV_finishset(L, t, k, v, slot);}

void luaV_finishget(struct lua_State* L, struct Table* t, StkId val, const TValue* slot);
void luaV_finishset(struct lua_State* L, struct Table* t, const TValue* key, StkId val, const TValue* slot);
int luaV_eqobject(struct lua_State* L, const TValue* a, const TValue* b);

void luaV_execute(struct lua_State* L);

#endif

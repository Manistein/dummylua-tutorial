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

#include "luavm.h"
#include "../common/luastring.h"
#include "luado.h"
#include "luagc.h"


// slot == NULL意味着t不是table类型
void luaV_finishget(struct lua_State* L, struct Table* t, StkId val, const TValue* slot) {
    if (slot == NULL) {
        luaD_throw(L, LUA_ERRRUN);
    }
    else {
        setnilvalue(val);
    }
}

void luaV_finishset(struct lua_State* L, struct Table* t, const TValue* key, StkId val, const TValue* slot) {
    if (slot == NULL) {
        luaD_throw(L, LUA_ERRRUN);
    }

    if (slot == luaO_nilobject) {
        slot = luaH_newkey(L, t, key);
    }

    setobj(cast(TValue*, slot), val);
    luaC_barrierback(L, t, slot);
}

int luaV_eqobject(struct lua_State* L, const TValue* a, const TValue* b) {
    if ((ttisnumber(a) && lua_numisnan(a->value_.n)) || 
            (ttisnumber(b) && lua_numisnan(b->value_.n))) {
        return 0;
    }

    if (a->tt_ != b->tt_) {
        // 只有数值，在类型不同的情况下可能相等
        if (novariant(a) == LUA_TNUMBER && novariant(b) == LUA_TNUMBER) {
            double fa = ttisinteger(a) ? a->value_.i : a->value_.n;
            double fb = ttisinteger(b) ? b->value_.i : b->value_.n;
            return fa == fb;
        }
        else {
            return 0;
        }
    }    

    switch(a->tt_) {
        case LUA_TNIL: return 1;
        case LUA_NUMFLT: return a->value_.n == b->value_.n;
        case LUA_NUMINT: return a->value_.i == b->value_.i;
        case LUA_SHRSTR: return luaS_eqshrstr(L, gco2ts(gcvalue(a)), gco2ts(gcvalue(b)));
        case LUA_LNGSTR: return luaS_eqlngstr(L, gco2ts(gcvalue(a)), gco2ts(gcvalue(b)));
        case LUA_TBOOLEAN: return a->value_.b == b->value_.b; 
        case LUA_TLIGHTUSERDATA: return a->value_.p == b->value_.p; 
        case LUA_TLCF: return a->value_.f == b->value_.f;
        default: {
            return gcvalue(a) == gcvalue(b);
        } break;
    } 

    return 0;
}

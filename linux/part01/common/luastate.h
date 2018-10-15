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

#ifndef luastate_h
#define luastate_h 

#include "luaobject.h"

#define LUA_EXTRASPACE sizeof(void*) 
#define G(L) ((L)->l_G)

typedef TValue* StkId;

struct CallInfo {
    StkId func;
    StkId top;
    int nresult;
    int callstatus;
    struct CallInfo* next;
    struct CallInfo* previous;
};

typedef struct lua_State {
    StkId stack;
    StkId stack_last;
    StkId top;
    int stack_size;
    struct lua_longjmp* errorjmp;
    int status;
    struct lua_State* next;
    struct lua_State* previous;
    struct CallInfo base_ci;
    struct CallInfo* ci;
    struct global_State* l_G;
    ptrdiff_t errorfunc;
    int ncalls;
} lua_State;

typedef struct global_State {
    struct lua_State* mainthread;
    lua_Alloc frealloc;
    void* ud; 
    lua_CFunction panic;
} global_State;

struct lua_State* lua_newstate(lua_Alloc alloc, void* ud);
void lua_close(struct lua_State* L);

void setivalue(StkId target, int integer);
void setfvalue(StkId target, lua_CFunction f);
void setfltvalue(StkId target, float number);
void setbvalue(StkId target, bool b);
void setnilvalue(StkId target);
void setpvalue(StkId target, void* p);

void setobj(StkId target, StkId value);

void increase_top(struct lua_State* L);
void lua_pushcfunction(struct lua_State* L, lua_CFunction f);
void lua_pushinteger(struct lua_State* L, int integer);
void lua_pushnumber(struct lua_State* L, float number);
void lua_pushboolean(struct lua_State* L, bool b);
void lua_pushnil(struct lua_State* L);
void lua_pushlightuserdata(struct lua_State* L, void* p);

lua_Integer lua_tointegerx(struct lua_State* L, int idx, int* isnum);
lua_Number lua_tonumberx(struct lua_State* L, int idx, int* isnum);
bool lua_toboolean(struct lua_State* L, int idx);
int lua_isnil(struct lua_State* L, int idx);

void lua_settop(struct lua_State* L, int idx);
int lua_gettop(struct lua_State* L);
void lua_pop(struct lua_State* L);

#endif 

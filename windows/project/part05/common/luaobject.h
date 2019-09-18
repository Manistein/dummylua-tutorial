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

#ifndef luaobject_h
#define luaobject_h 

#include "lua.h"

typedef struct lua_State lua_State;

typedef LUA_INTEGER lua_Integer;
typedef LUA_NUMBER lua_Number;
typedef unsigned char lu_byte;
typedef int (*lua_CFunction)(lua_State* L);
typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);

// lua number type 
#define LUA_NUMINT (LUA_TNUMBER | (0 << 4))
#define LUA_NUMFLT (LUA_TNUMBER | (1 << 4))

// lua function type 
#define LUA_TLCL (LUA_TFUNCTION | (0 << 4))
#define LUA_TLCF (LUA_TFUNCTION | (1 << 4))
#define LUA_TCCL (LUA_TFUNCTION | (2 << 4))

// string type 
#define LUA_LNGSTR (LUA_TSTRING | (0 << 4))
#define LUA_SHRSTR (LUA_TSTRING | (1 << 4))

// GCObject
#define CommonHeader struct GCObject* next; lu_byte tt_; lu_byte marked
#define LUA_GCSTEPMUL 200

// Closure
#define ClosureHeader CommonHeader; int nupvalues; struct GCObject* gclist

#define luaO_nilobject (&luaO_nilobject_)
#define MAXSHORTSTR 40

#define dummynode (&dummynode_)
#define twoto(lsize) (1 << lsize)
#define lmod(hash, size) check_exp((size) & (size - 1) == 0, (hash) & (size - 1))

#define lua_numeq(a, b) ((a) == (b))
#define lua_numisnan(a) (!lua_numeq(a, a))
#define lua_numbertointeger(n, p) \
    (n >= cast(lua_Number, INT_MIN)) && \
    (n <= cast(lua_Number, INT_MAX)) && \
    ((*p = cast(lua_Integer, n)), 1)

#define ttisinteger(o) ((o)->tt_ == LUA_NUMINT)
#define ttisnumber(o) ((o)->tt_ == LUA_NUMFLT)
#define ttisshrstr(o) ((o)->tt_ == LUA_SHRSTR)
#define ttislngstr(o) ((o)->tt_ == LUA_LNGSTR)
#define ttisdeadkey(o) ((o)->tt_ == LUA_TDEADKEY)
#define ttistable(o) ((o)->tt_ == LUA_TTABLE)
#define ttisnil(o) ((o)->tt_ == LUA_TNIL)

struct GCObject {
    CommonHeader;
};

typedef union lua_Value {
    struct GCObject* gc;
    void* p;
    int b;
    lua_Integer i;
    lua_Number n;
    lua_CFunction f;
} Value;

typedef struct lua_TValue {
    Value value_;
    int tt_;
} TValue;

extern const TValue luaO_nilobject_;

typedef struct TString {
    CommonHeader;
    unsigned int hash;          // string hash value

    // if TString is long string type, then extra = 1 means it has been hash, 
    // extra = 0 means it has not hash yet. if TString is short string type,
    // then extra = 0 means it can be reclaim by gc, or if extra is not 0,
    // gc can not reclaim it.
    unsigned short extra;       
    unsigned short shrlen;
    union {
        struct TString* hnext; // only for short string, if two different string encounter hash collision, then chain them
        size_t lnglen;
    } u;
    char data[0];
} TString;

// lua Table
typedef union TKey {
    struct {
        Value value_;
        int tt_;
        int next;
    } nk;
    TValue tvk;
} TKey;

typedef struct Node {
    TKey key;
    TValue value;
} Node;

const Node dummynode_;

struct Table {
    CommonHeader;
    TValue* array;
    unsigned int arraysize;
    Node* node;
    unsigned int lsizenode; // real hash size is 2 ^ lsizenode
    Node* lastfree;
    struct GCObject* gclist;
};

// compiler and vm structs
typedef struct LocVar {
    TString* varname;
    int startpc;
    int endpc;
} LocVar;

typedef struct Upvaldesc {
    int in_stack;
    int idx;
    TString* name;
} Upvaldesc;

typedef struct Proto {
    CommonHeader;
    int is_vararg;
    int nparam;
    Instruction* code;  // array of opcodes
    int sizecode;
    TValue* k;
    int sizek;
    LocVar* locvars;
    int sizelocvar;
    Upvaldesc* upvalues;
    int sizeupvalues;
    struct Proto** p;
    int sizep;
    TString* source;
    struct GCObject* gclist;
} Proto;

typedef struct LClosure {
    ClosureHeader;
    Proto* p;
    TValue* upval[1];
} LClosure;

typedef union Closure {
	LClosure l;
} Closure;

int luaO_ceillog2(int value);

#endif 

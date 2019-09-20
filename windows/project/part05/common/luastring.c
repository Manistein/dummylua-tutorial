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

#include "luastring.h"
#include "luamem.h"
#include "luaobject.h"

#define MINSTRTABLESIZE 128

#define MEMERRMSG "not enough memory"

void luaS_init(struct lua_State* L) {
    struct global_State* g = G(L);
    g->strt.nuse = 0;
    g->strt.size = 0;
    g->strt.hash = NULL;
    luaS_resize(L, MINSTRTABLESIZE);
    g->memerrmsg = luaS_newlstr(L, MEMERRMSG, strlen(MEMERRMSG)); 
    luaC_fix(L, obj2gco(g->memerrmsg));

    // strcache table can not hold the string objects which will be sweep soon
    for (int i = 0; i < STRCACHE_M; i ++) {
        for (int j = 0; j < STRCACHE_N; j ++)
            g->strcache[i][j] = g->memerrmsg;
    }
}

// attention please, luaS_resize is use for resize stringtable type,
// which is only use by short string. The second argument usually must 
// be 2 ^ n
int luaS_resize(struct lua_State* L, unsigned int nsize) {
    struct global_State* g = G(L);
    unsigned int osize = g->strt.size;
    if (nsize > osize) {
        luaM_reallocvector(L, g->strt.hash, osize, nsize, TString*);
        for (int i = osize; i < nsize; i ++) {
            g->strt.hash[i] = NULL;
        }
    }

    // all TString value will be rehash by nsize
    for (int i = 0; i < g->strt.size; i ++) {
       struct TString* ts = g->strt.hash[i];
       g->strt.hash[i] = NULL;

       while(ts) {
            struct TString* old_next = ts->u.hnext;
            unsigned int hash = lmod(ts->hash, nsize);
            ts->u.hnext = g->strt.hash[hash];
            g->strt.hash[hash] = ts;
            ts = old_next;
       }
    }

    // shrink string hash table 
    if (nsize < osize) {
        lua_assert(g->strt.hash[nsize] == NULL && g->strt.hash[osize - 1] == NULL);
        luaM_reallocvector(L, g->strt.hash, osize, nsize, TString*);
    }
    g->strt.size = nsize;

    return g->strt.size;
}

static struct TString* createstrobj(struct lua_State* L, const char* str, int tag, unsigned int l, unsigned int hash) {
    size_t total_size = sizelstring(l);

    struct GCObject* o = luaC_newobj(L, tag, total_size);
    struct TString* ts = gco2ts(o);
    memcpy(getstr(ts), str, l * sizeof(char));
    getstr(ts)[l] = '\0';
    ts->extra = 0;

    if (tag == LUA_SHRSTR) {
        ts->shrlen = cast(lu_byte, l);
        ts->hash = hash;
        ts->u.hnext = NULL;
    }
    else if (tag == LUA_LNGSTR) {
        ts->hash = 0;
        ts->u.lnglen = l;
    }
    else {
        lua_assert(0);
    }
    
    return ts;
}

// only short strings can be interal
static struct TString* internalstr(struct lua_State* L, const char* str, unsigned int l) {
    struct global_State* g = G(L);
    struct stringtable* tb = &g->strt;
    unsigned int h = luaS_hash(L, str, l, g->seed); 
    struct TString** list = &tb->hash[lmod(h, tb->size)];

    for (struct TString* ts = *list; ts; ts = ts->u.hnext) {
        if (ts->shrlen == l && (memcmp(getstr(ts), str, l * sizeof(char)) == 0)) {
            if (isdead(g, ts)) {
                changewhite(ts);
            }
            return ts;
        }
    }

    if (tb->nuse >= tb->size && tb->size < INT_MAX / 2) {
        luaS_resize(L, tb->size * 2);
        list = &tb->hash[lmod(h, tb->size)];
    }

    struct TString* ts = createstrobj(L, str, LUA_SHRSTR, l, h);
    ts->u.hnext = *list;
    *list = ts;
    tb->nuse++;

    return ts;
}

struct TString* luaS_newlstr(struct lua_State* L, const char* str, unsigned int l) {
    if (l <= MAXSHORTSTR) {
        return internalstr(L, str, l); 
    }
    else {
        return luaS_createlongstr(L, str, l); 
    }
}

struct TString* luaS_new(struct lua_State* L, const char* str, unsigned int l) {
    unsigned int hash = point2uint(str); 
    int i = hash % STRCACHE_M;
    for (int j = 0; j < STRCACHE_N; j ++) {
        struct TString* ts = G(L)->strcache[i][j];
        if (strcmp(getstr(ts), str) == 0) {
            return ts;
        }
    }

    for (int j = STRCACHE_N - 1; j > 0; j--) {
        G(L)->strcache[i][j] = G(L)->strcache[i][j - 1];
    }

    G(L)->strcache[i][0] = luaS_newlstr(L, str, l);
    return G(L)->strcache[i][0];
}

// remove TString from stringtable, only for short string
void luaS_remove(struct lua_State* L, struct TString* ts) {
    struct global_State* g = G(L);
    struct TString** list = &g->strt.hash[lmod(ts->hash, g->strt.size)];

    for (struct TString* o = *list; o; o = o->u.hnext) {
        if (o == ts) {
            *list = o->u.hnext;
            break;
        }
        list = &(*list)->u.hnext;
    }
}

void luaS_clearcache(struct lua_State* L) {
    struct global_State* g = G(L);
    for (int i = 0; i < STRCACHE_M; i ++) {
        for (int j = 0; j < STRCACHE_N; j ++) {
            if (iswhite(g->strcache[i][j])) {
                g->strcache[i][j] = g->memerrmsg;
            }
        }
    }
}

int luaS_eqshrstr(struct lua_State* L, struct TString* a, struct TString* b) {
    return (a == b) || (a->shrlen == b->shrlen && strcmp(getstr(a), getstr(b)) == 0);
}

int luaS_eqlngstr(struct lua_State* L, struct TString* a, struct TString* b) {
    return (a == b) || (a->u.lnglen == b->u.lnglen && strcmp(getstr(a), getstr(b)) == 0);
}

unsigned int luaS_hash(struct lua_State* L, const char* str, unsigned int l, unsigned int h) {
    h = h ^ l;
    unsigned int step = (l >> 5) + 1;
    for (int i = 0; i < l; i = i + step) {
        h ^= (h << 5) + (h >> 2) + cast(lu_byte, str[i]);
    }
    return h;
}

unsigned int luaS_hashlongstr(struct lua_State* L, struct TString* ts) {
    if (ts->extra == 0) {
        ts->hash = luaS_hash(L, getstr(ts), ts->u.lnglen, G(L)->seed);
        ts->extra = 1;
    }
    return ts->hash;
}

struct TString* luaS_createlongstr(struct lua_State* L, const char* str, size_t l) {
    return createstrobj(L, str, LUA_LNGSTR, l, G(L)->seed);
}

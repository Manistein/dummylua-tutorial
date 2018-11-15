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

#include "luagc.h"
#include "../common/luamem.h"

#define GCMAXSWEEPGCO 25

#define gettotalbytes(g) (g->totalbytes + g->GCdebt)
#define white2gray(o) resetbits((o)->marked, WHITEBITS)
#define gray2black(o) l_setbit((o)->marked, BLACKBIT)
#define black2gray(o) resetbit((o)->marked, BLACKBIT)
#define sweepwholelist(L, list) sweeplist(L, list, MAX_LUMEM)

struct GCObject* luaC_newobj(struct lua_State* L, int tt_, size_t size) {
    struct global_State* g = G(L);
    struct GCObject* obj = (struct GCObject*)luaM_realloc(L, NULL, 0, size);
    obj->marked = luaC_white(g);
    obj->next = g->allgc;
    obj->tt_ = tt_;
    g->allgc = obj;

    return obj;
}

void reallymarkobject(struct lua_State* L, struct GCObject* gco) {
    struct global_State* g = G(L);
    white2gray(gco);

    switch(gco->tt_) {
        case LUA_TTHREAD:{
            linkgclist(gco2th(gco), g->gray);            
        } break;
        case LUA_TSTRING:{ // just for gc test now
            gray2black(gco);
            g->GCmemtrav += sizeof(struct TString);
        } break;
        default:break;
    }
}

static l_mem get_debt(struct lua_State* L) {
    struct global_State* g = G(L);
    int stepmul = g->GCstepmul; 
    l_mem debt = g->GCdebt;
    if (debt <= 0) {
        return 0;
    }

    debt = debt / STEPMULADJ + 1;
    debt = debt >= (MAX_LMEM / STEPMULADJ) ? MAX_LMEM : debt * g->GCstepmul;

    return debt; 
}

// mark root
static void restart_collection(struct lua_State* L) {
    struct global_State* g = G(L);
    g->gray = g->grayagain = NULL;
    markobject(L, g->mainthread); 
}

static lu_mem traversethread(struct lua_State* L, struct lua_State* th) {
    TValue* o = th->stack;
    for (; o < th->top; o ++) {
        markvalue(L, o);
    }

    return sizeof(struct lua_State) + sizeof(TValue) * th->stack_size + sizeof(struct CallInfo) * th->nci;
}

static void propagatemark(struct lua_State* L) {
    struct global_State* g = G(L);
    if (!g->gray) {
        return;
    }
    struct GCObject* gco = g->gray;
    gray2black(gco);
    lu_mem size = 0;

    switch(gco->tt_) {
        case LUA_TTHREAD:{
            black2gray(gco);
            struct lua_State* th = gco2th(gco);
            g->gray = th->gclist;
            linkgclist(th, g->grayagain);
            size = traversethread(L, th);
        } break;
        default:break;
    }

    g->GCmemtrav += size;
}

static void propagateall(struct lua_State* L) {
    struct global_State* g = G(L);
    while(g->gray) {
        propagateall(L);
    }
}

static void atomic(struct lua_State* L) {
    struct global_State* g = G(L);
    g->gray = g->grayagain;
    g->grayagain = NULL;

    g->gcstate = GCSinsideatomic;
    propagateall(L);
    g->currentwhite = cast(lu_byte, otherwhite(g));
}

static lu_mem freeobj(struct lua_State* L, struct GCObject* gco) {
    switch(gco->tt_) {
        case LUA_TSTRING: {
            lu_mem sz = sizeof(TString);
            luaM_free(L, gco, sz); 
            return sz; 
        } break;
        default:{
            lua_assert(0);
        } break;
    }
    return 0;
}

static struct GCObject** sweeplist(struct lua_State* L, struct GCObject** p, size_t count) {
    struct global_State* g = G(L);
    lu_byte ow = otherwhite(g);
    while (*p != NULL && count > 0) {
        lu_byte marked = (*p)->marked;
        if (isdeadm(ow, marked)) {
            struct GCObject* gco = *p;
            *p = (*p)->next;
            g->GCmemtrav += freeobj(L, gco);
        } 
        else {
            (*p)->marked &= cast(lu_byte, ~(bitmask(BLACKBIT) | WHITEBITS));
            (*p)->marked |= luaC_white(g);
            p = &((*p)->next);
        }
        count --; 
    }
    return (*p) == NULL ? NULL : p; 
}

static void entersweep(struct lua_State* L) {
    struct global_State* g = G(L);
    g->gcstate = GCSsweepallgc; 
    g->sweepgc = sweeplist(L, &g->allgc, 1);
}

static void sweepstep(struct lua_State* L) {
    struct global_State* g = G(L);
    if (g->sweepgc) {
        g->sweepgc = sweeplist(L, g->sweepgc, GCMAXSWEEPGCO);
        g->GCestimate = gettotalbytes(g);

        if (g->sweepgc) {
            return;
        }
    }
    g->gcstate = GCSsweepend;
    g->sweepgc = NULL;
}

static lu_mem singlestep(struct lua_State* L) {
    struct global_State* g = G(L);
    switch(g->gcstate) {
        case GCSpause: {
            g->GCmemtrav = 0;
            restart_collection(L);
            g->gcstate = GCSpropagate;
            return g->GCmemtrav;
        } break;
        case GCSpropagate:{
            g->GCmemtrav = 0;
            propagatemark(L);
            if (g->gray == NULL) {
                g->gcstate = GCSatomic;
            }
            return g->GCmemtrav;
        } break;
        case GCSatomic:{
            g->GCmemtrav = 0;
            if (g->gray) {
                propagateall(L);
            }
            atomic(L);
            entersweep(L);
            g->GCestimate = gettotalbytes(g);
            return g->GCmemtrav;
        } break;
        case GCSsweepallgc: {
            g->GCmemtrav = 0;
            sweepstep(L);
            return g->GCmemtrav;
        } break;
        case GCSsweepend: {
            g->GCmemtrav = 0;
            g->gcstate = GCSpause;
            return 0;
        } break;
        default:break;
    }

    return g->GCmemtrav;
}

static void setdebt(struct lua_State* L, l_mem debt) {
    struct global_State* g = G(L);
    lu_mem totalbytes = gettotalbytes(g);

    g->totalbytes = totalbytes - debt;
    g->GCdebt = debt;
}

// when memory is twice bigger than current estimate, it will trigger gc
// again
static void setpause(struct lua_State* L) {
    struct global_State* g = G(L);
    l_mem estimate = g->GCestimate / GCPAUSE;
    estimate = (estimate * g->GCstepmul) >= MAX_LMEM ? MAX_LMEM : estimate * g->GCstepmul;
    
    l_mem debt = g->GCestimate - estimate;
    setdebt(L, debt);
}

void luaC_step(struct lua_State*L) {
    struct global_State* g = G(L);
    l_mem debt = get_debt(L);
    do {
        l_mem work = singlestep(L);
        debt -= work;
    }while(debt > -GCSTEPSIZE && G(L)->gcstate != GCSpause);
    
    if (G(L)->gcstate == GCSpause) {
        setpause(L);
    }
    else {
        debt = g->GCdebt / STEPMULADJ * g->GCstepmul;
        setdebt(L, debt);
    }
}

void luaC_freeallobjects(struct lua_State* L) {
    struct global_State* g = G(L);
    g->currentwhite = WHITEBITS; // all gc objects must reclaim
    sweepwholelist(L, &g->allgc);
}

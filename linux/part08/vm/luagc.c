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
#include "../common/luastring.h"
#include "../common/luatable.h"
#include "luafunc.h"

#define GCMAXSWEEPGCO 25

#define gettotalbytes(g) (g->totalbytes + g->GCdebt)
#define white2gray(o) resetbits((o)->marked, WHITEBITS)
#define gray2black(o) l_setbit((o)->marked, BLACKBIT)
#define black2gray(o) resetbit((o)->marked, BLACKBIT)
#define sweepwholelist(L, list) sweeplist(L, list, MAX_LUMEM)
#define makewhite(o) \
	(o)->marked &= cast(lu_byte, ~(bitmask(BLACKBIT) | WHITEBITS)); \
	(o)->marked |= luaC_white(g)

struct GCObject* luaC_newobj(struct lua_State* L, lu_byte tt_, size_t size) {
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
        case LUA_TTABLE:{
            linkgclist(gco2tbl(gco), g->gray);
        } break;
		case LUA_TLCL:{
			linkgclist(gco2lclosure(gco), g->gray);
		} break;
		case LUA_TCCL:{
			linkgclist(gco2cclosure(gco), g->gray);
		} break;
		case LUA_TPROTO:{
			linkgclist(gco2proto(gco), g->gray);
		} break;
        case LUA_SHRSTR:{ 
            gray2black(gco);
            struct TString* ts = gco2ts(gco);
            g->GCmemtrav += sizelstring(ts->shrlen);
        } break;
        case LUA_LNGSTR:{
            gray2black(gco);
            struct TString* ts = gco2ts(gco);
            g->GCmemtrav += sizelstring(ts->u.lnglen);
        } break;
        default:break;
    }
}

static l_mem get_debt(struct lua_State* L) {
    struct global_State* g = G(L);
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
    markvalue(L, &g->l_registry);
}

static lu_mem traverse_thread(struct lua_State* L, struct lua_State* th) {
    TValue* o = th->stack;
    for (; o < th->top; o ++) {
        markvalue(L, o);
    }

    return sizeof(struct lua_State) + sizeof(TValue) * th->stack_size + sizeof(struct CallInfo) * th->nci;
}

static lu_mem traverse_strong_table(struct lua_State* L, struct Table* t) {
    for (int i = 0; i < (int)t->arraysize; i++) {
        markvalue(L, &t->array[i]); 
    }

    for (int i = 0; i < twoto(t->lsizenode); i++) {
        Node* n = getnode(t, i);
        if (ttisnil(getval(n))) {
            TValue* deadkey = getwkey(n);
            deadkey->tt_ = LUA_TDEADKEY;
        }
        else {
            markvalue(L, getwkey(n));
            markvalue(L, getval(n));
        }
    }

    return sizeof(struct Table) + sizeof(TValue) * t->arraysize + sizeof(Node) * twoto(t->lsizenode);
}

static lu_mem traverse_proto(struct lua_State* L, struct Proto* p) {
	if (p->source) {
		markobject(L, p->source);
	}

	for (int i = 0; i < p->sizek; i++) {
		markvalue(L, &p->k[i]);
	}

	for (int i = 0; i < p->sizelocvar; i++) {
		if (p->locvars[i].varname) {
			markobject(L, p->locvars[i].varname);
		}
	}

	for (int i = 0; i < p->sizeupvalues; i++) {
		if (p->upvalues[i].name) {
			markobject(L, p->upvalues[i].name);
		}
	}

	for (int i = 0; i < p->sizep; i++) {
		if (p->p[i]) {
			markobject(L, p->p[i]);
		}
	}

	return luaF_sizeproto(L, p);
}

static lu_mem traverse_lclosure(struct lua_State* L, struct LClosure* cl) {
	markobject(L, cl->p);

	// TODO process upvalues;
	return sizeof(struct LClosure);
}

static lu_mem traverse_cclosure(struct lua_State* L, struct CClosure* cc) {
	// TODO process upvalues
	return sizeof(struct CClosure);
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
            struct lua_State* th = gco2th(gco);
            g->gray = th->gclist;
			if (g->mainthread == th) {
				black2gray(gco);
				linkgclist(th, g->grayagain);
			}
            size = traverse_thread(L, th);
        } break;
        case LUA_TTABLE:{
            struct Table* t = gco2tbl(gco);
            g->gray = t->gclist;
            size = traverse_strong_table(L, t);
        } break;
		case LUA_TLCL:{
			struct LClosure* cl = gco2lclosure(gco);
			g->gray = cl->gclist;
			size = traverse_lclosure(L, cl);
		} break;
		case LUA_TCCL:{
			struct CClosure* cc = gco2cclosure(gco);
			g->gray = cc->gclist;
			size = traverse_cclosure(L, cc);
		} break;
		case LUA_TPROTO: {
			struct Proto* f = gco2proto(gco);
			g->gray = f->gclist;
			size = traverse_proto(L, f);
		} break;
        default:break;
    }

    g->GCmemtrav += size;
}

static void propagateall(struct lua_State* L) {
    struct global_State* g = G(L);
    while(g->gray) {
        propagatemark(L);
    }
}

static void atomic(struct lua_State* L) {
    struct global_State* g = G(L);
    g->gray = g->grayagain;
    g->grayagain = NULL;

    g->gcstate = GCSinsideatomic;
    propagateall(L);
    g->currentwhite = cast(lu_byte, otherwhite(g));

    luaS_clearcache(L);
}

static lu_mem freeobj(struct lua_State* L, struct GCObject* gco) {
    switch(gco->tt_) {
        case LUA_SHRSTR: {
            struct TString* ts = gco2ts(gco);
            luaS_remove(L, ts);
            lu_mem sz = sizelstring(ts->shrlen);
            luaM_free(L, ts, sz); 
            return sz; 
        } break;
        case LUA_LNGSTR: {
            struct TString* ts = gco2ts(gco);
            lu_mem sz = sizelstring(ts->u.lnglen);
            luaM_free(L, ts, sz);
        } break;
        case LUA_TTABLE: {
            struct Table* tbl = gco2tbl(gco);
            lu_mem sz = sizeof(struct Table) + tbl->arraysize * sizeof(TValue) + twoto(tbl->lsizenode) * sizeof(Node);
            luaH_free(L, tbl);
            return sz;
        } break;
		case LUA_TTHREAD: {
			// TODO
		} break;
		case LUA_TLCL: {
			struct LClosure* cl = gco2lclosure(gco);
			lu_mem sz = sizeof(LClosure);
			luaF_freeLclosure(L, cl);
			return sz;
		} break;
		case LUA_TCCL: {
			struct CClosure* cc = gco2cclosure(gco);
			lu_mem sz = sizeof(struct CClosure);
			luaF_freeCclosure(L, cc);
			return sz;
		} break;
		case LUA_TPROTO: {
			struct Proto* f = gco2proto(gco);
			lu_mem sz = luaF_sizeproto(L, f);
			luaF_freeproto(L, f);
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
			makewhite(g->mainthread);
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
        debt = debt / g->GCstepmul * STEPMULADJ;
        setdebt(L, debt);
    }
}

void luaC_fix(struct lua_State* L, struct GCObject* o) {
    struct global_State* g = G(L);
    lua_assert(g->allgc == o);

    g->allgc = g->allgc->next;
    o->next = g->fixgc;
    g->fixgc = o;
    white2gray(o);
}

void luaC_barrierback_(struct lua_State* L, struct Table* t, const TValue* o) {
    struct global_State* g = G(L);
    lua_assert(isblack(t) && iswhite(o));
    black2gray(t);
    linkgclist(t, g->grayagain);
}


void luaC_freeallobjects(struct lua_State* L) {
    struct global_State* g = G(L);
    g->currentwhite = WHITEBITS; // all gc objects must reclaim
    sweepwholelist(L, &g->allgc);
    sweepwholelist(L, &g->fixgc);
}

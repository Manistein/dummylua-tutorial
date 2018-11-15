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

#include "luado.h"
#include "../common/luamem.h"

#define LUA_TRY(L, c, a) if (_setjmp((c)->b) == 0) { a } 

#ifdef _WINDOWS_PLATFORM_ 
#define LUA_THROW(c) longjmp((c)->b, 1) 
#else 
#define LUA_THROW(c) _longjmp((c)->b, 1) 
#endif 

struct lua_longjmp {
    struct lua_longjmp* previous;
    jmp_buf b;
    int status;
};

void seterrobj(struct lua_State* L, int error) {
    lua_pushinteger(L, error);
}

void luaD_checkstack(struct lua_State* L, int need) {
    if (L->top + need > L->stack_last) {
        luaD_growstack(L, need);
    }
}

void luaD_growstack(struct lua_State* L, int size) {
    if (L->stack_size > LUA_MAXSTACK) {
        luaD_throw(L, LUA_ERRERR);
    }

    int stack_size = L->stack_size * 2;
    int need_size = cast(int, L->top - L->stack) + size + LUA_EXTRASTACK;
    if (stack_size < need_size) {
        stack_size = need_size;
    }
    
    if (stack_size > LUA_MAXSTACK) {
        stack_size = LUA_MAXSTACK + LUA_ERRORSTACK;
        LUA_ERROR(L, "stack overflow");
    } 

    TValue* old_stack = L->stack;
    L->stack = luaM_realloc(L, L->stack, L->stack_size, stack_size * sizeof(TValue));
    L->stack_size = stack_size;
    L->stack_last = L->stack + stack_size - LUA_EXTRASTACK;
    int top_diff = cast(int, L->top - old_stack);
    L->top = restorestack(L, top_diff);

    struct CallInfo* ci;
    ci = &L->base_ci;
    while(ci) {
        int func_diff = cast(int, ci->func - old_stack);
        int top_diff = cast(int, ci->top - old_stack);
        ci->func = restorestack(L, func_diff);
        ci->top = restorestack(L, top_diff);

        ci = ci->next;
    }
}

void luaD_throw(struct lua_State* L, int error) {
    struct global_State* g = G(L);
    if (L->errorjmp) {
        L->errorjmp->status = error;
        LUA_THROW(L->errorjmp);
    }
    else {
        if (g->panic) {
            (*g->panic)(L);
        }
        abort();
    }
}

int luaD_rawrunprotected(struct lua_State* L, Pfunc f, void* ud) {
    int old_ncalls = L->ncalls;
    struct lua_longjmp lj;
    lj.previous = L->errorjmp;
    lj.status = LUA_OK;
    L->errorjmp = &lj;

    LUA_TRY(
        L,
        L->errorjmp,
        (*f)(L, ud);
    )

    L->errorjmp = lj.previous;
    L->ncalls = old_ncalls;
    return lj.status;
}

static struct CallInfo* next_ci(struct lua_State* L, StkId func, int nresult) {
    struct global_State* g = G(L);
    struct CallInfo* ci;

    if (L->ci->next) {
        ci = L->ci->next;
    }
    else {
        ci = luaM_realloc(L, NULL, 0, sizeof(struct CallInfo));
        ci->next = NULL;
        L->nci++;
    }
    
    ci->previous = L->ci;
    L->ci->next = ci;
    ci->nresult = nresult;
    ci->callstatus = LUA_OK;
    ci->func = func;
    ci->top = L->top + LUA_MINSTACK;
    L->ci = ci;

    return ci;
}

// prepare for function call. 
// if we call a c function, just directly call it
// if we call a lua function, just prepare for call it
int luaD_precall(struct lua_State* L, StkId func, int nresult) {
    switch(func->tt_) {
        case LUA_TLCF: {
            lua_CFunction f = func->value_.f;

            ptrdiff_t func_diff = savestack(L, func);
            luaD_checkstack(L, LUA_MINSTACK);
            func = restorestack(L, func_diff);

            next_ci(L, func, nresult);                        
            int n = (*f)(L);
            assert(L->ci->func + n <= L->ci->top);
            luaD_poscall(L, L->top - n, n);
            return 1; 
        } break;
        default:break;
    }

    return 0;
}

int luaD_poscall(struct lua_State* L, StkId first_result, int nresult) {
    StkId func = L->ci->func;
    int nwant = L->ci->nresult;

    switch(nwant) {
        case 0: {
            L->top = L->ci->func;
        } break;
        case 1: {
            if (nresult == 0) {
                first_result->value_.p = NULL;
                first_result->tt_ = LUA_TNIL;
            }
            setobj(func, first_result);
            first_result->value_.p = NULL;
            first_result->tt_ = LUA_TNIL;

            L->top = func + nwant;
        } break;
        case LUA_MULRET: {
            int nres = cast(int, L->top - first_result);
            int i;
            for (i = 0; i < nres; i++) {
                StkId current = first_result + i;
                setobj(func + i, current);
                current->value_.p = NULL;
                current->tt_ = LUA_TNIL;
            }
            L->top = func + nres;
        } break;
        default: {
            if (nwant > nresult) {
                int i;
                for (i = 0; i < nwant; i++) {
                    if (i < nresult) {
                        StkId current = first_result + i;
                        setobj(func + i, current);
                        current->value_.p = NULL;
                        current->tt_ = LUA_TNIL;
                    }
                    else {
                        StkId stack = func + i;
                        stack->tt_ = LUA_TNIL;
                    }
                }
                L->top = func + nwant;
            }
            else {
                int i;
                for (i = 0; i < nresult; i++) {
                    if (i < nwant) {
                        StkId current = first_result + i;
                        setobj(func + i, current);
                        current->value_.p = NULL;
                        current->tt_ = LUA_TNIL;
                    }
                    else {
                        StkId stack = func + i;
                        stack->value_.p = NULL;
                        stack->tt_ = LUA_TNIL; 
                    }
                }
                L->top = func + nresult;
            } 
        } break;
    }

    struct CallInfo* ci = L->ci;
    L->ci = ci->previous;
    
    return LUA_OK;
}

int luaD_call(struct lua_State* L, StkId func, int nresult) {
    if (++L->ncalls > LUA_MAXCALLS) {
        luaD_throw(L, 0);
    }

    if (!luaD_precall(L, func, nresult)) {
        // TODO luaV_execute(L);
    }
    
    L->ncalls--;
    return LUA_OK;
}

static void reset_unuse_stack(struct lua_State* L, ptrdiff_t old_top) {
    struct global_State* g = G(L);
    StkId top = restorestack(L, old_top);
    for (; top < L->top; top++) {
        top->tt_ = LUA_TNIL;
    }
}

int luaD_pcall(struct lua_State* L, Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef) {
    int status;
    struct CallInfo* old_ci = L->ci;
    ptrdiff_t old_errorfunc = L->errorfunc;    
    
    status = luaD_rawrunprotected(L, f, ud);
    if (status != LUA_OK) {
        reset_unuse_stack(L, oldtop);
        L->ci = old_ci;
        L->top = restorestack(L, oldtop);
        seterrobj(L, status);
    }
    
    L->errorfunc = old_errorfunc; 
    return status;
}

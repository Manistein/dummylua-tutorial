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

#ifndef lua_h
#define lua_h 

#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

static int POINTER_SIZE = sizeof(void*);

#if POINTER_SIZE  >= 8
#define LUA_INTEGER long
#define LUA_NUMBER double
#else
#define LUA_INTEGER int 
#define LUA_NUMBER float 
#endif 

#define LUA_UNSIGNED unsigned LUA_INTEGER 
#define lua_assert(c) ((void)0)
#define check_exp(c, e) (lua_assert(c), e)

// ERROR CODE 
#define LUA_OK 0
#define LUA_ERRERR 1
#define LUA_ERRMEM 2
#define LUA_ERRRUN 3 

#define cast(t, exp) ((t)(exp))
#define savestack(L, o) ((o) - (L)->stack)
#define restorestack(L, o) ((L)->stack + (o)) 
#define point2uint(p) ((unsigned int)((size_t)p & UINT_MAX))
#define novariant(o) ((o)->tt_ & 0xf)

// basic object type
#define LUA_TNUMBER 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TBOOLEAN 3
#define LUA_TSTRING 4
#define LUA_TNIL 5
#define LUA_TTABLE 6
#define LUA_TFUNCTION 7
#define LUA_TTHREAD 8
#define LUA_TNONE 9
#define LUA_NUMS LUA_TNONE 
#define LUA_TDEADKEY (LUA_NUMS + 1)

// stack define
#define LUA_MINSTACK 20
#define LUA_STACKSIZE (2 * LUA_MINSTACK)
#define LUA_EXTRASTACK 5
#define LUA_MAXSTACK 15000
#define LUA_ERRORSTACK 200
#define LUA_MULRET -1
#define LUA_MAXCALLS 200

// error tips
#define LUA_ERROR(L, s) printf("LUA ERROR:%s", s)

// mem define
typedef size_t      lu_mem;
typedef ptrdiff_t   l_mem;

typedef LUA_UNSIGNED lua_Unsigned;

// vm
typedef int Instruction;

#define MAX_LUMEM ((lu_mem)(~(lu_mem)0))
#define MAX_LMEM  (MAX_LUMEM >> 1)
#define BUFSIZ 512
#endif 

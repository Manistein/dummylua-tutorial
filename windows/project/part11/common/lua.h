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
#include <stdint.h>
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

#if SIZE_MAX != 0xffffffffui32
#define LUA_INTEGER int64_t
#define LUA_UNSIGNED uint64_t
#define LUA_NUMBER double

#define LUA_INTEGER_FORMAT "%lld"
#define LUA_NUMBER_FORMAT "%.14g"
#else
#define LUA_INTEGER int32_t
#define LUA_UNSIGNED uint32_t
#define LUA_NUMBER float 

#define LUA_INTEGER_FORMAT "%d"
#define LUA_NUMBER_FORMAT "%f"
#endif 

#define LUA_ENV "_ENV"
#define LUA_LOADED "_LOADED"
#define lua_assert(c) ((void)0)
#define check_exp(c, e) (lua_assert(c), e)

// ERROR CODE 
#define LUA_OK 0
#define LUA_ERRERR 1
#define LUA_ERRMEM 2
#define LUA_ERRRUN 3 
#define LUA_ERRLEXER 4
#define LUA_ERRPARSER 5

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
#define LUA_TPROTO 9
#define LUA_TUSERDATA 10
#define LUA_TNONE 11
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

// base operator
#define LUA_OPT_UMN		1
#define LUA_OPT_LEN		2
#define LUA_OPT_BNOT	3
#define LUA_OPT_NOT		4

#define	LUA_OPT_ADD		5
#define	LUA_OPT_SUB		6
#define	LUA_OPT_MUL		7
#define	LUA_OPT_DIV		8
#define LUA_OPT_IDIV    9
#define	LUA_OPT_MOD		10
#define	LUA_OPT_POW		11
#define	LUA_OPT_BAND    12
#define	LUA_OPT_BOR		13
#define	LUA_OPT_BXOR	14
#define	LUA_OPT_SHL		15
#define	LUA_OPT_SHR		16
#define	LUA_OPT_CONCAT  17
#define	LUA_OPT_GREATER 18
#define	LUA_OPT_LESS	19
#define	LUA_OPT_EQ		20
#define	LUA_OPT_GREATEQ 21
#define	LUA_OPT_LESSEQ  22
#define	LUA_OPT_NOTEQ	23
#define	LUA_OPT_AND		24
#define	LUA_OPT_OR		25

// mem define
typedef size_t      lu_mem;
typedef ptrdiff_t   l_mem;

typedef LUA_UNSIGNED lua_Unsigned;

// vm
typedef int Instruction;

#define MAX_LUMEM ((lu_mem)(~(lu_mem)0))
#define MAX_LMEM  (MAX_LUMEM >> 1)
#define BUFSIZE 512

// IO
#define lua_writestring(s) fwrite((s), sizeof(char), strlen((s)), stdout)
#define lua_writeline()    (lua_writestring("\n"), fflush(stdout))
#endif 

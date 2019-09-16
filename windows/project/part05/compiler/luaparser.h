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

#ifndef LUA_PARSER
#define LUA_PARSER

typedef enum expkind {
	VVOID,			// expression is void
	VNIL,			// expression is nil value
	VFLT,			// expression is float value
	VINT,			// expression is integer value
	VTRUE,			// expression is true value
	VFALSE,			// expression is false value
	VINDEXED,		// ind field of struct expdesc is in use
	VCALL,			// expression is a function call, info field of struct expdesc is represent instruction pc
	VLOCAL,			// expression is a local value, info field of struct expdesc is represent the pos of the stack
	VUPVAL,			// expression is a upvalue, ind is in use
	VK,				// expression is a constant, info field of struct expdesc is represent the index of k
	VRELOCATE,		// expression can put result in any register, info field represents the instruction pc
	VNONRELOC,		// expression has result in a register, info field represents the pos of the stack
} expkind;

typedef struct expdesc {
	expkind k;				// expkind
	union {
		int info;			
		lua_Integer i;		// for VINT
		lua_Number r;		// for VFLT

		struct {
			int t;		// the index of the upvalue or table
			int vt;		// whether 't' is a upvalue(VUPVAL) or table(VLOCAL)
			int idx;	// index (R/K)
		} ind;
	} u;
} expdesc;

// Token cache
typedef struct MBuffer {
	char* buffer;
	int n;
	int size;
} MBuffer;

typedef struct Dyndata {
	struct {
		short* arr;
		int n;
		int size;
	} actvar;
} Dyndata;

typedef struct FuncState {
	int firstlocal;
	FuncState* prev;
	LexState* fs;
	Proto* p;
	int pc;				// next code array index to save instruction
	int nk;				// next constant array index to save const value
	int nup;			// next upvalues array index to save upval
	int nlocvars;		// the number of local values
	int nactvars;		// the number of activate values
	int np;				// the number of protos
	int freereg;		// free register index
} FuncState;

LClosure* luaY_parser(struct lua_State* L, Zio* zio, MBuffer buffer, Dyndata* dyd, const char* name, int firstchar);
#endif
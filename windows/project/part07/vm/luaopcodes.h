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

#include "../common/luaobject.h"

#define SIZE_OP 6
#define SIZE_A  8
#define SIZE_B  9
#define SIZE_C  9
#define POS_A SIZE_OP
#define POS_B (SIZE_C + SIZE_A + SIZE_OP)
#define POS_C (SIZE_A + SIZE_OP)

#define LUA_IBIAS (1 << (SIZE_B + SIZE_C) / 2)

#define BITRK 256
#define MAININDEXRK (BITRK - 1)
#define RKMASK(v) (BITRK | v)
#define ISK(v) (v & ~(BITRK - 1))
#define RK(L, cl, v) ISK(v) ? (&cl->p->k[v - BITRK]) : (L->ci->l.base + v)

#define SET_OPCODE(i, r)  (i = (i & 0xFFFFFFC0) | r)
#define SET_ARG_A(i, r)   (i = (i & 0xFFFFC03F) | r << 6) 
#define SET_ARG_B(i, r)   (i = (i & 0x007FFFFF) | r << 23)
#define SET_ARG_C(i, r)   (i = (i & 0xFF803FFF) | r << 14)
#define SET_ARG_Bx(i, r)  (i = (i & 0x00003FFF) | r << 14)
#define SET_ARG_sBx(i, r) (i = (i & 0x00003FFF) | ((r + LUA_IBIAS) << 14))

#define GET_OPCODE(i) (i & 0x3F)
#define GET_ARG_A(i) ((i & 0x3FC0) >> POS_A)
#define GET_ARG_B(i) ((i & 0xFF800000) >> POS_B)
#define GET_ARG_C(i) ((i & 0x7FC000) >> POS_C)
#define GET_ARG_Bx(i) ((i & 0xFFFFC000) >> (SIZE_A + SIZE_OP))
#define GET_ARG_sBx(i) (GET_ARG_Bx(i) - LUA_IBIAS)

#define MAXARG_A ((1 << SIZE_A) - 1)
#define MAXARG_B ((1 << SIZE_B) - 1)
#define MAXARG_C ((1 << SIZE_C) - 1)
#define MAXARG_Bx ((1 << (SIZE_B + SIZE_C)) - 1)
#define MAXARG_sBx (MAXARG_Bx / 2)
#define TEST_MODE(op, m) ((op & 0x03) == m)

#define LFIELD_PER_FLUSH 50

enum OpCode {
	OP_MOVE,        // A, B; R[A] = R[B] load a value from a register to another register;
	OP_LOADK,       // A, B; R[A] = RK[B]; load a value from k to register
	OP_GETUPVAL,    // A, B; R[A] = UpValue[B]
	OP_CALL,        // A, B, C; R[A], ... R[A + C - 2] = R(A)(R[A + 1], ... , R[A + B - 1])
					// A index the function in the stack. 
					// B represents the number of params, if B is 1, the function has no parameters, else if B is greater than 1, the function has B - 1 parameters, and if B is 0
					// it means that the function parameters range from A+1 to the top of stack
					// C represents the number of return, if C is 1, there is no value return, else if C is greater than 1, then it has C - 1 return values, or if C is 0
					// the return values range from A to the top of stack
	OP_RETURN,      // A, B; return R[A], ... R[A + B - 2]
					// return the values to the calling function, B represent the number of results. if B is 1, that means no value return, if B is greater than 1, it means 
					// there are B - 1 values return. And finally, if B is 0, the set of values range from R[A] to the top of stack, are return to the calling function
	OP_GETTABUP,   // A, B, C; R[A] = Upval[B][RK[C]]
	OP_GETTABLE,    // A, B, C; R[A] = R[B][RK[C]]

	OP_SELF,        // A, B, C; R(A+1) = R(B); R(A) = R(B)[RK(C)];

	OP_TEST,		// A, B, C; if not (R(A) <=> C) then pc++];
	OP_TESTSET,		// A B C   if (R(B) <=> C) then R(A) := R(B) else pc++ 
	OP_JUMP,		// A sBx   pc+=sBx; if (A) close all upvalues >= R(A - 1)

	OP_UNM,			// A B     R(A) := -R(B)
	OP_LEN,			// A B     R(A) := length of R(B)
	OP_BNOT,		// A B     R(A) := ~R(B)
	OP_NOT,			// A B     R(A) := not R(B)

	OP_ADD,			// A B C   R(A) := RK(B) + RK(C)
	OP_SUB,			// A B C   R(A) := RK(B) - RK(C)
	OP_MUL,			// A B C   R(A) := RK(B) * RK(C)
	OP_DIV,			// A B C   R(A) := RK(B) / RK(C)
	OP_IDIV,		// A B C   R(A) := RK(B) // RK(C)
	OP_MOD,			// A B C   R(A) := RK(B) % RK(C)
	OP_POW,			// A B C   R(A) := RK(B) ^ RK(C)
	OP_BAND,		// A B C   R(A) := RK(B) & RK(C)
	OP_BOR,			// A B C   R(A) := RK(B) | RK(C)
	OP_BXOR,		// A B C   R(A) := RK(B) ~ RK(C)
	OP_SHL,			// A B C   R(A) := RK(B) << RK(C)
	OP_SHR,			// A B C   R(A) := RK(B) >> RK(C)
	OP_CONCAT,		// A B C   R(A) := R(B).. ... ..R(C)

	OP_EQ,			// A B C   if ((RK(B) == RK(C)) ~= A) then pc++
	OP_LT,			// A B C   if ((RK(B) <  RK(C)) ~= A) then pc++
	OP_LE,			// A B C   if ((RK(B) <= RK(C)) ~= A) then pc++

	OP_LOADBOOL,    // A B C   R(A) := (Bool)B; if(C) PC++;
	OP_LOADNIL,     // A B     R(A) := ... := R(B) := nil

	OP_SETUPVAL,    // A B     UpValue[B] := R(A)
	OP_SETTABUP,    // A B C   UpValue[A][RK(B)] := RK(C)
	OP_NEWTABLE,    // A B C   R(A) := {} (size = B,C)
	OP_SETLIST,     // A B C   R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
	OP_SETTABLE,    // A B C   R(A)[RK(B)] = RK(C)

	NUM_OPCODES,
};

// The whole instruction is 32 bits
// opcode is 6 bits, the description of instructions are as follow
/***
+--------+---------+---------+--------+--------+
|  iABC  |    B:9  |   C:9   |   A:8  |Opcode:6|
+--------+---------+---------+--------+--------+
|  iABx  |   Bx:18(unsigned) |   A:8  |Opcode:6|
+--------+---------+---------+--------+--------+
|  iAsBx |   sBx:18(signed)  |   A:8  |Opcode:6|
+--------+---------+---------+--------+--------+
***/
enum OpMode {
	iABC,
	iABx,
	iAsBx,
};

enum OpArgMask {
	OpArgU,     // argument is used, and it's value is in instruction
	OpArgR,     // argument is used, and it's value is in register, the position of register is indexed by instruction
	OpArgN,     // argument is not used
	OpArgK,     // argument is used, and it's value is in constant table, the index is in instruction
};

extern const lu_byte luaP_opmodes[NUM_OPCODES];
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

#define BITRK 256
#define MAININDEXRK (BITRK - 1)
#define RKMASK(v) (BITRK | v)
#define ISK(v) (v & ~(BITRK - 1))

#define SET_ARG_A(i, r) (i = (i & 0xFFFFC03F) | r << 6) 
#define SET_ARG_B(i, r) (i = (i & 0x007FFFFF) | r << 23)
#define SET_ARG_C(i, r) (i = (i & 0xff803fff) | r << 14)

#define GET_OPCODE(i) (i & 0x3f)
#define GET_ARG_A(i) ((i & 0x3fc0) >> POS_A)
#define GET_ARG_B(i) ((i & 0xff800000) >> POS_B)
#define GET_ARG_C(i) ((i & 0x7fc000) >> POS_C)
#define GET_ARG_Bx(i) ((i & 0xffffc000) >> (SIZE_A + SIZE_OP))

enum OpCode {
	OP_MOVE,        // A, B; R[A] = R[B] load a value from a register to another register;
	OP_LOADK,       // A, B; R[A] = RK[B]; load a value from k to register
	OP_GETUPVAL,    // A, B; R[A] = RK[B]
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
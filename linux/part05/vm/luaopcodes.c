#include "luaopcodes.h"

/***
t: if it is a test instruction or not
a: can register will be update by current instruction
b: OpArgMode
c: OpArgMode
m: mode(iABC,iABx,iAsBx)
***/
#define opmode(t, a, b, c, m) ((t << 7) | (a << 6) | (b << 4) | (c << 2) | m)

const lu_byte luaP_opmodes[NUM_OPCODES] = {
    //      t  a  b       c       m
     opmode(0, 1, OpArgR, OpArgN, iABC) // OP_MOVE,
    ,opmode(0, 1, OpArgK, OpArgN, iABC) // OP_LOADK,
    ,opmode(0, 1, OpArgU, OpArgN, iABC) // OP_GETUPVAL,
    ,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_CALL,
    ,opmode(0, 0, OpArgU, OpArgU, iABC) // OP_RETURN,
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_GETTABUP,
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_GETTABLE,
};
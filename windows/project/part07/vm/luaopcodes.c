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
     opmode(0, 1, OpArgR, OpArgN, iABC) // OP_MOVE
    ,opmode(0, 1, OpArgK, OpArgN, iABx) // OP_LOADK
    ,opmode(0, 1, OpArgU, OpArgN, iABC) // OP_GETUPVAL
    ,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_CALL
    ,opmode(0, 0, OpArgU, OpArgU, iABC) // OP_RETURN
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_GETTABUP
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_GETTABLE
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_SELF
	,opmode(1, 0, OpArgN, OpArgU, iABC) // OP_TEST
	,opmode(1, 1, OpArgR, OpArgU, iABC) // OP_TESTSET
	,opmode(0, 0, OpArgU, OpArgU, iAsBx) // OP_JUMP
	,opmode(0, 1, OpArgR, OpArgN, iABC) // OP_UNM
	,opmode(0, 1, OpArgR, OpArgN, iABC) // OP_LEN
	,opmode(0, 1, OpArgR, OpArgN, iABC) // OP_BNOT
	,opmode(0, 1, OpArgR, OpArgN, iABC) // OP_NOT
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_ADD
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_SUB
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_MUL
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_DIV
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_IDIV
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_MOD
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_POW
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_BAND
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_BOR
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_BXOR
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_SHL
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_SHR
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_CONCAT
	,opmode(1, 0, OpArgR, OpArgR, iABC) // OP_EQ
	,opmode(1, 0, OpArgR, OpArgR, iABC) // OP_LT
	,opmode(1, 0, OpArgR, OpArgR, iABC) // OP_LE
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_LOADBOOL
	,opmode(0, 1, OpArgR, OpArgN, iABC) // OP_LOADNIL
	,opmode(0, 0, OpArgU, OpArgN, iABC) // OP_SETUPVAL
	,opmode(0, 0, OpArgR, OpArgR, iABC) // OP_SETTABUP
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_NEWTABLE
	,opmode(0, 1, OpArgU, OpArgU, iABC) // OP_SETLIST
	,opmode(0, 1, OpArgR, OpArgR, iABC) // OP_SETTABLE
};
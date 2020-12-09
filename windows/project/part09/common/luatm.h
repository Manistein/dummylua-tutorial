#ifndef _lua_tm_h_
#define _lua_tm_h_

#include "luaobject.h"

typedef enum {
	TM_LT,
	TM_GT,
	TM_LE,
	TM_GE,
	TM_EQ,
	TM_CONCAT,

	TM_ADD,
	TM_SUB,
	TM_MUL,
	TM_DIV,
	TM_IDIV,
	TM_POW,
	TM_BAND,
	TM_BOR,
	TM_XOR,
	TM_SHL,
	TM_SHR,
	TM_MOD,

	TM_INDEX,
	TM_NEWINDEX,

	TM_GC,

	TM_TOTAL
}TMS;

void luaT_init(struct lua_State* L);
void luaT_trycallbinTM(struct lua_State* L, TValue* lhs, TValue* rhs, TMS tm);
TValue* luaT_gettmbyobj(struct lua_State* L, TValue* o, TMS tm);
void luaT_callTM(struct lua_State* L, TValue* f, TValue* p1, TValue* p2, TValue* p3, int hasres);

#endif
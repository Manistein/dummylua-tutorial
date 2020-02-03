#ifndef luacode_h_
#define luacode_h_

#include "../common/lua.h"
#include "lualexer.h"
#include "luaparser.h"

#define get_instruction(fs, e) (fs->p->code[e->u.info])

int luaK_exp2RK(FuncState* fs, expdesc* e);   // discharge expression to constant table or specific register
int luaK_stringK(FuncState* fs, TString* key); // generate an expression to index constant in constants vector
int luaK_nilK(FuncState* fs);
int luaK_floatK(FuncState* fs, lua_Number r);
int luaK_intK(FuncState* fs, lua_Integer i);
int luaK_boolK(FuncState* fs, int v);
int luaK_ret(FuncState* fs, int first, int nret);

void luaK_indexed(FuncState* fs, expdesc* e, expdesc* key); // generate an expression to index a key which is in a local table or a upvalue table

int luaK_codeABC(FuncState* fs, int opcode, int a, int b, int c);
int luaK_codeABx(FuncState* fs, int opcode, int a, int bx);

void luaK_dischargevars(FuncState* fs, expdesc* e);
int luaK_exp2nextreg(FuncState* fs, expdesc* e);    // discharge expression to next register 
int luaK_exp2anyreg(FuncState* fs, expdesc * e); 

#endif

#ifndef luacode_h_
#define luacode_h_

#include "../common/lua.h"
#include "lualexer.h"
#include "luaparser.h"

#define get_instruction(fs, e) (fs->p->code[e->u.info])
#define NO_JUMP -1
#define NO_REG -1

#define luaK_codeAsBx(fs, c, a, sbx) luaK_codeABx(fs, c, a, (sbx) + LUA_IBIAS)

/*
** Ensures final expression result is either in a register or it is
** a constant.
*/
void luaK_exp2val(FuncState* fs, expdesc* e);  
int luaK_exp2RK(FuncState* fs, expdesc* e);   // discharge expression to constant table or specific register
int luaK_stringK(FuncState* fs, TString* key); // generate an expression to index constant in constants vector
int luaK_nilK(FuncState* fs);
int luaK_floatK(FuncState* fs, lua_Number r);
int luaK_intK(FuncState* fs, lua_Integer i);
int luaK_boolK(FuncState* fs, int v);
int luaK_ret(FuncState* fs, int first, int nret);
void luaK_self(FuncState* fs, expdesc* e, expdesc* key);
int luaK_jump(FuncState* fs, expdesc* e);
int luaK_patchclose(FuncState* fs, int list, int dtarget, int reg, int vtarget);

void luaK_concat(FuncState* fs, int* l1, int l2);
void luaK_patchtohere(FuncState* fs, int pc);

void luaK_goiftrue(FuncState* fs, expdesc* e);
void luaK_goiffalse(FuncState* fs, expdesc* e);

void luaK_prefix(FuncState* fs, int op, expdesc* e);
void luaK_infix(FuncState* fs, int op, expdesc* e);
void luaK_posfix(FuncState* fs, int op, expdesc* e1, expdesc* e2);

void luaK_setreturns(FuncState* fs, expdesc* e, int nret);
void luaK_setoneret(FuncState* fs, expdesc* e);
void luaK_storevar(FuncState* fs, expdesc* var, expdesc* ex);
void luaK_reserveregs(FuncState* fs, int n);
void luaK_setlist(FuncState* fs, int base, int nelemts, int tostore);

void luaK_indexed(FuncState* fs, expdesc* e, expdesc* key); // generate an expression to index a key which is in a local table or a upvalue table
void luaK_nil(FuncState* fs, int from, int to);

int luaK_codeABC(FuncState* fs, int opcode, int a, int b, int c);
int luaK_codeABx(FuncState* fs, int opcode, int a, int bx);

void luaK_dischargevars(FuncState* fs, expdesc* e);
int luaK_exp2nextreg(FuncState* fs, expdesc* e);    // discharge expression to next register 
int luaK_exp2anyreg(FuncState* fs, expdesc* e); 
void luaK_exp2anyregup(FuncState* fs, expdesc* e);

#endif

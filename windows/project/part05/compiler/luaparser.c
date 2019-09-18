#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastate.h"
#include "../vm/luafunc.h"

static void open_func(LexState* ls, FuncState* fs) {
	fs->firstlocal = 0;
	fs->freereg = 0;
	fs->ls = ls;
	fs->nactvars = 0;
	fs->nlocvars = 0;
	fs->nk = 0;
	fs->np = 0;
	fs->nup = 0;
	fs->prev = ls->fs;
	ls->fs = fs;
	
	fs->pc = 0;
}

static void mainfunc(struct lua_State* L, LexState* ls, FuncState* fs) {
	open_func(ls, fs);
}

LClosure* luaY_parser(struct lua_State* L, Zio* zio, MBuffer* buffer, Dyndata* dyd, const char* name) {
	FuncState fs;
	LexState ls;
	const char* env = "_ENV";
	luaX_setinput(L, &ls, zio, buffer, dyd, luaS_newlstr(L, name, strlen(name)), luaS_newlstr(L, env, strlen(env)));
	ls.current = zget(ls.zio);
	
	fs.p = luaF_newproto(L);

	LClosure* closure = luaF_newLclosure(L, 0);
	setlclvalue(L->top, closure);
	increase_top(L);
	mainfunc(L, &ls, &fs);

	return NULL;
}
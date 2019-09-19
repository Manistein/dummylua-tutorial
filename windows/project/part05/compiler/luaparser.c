#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastate.h"
#include "../vm/luafunc.h"
#include "../common/luamem.h"
#include "../common/luastring.h"

static void init_exp(expdesc* e, expkind k, int i) {
	e->k = k;
	e->u.info = i;
}

static void newupvalues(FuncState* fs, expdesc* e) {
	struct lua_State* L = fs->ls->L;
	Proto* p = fs->p;
	luaM_growvector(L, p->upvalues, fs->nups, p->sizeupvalues, Upvaldesc, MAXUPVAL);
	
	for (int i = fs->nups; i < p->sizeupvalues; i++) {
		p->upvalues[i].idx = 0;
		p->upvalues[i].in_stack = 0;
		p->upvalues[i].name = NULL;
	}

	p->upvalues[fs->nups].idx = e->u.info;
	p->upvalues[fs->nups].in_stack = e->k == VLOCAL;
	p->upvalues[fs->nups].name = fs->ls->env;
}

static void open_func(LexState* ls, FuncState* fs) {
	fs->firstlocal = 0;
	fs->freereg = 0;
	fs->ls = ls;
	fs->nactvars = 0;
	fs->nlocvars = 0;
	fs->nk = 0;
	fs->np = 0;
	fs->nups = 0;
	fs->prev = ls->fs;
	ls->fs = fs;
	
	fs->pc = 0;
}

static void mainfunc(struct lua_State* L, LexState* ls, FuncState* fs) {
	expdesc e;
	init_exp(&e, VLOCAL, 0);

	open_func(ls, fs);
	newupvalues(fs, &e);
}

LClosure* luaY_parser(struct lua_State* L, Zio* zio, MBuffer* buffer, Dyndata* dyd, const char* name) {
	FuncState fs;
	LexState ls;
	const char* env = "_ENV";
	luaX_setinput(L, &ls, zio, buffer, dyd, luaS_newlstr(L, name, strlen(name)), luaS_newlstr(L, env, strlen(env)));
	ls.current = zget(ls.zio);
	
	LClosure* closure = luaF_newLclosure(L, 1);
	closure->p = fs.p = luaF_newproto(L);

	setlclvalue(L->top, closure);
	increase_top(L);
	mainfunc(L, &ls, &fs);

	return closure;
}
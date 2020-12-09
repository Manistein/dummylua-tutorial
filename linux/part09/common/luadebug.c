#include "luadebug.h"
#include "../vm/luado.h"
#include "luaobject.h"
#include "../vm/luagc.h"
#include "luastring.h"
#include "luastate.h"
#include "../vm/luaopcodes.h"

static TString* getfuncname(struct lua_State* L, struct CallInfo* ci) {
	TString* ts;
	if (!ci) {
		ts = luaS_newliteral(L, "[?]");
		return ts;
	}

	if (ci->func->tt_ == LUA_TLCL) {
		struct LClosure* cl = gco2lclosure(gcvalue(ci->func));
		int pc = ci->l.savedpc - cl->p->code - 1;
		Instruction i = cl->p->code[pc];
		if (GET_OPCODE(i) == OP_CALL) {
			int narg = GET_ARG_B(i);
			switch (GET_OPCODE(cl->p->code[pc - narg])) {
			case OP_MOVE: {
				int arg_b = GET_ARG_B(cl->p->code[pc - narg]);
				ts = cl->p->locvars[arg_b].varname;
			} break;
			case OP_GETTABLE: 
			case OP_GETTABUP: {
				int arg_c = GET_ARG_C(cl->p->code[pc - narg]);
				if (ISK(arg_c)) {
					ts = gco2ts(gcvalue(&cl->p->k[arg_c - 256]));
				}
				else {
					ts = gco2ts(gcvalue(ci->l.base + arg_c));
				}
			} break;
			default: {
				ts = luaS_newliteral(L, "[?]");
			} break;
			}
		}
		else {
			ts = luaS_newliteral(L, "[?]");
		}
	}
	else {
		ts = luaS_newliteral(L, "main");
	}

	return ts;
}

void luaG_runerror(struct lua_State* L, const char* fmt, ...) {
	int n = 1;

	va_list argp;
	va_start(argp, fmt);
	luaO_pushfvstring(L, fmt, argp);
	va_end(argp);

	for (struct CallInfo* ci = L->ci; ci != &L->base_ci; ci = ci->previous) {
		if (ci->callstatus & CIST_LUA) {
			n++;

			struct LClosure* cl = gco2lclosure(gcvalue(ci->func));
			struct Proto* p = cl->p;

			TString* ts = getfuncname(L, ci->previous);
			luaO_pushfstring(L, "\t %s line:%d in %s", getstr(p->source), p->line[ci->l.savedpc - p->code - 1], getstr(ts));

			luaO_concat(L, L->top - 2, L->top - 1, L->top - 2);
			L->top--;
		}
	}

	luaD_throw(L, LUA_ERRRUN);
}
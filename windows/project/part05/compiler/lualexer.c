#include "lualexer.h"
#include "luaparser.h"

void luaX_setinput(struct lua_State* L, LexState* ls, Zio* z, MBuffer* buffer, Dyndata* dyd, TString* source, TString* env) {
	ls->L = L;
	ls->source = source;
	ls->env = env;
	ls->current = 0;
	ls->buff = buffer;
	ls->dyd = dyd;
	ls->env = env;
	ls->fs = NULL;
	ls->linenumber = 1;
	ls->t.token = 0;
	ls->t.seminfo.i = 0;
	ls->zio = z;
}

Token luaX_next(struct lua_State* L, LexState* ls) {
	Token token;
	bufferclear(ls->buff);

	switch (ls->current)
	{
	case '-':
		break;
	case '+':
		break;
	case '*':
		break;
	case '/':
		break;
	case '~':
		break;
	case '%':
		break;
	case '.':
		break;
	case '"':
		break;
	case '(':
		break;
	case ')':
		break;
	case '[':
		break;
	case ']':
		break;
	case '{':
		break;
	case '}':
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		break;
	case '=':
		break;
	default:break;
	}

	return token;
}
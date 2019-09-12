#include "luaparser.h"
#include "lualexer.h"

void luaX_setinput(struct lua_State* L, LexState* ls, TString* source, TString* env) {
	ls->L = L;
	ls->source = source;
	ls->env = env;
}

Token luaX_next(struct lua_State* L, LexState* ls) {
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
		
}
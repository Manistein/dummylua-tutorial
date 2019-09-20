#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastring.h"

// the sequence must be the same as enum RESERVED
static const char* luaX_tokens[] = {
	"local", "nil", "true", "false", "function"
};

void luaX_init(struct lua_State* L)
{
	TString* env = luaS_newliteral(L, LUA_ENV);
	luaC_fix(L, obj2gco(env));

	for (int i = 0; i < NUM_RESERVED; i++) {
		TString* reserved = luaS_newliteral(L, luaX_tokens[i]);
		luaC_fix(L, obj2gco(reserved));
		reserved->extra = i + FIRST_REVERSED;
	}
}

void luaX_setinput(struct lua_State* L, LexState* ls, Zio* z, struct MBuffer* buffer, struct Dyndata* dyd, TString* source, TString* env) {
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
	case '>':
		break;
	case '<':
		break;
	case '=':
		break;
	case ',':
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		break;
	default:break;
	}

	return token;
}
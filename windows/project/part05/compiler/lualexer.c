#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastring.h"
#include "../vm/luado.h"

#define next(ls) (ls->current = zget(ls->zio))
#define currIsNewLine(ls) (ls->current == '\n' || ls->current == '\r')

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

// increase line number, skip to next line
static void inclinenumber(LexState* ls) {
	int old = ls->current;
	lua_assert(currIsNewLine(ls));
	next(ls);

	// skip '\n\r' or '\r\n'
	if (currIsNewLine(ls) && ls->current != old) {
		next(ls);
	}

	if (++ls->linenumber >= INT_MAX) {
		LUA_ERROR(ls->L, "function inclinenumber is reach INT_MAX");
		luaD_throw(ls->L, LUA_ERRCOMPILE);
	}
}

int luaX_next(struct lua_State* L, LexState* ls) {
	bufferclear(ls->buff);

	for (;;) {
		switch (ls->current)
		{
		// new line
		case '\n': case '\r': {
			inclinenumber(ls);
		} break;
		// skip spaces
		case ' ': case '\t': case '\v': {
			next(ls);
		} break;
		case '-': {
			next(ls);
			// this line is comment
			if (ls->current == '-') {
				while (!currIsNewLine(ls) && ls->current != EOF)
					next(ls);
			}
			else {
				ls->t.token = '-';
				return '-';
			}
		} break;
		case EOF:{
			ls->t.token = TK_EOS;
			return TK_EOS;
		}
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
	}

	return TK_EOS;
}
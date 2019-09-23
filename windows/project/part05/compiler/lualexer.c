#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastring.h"
#include "../vm/luado.h"
#include "luazio.h"

#define next(ls) (ls->current = zget(ls->zio))
#define save_and_next(L, ls) (save(L, ls); ls->current = next(ls);)
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

// save character into buffer
static void save(struct lua_State* L, LexState* ls) {
	if (ls->buff->n + 1 > luaZ_buffersize(ls)) {
		int size = luaZ_buffersize(ls) * 2;
		if (size < MIN_BUFF_SIZE) {
			size = MIN_BUFF_SIZE;
		}

		ls->buff->buffer = G(L)->frealloc(NULL, ls->buff->buffer, luaZ_buffersize(ls), size);
		if (ls->buff->buffer == NULL) {
			LUA_ERROR(L, "lualexer:the saved string is too long");
			luaD_throw(L, LUA_ERRCOMPILE);
		}
		ls->buff->size = size;
	}

	ls->buff->buffer[ls->buff->n++] = ls->current;
}

static void read_string(LexState* ls, int delimiter, Seminfo* seminfo) {
}

static int llex(LexState* ls, Seminfo* seminfo) {
	luaZ_resetbuffer(ls);

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
				return '-';
			}
		} break;
		case EOF:{
			next(ls);
			return TK_EOS;
		}
		case '+': {
			next(ls);
			return '+';
		} break;
		case '*': {
			next(ls);
			return '*';
		} break;
		case '/': {
			next(ls);
			return '/';
		} break;
		case '~': {
			next(ls);
			// not equal
			if (ls->current == '=') {
				next(ls);
				return TK_NOTEQUAL;
			}
			else {
				return '~';
			}
		} break;
		case '%': {
			next(ls);
			return TK_MOD;
		} break;
		case '.': {
			next(ls);
			if (ls->current == '.') {
				next(ls);
				// the '...' means vararg 
				if (ls->current == '.') {
					return TK_VARARG;
				}
				// the '..' means concat
				else {
					return TK_CONCAT;
				}
			}
			else {
				return TK_DOT;
			}
		} break;
		case '"': case '\'': { // process string

		} break;
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

int luaX_next(struct lua_State* L, LexState* ls) {
	ls->t.token = llex(ls, &ls->t.seminfo);
}
#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastring.h"
#include "../vm/luado.h"
#include "luazio.h"

#define next(ls) (ls->current = zget(ls->zio))
#define save_and_next(L, ls, c) save(L, ls, c); ls->current = next(ls)
#define currIsNewLine(ls) (ls->current == '\n' || ls->current == '\r')
#define is_digit(c) (c >= 48 && c <= 57)

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
		luaD_throw(ls->L, LUA_ERRLEXER);
	}
}

// save character into buffer
static void save(struct lua_State* L, LexState* ls, int character) {
	if (ls->buff->n + 1 > luaZ_buffersize(ls)) {
		int size = luaZ_buffersize(ls) * 2;
		if (size < MIN_BUFF_SIZE) {
			size = MIN_BUFF_SIZE;
		}

		ls->buff->buffer = G(L)->frealloc(NULL, ls->buff->buffer, luaZ_buffersize(ls), size);
		if (ls->buff->buffer == NULL) {
			LUA_ERROR(L, "lualexer:the saved string is too long");
			luaD_throw(L, LUA_ERRLEXER);
		}
		ls->buff->size = size;
	}

	ls->buff->buffer[ls->buff->n++] = character;
}

static int read_string(LexState* ls, int delimiter, Seminfo* seminfo) {
	next(ls);
	while (ls->current != delimiter) {
		int c = 0;
		switch (ls->current)
		{
		case '\n': case '\r': case EOF: {
			LUA_ERROR(ls->L, "uncomplete string");
			luaD_throw(ls->L, LUA_ERRLEXER);
		} break;
		case '\\': {
			next(ls);
			switch (ls->current)
			{
			case 't':{ c = '\t'; goto save_escape_sequence; }
			case 'v':{ c = '\v'; goto save_escape_sequence; }
			case 'a':{ c = '\a'; goto save_escape_sequence; }
			case 'b':{ c = '\b'; goto save_escape_sequence; }
			case 'f':{ c = '\f'; goto save_escape_sequence; }
			case 'n':{ c = '\n'; goto save_escape_sequence; }
			case 'r': {
				c = '\r';
			save_escape_sequence:
				save_and_next(ls->L, ls, c);
			} break;
			default: {
				save(ls->L, ls, '\\');
				save_and_next(ls->L, ls, ls->current);
			} break;
			}
		}
		default: {
			save_and_next(ls->L, ls, ls->current);
		} break;
		}
	}
	next(ls);

	seminfo->s = luaS_newliteral(ls->L, ls->buff->buffer);

	return TK_STRING;
}

static int str2number(LexState* ls, bool has_dot) {
	if (has_dot) {
		save(ls->L, ls, '0');
		save(ls->L, ls, '.');
	}

	while (is_digit(ls->current) || ls->current == '.') {
		if (ls->current == '.') {
			if (has_dot) {
				LUA_ERROR(ls->L, "unknow number");
				luaD_throw(ls->L, LUA_ERRLEXER);
			}
			has_dot = true;
		}
		save_and_next(ls->L, ls, ls->current);
	}
	save(ls->L, ls, '\0');

	
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
		}
		case '*': {
			next(ls);
			return '*';
		}
		case '/': {
			next(ls);
			return '/';
		}
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
		}
		case '%': {
			next(ls);
			return TK_MOD;
		}
		case '.': {
			next(ls);
			if (is_digit(ls->current)) {
				return str2number(ls);
			}
			else if (ls->current == '.') {
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
		}
		case '"': case '\'': { // process string
			return read_string(ls, ls->current, &ls->t.seminfo);
		}
		case '(': return '(';
		case ')': return ')';
		case '[': return '[';
		case ']': return ']';
		case '{': return '{';
		case '}': return '}';
		case '>': {
			next(ls);
			if (ls->current == '=') {
				next(ls);
				return TK_GREATEREQUAL;
			}
			else if (ls->current == '>') {
				next(ls);
				return TK_SHR;
			}
			else {
				return '>';
			}
		}
		case '<':{
			next(ls);
			if (ls->current == '=') {
				next(ls);
				return TK_LESSEQUAL;
			}
			else if (ls->current == '<')
			{
				next(ls);
				return TK_SHL;
			}
			else {
				return '<';
			}
		}
		case '=': {
			next(ls);
			if (ls->current == '=') {
				next(ls);
				return TK_EQUAL;
			}
			else {
				return '=';
			}
		}
		case ',': {
			return ',';
		}
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {

		}
		default:break;
		}
	}

	return TK_EOS;
}

int luaX_next(struct lua_State* L, LexState* ls) {
	ls->t.token = llex(ls, &ls->t.seminfo);
}
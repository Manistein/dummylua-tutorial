#include "lualexer.h"
#include "luaparser.h"
#include "../common/luastring.h"
#include "../vm/luado.h"
#include "luazio.h"
#include "../common/lua.h"
#include <ctype.h>

#define next(ls) (ls->current = zget(ls->zio))
#define save_and_next(L, ls, c) save(L, ls, c); ls->current = next(ls)
#define currIsNewLine(ls) (ls->current == '\n' || ls->current == '\r')

static int llex(LexState* ls, Seminfo* seminfo);

// the sequence must be the same as enum RESERVED
const char* luaX_tokens[] = {
	"local", "nil",  "true", "false", 
	"end",   "then", "if",   "elseif", 
	"else",  "not",  "and",  "or", 
	"do",    "for",  "in",   "while", 
	"repeat","until","break","return",
	"function"
};

void luaX_init(struct lua_State* L) {
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

int luaX_lookahead(struct lua_State* L, LexState* ls) {
	lua_assert(ls->lookahead.token == TK_EOS);
	ls->lookahead.token = llex(ls, &ls->lookahead.seminfo);
	return ls->lookahead.token;
}

static void lexerror(LexState* ls, const char* error_text) {
	luaO_pushfstring(ls->L, "%s", error_text);
	luaD_throw(ls->L, LUA_ERRLEXER);
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
		lexerror(ls, "function inclinenumber is reach INT_MAX");
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
			lexerror(ls, "lualexer:the saved string is too long");
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
			lexerror(ls, "uncomplete string");
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
	save(ls->L, ls, '\0');
	next(ls);

	seminfo->s = luaS_newliteral(ls->L, ls->buff->buffer);

	return TK_STRING;
}

static bool is_hex_digit(int c) {
	if (isdigit(c)) {
		return true;
	}

	switch (c) {
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': {
		return true;
	} break;
	default:break;
	}

	return false;
}

static int str2hex(LexState* ls) {
	int num_part_count = 0;
	while (is_hex_digit(ls->current)) {
		save_and_next(ls->L, ls, ls->current);
		num_part_count++;
	}
	save(ls->L, ls, '\0');

	if (num_part_count <= 0) {
		lexerror(ls, "malformed number near '0x'");
	}

	ls->t.seminfo.i = strtoll(ls->buff->buffer, NULL, 0);
	return TK_INT;
}

static int str2number(LexState* ls, bool has_dot) {
	if (has_dot) {
		save(ls->L, ls, '0');
		save(ls->L, ls, '.');
	}

	while (isdigit(ls->current) || ls->current == '.') {
		if (ls->current == '.') {
			if (has_dot) {
				lexerror(ls, "unknow number");
			}
			has_dot = true;
		}
		save_and_next(ls->L, ls, ls->current);
	}
	save(ls->L, ls, '\0');

	if (has_dot) {
		ls->t.seminfo.r = atof(ls->buff->buffer);
		return TK_FLOAT;
	}
	else {
		ls->t.seminfo.i = atoll(ls->buff->buffer);
		return TK_INT;
	}
}

static int llex(LexState* ls, Seminfo* seminfo) {
	for (;;) {
		luaZ_resetbuffer(ls);
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
			if (isdigit(ls->current)) {
				return str2number(ls, true);
			}
			else if (ls->current == '.') {
				next(ls);
				// the '...' means vararg 
				if (ls->current == '.') {
					next(ls);
					return TK_VARARG;
				}
				// the '..' means concat
				else {
					return TK_CONCAT;
				}
			}
			else {
				return '.';
			}
		}
		case '"': case '\'': { // process string
			return read_string(ls, ls->current, &ls->t.seminfo);
		}
		case '(': {
			next(ls);
			return '(';
		}
		case ')': {
			next(ls);
			return ')';
		}
		case '[': {
			next(ls);
			return '[';
		}
		case ']': {
			next(ls);
			return ']';
		}
		case '{': {
			next(ls);
			return '{';
		}
		case '}': {
			next(ls);
			return '}';
		}
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
			next(ls);
			return ',';
		}
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {
			if (ls->current == '0') {
				save_and_next(ls->L, ls, ls->current);
				if (ls->current == 'x' || ls->current == 'X') {
					save_and_next(ls->L, ls, ls->current);
					return str2hex(ls);
				}
				else {
					return str2number(ls, false);
				}
			}
			return str2number(ls, false);
		}
		default: {
			// TK_NAME or reserved name
			if (isalpha(ls->current) || ls->current == '_') {
				while (isalpha(ls->current) || ls->current == '_' || isdigit(ls->current)) {
					save_and_next(ls->L, ls, ls->current);
				}
				save(ls->L, ls, '\0');
				
				TString* s = luaS_newlstr(ls->L, ls->buff->buffer, strlen(ls->buff->buffer));
				if (s->extra > 0) {
					return s->extra;
				}
				else {
					ls->t.seminfo.s = s;
					return TK_NAME;
				}
			}
			else { // single char
				int c = ls->current;
				next(ls);
				return c;
			}
		}
		}
	}

	return TK_EOS;
}

int luaX_next(struct lua_State* L, LexState* ls) {
	if (ls->lookahead.token != TK_EOS) {
		ls->t.token = ls->lookahead.token;
		ls->lookahead.token = TK_EOS;
		return ls->t.token;
	}

	ls->t.token = llex(ls, &ls->t.seminfo);
	return ls->t.token;
}

void luaX_syntaxerror(struct lua_State* L, LexState* ls, const char* error_text) {
	// the path of the compiling file
	luaO_pushfstring(L, "%s \n syntax error near %s line:%d", error_text, getstr(ls->source), ls->linenumber);
	
	if (ls->t.token >= FIRST_REVERSED && ls->t.token <= TK_FUNCTION) {
		luaO_pushfstring(L, "%s", luaX_tokens[ls->t.token - FIRST_REVERSED]);
	}
	else {
		switch (ls->t.token) {
		case TK_STRING: case TK_NAME: {
			luaO_pushfstring(L, "%s", getstr(ls->t.seminfo.s));
		} break;
		case TK_FLOAT: {
			luaO_pushfstring(L, "%f", (float)ls->t.seminfo.r);
		} break;
		case TK_INT: {
			luaO_pushfstring(L, "%d", (int)ls->t.seminfo.i);
		} break;
		case TK_NOTEQUAL: {
			luaO_pushfstring(L, "%s", "~=");
		} break;
		case TK_EQUAL: {
			luaO_pushfstring(L, "%s", "==");
		} break;
		case TK_GREATEREQUAL: {
			luaO_pushfstring(L, "%s", ">=");
		} break;
		case TK_LESSEQUAL: {
			luaO_pushfstring(L, "%s", "<=");
		} break;
		case TK_SHL: {
			luaO_pushfstring(L, "%s", "<<");
		} break;
		case TK_SHR: {
			luaO_pushfstring(L, "%s", ">>");
		} break;
		case TK_MOD: {
			luaO_pushfstring(L, "%s", "%%");
		} break;
		case TK_DOT: {
			luaO_pushfstring(L, "%s", ".");
		} break;
		case TK_VARARG: {
			luaO_pushfstring(L, "%s", "...");
		} break;
		case TK_CONCAT: {
			luaO_pushfstring(L, "%s", "..");
		} break;
		case TK_EOS: {
			luaO_pushfstring(L, "%s", "eos");
		} break;
		default: {
			char buff[BUFSIZ];
			l_sprintf(buff, sizeof(buff), "%c", ls->t.token);
			luaO_pushfstring(L, "%s", buff);
		} break;
		}
	}

	luaO_concat(L, L->top - 2, L->top - 1, L->top - 2);
	L->top--;

	TString* ts = gco2ts(gcvalue(L->top - 1));

	luaD_throw(L, LUA_ERRPARSER);
}
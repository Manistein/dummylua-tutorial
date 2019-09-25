/* Copyright (c) 2018 Manistein,https://manistein.github.io/blog/  

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.*/

#ifndef LUA_LEXER
#define LUA_LEXER

#include "../common/lua.h"
#include "../common/luaobject.h"
#include "luazio.h"

#define FIRST_REVERSED 257

// 1~256 should reserve for ASCII character token
enum RESERVED {
	/* terminal token donated by reserved word */
	TK_LOCAL = FIRST_REVERSED,
	TK_NIL,
	TK_TRUE,
	TK_FALSE,
	TK_FUNCTION,

	/* other token */
	TK_STRING,
	TK_NAME,
	TK_FLOAT,
	TK_INT,
	TK_NOTEQUAL,
	TK_GREATEREQUAL,
	TK_LESSEQUAL,
	TK_SHL,
	TK_SHR,
	TK_MOD,
	TK_DOT,
	TK_VARARG,
	TK_CONCAT,
	TK_EOS,
};

#define NUM_RESERVED (TK_FUNCTION - FIRST_REVERSED + 1)

typedef union Seminfo {
    lua_Number r;
    lua_Integer i;
    TString* s; 
} Seminfo;

typedef struct Token {
    int token;          // token enum value
    Seminfo seminfo;    // token info
} Token;

typedef struct LexState {
	Zio* zio;		// get a char from stream
    int current;	// current char in file
	struct MBuffer* buff;	// we cache a series of characters into buff, and recognize which token it is
	Token t;		// current token
	int linenumber;
	struct Dyndata* dyd;
	struct FuncState* fs;
	lua_State* L;
	TString* source;
	TString* env;
} LexState;

void luaX_init(struct lua_State* L);
void luaX_setinput(struct lua_State* L, LexState* ls, Zio* z, struct MBuffer* buffer, struct Dyndata* dyd, TString* source, TString* env);
int luaX_next(struct lua_State* L, LexState* ls);

#endif
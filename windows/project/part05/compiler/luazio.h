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
#ifndef LUA_ZIO
#define LUA_ZIO

#include "../common/lua.h"

typedef char* (*lua_Reader)(struct lua_State* L, void* data, size_t* size);

typedef struct LoadF {
    FILE* f;
    char buff[BUFSIZ]; // read the file stream into buff
	int n;		       // how many char you have read
} LoadF;

typedef struct Zio {
	lua_Reader reader;		// read buffer to p
	int n;					// the number of unused bytes
	void* p;				// the pointer to buffer
	void* data;				// structure which holds FILE handler
	struct lua_State* L;
} Zio;

#endif
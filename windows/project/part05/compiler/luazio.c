#include "luazio.h"

void luaZ_init(struct lua_State* L, Zio* zio, lua_Reader reader, void* data) {
	lua_assert(L);
	lua_assert(zio);
	lua_assert(data);

	zio->L = L;
	zio->data = data;
	zio->n = 0;
	zio->p = NULL;
	zio->reader = reader;
}

int luaZ_fill(Zio* z) {
	int c = 0;

	size_t read_size = 0;
	z->p = (void*)z->reader(z->L, z->data, &read_size);
	
	if (read_size > 0) {
		z->n = (int)read_size;
		c = (int)(*z->p);

		z->p++;
		z->n--;
	}
	else {
		c = EOF;
	}
	
	return c;
}
#include "../clib/luaaux.h"
#include "../vm/luagc.h"
#include "p2_test.h"
#include <time.h>

#define ELEMENTNUM 5

void p2_test_main() {
   struct lua_State* L = luaL_newstate(); 

   int i = 0;
   for (; i < ELEMENTNUM; i ++) {
       luaL_pushnil(L);
   }

   int start_time = (int)time(NULL);
   int end_time = (int)time(NULL);
   size_t max_bytes = 0;
   struct global_State* g = G(L);
   int j = 0;
   for (; j < 500000000; j ++) {
        TValue* o = luaL_index2addr(L, (j % ELEMENTNUM) + 1);
        struct GCObject* gco = luaC_newobj(L, LUA_TSTRING, sizeof(TString));
        o->value_.gc = gco;
        o->tt_ = LUA_TSTRING;
        luaC_checkgc(L);

        if ((g->totalbytes + g->GCdebt) > max_bytes) {
            max_bytes = g->totalbytes + g->GCdebt;
        }

        if (j % 1000 == 0) {
            printf("timestamp:%d totalbytes:%f kb \n", (int)time(NULL), (float)(g->totalbytes + g->GCdebt) / 1024.0f);
        }
   }
   end_time = time(NULL);
   printf("finish test start_time:%d end_time:%d max_bytes:%f kb \n", start_time, end_time, (float)max_bytes / 1024.0f);

   luaL_close(L);
}

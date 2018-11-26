#include "p3_test.h"
#include "../clib/luaaux.h"
#include "../vm/luagc.h"
#include "../common/luastring.h"
#include <time.h>

#define ELEMENTNUM 5

const char* g_shrstr = "This is a short string";
const char* g_lngstr = "This is a long string. This is a long string. This is a long string. This is a long string.";

int print(struct lua_State* L) {
    char* str = luaL_tostring(L, -1);
    printf("%s\n", str);
    return 0;
}

void test_print(struct lua_State* L, const char* str) {
    luaL_pushcfunction(L, print);
    luaL_pushstring(L, str);

    luaL_pcall(L, 1, 0);
}

void test_internal(struct lua_State* L, const char* str, int is_variant) {
    printf("test_internal \n");
    struct GCObject* previous = NULL;
    for (int i = 0; i < 5; i++) {
        if (is_variant && strlen(str) <= MAXSHORTSTR) {
            char buff[256] = { 0 };
            sprintf_s(buff, 256, "%s %d", str, i);
            luaL_pushstring(L, buff);
        }
        else {
            luaL_pushstring(L, str);
        }
        TValue* addr = luaL_index2addr(L, -1);
        if (addr->value_.gc == previous) 
            printf("The same \n");
        else 
            printf("Not the same \n");
        previous = addr->value_.gc;
    }
}

void test_string_cache(struct lua_State* L, int is_same_str) {
    printf("test_string_cache\n");
    struct TString* previous_ts = NULL;
    for (int i = 0; i < 5; i ++) {
        struct TString* ts = NULL;
    
        if (is_same_str) {
            ts = luaS_new(L, g_lngstr, strlen(g_lngstr));
        }
        else {
            char* buff = (char*)malloc(256);
            printf("buff addr %x \n", (unsigned int)(size_t)buff);
            sprintf_s(buff, 256, "%s", g_lngstr);
            ts = luaS_new(L, buff, strlen(buff));
            free(buff);
        }

        if (previous_ts == ts) {
            printf("string cache is same \n");
        }
        else {
            printf("string cache is not same \n");
        }
        previous_ts = ts;
    }
}

void test_gc(struct lua_State* L) {
   int start_time = time(NULL);
   int end_time = time(NULL);
   size_t max_bytes = 0;
   struct global_State* g = G(L);
   int j = 0;
   for (; j < 50000; j ++) {
        luaL_pushcfunction(L, print);
        luaL_pushstring(L, g_lngstr);
        luaL_pcall(L, 1, 0);
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
}

void p3_test_main() {
    printf("short string len:%ld, long string len:%ld \n", strlen(g_shrstr), strlen(g_lngstr));

    struct lua_State* L = luaL_newstate();

    test_print(L, g_shrstr);
    test_print(L, g_lngstr);
    test_internal(L, g_shrstr, 0);
    test_internal(L, g_shrstr, 1);
    test_internal(L, g_lngstr, 0);
    test_string_cache(L, 1);
    test_string_cache(L, 0);
    test_gc(L);

    luaL_close(L);
}

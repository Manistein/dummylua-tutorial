#include "../clib/luaaux.h"

// test case 1
static int test_main01(struct lua_State* L) {
    lua_Integer i = luaL_tointeger(L, -1);
    printf("test_main01 luaL_tointeger value = %d\n", i);
    return 0;
}

void p1_test_result01() { // nwant = 0; and nresult = 0;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main01);
    luaL_pushinteger(L, 1);
    luaL_pcall(L, 1, 0);
    luaL_close(L);
}

// test case 2
static int test_main02(struct lua_State* L) {
    lua_Integer i = luaL_tointeger(L, -1);
    i++;
    luaL_pushinteger(L, i);

    printf("test_main02 luaL_tointeger value = %d\n", i);
    return 1;
}

void p1_test_result02() { // nwant = 0; and nresult = 1;
    struct lua_State* L = luaL_newstate();

    printf("p1_test_result02 before push args test_main02 stacksize = %d\n", luaL_stacksize(L));

    luaL_pushcfunction(L, &test_main02);
    luaL_pushinteger(L, 1);
    
    printf("p1_test_result02 before call test_main02 stacksize = %d\n", luaL_stacksize(L));

    luaL_pcall(L, 1, 0);
    
    printf("p1_test_result02 after call test_main02 stacksize = %d\n", luaL_stacksize(L));

    luaL_close(L);
}

// test case 3
static int nret = 5;
static int test_main03(struct lua_State* L) {
    int b = 0;
    if (luaL_toboolean(L, -1)) {
        b = 1;
    }
    else {
        b = 0;
    }

    lua_Number n = luaL_tonumber(L, -2);
    printf("test_main03 b:%d n:%f\n", b, n); 

    int i;
    for (i = 0; i < nret; i++) {
        luaL_pushinteger(L, i);
    }

    return nret;
}

void p1_test_result03() { // nwant = 0; and nresult > 1;
    struct lua_State* L = luaL_newstate();
    printf("p1_test_result03 before push arg test_main03 stacksize = %d\n", luaL_stacksize(L));
    luaL_pushcfunction(L, &test_main03);
    luaL_pushnumber(L, 1.0f);
    luaL_pushboolean(L, true);
    printf("p1_test_result03 before call test_main03 stacksize = %d\n", luaL_stacksize(L));
    luaL_pcall(L, 2, 0);
    printf("p1_test_result03 after call test_main03 stacksize = %d\n", luaL_stacksize(L));
    luaL_close(L);
}

// test case 4
static int test_main04(struct lua_State* L) {
    int arg1 = luaL_tointeger(L, 1);
    int arg2 = luaL_tointeger(L, 2);

    printf("test_main04 arg1:%d arg2:%d\n", arg1, arg2);

    return 0;
}

void p1_test_result04() { // nwant = 1; and nresult = 0;
    struct lua_State* L = luaL_newstate();
    printf("p1_test_result04 before push arg test_main04 stacksize = %d\n", luaL_stacksize(L));
    luaL_pushcfunction(L, &test_main04);
    luaL_pushinteger(L, 1);
    luaL_pushinteger(L, 2);
    printf("p1_test_result04 before call test_main04 stacksize = %d\n", luaL_stacksize(L));
    luaL_pcall(L, 2, 1);
    printf("p1_test_result04 after call test_main04 stacksize = %d\n", luaL_stacksize(L));

    int isnil = luaL_isnil(L, -1);
    luaL_pop(L);
    printf("p1_test_result04 isnil:%d\n", isnil); 
    printf("p1_test_result04 stacksize = %d\n", luaL_stacksize(L));

    luaL_close(L);
}

// test case 5
static int test_main05(struct lua_State* L) {
    lua_Number n = luaL_tonumber(L, -1);
    lua_Integer i = luaL_tointeger(L, -2);
    
    printf("test_main05 n:%f i:%d\n", n, i);

    luaL_pushboolean(L, true);

    return 1;
}

void p1_test_result05() { // nwant = 1; and nresult = 1;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main05);
    luaL_pushinteger(L, 1);
    luaL_pushnumber(L, 2.0f);
    luaL_pcall(L, 2, 1);
    
    bool b = luaL_toboolean(L, -1);
    printf("p1_test_result05 top_value:%d\n", b?1:0);
    luaL_pop(L);

    printf("p1_test_result05 before close stack_size:%d\n", luaL_stacksize(L));
    luaL_close(L);
}

// test case 6
static int ntest_result6_ret = 5;
static int test_main06(struct lua_State* L) {
    int i;
    for (i = 0; i < ntest_result6_ret; i++) {
       luaL_pushinteger(L, i);
    }
    
    return ntest_result6_ret;
}

void p1_test_result06() { // nwant = 1; and nresult > 1;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main06);
    luaL_pushinteger(L, 1);
    luaL_pushnumber(L, 2.0f);
    luaL_pcall(L, 2, 1);
    
    printf("p1_test_result06 after call stacksize:%d\n", luaL_stacksize(L));
    lua_Integer v = luaL_tointeger(L, -1);
    printf("p1_test_result06 top value:%d\n", v);
    luaL_pop(L);
    printf("p1_test_result06 final stacksize:%d\n", luaL_stacksize(L));
    
    luaL_close(L);
}

// test case 7
static int test_result07_nwant = 8;
static int test_main07(struct lua_State* L) {
    return 0;
}

void p1_test_result07() { // nwant > 1; and nresult = 0;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main07);
    luaL_pcall(L, 0, test_result07_nwant);
    
    printf("p1_test_result07 after call stack_size:%d\n", luaL_stacksize(L));
    int i;
    for (i = 0; i < test_result07_nwant; i++) {
        int isnil = luaL_isnil(L, -1);
        luaL_pop(L);
        printf("p1_test_result07 stack_idx:%d isnil:%d\n", test_result07_nwant - i, isnil);
    } 
    printf("p1_test_result07 final stack_size:%d\n", luaL_stacksize(L));
    
    luaL_close(L);
}

// test case 8
static int test_result08_nwant = 8;
static int test_result08_nresult = 7;
static int test_main08(struct lua_State* L) {
    int i;
    for (i = 0; i < test_result08_nresult; i++) {
        luaL_pushinteger(L, i + 10);
    }
    
    return test_result08_nresult;
}

void p1_test_result08() { // nwant > 1; and nresult > 0;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main08);
    luaL_pcall(L, 0, test_result08_nwant);
    
    printf("p1_test_result08 after call stack_size:%d\n", luaL_stacksize(L));
    int i;
    for (i = 0; i < test_result08_nwant; i++) {
        if (i != 0) {
            lua_Integer integer = luaL_tointeger(L, -1);
            luaL_pop(L);
            printf("p1_test_result08 stack_idx:%d integer:%d\n", test_result08_nwant - i, integer);
        }
        else {
            int isnil = luaL_isnil(L, -1);
            luaL_pop(L);
            printf("p1_test_result08 stack_idx:%d isnil:%d\n", test_result08_nwant - i, isnil);
        }
    } 
    printf("p1_test_result08 final stack_size:%d\n", luaL_stacksize(L));
    
    luaL_close(L);
}

// test case 9
static int test_main09(struct lua_State* L) {
    return 0;
}

void p1_test_result09() { // nwant = -1; and nresult = 0;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main09);
    printf("p1_test_result09 before call  stack_size:%d\n", luaL_stacksize(L));
    luaL_pcall(L, 0, -1);
    printf("p1_test_result09 after call stack_size:%d\n", luaL_stacksize(L));
    luaL_close(L);
}

// test case 10
static int test_main10(struct lua_State* L) {
    int ret_count = 10;

    int i;
    for (i = 0; i < ret_count; i++) {
        luaL_pushinteger(L, i);
    }

    return ret_count;
}

void p1_test_result10() { // nwant = -1; and nresult > 0;
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_main10);
    luaL_pcall(L, 0, LUA_MULRET);

    int stack_size = luaL_stacksize(L);
    printf("p1_test_result10 after call stack_size:%d\n", stack_size);

    int i;
    for (i = 0; i < stack_size; i++) {
        lua_Integer integer = luaL_tointeger(L, -1);
        luaL_pop(L);
        printf("stack value %d\n", integer);
    }
    printf("p1_test_result10 final stack_size:%d\n", luaL_stacksize(L));

    luaL_close(L);
}

// test case nest call 01
static int current_calls = 0;
static struct CallInfo* test_ci = NULL;
static int test_nestcal01(struct lua_State* L) {
    lua_Integer arg = luaL_tointeger(L, -1);
    current_calls += 1;
    int temp = current_calls;
    //printf("test_nestcal01 enter_calls %d\n", current_calls);

    if (current_calls < LUA_MAXCALLS - 1) {
        luaL_pushcfunction(L, &test_nestcal01);
        luaL_pushinteger(L, arg + 1);
        luaL_pcall(L, 1, 1);
        arg = luaL_tointeger(L, -1);
        luaL_pop(L);
    }
    //printf("test_nestcal01 current_calls:%d stack_size:%d\n", temp, luaL_stacksize(L));
    luaL_pushinteger(L, arg);

    return 1;
}
void p1_test_nestcall01() { // call count < LUA_MAXCALLS
    struct lua_State* L = luaL_newstate();
    luaL_pushcfunction(L, &test_nestcal01);
    luaL_pushinteger(L, 1);
    luaL_pcall(L, 1, 1);

    printf("p1_test_nestcall01 result = %d stack_size:%d\n", luaL_tointeger(L, -1), luaL_stacksize(L));
    luaL_pop(L);

    luaL_close(L);
}

void p1_test_nestcall02() { // call count >= LUA_MAXCALLS
}

void p1_test_main() {
    printf("\n--------------------test case 1--------------------------\n");
    p1_test_result01();
    printf("\n--------------------test case 2--------------------------\n");
    p1_test_result02();
    printf("\n--------------------test case 3--------------------------\n");
    p1_test_result03();
    printf("\n--------------------test case 4--------------------------\n");
    p1_test_result04();
    printf("\n--------------------test case 5--------------------------\n");
    p1_test_result05();
    printf("\n--------------------test case 6--------------------------\n");
    p1_test_result06();
    printf("\n--------------------test case 7--------------------------\n");
    p1_test_result07();
    printf("\n--------------------test case 8--------------------------\n");
    p1_test_result08();
    printf("\n--------------------test case 9--------------------------\n");
    p1_test_result09();
    printf("\n--------------------test case 10-------------------------\n");
    p1_test_result10();
    printf("\n--------------------test case 11-------------------------\n");
    p1_test_nestcall01();

}

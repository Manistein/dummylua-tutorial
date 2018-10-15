#include "clib/luaaux.h"
#include "test/p1_test.h"

int main(int argc, char** argv) {
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

	system("pause");
    return 0;
}

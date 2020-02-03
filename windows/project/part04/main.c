#include "clib/luaaux.h"
#include "test/p1_test.h"
#include "test/p2_test.h"
#include "test/p3_test.h"
#include "test/p4_test.h"
#include <process.h>

int main(int argc, char** argv) {
    // p1_test_main(); 
    // p2_test_main();
    // p3_test_main();
    p4_test_main();

	system("pause");
    return 0;
}

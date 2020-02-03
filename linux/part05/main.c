#include "test/p5_test.h"
#ifdef _WINDOWS_PLATFORM_
#include <process.h>
#endif

int main(int argc, char** argv) {
	p5_test_main();

#ifdef _WINDOWS_PLATFORM_
	system("pause");
#endif

	return 0;
}

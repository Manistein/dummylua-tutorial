#ifndef p1_test_h
#define p1_test_h

// test result
void p1_test_result01(); // nwant = 0; and nresult = 0;
void p1_test_result02(); // nwant = 0; and nresult = 1;
void p1_test_result03(); // nwant = 0; and nresult > 1;
void p1_test_result04(); // nwant = 1; and nresult = 0;
void p1_test_result05(); // nwant = 1; and nresult = 1;
void p1_test_result06(); // nwant = 1; and nresult > 1;
void p1_test_result07(); // nwant > 1; and nresult = 0;
void p1_test_result08(); // nwant > 1; and nresult > 0;
void p1_test_result09(); // nwant = -1; and nresult = 0;
void p1_test_result10(); // nwant = -1; and nresult > 0;

void p1_test_nestcall01(); // call count < LUA_MAXCALLS
void p1_test_nestcall02(); // call count >= LUA_MAXCALLS

#endif 

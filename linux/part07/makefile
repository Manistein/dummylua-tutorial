OUTPUT = ./bin
COMMON = ./common
VM = ./vm
COMPILER = ./compiler
CLIB = ./clib
TEST = ./test

CLIB_SRC = luaaux.c
COMMON_SRC = luabase.c luainit.c luamem.c luaobject.c luastate.c luastring.c luatable.c
COMPILER_SRC = luacode.c lualexer.c luaparser.c luazio.c
VM_SRC = luado.c luafunc.c luagc.c luaopcodes.c luavm.c
TEST_SRC = p*_test.c

dummylua: main.c $(foreach v, $(CLIB_SRC), $(CLIB)/$(v)) \
	$(foreach v, $(COMMON_SRC), $(COMMON)/$(v)) \
	$(foreach v, $(COMPILER_SRC), $(COMPILER)/$(v)) \
	$(foreach v, $(VM_SRC), $(VM)/$(v)) \
	$(foreach v, $(TEST_SRC), $(TEST)/$(v))
	gcc -g -o $@ $^ -I$(CLIB) \
		-I$(COMMON) \
		-I$(COMPILER) \
		-I$(VM) \
		-I$(TEST) \
		-lm

clean:
	rm dummylua 

OUTPUT = ./bin
COMMON = ./common
VM = ./vm
CLIB = ./clib
TEST = ./test
OBJECTS = main.o luaaux.o luastate.o luaobject.o luamem.o luado.o luagc.o luastring.o p1_test.o p2_test.o p3_test.o

dummylua: $(OBJECTS) 
	gcc -g -o $(OUTPUT)/dummylua \
		$(OUTPUT)/main.o \
		$(OUTPUT)/luaaux.o \
		$(OUTPUT)/luastate.o \
		$(OUTPUT)/luamem.o \
		$(OUTPUT)/luado.o \
		$(OUTPUT)/luagc.o \
		$(OUTPUT)/luastring.o \
		$(OUTPUT)/p1_test.o \
		$(OUTPUT)/p2_test.o \
		$(OUTPUT)/p3_test.o

main.o: main.c $(CLIB)/luaaux.h $(TEST)/p1_test.h $(TEST)/p2_test.h $(TEST)/p3_test.h
	gcc -c -g main.c -o $(OUTPUT)/main.o

luaaux.o: $(CLIB)/luaaux.c $(CLIB)/luaaux.h $(COMMON)/luastate.h $(VM)/luado.h
	gcc -c -g $(CLIB)/luaaux.c -o $(OUTPUT)/luaaux.o

luastate.o: $(COMMON)/luastate.c $(COMMON)/luastate.h $(COMMON)/luamem.h  $(COMMON)/luaobject.h $(COMMON)/luastring.h
	gcc -c -g $(COMMON)/luastate.c -o $(OUTPUT)/luastate.o

luaobject.o: $(COMMON)/luaobject.c $(COMMON)/luaobject.h $(COMMON)/lua.h
	gcc -c -g $(COMMON)/luaobject.c -o $(OUTPUT)/luaobject.o

luamem.o: $(COMMON)/luamem.c $(COMMON)/luastate.h $(VM)/luado.h
	gcc -c -g $(COMMON)/luamem.c -o $(OUTPUT)/luamem.o

luado.o: $(VM)/luado.c $(COMMON)/luastate.h $(COMMON)/luamem.h
	gcc -c -g $(VM)/luado.c -o $(OUTPUT)/luado.o

luagc.o: $(VM)/luagc.c $(COMMON)/luastate.h $(COMMON)/luamem.h 
	gcc -c -g $(VM)/luagc.c -o $(OUTPUT)/luagc.o

luastring.o: $(COMMON)/luastring.c $(COMMON)/luastate.h $(VM)/luagc.h $(COMMON)/luamem.h
	gcc -c -g $(COMMON)/luastring.c -o $(OUTPUT)/luastring.o

p1_test.o: $(TEST)/p1_test.c $(CLIB)/luaaux.h
	gcc -c -g $(TEST)/p1_test.c -o $(OUTPUT)/p1_test.o	

p2_test.o: $(TEST)/p2_test.c $(CLIB)/luaaux.h $(VM)/luagc.h
	gcc -c -g $(TEST)/p2_test.c -o $(OUTPUT)/p2_test.o

p3_test.o: $(TEST)/p3_test.c $(CLIB)/luaaux.h $(VM)/luagc.h
	gcc -c -g $(TEST)/p3_test.c -o $(OUTPUT)/p3_test.o

clean:
	rm $(OUTPUT)/* 

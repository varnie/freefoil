CCC=g++

STANDARD_INC = /usr/local/include/
INCDIRS   = -I${STANDARD_INC}
CFLAGS    = ${INCDIRS}

freefoil: main.o
	$(CCC) ${CFLAGS} -o freefoil main.cpp compiler.cpp memory_manager.cpp tree_analyzer.cpp codegen.cpp -I/usr/local/include/
all:
	${MAKE} freefoil
clean:
	-rm *.o
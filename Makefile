CCC=g++

STANDARD_INC = /usr/local/include/
INCDIRS   = -I${STANDARD_INC}
CFLAGS    = ${INCDIRS}

freefoil: main.o
	$(CCC) ${CFLAGS} -o freefoil main.cpp script.cpp -I/usr/local/include/
all:
	${MAKE} freefoil
clean:
	-rm *.o
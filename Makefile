main: main.o
	g++ -o freefoil main.o
    main.o: main.cpp 
	g++ -c main.cpp -I/usr/local/include/	
clean:
	rm *.o
all:	
	${MAKE} main
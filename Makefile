shell : shell.o util.o
	gcc -o shell shell.o util.o -lncurses -g

shell.o : shell.c header.h
	gcc -c shell.c -lncurses -g
	
clean :
	rm *.o shell

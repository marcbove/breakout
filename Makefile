# una pilota
mur0 : mur0.c winsuport.o
	gcc -Wall -g mur0.c winsuport.o -o mur0 -lcurses 

#  amb threads
mur1 : mur1.c winsuport.o
	gcc -Wall -g mur1.c winsuport.o -o mur1 -lcurses -lpthread

#  amb mutex
mur2 : mur2.c winsuport.o
	gcc -Wall -g mur2.c winsuport.o -o mur2 -lcurses -lpthread 

#  amb processos
mur3 : mur3.c winsuport2.o memoria.o
	gcc -Wall -g mur3.c winsuport2.o memoria.o -o mur3 -lcurses -lpthread 

pilota3 : pilota3.c winsuport2.o memoria.o
	gcc -Wall -g pilota3.c winsuport2.o -o pilota3 -lcurses


# auxiliars per a les curses
winsuport.o : winsuport.c winsuport.h
	gcc -c -Wall winsuport.c

winsuport2.o : winsuport2.c winsuport2.h
	gcc -c -Wall winsuport2.c 

memoria.o : memoria.c memoria.h
	gcc -c -Wall memoria.c -o memoria.o 

semafor.o : semafor.c semafor.h
	gcc -c -Wall semafor.c -o semafor.o 

missatge.o : missatge.c missatge.h
	gcc -c -Wall missatge.c -o missatge.o

clean:
	rm -f multifil mf_mutex

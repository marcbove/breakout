# una pilota
mur0 : mur0.c winsuport.o winsuport.h
	gcc -Wall -g mur0.c winsuport.o -o mur0 -lcurses 

#  amb threads
mur1 : mur1.c winsuport.o winsuport.h
	gcc -Wall -g mur1.c winsuport.o -o mur1 -lcurses -lpthread



# auxiliars per a les curses
winsuport.o : winsuport.c winsuport.h
	gcc -c -Wall winsuport.c 

clean:
	rm -f multifil mf_mutex

rbhash-demo: rbhash.o
	gcc -o rbhash-demo rbhash.o

rbhash.c rbhash.h: rbhash.cp
	cpppp rbhash.cp --features=print,demo --section-out public=rbhash.h -o rbhash.c

rbhash.o: rbhash.c rbhash.h
	gcc -c -O0 -g3 -Wall -o rbhash.o rbhash.c


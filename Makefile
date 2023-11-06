rbhash-demo: rbhash.o
	gcc -o rbhash-demo rbhash.o

rbhash.c rbhash.h: rbhash.cpppp
	cpppp rbhash.cpppp --features=print,demo --section-out public=rbhash.h -o rbhash.c

rbhash.o: rbhash.c rbhash.h
	gcc -c -O1 -g3 -Wall -o rbhash.o rbhash.c

clean:
	rm -f rbhash.c rbhash.c.* rbhash.h rbhash.h.* rbhash.o rbhash-demo

.PHONY: clean


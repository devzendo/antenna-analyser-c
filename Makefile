CFLAGS=-Wall -I/Developer/SDKs/MacOSX10.6.sdk/usr/include -I/Developer/usr/lib/gcc/i686-apple-darwin10/4.2.1/include

all: analyser

asy.c: asy.h

analyser.c: global.h util.h asy.h

analyser.o: asy.c analyser.c util.c

analyser: asy.o analyser.o util.o
	cc -o analyser asy.o analyser.o util.o -L/Developer/SDKs/MacOSX10.6.sdk/usr/lib

clean:
	rm -f *.o analyser


tags: ctags
ctags:
	rm -f TAGS tags
	ctags *.[ch]


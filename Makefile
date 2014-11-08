# Choose one platform

# For Mac OS X 10.7
PLATFORM=MACOSX

# For Ubuntu, Debian, Mint
#PLATFORM=DEBIAN

# For Red Hat, Fedora, CentOS
#PLATFORM=REDHAT

# For the Raspberry Pi, hurrah!
#PLATFORM=RASPBIAN

CFLAGS=-Wall -D${PLATFORM}

all: analyser

asy.c: asy.h

analyser.c: global.h util.h asy.h

analyser.o: asy.c analyser.c util.c

analyser: asy.o analyser.o util.o
	cc -o analyser asy.o analyser.o util.o

clean:
	rm -f *.o analyser


tags: ctags
ctags:
	rm -f TAGS tags
	ctags *.[ch]


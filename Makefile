CFLAGS=-m64 -Wall -mtune=native -O3 -std=c99
#CFLAGS=-m64 -Wall -mtune=native -O2 -std=c99
#CFLAGS=-g
MINGWFLAGS=-m64 -Wall -O -std=c99

CC=gcc
MINGWCC=x86_64-w64-mingw32-gcc.exe
CLANG=clang

all:	rescale rescale16

rescale:
	$(CC) $(CFLAGS) -o rescale rescale.c

mac:
	$(CLANG) -o rescale rescale.c	

rescale_dbg:
	$(CC) -g -o rescale_dbg rescale.c

rescale16:
	$(CC) $(CFLAGS) -DUINT16 -o rescale_uint16 rescale.c

rescale16_dbg:
	$(CC) $(CFLAGS) -g -DUINT16 -o rescale_uint16_dbg rescale.c

clean:
	rm rescale rescale_uint16

prof:
	$(CC) -g -pg -o rescale-prof rescale.c

windows:
	$(MINGWCC) $(MINGWFLAGS) -o rescale.exe rescale.c

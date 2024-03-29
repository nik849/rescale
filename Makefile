CFLAGS=-m64 -Wall -mtune=native -O3 -std=c99
#CFLAGS=-m64 -Wall -mtune=native -O2 -std=c99
#CFLAGS=-g
MINGWFLAGS=-m64 -Wall -O -std=c99
MACFLAGS=-Wall -O -std=c99

CC=gcc
MINGWCC=i686-w64-mingw32-gcc#x86_64-w64-mingw32-gcc.exe
CLANG=clang

all:	rescale rescale16

rescale:
	$(CC) $(CFLAGS) -o rescale rescale.c

mac:
	$(CLANG) $(MACFLAGS) -v -o rescale rescale.c

mac_dbg:
	$(CLANG) -g -o rescale_dbg rescale.c

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
	$(MINGWCC) $(MACFLAGS) -o rescale.exe rescale.c

#uncomment MWIDTH to enable 64-bit compilation
MWIDTH=-m64
#uncomment LARGE_FILE_SUPPORT to enable support for files larger than 2GB on 32-bit systems
LARGE_FILE_SUPPORT=-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_USE_LARGEFILE64


#uncomment for grid alignment with origin support
DFLAGS +=-DALIGN_ORIGIN

DFLAGS +=-DHARD_LIMIT="500000000"

CC=g++

CFLAGS=-O3 -DNDEBUG $(MWIDTH) -Wall -g $(LARGE_FILE_SUPPORT) 

INC=-I/usr/local/include/curl -I/usr/local/include/liblas
LIB=-L/usr/local/lib/curl -L/usr/local/lib -llas -lm -lcurl

OBJS=main.o Interpolation.o InCoreInterp.o OutCoreInterp.o GridMap.o GridFile.o

#all: interp ctags
all: interp

interp: $(OBJS)
	$(CC) $(CFLAGS) $(DFLAGS) $(LIB) $(INC) -o $@ $(OBJS)

.cpp.o:
	$(CC) $(CFLAGS) $(DFLAGS) $(INC) -c $<

clean:
	rm -f interp *.o
	rm -f tags

ctags:
	ctags *.[ch]	

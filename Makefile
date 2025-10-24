ifeq ($(CC),gcc)
CFLAGS = -O3
else
CC = g++
CFLAGS = -O3 -x c++
endif

CLFLAGS = -O3
OBJ = lnklst.o cmpile.o cclang.o intrpt.o bytecd.o ciclib.o ccmain.o main.o

cicada: $(OBJ)
	$(CC) $(CLFLAGS) -o cicada $(OBJ) -lm


lnklst.o: lnklst.h lnklst.c
cmpile.o: lnklst.h cmpile.h cmpile.c
cclang.o: lnklst.h cmpile.h cclang.h cclang.c
intrpt.o: lnklst.h cmpile.h cclang.h intrpt.h bytecd.h intrpt.c
bytecd.o: lnklst.h cmpile.h cclang.h intrpt.h bytecd.h ciclib.h bytecd.c
ciclib.o: lnklst.h cmpile.h cclang.h intrpt.h bytecd.h ciclib.h ccmain.h ciclib.c
ccmain.o: lnklst.h cmpile.h cclang.h intrpt.h bytecd.h ciclib.h ccmain.h ccmain.c
ifeq ($(CC),gcc)
main.o: lnklst.h ccmain.h main.h main.c
else
main.o: lnklst.h ccmain.h main.h main.cpp
endif

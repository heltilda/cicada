CFLAGS = -O3
CC = gcc
OBJ = lnklst.o cmpile.o cicada.o intrpt.o bytecd.o ciclib.o userfn.o ccmain.o

cicada: $(OBJ)
	$(CC) $(CFLAGS) -o cicada $(OBJ) -lm

lnklst.o: lnklst.h lnklst.c
cmpile.o: lnklst.h cmpile.h cmpile.c
cicada.o: lnklst.h cmpile.h cicada.h cicada.c
intrpt.o: lnklst.h cmpile.h cicada.h intrpt.h bytecd.h userfn.h intrpt.c
bytecd.o: lnklst.h cmpile.h cicada.h intrpt.h bytecd.h ciclib.h userfn.h bytecd.c
ciclib.o: lnklst.h cmpile.h cicada.h intrpt.h bytecd.h ciclib.h userfn.h ccmain.h ciclib.c
userfn.o: lnklst.h intrpt.h userfn.h ccmain.h userfn.c
ccmain.o: lnklst.h cmpile.h cicada.h intrpt.h bytecd.h ciclib.h userfn.h ccmain.h ccmain.c

AC_INIT([cicada],[1.0],[cicada.scripting@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
dnl AM_PROG_AR
AC_PROG_RANLIB

AC_CHECK_HEADERS([errno.h float.h limits.h math.h stdarg.h stdbool.h stdio.h stdlib.h string.h time.h])
AC_C_CONST

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

dnl $(LIBTOOL) -static -o libcicada.a lnklst.o cmpile.o cclang.o intrpt.o bytecd.o ciclib.o ccmain.o
dnl $(AR) $(ARFLAGS) libcicada.a lnklst.o cmpile.o cclang.o intrpt.o bytecd.o ciclib.o ccmain.o
dnl $(AR) cr libcicada.a lnklst.o cmpile.o cclang.o intrpt.o bytecd.o ciclib.o ccmain.o
dnl $(RANLIB) libcicada.a

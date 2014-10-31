AC_PREREQ([2.59])
AC_INIT([test-server-types], [0.1], [https://github.com/jwendell/test-server-types/issues/new], [test-server-types], [https://github.com/jwendell/test-server-types])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/client.c])

AM_INIT_AUTOMAKE([1.11.1 -Wall])
AM_SILENT_RULES([yes])

#AC_CANONICAL_SYSTEM
AC_PROG_CC

PKG_CHECK_MODULES([libevent], [libevent >= 2.0])
AC_SUBST([libevent_CFLAGS])
AC_SUBST([libevent_LIBS])

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT


# An autotools project (tests/integration/run.sh: a copy, autoreconf -i, then
#   ./configure CXX=<prefix>/bin/ycxx-c++ CC=<prefix>/bin/ycxx-cc && make check).
AC_INIT([ycxx-integration-autotools], [1.0])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AC_CHECK_HEADERS([optional string_view], [], [AC_MSG_ERROR([missing a C++17 header])])
AC_MSG_CHECKING([for std::format])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <format>]], [[return std::format("{}", 1).size() == 1 ? 0 : 1;]])],
  [AC_MSG_RESULT([yes])], [AC_MSG_ERROR([no std::format])])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

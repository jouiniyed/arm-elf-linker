#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([Arm Examples],[1.0],[Guillaume.Huard@imag.fr],[arm-examples])

AC_MSG_NOTICE([=======> configuring ARM examples for merge (fusion)])

AC_CONFIG_AUX_DIR([../build-aux])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([file1.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
VD_CONFIG_CROSS_ARM

AS_CASE(["$have_arm_compiler"],
[yes], [
  AX_CHECK_COMPILE_FLAG([-no-pie], [VD_CROSS_NOPIE_CFLAGS="-no-pie"], [], [-Werror])
])
AC_SUBST([VD_CROSS_NOPIE_CFLAGS])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libdxfrw], [2.0.0], [https://sourceforge.net/projects/libdxfrw/])
AM_INIT_AUTOMAKE([foreign -Wall -Werror dist-bzip2 subdir-objects])
AC_CONFIG_SRCDIR([src/drw_interface.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Match the public ABI major used by CMake's SOVERSION.
LIBDXFRW_ABI_MAJOR=2
AC_SUBST([LIBDXFRW_ABI_MAJOR])

# Check the required language/library without running target executables.
AC_LANG_PUSH([C++])
LIBDXFRW_ENTITY_WARNING_FLAGS=
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#if !defined(__GNUC__) || defined(__clang__)
#error not the GNU C++ compiler
#endif]], [[]])], [LIBDXFRW_ENTITY_WARNING_FLAGS=-Wno-extra])
AC_SUBST([LIBDXFRW_ENTITY_WARNING_FLAGS])
CXXFLAGS="$CXXFLAGS -std=c++17"
AC_MSG_CHECKING([for C++17 and the filesystem library])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <filesystem>
#include <optional>
#include <string_view>
#if __cplusplus < 201703L
#error C++17 is required
#endif]], [[std::optional<std::string_view> name("libdxfrw");
std::filesystem::path path(*name); return path.empty();]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_FAILURE([a C++17 compiler and standard library with filesystem support are required])])

PTHREAD_CFLAGS=
PTHREAD_LIBS=
libdxfrw_saved_cxxflags=$CXXFLAGS
libdxfrw_saved_libs=$LIBS
libdxfrw_threads=no
AC_MSG_CHECKING([for C++ threads])
for libdxfrw_thread_option in pthread none library; do
    PTHREAD_CFLAGS=
    PTHREAD_LIBS=
    AS_CASE([$libdxfrw_thread_option],
        [pthread], [PTHREAD_CFLAGS=-pthread; PTHREAD_LIBS=-pthread],
        [library], [PTHREAD_LIBS=-lpthread])
    CXXFLAGS="$libdxfrw_saved_cxxflags $PTHREAD_CFLAGS -Werror"
    LIBS="$PTHREAD_LIBS $libdxfrw_saved_libs"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <thread>
#include <mutex>]], [[std::mutex mutex; mutex.lock(); mutex.unlock();
std::thread thread([] {}); thread.join();]])], [libdxfrw_threads=yes; break])
done
CXXFLAGS=$libdxfrw_saved_cxxflags
LIBS=$libdxfrw_saved_libs
AS_IF([test "$libdxfrw_threads" != yes], [AC_MSG_FAILURE([C++ thread support is required])])
AC_MSG_RESULT([$libdxfrw_threads $PTHREAD_CFLAGS $PTHREAD_LIBS])
AC_SUBST([PTHREAD_CFLAGS])
AC_SUBST([PTHREAD_LIBS])

COREFOUNDATION_LIBS=
AS_CASE([$host_os], [darwin*], [
    COREFOUNDATION_LIBS="-framework CoreFoundation"
    LIBS="$COREFOUNDATION_LIBS $libdxfrw_saved_libs"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <CoreFoundation/CoreFoundation.h>]],
        [[CFStringRef value = CFStringCreateWithCString(nullptr, "libdxfrw", kCFStringEncodingUTF8); CFRelease(value);]])],
        [], [AC_MSG_FAILURE([CoreFoundation is required for the macOS converter])])
    LIBS=$libdxfrw_saved_libs
])
AC_SUBST([COREFOUNDATION_LIBS])
AC_LANG_POP([C++])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([sqrt])

AC_ENABLE_SHARED
AC_ENABLE_STATIC

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	dwg2dxf/Makefile
	libdxfrw.pc:libdxfrw-autotools.pc.in
	])
AC_OUTPUT

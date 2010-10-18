#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(vcmi, 0.82b)				# Follow NAME_VER in global.h
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([aclocal/m4])

# Checks for build toolchain
AC_LANG(C++)
AC_PROG_CXX
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Use silent rules if supported.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE(debug,
		[  --disable-debug         turn off debugging (default=enable)],
		[ if test x$enableval = xyes ; then
		  	 enable_debug="yes"
		  else
		     enable_debug="no"
		  fi],
		  [ enable_debug="yes" ])
if test "x$GXX" = "xyes" -a "x$enable_debug" = "xyes" ; then
   CXXFLAGS="-O0 -ggdb"
fi

if test "x$GXX" = "xyes" ; then
   CXXFLAGS="$CXXFLAGS -Wall -Wno-switch -Wno-sign-compare -Wcast-align -Wpointer-arith"
fi

CXXFLAGS="$CXXFLAGS -DDATA_DIR=\\\"\$(pkgdatadir)\\\" -DBIN_DIR=\\\"\$(bindir)\\\" -DLIB_DIR=\\\"\$(pkglibdir)\\\""

# Check for Boost libraries
AX_BOOST_BASE([1.36])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_THREAD
AX_BOOST_IOSTREAMS

# Check for SDL libraries
saved_LIBS=$LIBS
AC_CHECK_PROG(HAVE_SDL_CONFIG, sdl-config, yes, no)
if test "$HAVE_SDL_CONFIG" = no; then
AC_MSG_ERROR(sdl-config doesn't exist. Try to check the PATH enviroment.)
fi

AC_CHECK_LIB(SDL, SDL_Init,,AC_MSG_ERROR([SDL library not found. Please install it.]))
AC_CHECK_LIB(SDL_mixer,main,,AC_MSG_ERROR([SDL_mixer library not found. Please install it.]))
AC_CHECK_LIB(SDL_image,IMG_Load,,AC_MSG_ERROR([SDL_image library not found. Please install it.]))
AC_CHECK_LIB(SDL_ttf,main,,AC_MSG_ERROR([SDL_ttf library not found. Please install it.]))

SDL_CXXFLAGS=`sdl-config --cflags`
SDL_LIBS="`sdl-config --libs` $LIBS"
LIBS=$saved_LIBS

# Check for ffmpeg libraries
saved_LIBS=$LIBS
AC_CHECK_LIB(avformat,main,,AC_MSG_ERROR([FFMpeg avformat library not found. Please install it.]))
AC_CHECK_LIB(swscale,main,,AC_MSG_ERROR([FFMpeg swscale library not found. Please install it.]))

FFMPEG_CXXFLAGS=`pkg-config --cflags libavformat libswscale`
FFMPEG_LIBS=`pkg-config --libs libavformat libswscale`
LIBS=$saved_LIBS

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h])
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_CHECK_HEADERS([boost/filesystem.hpp boost/algorithm/string.hpp boost/algorithm/string/replace.hpp boost/filesystem/operations.hpp boost/assign/std/vector.hpp boost/algorithm/string/find.hpp boost/function.hpp boost/bind.hpp],,AC_MSG_ERROR([Required boost headers not found. Please install it.]))

# Checks for library functions.
AC_CHECK_FUNCS([atexit memset pow select sqrt])

AC_SUBST(SDL_LIBS)
AC_SUBST(SDL_CXXFLAGS)
AC_SUBST(FFMPEG_LIBS)
AC_SUBST(FFMPEG_CXXFLAGS)

AC_OUTPUT(Makefile lib/Makefile client/Makefile server/Makefile AI/Makefile AI/GeniusAI/Makefile AI/EmptyAI/Makefile)

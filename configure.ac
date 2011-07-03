#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(vcmi, 0.84b)				# Follow NAME_VER in global.h
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([aclocal/m4])

# Checks for build toolchain
AC_LANG(C++)
AC_PROG_CXX
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AX_COMPILER_VENDOR

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
   CXXFLAGS="$CXXFLAGS -O0 -ggdb"
fi

# For gcc and compatible compilers, enable compilation warnings, but
# selectively disable some because too many are generated.
if test "x$GXX" = "xyes" ; then
   CXXFLAGS="$CXXFLAGS -Wall -Wno-switch -Wno-sign-compare -Wcast-align -Wpointer-arith -Wno-unknown-pragmas"
fi

# extra clang parameters
if test "x${ax_cv_cxx_compiler_vendor}"  =  "xclang" ; then
   CXXFLAGS="$CXXFLAGS -Wno-address-of-temporary -Wno-unused-value -Wno-cast-align"
fi

# extra Intel icc parameters
if test "x${ax_cv_cxx_compiler_vendor}"  =  "xintel" ; then
   CXXFLAGS="$CXXFLAGS -diag-disable 383,981,1418,869,1563,2259,1782,444,271,1,10156,1419"
fi

# Check for Boost libraries
AX_BOOST_BASE([1.36])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_THREAD
AX_BOOST_IOSTREAMS
AX_BOOST_PROGRAM_OPTIONS

# Check for SDL headers and libraries
saved_LIBS=$LIBS
saved_CFLAGS=$CFLAGS

AC_CHECK_PROG(HAVE_SDL_CONFIG, sdl-config, yes, no)
if test "$HAVE_SDL_CONFIG" = no; then
AC_MSG_ERROR(sdl-config doesn't exist. Try to check the PATH enviroment.)
fi

SDL_CXXFLAGS="`sdl-config --cflags`"
LIBS="$LIBS `sdl-config --libs`"

CFLAGS="$CFLAGS $SDL_CXXFLAGS"

AC_CHECK_HEADERS([SDL/SDL.h SDL/SDL_mixer.h SDL/SDL_image.h SDL/SDL_ttf.h], [], [AC_MSG_ERROR([Header not found. Install missing devel package.])])

AC_CHECK_LIB(SDL,SDL_Init,,AC_MSG_ERROR([SDL library not found. Please install it.]))
AC_CHECK_LIB(SDL_mixer,main,,AC_MSG_ERROR([SDL_mixer library not found. Please install it.]))
AC_CHECK_LIB(SDL_image,IMG_Load,,AC_MSG_ERROR([SDL_image library not found. Please install it.]))
AC_CHECK_LIB(SDL_ttf,main,,AC_MSG_ERROR([SDL_ttf library not found. Please install it.]))

SDL_LIBS="$LIBS"

LIBS=$saved_LIBS
CFLAGS=$saved_CFLAGS

# Check for ffmpeg libraries
saved_LIBS=$LIBS
saved_CFLAGS=$CFLAGS

FFMPEG_CXXFLAGS=""
FFMPEG_LIBS=

LIBS="$LIBS `pkg-config --libs libavformat libswscale`"
CFLAGS="$CFLAGS `pkg-config --cflags libavformat libswscale`"

AC_CHECK_LIB(avformat,av_open_input_file,,AC_MSG_ERROR([FFMpeg avformat library not found. Please install it.]))
AC_CHECK_LIB(swscale,sws_getContext,,AC_MSG_ERROR([FFMpeg swscale library not found. Please install it.]))
AC_CHECK_LIB(avcodec,avcodec_decode_video2,AVCODEC_DECODE_VIDEO2="-DWITH_AVCODEC_DECODE_VIDEO2",)
AC_CHECK_LIB(avcodec,av_register_protocol2,AVCODEC_DECODE_VIDEO2="-DWITH_AV_REGISTER_PROTOCOL2",)

FFMPEG_LIBS="$LIBS"
FFMPEG_CXXFLAGS="$CFLAGS $AVCODEC_DECODE_VIDEO2"

LIBS=$saved_LIBS
CFLAGS=$saved_CFLAGS

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h])
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_CHECK_HEADERS([boost/filesystem.hpp boost/algorithm/string.hpp boost/algorithm/string/replace.hpp boost/filesystem/operations.hpp boost/assign/std/vector.hpp boost/algorithm/string/find.hpp boost/function.hpp boost/bind.hpp],,AC_MSG_ERROR([Required boost headers not found. Please install it.]))

# Checks for library functions.
AC_CHECK_FUNCS([atexit memset pow select sqrt])

CXXFLAGS="$CXXFLAGS -DDATA_DIR=\\\"\$(pkgdatadir)\\\" -DBIN_DIR=\\\"\$(bindir)\\\" -DLIB_DIR=\\\"\$(pkglibdir)\\\""

AC_SUBST(SDL_LIBS)
AC_SUBST(SDL_CXXFLAGS)
AC_SUBST(FFMPEG_LIBS)
AC_SUBST(FFMPEG_CXXFLAGS)

VCMI_AI_LIBS_DIR="$libdir/vcmi/AI"
AC_SUBST(VCMI_AI_LIBS_DIR)

VCMI_SCRIPTING_LIBS_DIR="$libdir/vcmi/Scripting"
AC_SUBST(VCMI_SCRIPTING_LIBS_DIR)

AC_OUTPUT(Makefile lib/Makefile client/Makefile server/Makefile AI/Makefile AI/StupidAI/Makefile AI/GeniusAI/Makefile AI/EmptyAI/Makefile Scripting/ERM/Makefile)

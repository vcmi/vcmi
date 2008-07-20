#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(vcmi, VERSION)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AX_BOOST_BASE([1.33])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB

CFLAGS="-g"
CXXFLAGS="-g"
AC_CHECK_LIB([stdc++], [main])
if test "x$GCC" = "xyes"; then
      stdcxx_name=`$CXX -print-file-name=libstdc++.so`
      stdcxx_path=`dirname $stdcxx_name`
LIBS="-L$stdcxx_path $LIBS"
fi
AC_CHECK_PROG(HAVE_SDL_CONFIG, sdl-config, yes, no)

AC_ARG_WITH(lua-includes,AC_HELP_STRING([--with-lua-includes=INCLUDE_PATH],[Use Lua library headers installed at specified
	     path(default=check)]),LUA_INCLUDES="$with_lua_includes")
if test "$LUA_INCLUDES" != ""; then
AC_SUBST([CPPFLAGS],["$CPPFLAGS -I$LUA_INCLUDES"])
fi

AC_ARG_WITH(lua-libs,AC_HELP_STRING([--with-lua-libs=INCLUDE_PATH],[Use Lua library installed at specified
	     path(default=check)]),LUA_LIBS="$with_lua_libs")
if test "$LUA_LIBS" != ""; then
AC_SUBST([LIBS],["$LIBS -L$LUA_LIBS"])
fi

# Checks for libraries.
if test "$HAVE_SDL_CONFIG" = no; then
AC_MSG_ERROR(sdl-config doesn't exist. Try to check the PATH enviroment.)
	else
AC_CHECK_LIB(SDL, SDL_Init,SDL_CFLAGS=`sdl-config --cflags`
	     SDL_LIBS=`sdl-config --libs`)
fi

LIBS="$LIBS $SDL_LIBS"
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"

AC_CHECK_LIB(SDL_mixer,main,,AC_MSG_ERROR([SDL_mixer library not found. Please install it.]))
AC_CHECK_LIB(SDL_image,IMG_Load,,AC_MSG_ERROR([SDL_image library not found. Please install it.]))
AC_CHECK_LIB(SDL_ttf,main,,AC_MSG_ERROR([SDL_ttf library not found. Please install it.]))
AC_CHECK_LIB(lua,main,,AC_MSG_ERROR([lua library not found. Please install it.]))

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h])
AC_CHECK_HEADER(lstate.h,,AC_MSG_ERROR([lstate.h header not found. Please install it.]))
AC_LANG_PUSH(C++)
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_CHECK_HEADERS([boost/filesystem.hpp boost/algorithm/string.hpp boost/algorithm/string/replace.hpp boost/filesystem/operations.hpp boost/assign/std/vector.hpp boost/algorithm/string/find.hpp boost/function.hpp boost/bind.hpp],,AC_MSG_ERROR([Required boost headers not found. Please install it.]))
AC_LANG_POP(C++)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([lua_pushhstring lua_calchash],, AC_MSG_ERROR([Lua required fuctions not found]))
AC_CHECK_FUNCS([atexit memset pow select sqrt])

AC_OUTPUT(Makefile hch/Makefile)

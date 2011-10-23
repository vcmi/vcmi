bin_PROGRAMS = odpalarka

odpalarka_LDADD = $(top_builddir)/lib/libvcmi.la @BOOST_PROGRAM_OPTIONS_LIB@
odpalarka_CXXFLAGS = @SDL_CXXFLAGS@
odpalarka_LDFLAGS = -L$(top_builddir)/lib
odpalarka_SOURCES = \
	main.cpp
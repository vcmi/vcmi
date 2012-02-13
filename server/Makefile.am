bin_PROGRAMS = vcmiserver
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -c $<

vcmiserver_LDADD = $(top_builddir)/lib/libvcmi.la @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ -lpthread
vcmiserver_CXXFLAGS = @SDL_CFLAGS@
vcmiserver_LDFLAGS = -L$(top_builddir)/lib
vcmiserver_SOURCES = \
	CGameHandler.cpp \
	CGameHandler.h \
	CVCMIServer.cpp \
	CVCMIServer.h \
	NetPacksServer.cpp

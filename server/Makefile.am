bin_PROGRAMS = vcmiserver

vcmiserver_LDADD = $(top_builddir)/lib/libvcmi.la @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ -lrt
vcmiserver_CXXFLAGS = @SDL_CFLAGS@
vcmiserver_LDFLAGS = -L$(top_builddir)/lib
vcmiserver_SOURCES = \
	../CConsoleHandler.cpp \
	../CConsoleHandler.h \
	../CThreadHelper.cpp \
	../CThreadHelper.h \
	CGameHandler.cpp \
	CGameHandler.h \
	CVCMIServer.cpp \
	CVCMIServer.h \
	NetPacksServer.cpp \
	stdafx.cpp \
	stdafx.h

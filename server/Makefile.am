bin_PROGRAMS = vcmiserver

vcmiserver_LDADD = $(top_builddir)/lib/libvcmi.la
vcmiserver_CXXFLAGS = @SDL_CXXFLAGS@
vcmiserver_LDFLAGS = -L$(top_builddir)/lib
vcmiserver_SOURCES = \
	../CConsoleHandler.cpp \
	../CConsoleHandler.h \
	CGameHandler.cpp \
	CGameHandler.h \
	CVCMIServer.cpp \
	CVCMIServer.h \
	NetPacksServer.cpp \
	stdafx.cpp \
	stdafx.h

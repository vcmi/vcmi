bin_PROGRAMS = vcmiserver

vcmiserver_LDADD = $(top_builddir)/lib/libvcmi.la
vcmiserver_CXXFLAGS = @SDL_CXXFLAGS@
vcmiserver_LDFLAGS = -L$(top_builddir)/lib
vcmiserver_SOURCES = \
	CGameHandler.cpp \
	CGameHandler.h \
	CVCMIServer.cpp \
	CVCMIServer.h \
	NetPacksServer.cpp

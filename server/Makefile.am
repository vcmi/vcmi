bin_PROGRAMS = vcmiserver

vcmiserver_LDADD = $(top_builddir)/lib/libvcmi.la $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
vcmiserver_LDFLAGS = -L$(top_builddir)/lib
vcmiserver_SOURCES = \
	./CGameHandler.cpp \
	./CGameHandler.h \
	./CVCMIServer.cpp \
	./CVCMIServer.h \
	./NetPacksServer.cpp

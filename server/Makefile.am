AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS = vcmiserver
vcmiserver_LDADD = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
#vcmi_LDADD = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
#AM_CXXFLAGS = $(BOOST_CPPFLAGS)
vcmiserver_LDFLAGS = -L$(top_builddir)/lib
vcmiserver_LDADD += -lvcmi
vcmiserver_SOURCES = \
./CGameHandler.cpp \
./CVCMIServer.cpp \
./CGameHandler.h \
./CVCMIServer.h

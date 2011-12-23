vcmiaidir = $(VCMI_AI_LIBS_DIR)
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

vcmiai_LTLIBRARIES = libStupidAI.la
libStupidAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libStupidAI_la_CXXFLAGS = -DVCMI_DLL
libStupidAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libStupidAI_la_SOURCES = \
	main.cpp\
	StupidAI.cpp\
	StupidAI.h

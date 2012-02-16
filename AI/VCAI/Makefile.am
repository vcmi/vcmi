vcmiaidir = $(VCMI_AI_LIBS_DIR)
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

vcmiai_LTLIBRARIES = libVCAI.la
libVCAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libVCAI_la_CXXFLAGS = -DVCMI_DLL
libVCAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libVCAI_la_SOURCES = \
	main.cpp \
	VCAI.cpp \
	VCAI.h

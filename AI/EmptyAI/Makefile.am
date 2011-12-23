vcmiaidir = $(VCMI_AI_LIBS_DIR)
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

vcmiai_LTLIBRARIES = libEmptyAI.la
libEmptyAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libEmptyAI_la_CXXFLAGS = -DVCMI_DLL
libEmptyAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libEmptyAI_la_SOURCES = \
	CEmptyAI.cpp \
	CEmptyAI.h \
	exp_funcs.cpp

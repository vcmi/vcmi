vcmiaidir = $(VCMI_AI_LIBS_DIR)

vcmiai_LTLIBRARIES = libEmptyAI.la
libEmptyAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libEmptyAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libEmptyAI_la_SOURCES = \
	CEmptyAI.cpp \
	CEmptyAI.h \
	exp_funcs.cpp


vcmiaidir = $(VCMI_AI_LIBS_DIR)

vcmiai_LTLIBRARIES = libStupidAI.la
libStupidAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libStupidAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libStupidAI_la_SOURCES = \
	main.cpp\
	stdafx.cpp\
	stdafx.h\
	StupidAI.cpp\
	StupidAI.h


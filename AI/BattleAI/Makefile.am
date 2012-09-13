vcmiaidir = $(VCMI_AI_LIBS_DIR)
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

vcmiai_LTLIBRARIES = libBattleAI.la
libBattleAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libBattleAI_la_CXXFLAGS = -DVCMI_DLL
libBattleAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libBattleAI_la_SOURCES = \
	main.cpp\
	BattleAI.cpp\
	BattleAI.h

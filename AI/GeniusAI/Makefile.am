vcmiaidir = $(VCMI_AI_LIBS_DIR)
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

vcmiai_LTLIBRARIES = libGeniusAI.la
libGeniusAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
libGeniusAI_la_CXXFLAGS = -DVCMI_DLL
libGeniusAI_la_LDFLAGS = -L$(top_builddir)/lib -module -avoid-version
libGeniusAI_la_SOURCES = \
	AIPriorities.cpp \
	AIPriorities.h \
	BattleHelper.cpp \
	BattleHelper.h \
	BattleLogic.cpp \
	BattleLogic.h \
	CGeniusAI.cpp \
	CGeniusAI.h \
	Common.h \
	DLLMain.cpp \
	GeneralAI.cpp \
	GeneralAI.h \
	neuralNetwork.cpp \
	neuralNetwork.h

vcmiaidir = $(VCMI_AI_LIBS_DIR)

vcmiai_LTLIBRARIES = libGeniusAI.la
libGeniusAI_la_LIBADD = $(top_builddir)/lib/libvcmi.la
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

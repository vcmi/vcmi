bin_PROGRAMS = vcmiclient

vcmiclient_LDADD = $(top_builddir)/lib/libvcmi.la @SDL_LIBS@ @FFMPEG_LIBS@ @BOOST_PROGRAM_OPTIONS_LIB@ -lrt
vcmiclient_CXXFLAGS = @SDL_CXXFLAGS@ @FFMPEG_CXXFLAGS@
vcmiclient_LDFLAGS = -L$(top_builddir)/lib
vcmiclient_SOURCES =  \
	../CCallback.cpp \
	../CCallback.h \
	../CConsoleHandler.cpp \
	../CConsoleHandler.h \
	../CThreadHelper.cpp \
	../CThreadHelper.h \
	AdventureMapButton.cpp \
	AdventureMapButton.h \
	CAdvmapInterface.cpp \
	CAdvmapInterface.h \
	CAnimation.cpp \
	CAnimation.h \
	CBattleInterface.cpp \
	CBattleInterface.h \
	CBitmapHandler.cpp \
	CBitmapHandler.h \
	CCastleInterface.cpp \
	CCastleInterface.h \
	CConfigHandler.cpp \
	CConfigHandler.h \
	CCreatureAnimation.cpp \
	CCreatureAnimation.h \
	CCreatureWindow.cpp \
	CCreatureWindow.h \
	CCursorHandler.cpp \
	CCursorHandler.h \
	CDefHandler.cpp \
	CDefHandler.h \
	CGameInfo.cpp \
	CGameInfo.h \
	CHeroWindow.cpp \
	CHeroWindow.h \
	CKingdomInterface.cpp \
	CKingdomInterface.h \
	Client.cpp \
	Client.h \
	CMessage.cpp \
	CMessage.h \
	CMT.cpp \
	CMusicBase.h \
	CMusicHandler.cpp \
	CMusicHandler.h \
	CPlayerInterface.cpp \
	CPlayerInterface.h \
	CPreGame.cpp \
	CPreGame.h \
	CSndHandler.cpp \
	CSndHandler.h \
	CSoundBase.h \
	CSpellWindow.cpp \
	CSpellWindow.h \
	CVideoHandler.cpp \
	CVideoHandler.h \
	FontBase.h \
	FunctionList.h \
	Graphics.cpp \
	Graphics.h \
	GUIBase.cpp \
	GUIBase.h \
	GUIClasses.cpp \
	GUIClasses.h \
	mapHandler.cpp \
	mapHandler.h \
	NetPacksClient.cpp \
	SDL_Extensions.cpp \
	SDL_Extensions.h \
	SDL_framerate.cpp \
	SDL_framerate.h

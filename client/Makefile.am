bin_PROGRAMS = vcmiclient

desktopdir = $(datadir)/applications
desktop_DATA = vcmiclient.desktop

icondir = $(datadir)/icons
dist_icon_DATA = vcmiclient.png

EXTRA_DIST = \
  $(pixmaps_extra_DATA) \
  ${desktop_DATA}

BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -c $<

vcmiclient_LDADD = $(top_builddir)/lib/libvcmi.la @SDL_LIBS@ @FFMPEG_LIBS@ @BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_IOSTREAMS_LIB@ -lz
vcmiclient_CXXFLAGS = @SDL_CFLAGS@ @FFMPEG_CXXFLAGS@
vcmiclient_LDFLAGS = -L$(top_builddir)/lib
vcmiclient_SOURCES =  \
	../CCallback.cpp \
	../CCallback.h \
	./BattleInterface/CBattleAnimations.cpp \
	./BattleInterface/CBattleAnimations.h \
	./BattleInterface/CBattleInterface.cpp \
	./BattleInterface/CBattleInterface.h \
	./BattleInterface/CBattleInterfaceClasses.cpp \
	./BattleInterface/CBattleInterfaceClasses.h \
	./BattleInterface/CCreatureAnimation.h \
	./BattleInterface/CCreatureAnimation.cpp \
	./UIFramework/CGuiHandler.cpp \
	./UIFramework/CGuiHandler.h \
	./UIFramework/CIntObject.cpp \
	./UIFramework/CIntObject.h \
	./UIFramework/CIntObjectClasses.cpp \
	./UIFramework/CIntObjectClasses.h \
	./UIFramework/Geometries.cpp \
	./UIFramework/Geometries.h \
	./UIFramework/CCursorHandler.cpp \
	./UIFramework/CCursorHandler.h \
	./UIFramework/SDL_Extensions.cpp \
	./UIFramework/SDL_Extensions.h \
	./UIFramework/SDL_Pixels.h \
	AdventureMapClasses.cpp \
	AdventureMapClasses.h \
	CAdvmapInterface.cpp \
	CAdvmapInterface.h \
	CAnimation.cpp \
	CAnimation.h \
	CBitmapHandler.cpp \
	CBitmapHandler.h \
	CCastleInterface.cpp \
	CCastleInterface.h \
	CConfigHandler.cpp \
	CConfigHandler.h \
	CCreatureWindow.cpp \
	CCreatureWindow.h \
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
	CQuestLog.cpp \
	CQuestLog.h \
	FontBase.h \
	FunctionList.h \
	Graphics.cpp \
	Graphics.h \
	GUIClasses.cpp \
	GUIClasses.h \
	mapHandler.cpp \
	mapHandler.h \
	NetPacksClient.cpp

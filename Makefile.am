ACLOCAL_AMFLAGS = -I aclocal/m4
EXTRA_DIST = aclocal/m4
bin_PROGRAMS = vcmi
vcmi_LDADD = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB)
AM_CXXFLAGS = $(BOOST_CPPFLAGS)
vcmi_SOURCES = \
	       AdventureMapButton.cpp  CGameState.cpp        CScreenHandler.h \
	       AdventureMapButton.h    CGameState.h          global.h \
	       AI_Base.h               CHeroWindow.cpp       int3.h \
	       CAdvmapInterface.cpp    CHeroWindow.h         map.cpp \
	       CAdvmapInterface.h      CLua.cpp              map.h \
	       CBattleInterface.cpp    CLua.h                mapHandler.cpp \
	       CBattleInterface.h      CLuaHandler.cpp       mapHandler.h \
	       CCallback.cpp           CLuaHandler.h         nodrze.h \
	       CCallback.h             CMessage.cpp          SDL_Extensions.cpp \
	       CCastleInterface.cpp    CMessage.h            SDL_Extensions.h \
	       CCastleInterface.h      CMT.cpp               SDL_framerate.cpp \
	       CConsoleHandler.cpp     SDL_framerate.h \
	       CConsoleHandler.h       CPathfinder.cpp       SDL_rotozoom.cpp \
	       CCursorHandler.cpp      CPathfinder.h         SDL_rotozoom.h \
	       CCursorHandler.h        CPlayerInterface.cpp  StartInfo.h \
	       CGameInfo.cpp           CPlayerInterface.h    stdafx.cpp \
	       CGameInfo.h             CPreGame.cpp          stdafx.h \
	       CGameInterface.cpp      CPreGame.h            tchar_amigaos4.h \
	       CGameInterface.h        CScreenHandler.cpp    timeHandler.h \
	       hch/CAbilityHandler.cpp   hch/CDefObjInfoHandler.cpp   hch/CSemiDefHandler.cpp \
	       hch/CAbilityHandler.h     hch/CDefObjInfoHandler.h     hch/CSemiDefHandler.h \
	       hch/CAmbarCendamo.cpp     hch/CGeneralTextHandler.cpp  hch/CSemiLodHandler.cpp \
	       hch/CAmbarCendamo.h       hch/CGeneralTextHandler.h    hch/CSemiLodHandler.h \
	       hch/CArtHandler.cpp       hch/CHeroHandler.cpp         hch/CSndHandler.cpp \
	       hch/CArtHandler.h         hch/CHeroHandler.h           hch/CSndHandler.h \
	       hch/CBuildingHandler.cpp  hch/CLodHandler.cpp          hch/CSpellHandler.cpp \
	       hch/CBuildingHandler.h    hch/CLodHandler.h            hch/CSpellHandler.h \
	       hch/CCastleHandler.cpp    hch/CMusicHandler.cpp        hch/CTownHandler.cpp \
	       hch/CCastleHandler.h      hch/CMusicHandler.h          hch/CTownHandler.h \
	       hch/CCreatureHandler.cpp  hch/CObjectHandler.cpp       hch/CVideoHandler.cpp \
	       hch/CCreatureHandler.h    hch/CObjectHandler.h         hch/CVideoHandler.h \
	       hch/CDefHandler.cpp       hch/CPreGameTextHandler.cpp \
	       hch/CDefHandler.h         hch/CPreGameTextHandler.h 


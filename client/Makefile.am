AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS = vcmiclient
#vcmi_LDADD = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
#AM_CXXFLAGS = $(BOOST_CPPFLAGS)
vcmiclient_LDADD = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
#vcmiclient_LDADD = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
vcmiclient_LDFLAGS = -L$(top_builddir)/lib
vcmiclient_LDADD += -lvcmi
vcmiclient_SOURCES =  \
../AdventureMapButton.cpp \
../CAdvmapInterface.cpp \
../CBattleInterface.cpp \
./CBitmapHandler.cpp \
../CCallback.cpp \
../CCastleInterface.cpp \
./CConfigHandler.cpp \
./CCreatureAnimation.cpp \
../CCursorHandler.cpp \
../hch/CDefHandler.cpp \
../CGameInfo.cpp \
../CGameInterface.cpp \
../CHeroWindow.cpp \
./Client.cpp \
../CMessage.cpp \
../CMT.cpp \
../hch/CMusicHandler.cpp \
../CPathfinder.cpp \
../CPlayerInterface.cpp \
../CPreGame.cpp \
../hch/CSndHandler.cpp \
./CSpellWindow.cpp \
../CThreadHelper.cpp \
./Graphics.cpp \
../mapHandler.cpp \
./NetPacksClient.cpp \
../SDL_Extensions.cpp \
../SDL_framerate.cpp \
../AdventureMapButton.h \
../AI_Base.h \
../CAdvmapInterface.h \
../hch/CAmbarCendamo.h \
../CBattleInterface.h \
./CBitmapHandler.h \
../hch/CBuildingHandler.h \
../CCallback.h \
../CCastleInterface.h \
./CConfigHandler.h \
./CCreatureAnimation.h \
../CCursorHandler.h \
../hch/CDefHandler.h \
../CGameInfo.h \
../CGameInterface.h \
../hch/CHeroHandler.h \
../CHeroWindow.h \
./Client.h \
../CMessage.h \
../hch/CMusicHandler.h \
../hch/CObjectHandler.h \
../CPathfinder.h \
../CPlayerInterface.h \
../CPreGame.h \
../hch/CSndHandler.h \
./CSpellWindow.h \
../CThreadHelper.h \
./FunctionList.h \
../global.h \
./Graphics.h \
../int3.h \
../map.h \
../mapHandler.h \
../nodrze.h \
../SDL_framerate.h \
../StartInfo.h \
../stdafx.h \
../timeHandler.h

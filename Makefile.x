CC		= g++
CFLAGS		= -I/Library/Frameworks/SDL_mixer.framework/Headers -I/Library/Frameworks/SDL.framework/Headers -I/Library/Frameworks/SDL_image.framework/Headers -I/Library/Frameworks/SDL_ttf.framework/Headers -I/opt/local/include
OPTIM		= -O2
#DEBUG		= -ggdb -D_DEBUG
LDFLAGS		= -Wl,-framework,SDL -Wl,-framework,SDL_mixer -Wl,-framework,SDL_image -Wl,-framework,SDL_ttf -Wl,-framework,Cocoa
BOOSTLIBS	= -L/opt/local/lib -lboost_system-mt -lboost_filesystem-mt -lboost_thread-mt
LIBS		= $(BOOSTLIBS) -llua -ljpeg -lpng -lm -lpthread -ldl -lauto -lz
VCMI_CLIENT	= vcmiclient
VCMI_LIB	= VCMI_Lib.dylib
VCMI_SERVER	= vcmiserver
GENIUS_AI	= GeniusAI.dll

CLIENT_SOURCES = AdventureMapButton.cpp	\
	CAdvmapInterface.cpp		\
	CBattleInterface.cpp		\
	CCallback.cpp			\
	CCastleInterface.cpp		\
	CCursorHandler.cpp		\
	CGameInfo.cpp			\
	CGameInterface.cpp		\
	CHeroWindow.cpp			\
	CMT.cpp				\
	CMessage.cpp			\
	CPlayerInterface.cpp		\
	CPreGame.cpp			\
	CThreadHelper.cpp		\
	SDL_Extensions.cpp		\
	SDL_framerate.cpp		\
	client/CBitmapHandler.cpp	\
	client/CConfigHandler.cpp	\
	client/CCreatureAnimation.cpp	\
	client/CSpellWindow.cpp		\
	client/Client.cpp		\
	client/Graphics.cpp		\
	hch/CDefHandler.cpp		\
	hch/CMusicHandler.cpp		\
	hch/CSndHandler.cpp		\
	mapHandler.cpp			\
	client/NetPacksClient.cpp	\
	SDLMain.m

LIB_SOURCES = CConsoleHandler.cpp	\
	CGameState.cpp			\
	hch/CArtHandler.cpp		\
	hch/CBuildingHandler.cpp	\
	hch/CCreatureHandler.cpp	\
	hch/CDefObjInfoHandler.cpp	\
	hch/CGeneralTextHandler.cpp	\
	hch/CHeroHandler.cpp		\
	hch/CLodHandler.cpp		\
	hch/CObjectHandler.cpp		\
	hch/CSpellHandler.cpp		\
	hch/CTownHandler.cpp		\
	lib/Connection.cpp		\
	lib/IGameCallback.cpp		\
	lib/VCMI_Lib.cpp		\
	lib/NetPacksLib.cpp		\
	lib/RegisterTypes.cpp		\
	map.cpp

SERVER_SOURCES = \
	server/CGameHandler.cpp		\
	server/NetPacksServer.cpp	\
	server/CVCMIServer.cpp

GENIUS_SOURCES = \
	AI/GeniusAI/CGeniusAI.cpp	\
	AI/GeniusAI/DLLMain.cpp

OBJECTS=$(CLIENT_SOURCES:.cpp=.o)
CLIENT_OBJECTS=$(OBJECTS:.m=.o)
LIB_OBJECTS=$(LIB_SOURCES:.cpp=.o)
SERVER_OBJECTS=$(SERVER_SOURCES:.cpp=.o)
GENIUS_OBJECTS=$(GENIUS_SOURCES:.cpp=.o)

all: $(CLIENT_SOURCES) $(LIB_SOURCES) $(SERVER_SOURCES) $(VCMI_CLIENT) $(VCMI_SERVER) $(GENIUS_AI)

update:
	svn co https://vcmi.svn.sourceforge.net/svnroot/vcmi/trunk .

$(VCMI_CLIENT): $(CLIENT_OBJECTS) $(VCMI_LIB)
	$(CC) $(LDFLAGS) $(CLIENT_OBJECTS) $(VCMI_LIB) -o $@ $(BOOSTLIBS) -lz

$(VCMI_SERVER): $(SERVER_OBJECTS) $(VCMI_LIB)
	$(CC) $(SERVER_OBJECTS) $(VCMI_LIB) -o $@ $(BOOSTLIBS)

$(GENIUS_AI): $(GENIUS_OBJECTS) $(VCMI_LIB)
	$(CC) -dynamiclib -install_name $@ $(GENIUS_OBJECTS) $(VCMI_LIB) -o $@

$(VCMI_LIB): $(LIB_OBJECTS)
	$(CC) -dynamiclib -install_name $@ $(LIB_OBJECTS) -o $@ $(BOOSTLIBS) -lz

.cpp.o:
	$(CC) -c $(OPTIM) $(DEBUG) $(CFLAGS) $< -o $@

.m.o:
	$(CC) -c $(OPTIM) $(DEBUG) $(CFLAGS) $< -o $@
	
clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(LIB_OBJECTS) $(GENIUS_OBJECTS) $(VCMI_CLIENT) $(VCMI_SERVER) $(GENIUS_AI) $(VCMI_LIB)

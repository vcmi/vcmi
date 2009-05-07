CC		= ppc-amigaos-g++
CFLAGS  = -I. `sdl-config --cflags` -I/usr/local/include/boost-1_35 -Ilua -fpermissive
#OPTIM	= -O3
DEBUG	= -g -D_DEBUG
LDFLAGS = -use-dynld
BOOSTLIBS = -lboost_system-gcc42-mt-1_35 -lboost_filesystem-gcc42-mt-1_35
LIBS    = $(BOOSTLIBS) -llua -lSDL -lSDL_image -ltiff -ljpeg -lpng -lSDL_ttf -lft2 -lSDL_mixer -lvorbisfile -lvorbis -logg -lSMPEG -lSDL -lm -lz -lpthread -ldl -lunix -lauto
EXE		= vcmi

SOURCES = AdventureMapButton.cpp \
	CAdvmapInterface.cpp   		\
	CBattleInterface.cpp   		\
	CCallback.cpp          		\
	CCastleInterface.cpp   		\
	CConsoleHandler.cpp    		\
	CCursorHandler.cpp     		\
	CGameInfo.cpp          		\
	CGameInterface.cpp     		\
	CGameState.cpp         		\
	CHeroWindow.cpp        		\
	CLua.cpp               		\
	CLuaHandler.cpp        		\
	CMT.cpp                		\
	CMessage.cpp           		\
	CPathfinder.cpp        		\
	CPlayerInterface.cpp   		\
	CPreGame.cpp           		\
	CScreenHandler.cpp     		\
	SDL_Extensions.cpp     		\
	SDL_framerate.cpp      		\
	SDL_rotozoom.cpp       		\
	map.cpp                		\
	mapHandler.cpp         		\
	stdafx.cpp             		\
	hch/CAbilityHandler.cpp		\
	hch/CAmbarCendamo.cpp  		\
	hch/CArtHandler.cpp			\
	hch/CBuildingHandler.cpp	\
	hch/CCastleHandler.cpp		\
	hch/CCreatureHandler.cpp	\
	hch/CDefHandler.cpp			\
	hch/CDefObjInfoHandler.cpp	\
	hch/CGeneralTextHandler.cpp	\
	hch/CHeroHandler.cpp		\
	hch/CLodHandler.cpp			\
	hch/CMusicHandler.cpp		\
	hch/CObjectHandler.cpp		\
	hch/CPreGameTextHandler.cpp	\
	hch/CSemiDefHandler.cpp		\
	hch/CSemiLodHandler.cpp		\
	hch/CSndHandler.cpp			\
	hch/CSpellHandler.cpp		\
	hch/CTownHandler.cpp		

OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) -c $(OPTIM) $(DEBUG) $(CFLAGS) $< -o $@
	
clean:
	rm -f *.o hch/*.o $(EXE)
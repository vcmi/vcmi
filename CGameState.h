#ifndef CGAMESTATE_H
#define CGAMESTATE_H

#include "mapHandler.h"

class CHeroInstance;
class CTownInstance;
class CCallback;

struct PlayerState
{
public:
	int color;
	//std::vector<std::vector<std::vector<char> > > fogOfWarMap; //true - visible, false - hidden
	PseudoV< PseudoV< PseudoV<unsigned char> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<int> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	PlayerState():color(-1){};
};

class CGameState
{
	int currentPlayer;

	int day; //total number of days in game
	std::map<int,PlayerState> players; //color <-> playerstate
public:
	friend CCallback;
	friend int _tmain(int argc, _TCHAR* argv[]);
	friend void initGameState(CGameInfo * cgi);
	//CCallback * cb; //for communication between PlayerInterface/AI and GameState

	friend SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap); //todo: wywalic koniecznie, tylko do flag obecnie!!!!
};

#endif //CGAMESTATE_H

#ifndef CGAMESTATE_H
#define CGAMESTATE_H
#include "mapHandler.h"
#include <set>
class CScriptCallback;
class CHeroInstance;
class CTownInstance;
class CCallback;
class CLuaCallback;
class CCPPObjectScript;
struct PlayerState
{
public:
	int color, serial;
	//std::vector<std::vector<std::vector<char> > > fogOfWarMap; //true - visible, false - hidden
	PseudoV< PseudoV< PseudoV<unsigned char> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<int> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	PlayerState():color(-1){};
};

class CGameState
{
private:
	int currentPlayer;
	int day; //total number of days in game
	std::map<int,PlayerState> players; //color <-> playerstate
	std::set<CCPPObjectScript *> cppscripts;
	std::map<int, std::map<std::string, CObjectScript*> > objscr; //custom user scripts (as for now only Lua)
	

	bool checkFunc(int obid, std::string name)
	{
		if (objscr.find(obid)!=objscr.end())
		{
			if(objscr[obid].find(name)!=objscr[obid].end())
			{
				return true;
			}
		}
		return false;
	}
	CGHeroInstance * getHero(int ID, int mode)
	{
		if (mode != 0)
			throw new std::exception("gs->getHero: This mode is not supported!");
		for ( std::map<int, PlayerState>::iterator i=players.begin() ; i!=players.end();i++)
		{
			for (int j=0;j<(*i).second.heroes.size();j++)
			{
				if (i->second.heroes[j]->subID == ID)
					return i->second.heroes[j];
			}
		}
		return NULL;
	}
public:
	friend CCallback;
	friend CLuaCallback;
	friend int _tmain(int argc, _TCHAR* argv[]);
	friend void initGameState(CGameInfo * cgi);
	friend CScriptCallback;
	friend void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script);
	//CCallback * cb; //for communication between PlayerInterface/AI and GameState

	friend SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap); //todo: wywalic koniecznie, tylko do flag obecnie!!!!
};

#endif //CGAMESTATE_H

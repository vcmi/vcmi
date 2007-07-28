#ifndef CTOWNHANDLER_H
#define CTOWNHANDLER_H
#include "CDefHandler.h"
#include "CCreatureHandler.h"
#include "SDL.h"
#include "int3.h"
#include <string>
#include <vector>
class CTown
{
public:
	std::string name;
	int bonus; //pic number
};
class CTownHandler
{
	CDefHandler * smallIcons;
public:
	CTownHandler();
	~CTownHandler();
	std::vector<CTown> towns;
	void loadNames();
	SDL_Surface * getPic(int ID, bool fort=true, bool builded=false); //ID=-1 - blank; -2 - border; -3 - random
};
class CTownInstance
{
	int type; //type of town
	int owner; //ID of owner
	int3 pos; //position
	CTown * town;
	std::string name; // name of town
	CCreatureSet garrison;
	int builded; //how many buildings has been built this turn
	int destroyed; //how many buildings has been destroyed this turn
	
	//TODO:
	//buildings
	//creatures to be recruited
	//hero in garrison
	//spells in mage guild
};
#endif //CTOWNHANDLER_H
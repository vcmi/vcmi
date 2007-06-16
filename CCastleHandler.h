#ifndef CCASTLEHANDLER_H
#define CCASTLEHANDLER_H

#include "CBuildingHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"

class CCastleEvent
{
public:
};

class CCastleObjInfo : public CSpecObjInfo //castle class
{
public:
	int x, y, z; //posiotion
	std::vector<CBuilding> buildings; //buildings we can build in this castle
	std::vector<bool> isBuild; //isBuild[i] is true, when building buildings[i] has been built
	std::vector<bool> isLocked; //isLocked[i] is true, when building buildings[i] canot be built
	CHero * visitingHero;
	CHero * garnisonHero;

	unsigned char bytes[4]; //identifying bytes
	unsigned char player; //255 - nobody, players 0 - 7
	std::string name; //town name
	CCreatureSet garrison;

	std::vector<CSpell *> possibleSpells;
	std::vector<CSpell *> obligatorySpells;
	std::vector<CSpell *> availableSpells;
	
	std::vector<CCastleEvent> events;
};

#endif //CCASTLEHANDLER_H
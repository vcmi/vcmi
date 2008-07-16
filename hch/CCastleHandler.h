#ifndef CCASTLEHANDLER_H
#define CCASTLEHANDLER_H

#include "CBuildingHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"

class CCastleEvent
{
public:
	std::string name, message;
	int wood, mercury, ore, sulfur, crystal, gems, gold; //gain / loss of resources
	unsigned char players; //players for whom this event can be applied
	bool forHuman, forComputer;
	int firstShow; //postpone of first encounter time in days
	int forEvery; //every n days this event will occure

	unsigned char bytes[6]; //build specific buildings (raw format, similar to town's)

	int gen[7]; //additional creatures in i-th level dwelling
};

class CCastleObjInfo : public CSpecObjInfo //castle class
{
public:
	int x, y, z; //posiotion
	std::vector<CBuilding> buildings; //buildings we can build in this castle
	std::vector<bool> isBuild; //isBuild[i] is true, when building buildings[i] has been built
	std::vector<bool> isLocked; //isLocked[i] is true, when building buildings[i] canot be built
	bool unusualBuildins; //if true, intrepret bytes below
	unsigned char buildingSettings[12]; //raw format for two vectors above (greatly depends on town type)
	bool hasFort; //used only if unusualBuildings is false
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

	unsigned char alignment; //255 - same as owner/random, 0 - same as red, 1 - same as blue, etc
};

#endif //CCASTLEHANDLER_H

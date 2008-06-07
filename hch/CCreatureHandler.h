#ifndef CCREATUREHANDLER_H
#define CCREATUREHANDLER_H

#include <string>
#include <vector>
#include <map>
#include <set>

class CDefHandler;
struct SDL_Surface;

class CCreature
{
public:
	std::string namePl, nameSing, nameRef; //name in singular and plural form; and reference name
	std::vector<int> cost; //cost[res_id] - amount of that resource
	std::set<int> upgrades; // IDs of creatures to which this creature can be upgraded
	int fightValue, AIValue, growth, hordeGrowth, hitPoints, speed, attack, defence, shots, spells;
	int damageMin, damageMax;
	int ammMin, ammMax;
	int level; // 0 - unknown
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in textformat
	std::string animDefName;
	int idNumber;
	int faction; //-1 = neutral

	///animation info
	float timeBetweenFidgets, walkAnimationTime, attackAnimationTime, flightAnimationDistance;
	int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX, upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;
	float missleFrameAngles[12];
	int troopCountLocationOffset, attackClimaxFrame;
	///end of anim info

	//for some types of towns
	bool isDefinite; //if the creature type is wotn dependent, it should be true
	int indefLevel; //only if indefinite
	bool indefUpgraded; //onlu if inddefinite
	//end
	//CDefHandler * battleAnimation;
	//TODO - zdolnoœci - na typie wyliczeniowym czy czymœ

	static int getQuantityID(int quantity); //0 - a few, 1 - several, 2 - pack, 3 - lots, 4 - horde, 5 - throng, 6 - swarm, 7 - zounds, 8 - legion
	bool isDoubleWide(); //returns true if unit is double wide on battlefield
	bool isFlying(); //returns true if it is a flying unit
	int maxAmount(const std::vector<int> &res) const; //how many creatures can be bought
};

class CCreatureSet //seven combined creatures
{
public:
	std::map<int,std::pair<CCreature*,int> > slots;
	bool formation; //false - wide, true - tight
};

class CCreatureHandler
{
public:
	std::map<int,SDL_Surface*> smallImgs; //creature ID -> small 32x32 img of creature; //ID=-2 is for blank (black) img; -1 for the border
	std::map<int,SDL_Surface*> bigImgs; //creature ID -> big 58x64 img of creature; //ID=-2 is for blank (black) img; -1 for the border
	std::map<int,SDL_Surface*> backgrounds; //castle ID -> 100x130 background creature image // -1 is for neutral
	std::vector<CCreature> creatures; //creature ID -> creature info
	std::map<int,std::vector<CCreature*> > levelCreatures; //level -> list of creatures
	std::map<std::string,int> nameToID;
	void loadCreatures();
	void loadAnimationInfo();
	void loadUnitAnimInfo(CCreature & unit, std::string & src, int & i);
};
#endif //CCREATUREHANDLER_H
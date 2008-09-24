#ifndef CCREATUREHANDLER_H
#define CCREATUREHANDLER_H
#include "../global.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class CLodHandler;

class DLL_EXPORT CCreature
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
	ui32 idNumber;
	std::set<EAbilities> abilities;
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

	//TODO - zdolnoœci (abilities) - na typie wyliczeniowym czy czymœ - albo lepiej secie czegoœ

	bool isDoubleWide() const; //returns true if unit is double wide on battlefield
	bool isFlying() const; //returns true if it is a flying unit
	bool isShooting() const; //returns true if unit can shoot
	si32 maxAmount(const std::vector<si32> &res) const; //how many creatures can be bought
	static int getQuantityID(const int & quantity); //0 - a few, 1 - several, 2 - pack, 3 - lots, 4 - horde, 5 - throng, 6 - swarm, 7 - zounds, 8 - legion
};


class DLL_EXPORT CCreatureHandler
{
public:
	std::set<int> notUsedMonsters;
	std::vector<CCreature> creatures; //creature ID -> creature info
	std::map<int,std::vector<CCreature*> > levelCreatures; //level -> list of creatures
	std::map<std::string,int> nameToID;
	std::map<int,std::string> idToProjectile;
	std::map<int,bool> idToProjectileSpin; //if true, appropriate projectile is spinning during flight
	void loadCreatures();
	void loadAnimationInfo();
	void loadUnitAnimInfo(CCreature & unit, std::string & src, int & i);
	CCreatureHandler();
};
#endif //CCREATUREHANDLER_H

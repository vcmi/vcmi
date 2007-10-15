#ifndef CCREATUREHANDLER_H
#define CCREATUREHANDLER_H

#include <string>
#include <vector>
#include <map>
#include "CDefHandler.h"

class CCreature
{
public:
	std::string namePl, nameSing, nameRef; //name in singular and plural form; and reference name
	int wood, mercury, ore, sulfur, crystal, gems, gold, fightValue, AIValue, growth, hordeGrowth, hitPoints, speed, attack, defence, shots, spells;
	int low1, low2, high1, high2; //TODO - co to w ogóle jest???
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in textformat
	int idNumber;

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
	CDefHandler * battleAnimation;
	//TODO - zdolnoœci - na typie wyliczeniowym czy czymœ
};

class CCreatureSet //seven combined creatures
{
public:
	std::map<int,std::pair<CCreature*,int> > slots;
	//CCreature * slot1, * slot2, * slot3, * slot4, * slot5, * slot6, * slot7; //types of creatures on each slot
	//unsigned int s1, s2, s3, s4, s5, s6, s7; //amounts of units in slots
	bool formation; //false - wide, true - tight
};

class CCreatureHandler
{
public:
	std::vector<CCreature> creatures;
	std::map<std::string,int> nameToID;
	void loadCreatures();
	void loadAnimationInfo();
	void loadUnitAnimInfo(CCreature & unit, std::string & src, int & i);
	void loadUnitAnimations();
};

#endif //CCREATUREHANDLER_H
#ifndef __CCREATUREHANDLER_H__
#define __CCREATUREHANDLER_H__
#include "../global.h"
#include <string>
#include <vector>
#include <map>
#include <set>

#include "CSoundBase.h"
#include "lib/StackFeature.h"

/*
 * CCreatureHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLodHandler;

class DLL_EXPORT CCreature
{
public:
	std::string namePl, nameSing, nameRef; //name in singular and plural form; and reference name
	std::vector<ui32> cost; //cost[res_id] - amount of that resource
	std::set<ui32> upgrades; // IDs of creatures to which this creature can be upgraded
	ui32 fightValue, AIValue, growth, hordeGrowth, hitPoints, speed, attack, defence, shots, spells;
	ui32 damageMin, damageMax;
	ui32 ammMin, ammMax;
	ui8 level; // 0 - unknown
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in textformat
	std::string animDefName;
	ui32 idNumber;
	std::vector<StackFeature> abilities;
	si8 faction; //-1 = neutral

	///animation info
	float timeBetweenFidgets, walkAnimationTime, attackAnimationTime, flightAnimationDistance;
	int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX, upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;
	float missleFrameAngles[12];
	int troopCountLocationOffset, attackClimaxFrame;
	///end of anim info

	// Sound infos
	struct {
		soundBase::soundID attack;
		soundBase::soundID defend;
		soundBase::soundID killed; // was killed died
		soundBase::soundID move;
		soundBase::soundID shoot; // range attack
		soundBase::soundID wince; // attacked but did not die
		soundBase::soundID ext1;  // creature specific extension
		soundBase::soundID ext2;  // creature specific extension
		soundBase::soundID startMoving; // usually same as ext1
		soundBase::soundID endMoving;	// usually same as ext2
	} sounds;

	bool isDoubleWide() const; //returns true if unit is double wide on battlefield
	bool isFlying() const; //returns true if it is a flying unit
	bool isShooting() const; //returns true if unit can shoot
	bool isUndead() const; //returns true if unit is undead
	si32 maxAmount(const std::vector<si32> &res) const; //how many creatures can be bought
	static int getQuantityID(const int & quantity); //0 - a few, 1 - several, 2 - pack, 3 - lots, 4 - horde, 5 - throng, 6 - swarm, 7 - zounds, 8 - legion

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & namePl & nameSing & nameRef
			& cost & upgrades 
			& fightValue & AIValue & growth & hordeGrowth & hitPoints & speed & attack & defence & shots & spells
			& damageMin & damageMax & ammMin & ammMax & level
			& abilityText & abilityRefs & animDefName
			& idNumber & abilities & faction

			& timeBetweenFidgets & walkAnimationTime & attackAnimationTime & flightAnimationDistance
			& upperRightMissleOffsetX & rightMissleOffsetX & lowerRightMissleOffsetX & upperRightMissleOffsetY & rightMissleOffsetY & lowerRightMissleOffsetY
			& missleFrameAngles & troopCountLocationOffset & attackClimaxFrame;
	}
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
	~CCreatureHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//TODO: should be optimized, not all these informations needs to be serialized (same for ccreature)
		h & notUsedMonsters & creatures & nameToID & idToProjectile & idToProjectileSpin;

		if(!h.saving)
		{
			for (int i=0; i<creatures.size(); i++) //recreate levelCreatures map
			{
				levelCreatures[creatures[i].level].push_back(&creatures[i]);
			}
		}
	}
};

#endif // __CCREATUREHANDLER_H__

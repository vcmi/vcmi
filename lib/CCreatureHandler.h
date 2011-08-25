#ifndef __CCREATUREHANDLER_H__
#define __CCREATUREHANDLER_H__
#include "../global.h"
#include <string>
#include <vector>
#include <map>
#include <set>

#include "../lib/HeroBonus.h"
#include "../lib/ConstTransitivePtr.h"
#include "ResourceSet.h"

/*
 * CCreatureHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#define ALLCREATURESGETDOUBLEMONTHS false

class CLodHandler;
class CCreatureHandler;
class CCreature;

class DLL_EXPORT CCreature : public CBonusSystemNode
{
public:
	std::string namePl, nameSing, nameRef; //name in singular and plural form; and reference name
	TResources cost; //cost[res_id] - amount of that resource
	std::set<ui32> upgrades; // IDs of creatures to which this creature can be upgraded
	ui32 hitPoints, speed, attack, defence;
	ui32 fightValue, AIValue, growth, hordeGrowth, shots, spells;
	ui32 damageMin, damageMax;
	ui32 ammMin, ammMax;
	ui8 level; // 0 - unknown
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in text format
	std::string animDefName;
	si32 idNumber;
	si8 faction; //-1 = neutral
	ui8 doubleWide;

	///animation info
	float timeBetweenFidgets, walkAnimationTime, attackAnimationTime, flightAnimationDistance;
	int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX, upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;
	float missleFrameAngles[12];
	int troopCountLocationOffset, attackClimaxFrame;
	///end of anim info

	bool isDoubleWide() const; //returns true if unit is double wide on battlefield
	bool isFlying() const; //returns true if it is a flying unit
	bool isShooting() const; //returns true if unit can shoot
	bool isUndead() const; //returns true if unit is undead
	bool isGood () const;
	bool isEvil () const;
	si32 maxAmount(const std::vector<si32> &res) const; //how many creatures can be bought
	static int getQuantityID(const int & quantity); //0 - a few, 1 - several, 2 - pack, 3 - lots, 4 - horde, 5 - throng, 6 - swarm, 7 - zounds, 8 - legion
	static int estimateCreatureCount(int countID); //reverse version of above function, returns middle of range
	bool isMyUpgrade(const CCreature *anotherCre) const;

	bool valid() const;

	void addBonus(int val, int type, int subtype = -1);
	std::string nodeName() const OVERRIDE;
	//void getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const;

	template<typename RanGen>
	int getRandomAmount(RanGen &ranGen)
	{
		if(ammMax == ammMin)
			return ammMax;
		else
			return ammMin + (ranGen() % (ammMax - ammMin));
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & namePl & nameSing & nameRef
			& cost & upgrades 
			& fightValue & AIValue & growth & hordeGrowth & hitPoints & speed & attack & defence & shots & spells
			& damageMin & damageMax & ammMin & ammMax & level
			& abilityText & abilityRefs & animDefName
			& idNumber & faction

			& timeBetweenFidgets & walkAnimationTime & attackAnimationTime & flightAnimationDistance
			& upperRightMissleOffsetX & rightMissleOffsetX & lowerRightMissleOffsetX & upperRightMissleOffsetY & rightMissleOffsetY & lowerRightMissleOffsetY
			& missleFrameAngles & troopCountLocationOffset & attackClimaxFrame;

		h & doubleWide;
	}


	CCreature();
	friend class CCreatureHandler;
};

class DLL_EXPORT CCreatureHandler
{
private: //?
	CBonusSystemNode allCreatures, creaturesOfLevel[CREATURES_PER_TOWN + 1];//index 0 is used for creatures of unknown tier or outside <1-7> range
public:
	std::set<int> notUsedMonsters;
	std::set<TCreature> doubledCreatures; //they get double week
	std::vector<ConstTransitivePtr<CCreature> > creatures; //creature ID -> creature info
	bmap<std::string,int> nameToID;
	bmap<int,std::string> idToProjectile;
	bmap<int,bool> idToProjectileSpin; //if true, appropriate projectile is spinning during flight
	std::vector<si8> factionAlignments; //1 for good, 0 for neutral and -1 for evil with faction ID as index
	int factionToTurretCreature[F_NUMBER]; //which creature's animation should be used to dispaly creature in turret while siege

	std::map<TBonusType, std::pair<std::string, std::string> > stackBonuses; // bonus => name, description
	std::vector<std::vector<ui32> > expRanks; // stack experience needed for certain rank, index 0 for other tiers (?)
	std::vector<ui32> maxExpPerBattle; //%, tiers same as above
	si8 expAfterUpgrade;//multiplier in %


	void deserializationFix();
	void loadCreatures();
	void buildBonusTreeForTiers();
	void loadAnimationInfo();
	void loadUnitAnimInfo(CCreature & unit, std::string & src, int & i);
	void loadStackExp(Bonus & b, BonusList & bl, std::string & src, int & it);
	void loadMindImmunity(Bonus & b, BonusList & bl, std::string & src, int & it); //multiple bonuses at once
	int stringToNumber(std::string & s);//help function for parsing CREXPBON.txt

	bool isGood (si8 faction) const;
	bool isEvil (si8 faction) const;

	int pickRandomMonster(const boost::function<int()> &randGen = 0, int tier = -1) const; //tier <1 - CREATURES_PER_TOWN> or -1 for any
	void addBonusForTier(int tier, Bonus *b); //tier must be <1-7>
	void addBonusForAllCreatures(Bonus *b);

	CCreatureHandler();
	~CCreatureHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//TODO: should be optimized, not all these informations needs to be serialized (same for ccreature)
		h & notUsedMonsters & creatures & nameToID & idToProjectile & idToProjectileSpin & factionToTurretCreature;
		h & stackBonuses & expRanks & maxExpPerBattle & expAfterUpgrade;
		h & allCreatures;
		h & creaturesOfLevel;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

#endif // __CCREATUREHANDLER_H__

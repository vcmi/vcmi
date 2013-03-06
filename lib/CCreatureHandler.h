#pragma once


#include "HeroBonus.h"
#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "GameConstants.h"
#include "JsonNode.h"

/*
 * CCreatureHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLegacyConfigParser;
class CCreatureHandler;
class CCreature;

class DLL_LINKAGE CCreature : public CBonusSystemNode
{
public:
	std::string nameRef; // reference name, stringID
	std::string nameSing;// singular name, e.g. Centaur
	std::string namePl;  // plural name, e.g. Centaurs

	std::string abilityText; //description of abilities

	CreatureID idNumber;
	TFaction faction;
	ui8 level; // 0 - unknown

	//stats that are not handled by bonus system
	ui32 fightValue, AIValue, growth, hordeGrowth;
	ui32 ammMin, ammMax; // initial size of stack of these creatures on adventure map (if not set in editor)

	bool doubleWide;
	bool special; // Creature is not available normally (war machines, commanders, several unused creatures, etc

	TResources cost; //cost[res_id] - amount of that resource required to buy creature from dwelling
	std::set<CreatureID> upgrades; // IDs of creatures to which this creature can be upgraded

	std::string animDefName; // creature animation used during battles
	std::string advMapDef; //for new creatures only, image for adventure map
	si32 iconIndex; // index of icon in files like twcrport

	struct CreatureAnimation
	{
		double timeBetweenFidgets, walkAnimationTime, attackAnimationTime, flightAnimationDistance;
		int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX,
		    upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;

		double missleFrameAngles[12];
		int troopCountLocationOffset, attackClimaxFrame;

		std::string projectileImageName;
		bool projectileSpin; //if true, appropriate projectile is spinning during flight

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & timeBetweenFidgets & walkAnimationTime & attackAnimationTime & flightAnimationDistance;
			h & upperRightMissleOffsetX & rightMissleOffsetX & lowerRightMissleOffsetX;
			h & upperRightMissleOffsetY & rightMissleOffsetY & lowerRightMissleOffsetY;
			h & missleFrameAngles & troopCountLocationOffset & attackClimaxFrame;
			h & projectileImageName & projectileSpin;
		}
	} animation;

	//sound info
	struct CreatureBattleSounds
	{
		std::string attack;
		std::string defend;
		std::string killed; // was killed or died
		std::string move;
		std::string shoot; // range attack
		std::string wince; // attacked but did not die
		std::string ext1;  // creature specific extension
		std::string ext2;  // creature specific extension
		std::string startMoving; // usually same as ext1
		std::string endMoving;	// usually same as ext2

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & attack & defend & killed & move & shoot & wince & ext1 & ext2 & startMoving & endMoving;
		}
	} sounds;

	bool isItNativeTerrain(int terrain) const;
	bool isDoubleWide() const; //returns true if unit is double wide on battlefield
	bool isFlying() const; //returns true if it is a flying unit
	bool isShooting() const; //returns true if unit can shoot
	bool isUndead() const; //returns true if unit is undead
	bool isGood () const;
	bool isEvil () const;
	si32 maxAmount(const std::vector<si32> &res) const; //how many creatures can be bought
	static int getQuantityID(const int & quantity); //0 - a few, 1 - several, 2 - pack, 3 - lots, 4 - horde, 5 - throng, 6 - swarm, 7 - zounds, 8 - legion
	static int estimateCreatureCount(ui32 countID); //reverse version of above function, returns middle of range
	bool isMyUpgrade(const CCreature *anotherCre) const;

	bool valid() const;

	void addBonus(int val, Bonus::BonusType type, int subtype = -1);
	std::string nodeName() const override;

	template<typename RanGen>
	int getRandomAmount(RanGen ranGen) const
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
			& fightValue & AIValue & growth & hordeGrowth
			& ammMin & ammMax & level
			& abilityText & animDefName & advMapDef;
		h & iconIndex;

		h & idNumber & faction & sounds & animation;

		h & doubleWide & special;
	}

	CCreature();
};

class DLL_LINKAGE CCreatureHandler
{
private:
	CBonusSystemNode allCreatures;
	CBonusSystemNode creaturesOfLevel[GameConstants::CREATURES_PER_TOWN + 1];//index 0 is used for creatures of unknown tier or outside <1-7> range

	void loadStackExperience(CCreature * creature, const JsonNode &input);
	void loadCreatureJson(CCreature * creature, const JsonNode & config);
public:
	std::set<CreatureID> doubledCreatures; //they get double week
	std::vector<ConstTransitivePtr<CCreature> > creatures; //creature ID -> creature info.

	//stack exp
	std::vector<std::vector<ui32> > expRanks; // stack experience needed for certain rank, index 0 for other tiers (?)
	std::vector<ui32> maxExpPerBattle; //%, tiers same as above
	si8 expAfterUpgrade;//multiplier in %

	//Commanders
	BonusList commanderLevelPremy; //bonus values added with each level-up
	std::vector< std::vector <ui8> > skillLevels; //how much of a bonus will be given to commander with every level. SPELL_POWER also gives CASTS and RESISTANCE
	std::vector <std::pair <Bonus, std::pair <ui8, ui8> > > skillRequirements; // first - Bonus, second - which two skills are needed to use it

	/// loading functions

	/// adding abilities from ZCRTRAIT.TXT
	void loadBonuses(CCreature & creature, std::string bonuses);
	/// load all creatures from H3 files
	void load();
	/// load creature from json structure
	void load(std::string creatureID, const JsonNode & node);
	/// load one creature from json config
	CCreature * loadCreature(const JsonNode & node);
	/// generates tier-specific bonus tree entries
	void buildBonusTreeForTiers();
	/// read cranim.txt file from H3
	void loadAnimationInfo();
	/// read one line from cranim.txt
	void loadUnitAnimInfo(CCreature & unit, CLegacyConfigParser &parser);
	/// parse crexpbon.txt file from H3
	void loadStackExp(Bonus & b, BonusList & bl, CLegacyConfigParser &parser);
	/// help function for parsing CREXPBON.txt
	int stringToNumber(std::string & s);

	CCreatureHandler();
	~CCreatureHandler();

	void deserializationFix();
	CreatureID pickRandomMonster(const boost::function<int()> &randGen = 0, int tier = -1) const; //tier <1 - CREATURES_PER_TOWN> or -1 for any
	void addBonusForTier(int tier, Bonus *b); //tier must be <1-7>
	void addBonusForAllCreatures(Bonus *b);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//TODO: should be optimized, not all these informations needs to be serialized (same for ccreature)
		h & doubledCreatures & creatures;
		h & expRanks & maxExpPerBattle & expAfterUpgrade;
		h & skillLevels & skillRequirements & commanderLevelPremy;
		h & allCreatures;
		h & creaturesOfLevel;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

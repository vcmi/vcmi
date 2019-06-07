/*
 * CCreatureHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Creature.h>
#include <vcmi/CreatureService.h>

#include "HeroBonus.h"
#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "GameConstants.h"
#include "JsonNode.h"
#include "IHandlerBase.h"
#include "CRandomGenerator.h"

class CLegacyConfigParser;
class CCreatureHandler;
class CCreature;

class DLL_LINKAGE CCreature : public Creature, public CBonusSystemNode
{
public:
	std::string identifier;

	std::string nameRef; // reference name, stringID
	std::string nameSing;// singular name, e.g. Centaur
	std::string namePl;  // plural name, e.g. Centaurs

	std::string abilityText; //description of abilities

	CreatureID idNumber;
	TFaction faction;
	ui8 level; // 0 - unknown; 1-7 for "usual" creatures

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

	/// names of files with appropriate icons. Used only during loading
	std::string smallIconName;
	std::string largeIconName;

	struct CreatureAnimation
	{
		double timeBetweenFidgets, idleAnimationTime,
			   walkAnimationTime, attackAnimationTime, flightAnimationDistance;
		int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX,
		    upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;

		std::vector<double> missleFrameAngles;
		int troopCountLocationOffset, attackClimaxFrame;

		std::string projectileImageName;
		//bool projectileSpin; //if true, appropriate projectile is spinning during flight

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & timeBetweenFidgets;
			h & idleAnimationTime;
			h & walkAnimationTime;
			h & attackAnimationTime;
			h & flightAnimationDistance;
			h & upperRightMissleOffsetX;
			h & rightMissleOffsetX;
			h & lowerRightMissleOffsetX;
			h & upperRightMissleOffsetY;
			h & rightMissleOffsetY;
			h & lowerRightMissleOffsetY;
			h & missleFrameAngles;
			h & troopCountLocationOffset;
			h & attackClimaxFrame;
			h & projectileImageName;
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
		std::string startMoving;
		std::string endMoving;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & attack;
			h & defend;
			h & killed;
			h & move;
			h & shoot;
			h & wince;
			h & startMoving;
			h & endMoving;
		}
	} sounds;

	ArtifactID warMachine;

	int32_t getIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	CreatureID getId() const override;
	const std::string & getPluralName() const override;
	const std::string & getSingularName() const override;
	uint32_t getMaxHealth() const override;

	bool isItNativeTerrain(int terrain) const;
	bool isDoubleWide() const; //returns true if unit is double wide on battlefield
	bool isFlying() const; //returns true if it is a flying unit
	bool isShooting() const; //returns true if unit can shoot
	bool isUndead() const; //returns true if unit is undead
	bool isGood () const;
	bool isEvil () const;
	si32 maxAmount(const std::vector<si32> &res) const; //how many creatures can be bought
	static int getQuantityID(const int & quantity); //1 - a few, 2 - several, 3 - pack, 4 - lots, 5 - horde, 6 - throng, 7 - swarm, 8 - zounds, 9 - legion
	static int estimateCreatureCount(ui32 countID); //reverse version of above function, returns middle of range
	bool isMyUpgrade(const CCreature *anotherCre) const;

	bool valid() const;

	void setId(CreatureID ID); //assigns idNumber and updates bonuses to reference it
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
		h & namePl;
		h & nameSing;
		h & nameRef;
		h & cost;
		h & upgrades;
		h & fightValue;
		h & AIValue;
		h & growth;
		h & hordeGrowth;
		h & ammMin;
		h & ammMax;
		h & level;
		h & abilityText;
		h & animDefName;
		h & advMapDef;
		h & iconIndex;
		h & smallIconName;
		h & largeIconName;

		h & idNumber;
		h & faction;
		h & sounds;
		h & animation;

		h & doubleWide;
		h & special;
		if(version>=759)
		{
			h & identifier;
		}
		if(version >= 771)
		{
			h & warMachine;
		}
		else if(!h.saving)
		{
			fillWarMachine();
		}
	}

	CCreature();

private:
	void fillWarMachine();
};

class DLL_LINKAGE CCreatureHandler : public IHandlerBase, public CreatureService
{
private:
	CBonusSystemNode allCreatures;
	CBonusSystemNode creaturesOfLevel[GameConstants::CREATURES_PER_TOWN + 1];//index 0 is used for creatures of unknown tier or outside <1-7> range

	/// load one creature from json config
	CCreature * loadFromJson(const JsonNode & node, const std::string & identifier);

	void loadJsonAnimation(CCreature * creature, const JsonNode & graphics);
	void loadStackExperience(CCreature * creature, const JsonNode &input);
	void loadCreatureJson(CCreature * creature, const JsonNode & config);

	/// loading functions

	/// adding abilities from ZCRTRAIT.TXT
	void loadBonuses(JsonNode & creature, std::string bonuses);
	/// load all creatures from H3 files
	void load();
	void loadCommanders();
	/// load creature from json structure
	void load(std::string creatureID, const JsonNode & node);
	/// read cranim.txt file from H3
	void loadAnimationInfo(std::vector<JsonNode> & h3Data);
	/// read one line from cranim.txt
	void loadUnitAnimInfo(JsonNode & unit, CLegacyConfigParser &parser);
	/// parse crexpbon.txt file from H3
	void loadStackExp(Bonus & b, BonusList & bl, CLegacyConfigParser &parser);
	/// help function for parsing CREXPBON.txt
	int stringToNumber(std::string & s);

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
	std::vector <std::pair <std::shared_ptr<Bonus>, std::pair <ui8, ui8> > > skillRequirements; // first - Bonus, second - which two skills are needed to use it

	const CCreature * getCreature(const std::string & scope, const std::string & identifier) const;

	void deserializationFix();
	CreatureID pickRandomMonster(CRandomGenerator & rand, int tier = -1) const; //tier <1 - CREATURES_PER_TOWN> or -1 for any
	void addBonusForTier(int tier, std::shared_ptr<Bonus> b); //tier must be <1-7>
	void addBonusForAllCreatures(std::shared_ptr<Bonus> b);

	CCreatureHandler();
	~CCreatureHandler();

	const Entity * getBaseByIndex(const int32_t index) const override;

	const Creature * getById(const CreatureID & id) const override;
	const Creature * getByIndex(const int32_t index) const override;

	void forEachBase(const std::function<void(const Entity * entity, bool & stop)> & cb) const override;
	void forEach(const std::function<void(const Creature * entity, bool & stop)> & cb) const override;

	/// load all creatures from H3 files
	void loadCrExpBon();
	/// generates tier-specific bonus tree entries
	void buildBonusTreeForTiers();

	void afterLoadFinalization() override;

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//TODO: should be optimized, not all these informations needs to be serialized (same for ccreature)
		h & doubledCreatures;
		h & creatures;
		h & expRanks;
		h & maxExpPerBattle;
		h & expAfterUpgrade;
		h & skillLevels;
		h & skillRequirements;
		h & commanderLevelPremy;
		h & allCreatures;
		h & creaturesOfLevel;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

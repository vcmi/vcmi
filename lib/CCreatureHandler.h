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

VCMI_LIB_NAMESPACE_BEGIN

class CLegacyConfigParser;
class CCreatureHandler;
class CCreature;
class JsonSerializeFormat;

class DLL_LINKAGE CCreature : public Creature, public CBonusSystemNode
{
	friend class CCreatureHandler;
	std::string modScope;
	std::string identifier;

//	std::string nameRef; // reference name, stringID
//	std::string nameSing;// singular name, e.g. Centaur
//	std::string namePl;  // plural name, e.g. Centaurs

	const std::string & getName() const override;
	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;

public:
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
		struct RayColor {
			uint8_t r1, g1, b1, a1;
			uint8_t r2, g2, b2, a2;

			template <typename Handler> void serialize(Handler &h, const int version)
			{
				h & r1 & g1 & b1 & a1 & r2 & g2 & b2 & a2;
			}
		};

		double timeBetweenFidgets, idleAnimationTime,
			   walkAnimationTime, attackAnimationTime, flightAnimationDistance;
		int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX,
		    upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;

		std::vector<double> missleFrameAngles;
		int troopCountLocationOffset, attackClimaxFrame;

		std::string projectileImageName;
		std::vector<RayColor> projectileRay;
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
			h & projectileRay;
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

	std::string getNamePluralTranslated() const override;
	std::string getNameSingularTranslated() const override;

	std::string getNamePluralTextID() const override;
	std::string getNameSingularTextID() const override;

	bool isItNativeTerrain(TerrainId terrain) const;
	/**
	Returns creature native terrain considering some terrain bonuses.
	@param considerBonus is used to avoid Dead Lock when this method is called inside getAllBonuses
	considerBonus = true is called from Pathfinder and fills actual nativeTerrain considering bonus(es).
	considerBonus = false is called on Battle init and returns already prepared nativeTerrain without Bonus system calling.
	*/
	TerrainId getNativeTerrain() const;
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	CreatureID getId() const override;
	virtual const IBonusBearer * accessBonuses() const override;
	uint32_t getMaxHealth() const override;

	int32_t getAdvMapAmountMin() const override;
	int32_t getAdvMapAmountMax() const override;
	int32_t getAIValue() const override;
	int32_t getFightValue() const override;
	int32_t getLevel() const override;
	int32_t getGrowth() const override;
	int32_t getHorde() const override;
	int32_t getFactionIndex() const override;

	int32_t getBaseAttack() const override;
	int32_t getBaseDefense() const override;
	int32_t getBaseDamageMin() const override;
	int32_t getBaseDamageMax() const override;
	int32_t getBaseHitPoints() const override;
	int32_t getBaseSpellPoints() const override;
	int32_t getBaseSpeed() const override;
	int32_t getBaseShots() const override;

	int32_t getCost(int32_t resIndex) const override;
	bool isDoubleWide() const override; //returns true if unit is double wide on battlefield

	bool isGood () const;
	bool isEvil () const;
	si32 maxAmount(const std::vector<si32> &res) const; //how many creatures can be bought
	static int getQuantityID(const int & quantity); //1 - a few, 2 - several, 3 - pack, 4 - lots, 5 - horde, 6 - throng, 7 - swarm, 8 - zounds, 9 - legion
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

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & cost;
		h & upgrades;
		h & fightValue;
		h & AIValue;
		h & growth;
		h & hordeGrowth;
		h & ammMin;
		h & ammMax;
		h & level;
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
		h & identifier;
		h & modScope;
		h & warMachine;
	}

	CCreature();

private:
	void fillWarMachine();
};

class DLL_LINKAGE CCreatureHandler : public CHandlerBase<CreatureID, Creature, CCreature, CreatureService>
{
private:
	CBonusSystemNode allCreatures;
	CBonusSystemNode creaturesOfLevel[GameConstants::CREATURES_PER_TOWN + 1];//index 0 is used for creatures of unknown tier or outside <1-7> range

	void loadJsonAnimation(CCreature * creature, const JsonNode & graphics);
	void loadStackExperience(CCreature * creature, const JsonNode &input);
	void loadCreatureJson(CCreature * creature, const JsonNode & config);

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

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CCreature * loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;

public:
	std::set<CreatureID> doubledCreatures; //they get double week

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
	void addBonusForTier(int tier, const std::shared_ptr<Bonus> & b); //tier must be <1-7>
	void addBonusForAllCreatures(const std::shared_ptr<Bonus> & b); //due to CBonusSystem::addNewBonus(const std::shared_ptr<Bonus>& b);
	void removeBonusesFromAllCreatures();

	CCreatureHandler();
	~CCreatureHandler();

	/// load all creatures from H3 files
	void loadCrExpBon();
	/// generates tier-specific bonus tree entries
	void buildBonusTreeForTiers();

	void afterLoadFinalization() override;

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//TODO: should be optimized, not all these informations needs to be serialized (same for ccreature)
		h & doubledCreatures;
		h & objects;
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

VCMI_LIB_NAMESPACE_END

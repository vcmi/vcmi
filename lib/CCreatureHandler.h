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

#include "HeroBonus.h"
#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "GameConstants.h"
#include "JsonNode.h"
#include "IHandlerBase.h"
#include "CRandomGenerator.h"
#include "Color.h"

#include <vcmi/Creature.h>
#include <vcmi/CreatureService.h>

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

	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;

	CreatureID idNumber;

	FactionID faction = FactionID::NEUTRAL;
	ui8 level = 0; // 0 - unknown; 1-7 for "usual" creatures

	//stats that are not handled by bonus system
	ui32 fightValue, AIValue, growth, hordeGrowth;

	bool doubleWide = false;

	si32 iconIndex = -1; // index of icon in files like twcrport

	TResources cost; //cost[res_id] - amount of that resource required to buy creature from dwelling

public:
	ui32 ammMin, ammMax; // initial size of stack of these creatures on adventure map (if not set in editor)

	bool special = true; // Creature is not available normally (war machines, commanders, several unused creatures, etc

	std::set<CreatureID> upgrades; // IDs of creatures to which this creature can be upgraded

	std::string animDefName; // creature animation used during battles
	std::string advMapDef; //for new creatures only, image for adventure map

	/// names of files with appropriate icons. Used only during loading
	std::string smallIconName;
	std::string largeIconName;

	enum class CreatureQuantityId
	{
		FEW = 1,
		SEVERAL,
		PACK,
		LOTS,
		HORDE,
		THRONG,
		SWARM,
		ZOUNDS,
		LEGION
	};

	struct CreatureAnimation
	{
		struct RayColor {
			ColorRGBA start;
			ColorRGBA end;

			template <typename Handler> void serialize(Handler &h, const int version)
			{
				h & start & end;
			}
		};

		double timeBetweenFidgets, idleAnimationTime,
			   walkAnimationTime, attackAnimationTime;
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

			if (version < 814)
			{
				float unused = 0.f;
				h & unused;
			}

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

	FactionID getFaction() const override;
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	CreatureID getId() const override;
	virtual const IBonusBearer * getBonusBearer() const override;
	uint32_t getMaxHealth() const override;

	int32_t getAdvMapAmountMin() const override;
	int32_t getAdvMapAmountMax() const override;
	int32_t getAIValue() const override;
	int32_t getFightValue() const override;
	int32_t getLevel() const override;
	int32_t getGrowth() const override;
	int32_t getHorde() const override;

	int32_t getBaseAttack() const override;
	int32_t getBaseDefense() const override;
	int32_t getBaseDamageMin() const override;
	int32_t getBaseDamageMax() const override;
	int32_t getBaseHitPoints() const override;
	int32_t getBaseSpellPoints() const override;
	int32_t getBaseSpeed() const override;
	int32_t getBaseShots() const override;

	int32_t getRecruitCost(GameResID resIndex) const override;
	TResources getFullRecruitCost() const override;
	bool isDoubleWide() const override; //returns true if unit is double wide on battlefield
	bool hasUpgrades() const override;

	bool isGood () const;
	bool isEvil () const;
	si32 maxAmount(const TResources &res) const; //how many creatures can be bought
	static CCreature::CreatureQuantityId getQuantityID(const int & quantity);
	static std::string getQuantityRangeStringForId(const CCreature::CreatureQuantityId & quantityId);
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
	static const std::map<CreatureQuantityId, std::string> creatureQuantityRanges;
};

class DLL_LINKAGE CCreatureHandler : public CHandlerBase<CreatureID, Creature, CCreature, CreatureService>
{
private:
	void loadJsonAnimation(CCreature * creature, const JsonNode & graphics) const;
	void loadStackExperience(CCreature * creature, const JsonNode & input) const;
	void loadCreatureJson(CCreature * creature, const JsonNode & config) const;

	/// adding abilities from ZCRTRAIT.TXT
	void loadBonuses(JsonNode & creature, std::string bonuses) const;
	/// load all creatures from H3 files
	void load();
	void loadCommanders();
	/// load creature from json structure
	void load(std::string creatureID, const JsonNode & node);
	/// read cranim.txt file from H3
	void loadAnimationInfo(std::vector<JsonNode> & h3Data) const;
	/// read one line from cranim.txt
	void loadUnitAnimInfo(JsonNode & unit, CLegacyConfigParser & parser) const;
	/// parse crexpbon.txt file from H3
	void loadStackExp(Bonus & b, BonusList & bl, CLegacyConfigParser & parser) const;
	/// help function for parsing CREXPBON.txt
	int stringToNumber(std::string & s) const;

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

	CreatureID pickRandomMonster(CRandomGenerator & rand, int tier = -1) const; //tier <1 - CREATURES_PER_TOWN> or -1 for any

	CCreatureHandler();
	~CCreatureHandler();

	/// load all stack experience bonuses from H3 files
	void loadCrExpBon(CBonusSystemNode & globalEffects);

	void afterLoadFinalization() override;

	std::vector<JsonNode> loadLegacyData() override;

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
	}
};

VCMI_LIB_NAMESPACE_END

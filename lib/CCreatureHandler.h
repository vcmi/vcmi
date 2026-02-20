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

#include "bonuses/Bonus.h"
#include "bonuses/CBonusSystemNode.h"
#include "ResourceSet.h"
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "Color.h"
#include "filesystem/ResourcePath.h"

#include <vcmi/Creature.h>
#include <vcmi/CreatureService.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

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

	TResources cost; //cost[res_id] - amount of that resource required to buy creature from dwelling

public:
	std::string getDescriptionTranslated() const;
	std::string getDescriptionTextID() const;
	std::string getBonusTextID(const std::string & bonusID) const;

	ui32 ammMin; // initial size of stack of these creatures on adventure map (if not set in editor)
	ui32 ammMax;

	bool special = true; // Creature is not available normally (war machines, commanders, several unused creatures, etc
	bool excludeFromRandomization = false;

	std::set<CreatureID> upgrades; // IDs of creatures to which this creature can be upgraded

	AnimationPath animDefName; // creature animation used during battles
	ImagePath mapAttackFromLeft; // adventure map creature image when attacked from left
	ImagePath mapAttackFromRight; // adventure map creature image when attacked from right

	si32 iconIndex = -1; // index of icon in files like twcrport, used in tests now.
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
		};

		double timeBetweenFidgets, idleAnimationTime,
			   walkAnimationTime, attackAnimationTime;
		int upperRightMissileOffsetX, rightMissileOffsetX, lowerRightMissileOffsetX,
		    upperRightMissileOffsetY, rightMissileOffsetY, lowerRightMissileOffsetY;

		std::vector<double> missileFrameAngles;
		int attackClimaxFrame;

		AnimationPath projectileImageName;
		std::vector<RayColor> projectileRay;

	} animation;

	//sound info
	struct CreatureBattleSounds
	{
		AudioPath attack;
		AudioPath defend;
		AudioPath killed; // was killed or died
		AudioPath move;
		AudioPath shoot; // range attack
		AudioPath wince; // attacked but did not die
		AudioPath startMoving;
		AudioPath endMoving;
	} sounds;

	ArtifactID warMachine;

	std::string getNamePluralTranslated() const override;
	std::string getNameSingularTranslated() const override;

	std::string getNamePluralTextID() const override;
	std::string getNameSingularTextID() const override;

	FactionID getFactionID() const override;
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override;
	CreatureID getId() const override;
	const IBonusBearer * getBonusBearer() const override;

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
	const TResources & getFullRecruitCost() const override;
	bool isDoubleWide() const override; //returns true if unit is double wide on battlefield
	bool hasUpgrades() const override;

	bool isGood () const;
	bool isEvil () const;
	si32 maxAmount(const TResources &res) const; //how many creatures can be bought
	static CCreature::CreatureQuantityId getQuantityID(const int & quantity);
	static std::string getQuantityRangeStringForId(const CCreature::CreatureQuantityId & quantityId);
	static int estimateCreatureCount(ui32 countID); //reverse version of above function, returns middle of range

	/// Returns true if this creature can be directly upgraded to target
	bool isMyDirectUpgrade(const CCreature * target) const;

	/// Returns true if this creature can be upgraded to target
	/// Performs full search through potential upgrades of upgrades
	bool isMyDirectOrIndirectUpgrade(const CCreature *target) const;

	void addBonus(int val, BonusType type);
	void addBonus(int val, BonusType type, BonusSubtypeID subtype);
	std::string nodeName() const override;

	int getRandomAmount(vstd::RNG & ranGen) const;
	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

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
	std::shared_ptr<CCreature> loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;

public:
	std::set<CreatureID> doubledCreatures; //they get double week

	//stack exp
	std::vector<std::vector<ui32> > expRanks; // stack experience needed for certain rank, index 0 for other tiers (?)
	std::vector<ui32> maxExpPerBattle; //%, tiers same as above
	si8 expAfterUpgrade;//multiplier in %

	//Commanders
	BonusList commanderLevelPremy; //bonus values added with each level-up
	std::vector< std::vector <ui8> > skillLevels; //how much of a bonus will be given to commander with every level. SPELL_POWER also gives CASTS and RESISTANCE
	std::vector <std::pair <std::vector<std::shared_ptr<Bonus> >, std::pair <ui8, ui8> > > skillRequirements; // first - Bonus, second - which two skills are needed to use it


	CCreatureHandler();
	~CCreatureHandler();

	/// load all stack experience bonuses from H3 files
	void loadCrExpBon(CBonusSystemNode & globalEffects);

	/// load all stack modifier bonuses from H3 files. TODO: move this to json
	void loadCrExpMod();

	void afterLoadFinalization() override;

	std::vector<JsonNode> loadLegacyData() override;

	std::set<CreatureID> getDefaultAllowed() const;

};

VCMI_LIB_NAMESPACE_END

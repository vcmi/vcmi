/*
 * CGHeroInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/spells/Caster.h>

#include "IOwnableObject.h"

#include "army/CArmedInstance.h"
#include "army/CCommanderInstance.h"

#include "../bonuses/BonusCache.h"
#include "../entities/hero/EHeroGender.h"

VCMI_LIB_NAMESPACE_BEGIN

class CHero;
class CGBoat;
class CGTownInstance;
class CMap;
class UpgradeInfo;
class TurnInfo;

struct TerrainTile;
struct TurnInfoCache;

class DLL_LINKAGE CGHeroPlaceholder : public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	/// if this is placeholder by power, then power rank of desired hero
	std::optional<ui8> powerRank;

	/// if this is placeholder by type, then hero type of desired hero
	std::optional<HeroTypeID> heroType;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & powerRank;
		h & heroType;
	}
	
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};


class DLL_LINKAGE CGHeroInstance : public CArmedInstance, public IBoatGenerator, public CArtifactSet, public spells::Caster, public AFactionMember, public ICreatureUpgrader, public IOwnableObject
{
	// We serialize heroes into JSON for crossover
	friend class CampaignState;
	friend class CMapLoaderH3M;
	friend class CMapFormatJson;

	PrimarySkillsCache primarySkills;
	MagicSchoolMasteryCache magicSchoolMastery;
	BonusValueCache manaPerKnowledgeCached;
	std::unique_ptr<TurnInfoCache> turnInfoCache;
	std::unique_ptr<CCommanderInstance> commander;

	std::set<SpellID> spells; //known spells (spell IDs)
	ObjectInstanceID visitedTown; //set if hero is visiting town or in the town garrison
	ObjectInstanceID boardedBoat; //set to CGBoat when sailing

	ui32 movement; //remaining movement points
	bool inTownGarrison; // if hero is in town garrison

	IGameInfoCallback * getCallback() const final { return cb; }

public:
	//////////////////////////////////////////////////////////////////////////
	//format:   123
	//          8 4
	//          765
	ui8 moveDir;
	mutable ui8 tacticFormationEnabled;

	//////////////////////////////////////////////////////////////////////////

	TExpType exp; //experience points
	ui32 level; //current level of hero

	/// If not NONE - then hero should use portrait from referenced hero type
	HeroTypeID customPortraitSource;
	si32 mana; // remaining spell points
	std::vector<std::pair<SecondarySkill,ui8> > secSkills; //first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert); if hero has ability (-1, -1) it meansthat it should have default secondary abilities
	EHeroGender gender;

	std::string nameCustomTextId;
	std::string biographyCustomTextId;

	static constexpr si32 UNINITIALIZED_MANA = -1;
	static constexpr ui32 UNINITIALIZED_MOVEMENT = -1;
	static constexpr auto UNINITIALIZED_EXPERIENCE = std::numeric_limits<TExpType>::max();
	static const ui32 NO_PATROLLING;

	std::set<ObjectInstanceID> visitedObjects;

	struct DLL_LINKAGE Patrol
	{
		bool patrolling{false};
		int3 initialPos;
		ui32 patrolRadius{NO_PATROLLING};
		template <typename Handler> void serialize(Handler &h)
		{
			h & patrolling;
			h & initialPos;
			h & patrolRadius;
		}
	} patrol;

	inline bool isInitialized() const
	{ // has this hero been on the map at least once?
		return movement != UNINITIALIZED_MOVEMENT && mana != UNINITIALIZED_MANA;
	}

	//int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	int getSightRadius() const override; //sight distance (should be used if player-owned structure)
	//////////////////////////////////////////////////////////////////////////

	BoatId getBoatType() const override; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	void getOutOffsets(std::vector<int3> &offsets) const override; //offsets to obj pos when we boat can be placed
	const IObjectInterface * getObject() const override;

	//////////////////////////////////////////////////////////////////////////

	std::string getBiographyTranslated() const;
	std::string getBiographyTextID() const;

	std::string getNameTextID() const;
	std::string getNameTranslated() const;

	HeroTypeID getPortraitSource() const;
	int32_t getIconIndex() const;

	std::string getClassNameTranslated() const;
	std::string getClassNameTextID() const;

	bool inBoat() const;
	CGBoat * getBoat();
	const CGBoat * getBoat() const;
	void setBoat(CGBoat * getBoat);

	bool hasSpellbook() const;
	int maxSpellLevel() const;
	void addSpellToSpellbook(const SpellID & spell);
	void removeSpellFromSpellbook(const SpellID & spell);
	bool spellbookContainsSpell(const SpellID & spell) const;
	std::vector<BonusSourceID> getSourcesForSpell(const SpellID & spell) const;
	void removeSpellbook();
	void removeAllSpells();
	const std::set<SpellID> & getSpellsInSpellbook() const;
	EAlignment getAlignment() const;
	bool needsLastStack()const override;

	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;
	const IOwnableObject * asOwnable() const final;

	//INativeTerrainProvider
	FactionID getFactionID() const override;
	TerrainId getNativeTerrain() const override;
	int getLowestCreatureSpeed() const;
	si32 manaRegain() const; //how many points of mana can hero regain "naturally" in one day
	si32 getManaNewTurn() const; //calculate how much mana this hero is going to have the next day
	int getCurrentLuck(int stack=-1, bool town=false) const;
	int32_t getSpellCost(const spells::Spell * sp) const; //do not use during battles -> bonuses from army would be ignored

	bool canLearnSpell(const spells::Spell * spell,  bool allowBanned = false) const;
	bool canCastThisSpell(const spells::Spell * spell) const; //determines if this hero can cast given spell; takes into account existing spell in spellbook, existing spellbook and artifact bonuses

	/// convert given position between map position (CGObjectInstance::pos) and visitable position used for hero interactions
	int3 convertToVisitablePos(const int3 & position) const;
	int3 convertFromVisitablePos(const int3 & position) const;

	// ----- primary and secondary skill, experience, level handling -----

	/// Returns true if hero has lower level than should upon his experience.
	bool gainsLevel() const;

	/// Selects 0-2 skills for player to select on levelup
	std::vector<SecondarySkill> getLevelupSkillCandidates(IGameRandomizer & gameRandomizer) const;

	ui8 getSecSkillLevel(const SecondarySkill & skill) const; //0 - no skill
	int getPrimSkillLevel(PrimarySkill id) const;

	/// Returns true if hero has free secondary skill slot.
	bool canLearnSkill() const;
	bool canLearnSkill(const SecondarySkill & which) const;

	void setExperience(si64 value, ChangeValueMode mode);
	void setPrimarySkill(PrimarySkill primarySkill, si64 value, ChangeValueMode mode);
	void setSecSkillLevel(const SecondarySkill & which, int val, ChangeValueMode mode); // abs == 0 - changes by value; 1 - sets to value
	void levelUp();

	void setMovementPoints(int points);
	int movementPointsRemaining() const;
	int movementPointsLimit(bool onLand) const;
	//cached version is much faster, TurnInfo construction is costly
	int movementPointsLimitCached(bool onLand, const TurnInfo * ti) const;

	int movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark, const TurnInfo * ti) const;

	std::unique_ptr<TurnInfo> getTurnInfo(int days) const;

	double getFightingStrength() const; // takes attack / defense skill into account
	double getMagicStrength() const; // takes knowledge / spell power skill but also current mana, whether the hero owns a spell-book and whether that books contains anything into account
	double getHeroStrength() const; // includes fighting and magic strength

	/// Returns true if 'left' hero is stronger than 'right' when considering campaign transfer priority
	static bool compareCampaignValue(const CGHeroInstance * left, const CGHeroInstance * right);
	uint64_t getValueForDiplomacy() const;
	
	ui64 getTotalStrength() const; // includes fighting strength and army strength
	TExpType calculateXp(TExpType exp) const; //apply learning skill
	int getBasePrimarySkillValue(PrimarySkill which) const; //the value of a base-skill without items or temporary bonuses

	CStackBasicDescriptor calculateNecromancy (const BattleResult &battleResult) const;
	EDiggingStatus diggingStatus() const;

	//////////////////////////////////////////////////////////////////////////

	const CHeroClass * getHeroClass() const;
	HeroClassID getHeroClassID() const;

	const CHero * getHeroType() const;
	HeroTypeID getHeroTypeID() const;
	void setHeroType(HeroTypeID type);

	bool isGarrisoned() const;
	const CGTownInstance * getVisitedTown() const;
	CGTownInstance * getVisitedTown();
	void setVisitedTown(const CGTownInstance * town, bool garrisoned);

	const CCommanderInstance * getCommander() const;
	CCommanderInstance * getCommander();

	void initObj(IGameRandomizer & gameRandomizer) override;
	void initHero(IGameRandomizer & gameRandomizer);
	void initHero(IGameRandomizer & gameRandomizer, const HeroTypeID & SUBID);

	ArtPlacementMap putArtifact(const ArtifactPosition & pos, const CArtifactInstance * art) override;
	void removeArtifact(const ArtifactPosition & pos) override;
	void initExp(vstd::RNG & rand);
	void initArmy(vstd::RNG & rand, IArmyDescriptor *dst = nullptr);
	void pushPrimSkill(PrimarySkill which, int val);
	ui8 maxlevelsToMagicSchool() const;
	ui8 maxlevelsToWisdom() const;
	void recreateSecondarySkillsBonuses();
	void updateSkillBonus(const SecondarySkill & which, int val);

	void fillUpgradeInfo(UpgradeInfo & info, const CStackInstance &stack) const override;

	bool hasVisions(const CGObjectInstance * target, BonusSubtypeID masteryLevel) const;
	/// If this hero perishes, the scenario is failed
	bool isMissionCritical() const;

	CGHeroInstance(IGameInfoCallback *cb);
	virtual ~CGHeroInstance();

	PlayerColor getOwner() const override;

	///ArtBearer
	ArtBearer bearerType() const override;

	///IBonusBearer
	CBonusSystemNode & whereShouldBeAttached(CGameState & gs) override;
	std::string nodeName() const override;
	si32 manaLimit() const override;

	///IConstBonusProvider
	const IBonusBearer* getBonusBearer() const override;

	///spells::Caster
	int32_t getCasterUnitId() const override;
	int32_t getSpellSchoolLevel(const spells::Spell * spell, SpellSchool * outSelectedSchool = nullptr) const override;
	int64_t getSpellBonus(const spells::Spell * spell, int64_t base, const battle::Unit * affectedStack) const override;
	int64_t getSpecificSpellBonus(const spells::Spell * spell, int64_t base) const override;

	int32_t getEffectLevel(const spells::Spell * spell) const override;
	int32_t getEffectPower(const spells::Spell * spell) const override;
	int32_t getEnchantPower(const spells::Spell * spell) const override;
	int64_t getEffectValue(const spells::Spell * spell) const override;
	int64_t getEffectRange(const spells::Spell * spell) const override;

	PlayerColor getCasterOwner() const override;
	const CGHeroInstance * getHeroCaster() const override;

	void getCasterName(MetaString & text) const override;
	void getCastDescription(const spells::Spell * spell, const battle::Units & attacked, MetaString & text) const override;
	void spendMana(ServerCallback * server, const int spellCost) const override;

	void updateAppearance();

	void pickRandomObject(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	std::string getObjectName() const override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getMovementPointsTextIfOwner(PlayerColor player) const;

	TObjectTypeHandler getObjectHandler() const override;

	void afterAddToMap(CMap * map) override;
	void afterRemoveFromMap(CMap * map) override;
	void attachToBonusSystem(CGameState & gs) override;
	void detachFromBonusSystem(CGameState & gs) override;
	void restoreBonusSystem(CGameState & gs) override;

	void updateFrom(const JsonNode & data) override;

	bool isCoastVisitable() const override;
	bool isBlockedVisitable() const override;
	BattleField getBattlefield() const override;

	bool isCampaignYog() const;
	bool isCampaignGem() const;

protected:
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;//synchr
	///common part of hero instance and hero definition
	void serializeCommonOptions(JsonSerializeFormat & handler);

	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	void levelUpAutomatically(IGameRandomizer & gameRandomizer);
	void attachCommanderToArmy();

public:
	std::string getHeroTypeName() const;
	void setHeroTypeName(const std::string & identifier);

	void serializeJsonDefinition(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<CArtifactSet&>(*this);
		h & exp;
		h & level;
		h & nameCustomTextId;
		h & biographyCustomTextId;
		h & customPortraitSource;
		h & mana;
		h & secSkills;
		h & movement;
		h & gender;
		h & inTownGarrison;
		h & spells;
		h & patrol;
		h & moveDir;
		if (!h.hasFeature(Handler::Version::RANDOMIZATION_REWORK))
		{
			ui8 magicSchoolCounter = 0;
			ui8 wisdomCounter = 0;

			h & magicSchoolCounter;
			h & wisdomCounter;
		}

		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			h & visitedTown;
			h & boardedBoat;
		}
		else
		{
			std::shared_ptr<CGObjectInstance> ptrTown;
			std::shared_ptr<CGObjectInstance> ptrBoat;
			h & ptrTown;
			h & ptrBoat;

			visitedTown = ptrTown ? ptrTown->id : ObjectInstanceID();
			boardedBoat = ptrBoat ? ptrBoat->id : ObjectInstanceID();
		}

		h & commander;
		h & visitedObjects;

		if(!h.saving && h.loadingGamestate)
			attachCommanderToArmy();
	}
};

VCMI_LIB_NAMESPACE_END

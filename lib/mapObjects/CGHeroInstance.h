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

#include "CObjectHandler.h"
#include "CArmedInstance.h"

#include "../CArtHandler.h" // For CArtifactSet
#include "../CRandomGenerator.h"

VCMI_LIB_NAMESPACE_BEGIN

class CHero;
class CGBoat;
class CGTownInstance;
class CMap;
struct TerrainTile;
struct TurnInfo;

class CGHeroPlaceholder : public CGObjectInstance
{
public:
	//subID stores id of hero type. If it's 0xff then following field is used
	ui8 power;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & power;
	}
};


class DLL_LINKAGE CGHeroInstance : public CArmedInstance, public IBoatGenerator, public CArtifactSet, public spells::Caster
{
	// We serialize heroes into JSON for crossover
	friend class CCampaignState;
	friend class CMapLoaderH3M;

private:
	std::set<SpellID> spells; //known spells (spell IDs)

public:
	//////////////////////////////////////////////////////////////////////////

	ui8 moveDir; //format:	123
					//		8 4
					//		765
	mutable ui8 isStanding, tacticFormationEnabled;

	//////////////////////////////////////////////////////////////////////////

	ConstTransitivePtr<CHero> type;
	TExpType exp; //experience points
	ui32 level; //current level of hero
	std::string name; //may be custom
	std::string biography; //if custom
	si32 portrait; //may be custom
	si32 mana; // remaining spell points
	std::vector<std::pair<SecondarySkill,ui8> > secSkills; //first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert); if hero has ability (-1, -1) it meansthat it should have default secondary abilities
	ui32 movement; //remaining movement points
	ui8 sex;
	bool inTownGarrison; // if hero is in town garrison
	ConstTransitivePtr<CGTownInstance> visitedTown; //set if hero is visiting town or in the town garrison
	ConstTransitivePtr<CCommanderInstance> commander;
	const CGBoat *boat; //set to CGBoat when sailing

	static const si32 UNINITIALIZED_PORTRAIT = -1;
	static const si32 UNINITIALIZED_MANA = -1;
	static const ui32 UNINITIALIZED_MOVEMENT = -1;

	//std::vector<const CArtifact*> artifacts; //hero's artifacts from bag
	//std::map<ui16, const CArtifact*> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::set<ObjectInstanceID> visitedObjects;

	struct DLL_LINKAGE Patrol
	{
		Patrol(){patrolling=false;initialPos=int3();patrolRadius=-1;};
		bool patrolling;
		int3 initialPos;
		ui32 patrolRadius;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & patrolling;
			h & initialPos;
			h & patrolRadius;
		}
	} patrol;

	// deprecated - used only for loading of old saves
	struct HeroSpecial : CBonusSystemNode
	{
		bool growsWithLevel;

		HeroSpecial(){growsWithLevel = false;};

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & static_cast<CBonusSystemNode&>(*this);
			h & growsWithLevel;
		}
	};

	struct DLL_LINKAGE SecondarySkillsInfo
	{
		//skills are determined, initialized at map start
		//FIXME remove mutable
		mutable CRandomGenerator rand;
		ui8 magicSchoolCounter;
		ui8 wisdomCounter;

		SecondarySkillsInfo();

		void resetMagicSchoolCounter();
		void resetWisdomCounter();

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & magicSchoolCounter;
			h & wisdomCounter;
			h & rand;
		}
	} skillsInfo;

	inline bool isInitialized() const
	{ // has this hero been on the map at least once?
		return movement != UNINITIALIZED_MOVEMENT && mana != UNINITIALIZED_MANA;
	}

	//int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	int getSightRadius() const override; //sight distance (should be used if player-owned structure)
	//////////////////////////////////////////////////////////////////////////

	int getBoatType() const override; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	void getOutOffsets(std::vector<int3> &offsets) const override; //offsets to obj pos when we boat can be placed

	//////////////////////////////////////////////////////////////////////////

	bool hasSpellbook() const;
	int maxSpellLevel() const;
	void addSpellToSpellbook(SpellID spell);
	void removeSpellFromSpellbook(SpellID spell);
	bool spellbookContainsSpell(SpellID spell) const;
	void removeSpellbook();
	const std::set<SpellID> & getSpellsInSpellbook() const;
	EAlignment::EAlignment getAlignment() const;
	const std::string &getBiography() const;
	bool needsLastStack()const override;

	ui32 getTileCost(const TerrainTile &dest, const TerrainTile &from, const TurnInfo * ti) const; //move cost - applying pathfinding skill, road and terrain modifiers. NOT includes diagonal move penalty, last move levelling
	TerrainId getNativeTerrain() const;
	ui32 getLowestCreatureSpeed() const;
	int3 getPosition(bool h3m = false) const; //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
	si32 manaRegain() const; //how many points of mana can hero regain "naturally" in one day
	si32 getManaNewTurn() const; //calculate how much mana this hero is going to have the next day
	int getCurrentLuck(int stack=-1, bool town=false) const;
	int32_t getSpellCost(const spells::Spell * sp) const; //do not use during battles -> bonuses from army would be ignored

	bool canLearnSpell(const spells::Spell * spell) const;
	bool canCastThisSpell(const spells::Spell * spell) const; //determines if this hero can cast given spell; takes into account existing spell in spellbook, existing spellbook and artifact bonuses


	// ----- primary and secondary skill, experience, level handling -----

	/// Returns true if hero has lower level than should upon his experience.
	bool gainsLevel() const;

	/// Returns the next primary skill on level up. Can only be called if hero can gain a level up.
	PrimarySkill::PrimarySkill nextPrimarySkill(CRandomGenerator & rand) const;

	/// Returns the next secondary skill randomly on level up. Can only be called if hero can gain a level up.
	boost::optional<SecondarySkill> nextSecondarySkill(CRandomGenerator & rand) const;

	/// Gets 0, 1 or 2 secondary skills which are proposed on hero level up.
	std::vector<SecondarySkill> getLevelUpProposedSecondarySkills() const;

	ui8 getSecSkillLevel(SecondarySkill skill) const; //0 - no skill

	/// Returns true if hero has free secondary skill slot.
	bool canLearnSkill() const;
	bool canLearnSkill(SecondarySkill which) const;

	void setPrimarySkill(PrimarySkill::PrimarySkill primarySkill, si64 value, ui8 abs);
	void setSecSkillLevel(SecondarySkill which, int val, bool abs);// abs == 0 - changes by value; 1 - sets to value
	void levelUp(std::vector<SecondarySkill> skills);

	int maxMovePoints(bool onLand) const;
	//cached version is much faster, TurnInfo construction is costly
	int maxMovePointsCached(bool onLand, const TurnInfo * ti) const;

	int movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark = false, const TurnInfo * ti = nullptr) const;

	static int3 convertPosition(int3 src, bool toh3m); //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
	double getFightingStrength() const; // takes attack / defense skill into account
	double getMagicStrength() const; // takes knowledge / spell power skill into account
	double getHeroStrength() const; // includes fighting and magic strength
	ui64 getTotalStrength() const; // includes fighting strength and army strength
	TExpType calculateXp(TExpType exp) const; //apply learning skill

	CStackBasicDescriptor calculateNecromancy (const BattleResult &battleResult) const;
	void showNecromancyDialog(const CStackBasicDescriptor &raisedStack, CRandomGenerator & rand) const;
	EDiggingStatus diggingStatus() const;

	//////////////////////////////////////////////////////////////////////////

	void setType(si32 ID, si32 subID) override;

	void initHero(CRandomGenerator & rand);
	void initHero(CRandomGenerator & rand, HeroTypeID SUBID);

	void putArtifact(ArtifactPosition pos, CArtifactInstance * art) override;
	void putInBackpack(CArtifactInstance *art);
	void initExp(CRandomGenerator & rand);
	void initArmy(CRandomGenerator & rand, IArmyDescriptor *dst = nullptr);
	//void giveArtifact (ui32 aid);
	void pushPrimSkill(PrimarySkill::PrimarySkill which, int val);
	ui8 maxlevelsToMagicSchool() const;
	ui8 maxlevelsToWisdom() const;
	void recreateSecondarySkillsBonuses();
	void updateSkillBonus(SecondarySkill which, int val);

	bool hasVisions(const CGObjectInstance * target, const int subtype) const;
	/// If this hero perishes, the scenario is failed
	bool isMissionCritical() const;

	CGHeroInstance();
	virtual ~CGHeroInstance();

	PlayerColor getOwner() const override;

	///ArtBearer
	ArtBearer::ArtBearer bearerType() const override;

	///IBonusBearer
	CBonusSystemNode & whereShouldBeAttached(CGameState * gs) override;
	std::string nodeName() const override;

	CBonusSystemNode * whereShouldBeAttachedOnSiege(const bool isBattleOutsideTown) const;
	CBonusSystemNode * whereShouldBeAttachedOnSiege(CGameState * gs);

	///spells::Caster
	int32_t getCasterUnitId() const override;
	int32_t getSpellSchoolLevel(const spells::Spell * spell, int32_t * outSelectedSchool = nullptr) const override;
	int64_t getSpellBonus(const spells::Spell * spell, int64_t base, const battle::Unit * affectedStack) const override;
	int64_t getSpecificSpellBonus(const spells::Spell * spell, int64_t base) const override;

	int32_t getEffectLevel(const spells::Spell * spell) const override;
	int32_t getEffectPower(const spells::Spell * spell) const override;
	int32_t getEnchantPower(const spells::Spell * spell) const override;
	int64_t getEffectValue(const spells::Spell * spell) const override;

	PlayerColor getCasterOwner() const override;

	void getCasterName(MetaString & text) const override;
	void getCastDescription(const spells::Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const override;
	void spendMana(ServerCallback * server, const int spellCost) const override;

	void deserializationFix();

	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	std::string getObjectName() const override;

	void afterAddToMap(CMap * map) override;
	void afterRemoveFromMap(CMap * map) override;

	void updateFrom(const JsonNode & data) override;

	BattleField getBattlefield() const override;
protected:
	void setPropertyDer(ui8 what, ui32 val) override;//synchr
	///common part of hero instance and hero definition
	void serializeCommonOptions(JsonSerializeFormat & handler);

	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	void levelUpAutomatically(CRandomGenerator & rand);

public:
	std::string getHeroTypeName() const;
	void setHeroTypeName(const std::string & identifier);

	void serializeJsonDefinition(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<CArtifactSet&>(*this);
		h & exp;
		h & level;
		h & name;
		h & biography;
		h & portrait;
		h & mana;
		h & secSkills;
		h & movement;
		h & sex;
		h & inTownGarrison;
		h & spells;
		h & patrol;
		h & moveDir;
		h & skillsInfo;
		h & visitedTown;
		h & boat;
		h & type;
		h & commander;
		h & visitedObjects;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

VCMI_LIB_NAMESPACE_END

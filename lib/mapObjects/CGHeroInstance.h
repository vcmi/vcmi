#pragma once

#include "CObjectHandler.h"
#include "CArmedInstance.h"
#include "../spells/Magic.h"

#include "../CArtHandler.h" // For CArtifactSet
#include "../CRandomGenerator.h"

/*
 * CGHeroInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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


class DLL_LINKAGE CGHeroInstance : public CArmedInstance, public IBoatGenerator, public CArtifactSet, public ISpellCaster
{
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

	static const ui32 UNINITIALIZED_PORTRAIT = -1;
	static const ui32 UNINITIALIZED_MANA = -1;
	static const ui32 UNINITIALIZED_MOVEMENT = -1;

	//std::vector<const CArtifact*> artifacts; //hero's artifacts from bag
	//std::map<ui16, const CArtifact*> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::set<SpellID> spells; //known spells (spell IDs)
	std::set<ObjectInstanceID> visitedObjects;

	struct DLL_LINKAGE Patrol
	{
		Patrol(){patrolling=false;initialPos=int3();patrolRadious=-1;};
		bool patrolling;
		int3 initialPos;
		ui32 patrolRadious;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & patrolling;
			if(version >= 755) //save format backward compatibility
			{
				h & initialPos;
			}
			else if(!h.saving)
			{
				patrolling = false;
				initialPos = int3();
			}
			h & patrolRadious;
		}
	} patrol;

	struct DLL_LINKAGE HeroSpecial : CBonusSystemNode
	{
		bool growsWithLevel;

		HeroSpecial(){growsWithLevel = false;};

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & static_cast<CBonusSystemNode&>(*this);
			h & growsWithLevel;
		}
	};

	std::vector<HeroSpecial*> specialty;

	struct DLL_LINKAGE SecondarySkillsInfo
	{
		//skills are determined, initialized at map start
		//FIXME remove mutable
		mutable CRandomGenerator rand;
		ui8 magicSchoolCounter;
		ui8 wisdomCounter;

		void resetMagicSchoolCounter();
		void resetWisdomCounter();

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & magicSchoolCounter & wisdomCounter & rand;
		}
	} skillsInfo;

	inline bool isInitialized() const
	{ // has this hero been on the map at least once?
		return movement != UNINITIALIZED_MOVEMENT && mana != UNINITIALIZED_MANA;
	}

	//int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	int getSightRadious() const override; //sight distance (should be used if player-owned structure)
	//////////////////////////////////////////////////////////////////////////

	int getBoatType() const override; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	void getOutOffsets(std::vector<int3> &offsets) const override; //offsets to obj pos when we boat can be placed

	//////////////////////////////////////////////////////////////////////////

	bool hasSpellbook() const;
	EAlignment::EAlignment getAlignment() const;
	const std::string &getBiography() const;
	bool needsLastStack()const override;
	ui32 getTileCost(const TerrainTile &dest, const TerrainTile &from, const TurnInfo * ti) const; //move cost - applying pathfinding skill, road and terrain modifiers. NOT includes diagonal move penalty, last move levelling
	int getNativeTerrain() const;
	ui32 getLowestCreatureSpeed() const;
	int3 getPosition(bool h3m = false) const; //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
	si32 manaRegain() const; //how many points of mana can hero regain "naturally" in one day
	si32 getManaNewTurn() const; //calculate how much mana this hero is going to have the next day
	int getCurrentLuck(int stack=-1, bool town=false) const;
	int getSpellCost(const CSpell *sp) const; //do not use during battles -> bonuses from army would be ignored

	// ----- primary and secondary skill, experience, level handling -----

	/// Returns true if hero has lower level than should upon his experience.
	bool gainsLevel() const;

	/// Returns the next primary skill on level up. Can only be called if hero can gain a level up.
	PrimarySkill::PrimarySkill nextPrimarySkill() const;

	/// Returns the next secondary skill randomly on level up. Can only be called if hero can gain a level up.
	boost::optional<SecondarySkill> nextSecondarySkill() const;

	/// Gets 0, 1 or 2 secondary skills which are proposed on hero level up.
	std::vector<SecondarySkill> getLevelUpProposedSecondarySkills() const;

	ui8 getSecSkillLevel(SecondarySkill skill) const; //0 - no skill

	/// Returns true if hero has free secondary skill slot.
	bool canLearnSkill() const;

	void setPrimarySkill(PrimarySkill::PrimarySkill primarySkill, si64 value, ui8 abs);
	void setSecSkillLevel(SecondarySkill which, int val, bool abs);// abs == 0 - changes by value; 1 - sets to value
	void levelUp(std::vector<SecondarySkill> skills);

	int maxMovePoints(bool onLand, const TurnInfo * ti = nullptr) const;
	int movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark = false, const TurnInfo * ti = nullptr) const;

	static int3 convertPosition(int3 src, bool toh3m); //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
	double getFightingStrength() const; // takes attack / defense skill into account
	double getMagicStrength() const; // takes knowledge / spell power skill into account
	double getHeroStrength() const; // includes fighting and magic strength
	ui64 getTotalStrength() const; // includes fighting strength and army strength
	TExpType calculateXp(TExpType exp) const; //apply learning skill

	bool canCastThisSpell(const CSpell * spell) const; //determines if this hero can cast given spell; takes into account existing spell in spellbook, existing spellbook and artifact bonuses
	CStackBasicDescriptor calculateNecromancy (const BattleResult &battleResult) const;
	void showNecromancyDialog(const CStackBasicDescriptor &raisedStack) const;
	EDiggingStatus diggingStatus() const;

	//////////////////////////////////////////////////////////////////////////

	void setType(si32 ID, si32 subID) override;

	void initHero();
	void initHero(HeroTypeID SUBID);

	void putArtifact(ArtifactPosition pos, CArtifactInstance *art);
	void putInBackpack(CArtifactInstance *art);
	void initExp();
	void initArmy(IArmyDescriptor *dst = nullptr);
	//void giveArtifact (ui32 aid);
	void pushPrimSkill(PrimarySkill::PrimarySkill which, int val);
	ui8 maxlevelsToMagicSchool() const;
	ui8 maxlevelsToWisdom() const;
	void Updatespecialty();
	void recreateSecondarySkillsBonuses();
	void updateSkill(SecondarySkill which, int val);

	bool hasVisions(const CGObjectInstance * target, const int subtype) const;
	/// If this hero perishes, the scenario is failed
	bool isMissionCritical() const;

	CGHeroInstance();
	virtual ~CGHeroInstance();

	///ArtBearer
	ArtBearer::ArtBearer bearerType() const override;

	///IBonusBearer
	CBonusSystemNode *whereShouldBeAttached(CGameState *gs) override;
	std::string nodeName() const override;

	///ISpellCaster
	ui8 getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool = nullptr) const override;
	ui32 getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const override;

	///default spell school level for effect calculation
	int getEffectLevel(const CSpell * spell) const override;

	///default spell-power for damage/heal calculation
	int getEffectPower(const CSpell * spell) const override;

	///default spell-power for timed effects duration
	int getEnchantPower(const CSpell * spell) const override;

	///damage/heal override(ignores spell configuration, effect level and effect power)
	int getEffectValue(const CSpell * spell) const override;

	const PlayerColor getOwner() const override;

	void deserializationFix();

	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	std::string getObjectName() const override;
protected:
	void setPropertyDer(ui8 what, ui32 val) override;//synchr

private:
	void levelUpAutomatically();

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<CArtifactSet&>(*this);
		h & exp & level & name & biography & portrait & mana & secSkills & movement
			& sex & inTownGarrison & spells & patrol & moveDir & skillsInfo;
		h & visitedTown & boat;
		h & type & specialty & commander & visitedObjects;
		BONUS_TREE_DESERIALIZATION_FIX
		//visitied town pointer will be restored by map serialization method
	}
};

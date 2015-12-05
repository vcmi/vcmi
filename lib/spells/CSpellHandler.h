/*
 * CSpellHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Magic.h"
#include "../IHandlerBase.h"
#include "../ConstTransitivePtr.h"
#include "../int3.h"
#include "../GameConstants.h"
#include "../BattleHex.h"
#include "../HeroBonus.h"

class CGObjectInstance;
class CSpell;
class ISpellMechanics;
class CLegacyConfigParser;
class CGHeroInstance;
class CStack;
class CBattleInfoCallback;
class BattleInfo;
struct CPackForClient;
struct BattleSpellCast;
class CGameInfoCallback;
class CRandomGenerator;
class CMap;
struct AdventureSpellCastParameters;
struct BattleSpellCastParameters;
class SpellCastEnvironment;

struct SpellSchoolInfo
{
	ESpellSchool id; //backlink
	Bonus::BonusType damagePremyBonus;
	Bonus::BonusType immunityBonus;
	std::string jsonName;
	SecondarySkill::ESecondarySkill skill;
	Bonus::BonusType knoledgeBonus;
};


enum class VerticalPosition : ui8{TOP, CENTER, BOTTOM};

class DLL_LINKAGE CSpell
{
public:
	struct ProjectileInfo
	{
		///in radians. Only positive value. Negative angle is handled by vertical flip
		double minimumAngle;

		///resource name
		std::string resourceName;

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & minimumAngle & resourceName;
		}
	};

	struct AnimationItem
	{
		std::string resourceName;
		VerticalPosition verticalPosition;
		int pause; 

		AnimationItem();

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & resourceName & verticalPosition;
			if(version >= 754) //save format backward compatibility
			{
				h & pause;
			}			
			else if(!h.saving)
			{
				pause = 0;
			}
		}
	};

	typedef AnimationItem TAnimation;
	typedef std::vector<TAnimation> TAnimationQueue;

	struct DLL_LINKAGE AnimationInfo
	{
		AnimationInfo();
		~AnimationInfo();

		///displayed on all affected targets.
		TAnimationQueue affect;

		///displayed on caster.
		TAnimationQueue cast;

		///displayed on target hex. If spell was cast with no target selection displayed on entire battlefield (f.e. ARMAGEDDON)
		TAnimationQueue hit;

		///displayed "between" caster and (first) target. Ignored if spell was cast with no target selection.
		///use selectProjectile to access
		std::vector<ProjectileInfo> projectile;

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & projectile & hit & cast;
		}

		std::string selectProjectile(const double angle) const;
	} animationInfo;
public:
	struct LevelInfo
	{
		std::string description; //descriptions of spell for skill level
		si32 cost;
		si32 power;
		si32 AIValue;

		bool smartTarget;
		bool clearTarget;
		bool clearAffected;
		std::string range;

		std::vector<Bonus> effects;
		
		std::vector<Bonus *> effectsTmp; //TODO: this should replace effects 

		LevelInfo();
		~LevelInfo();

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & description & cost & power & AIValue & smartTarget & range & effects;
			h & clearTarget & clearAffected;
		}
	};

	/** \brief Low level accessor. Don`t use it if absolutely necessary
	 *
	 * \param level. spell school level
	 * \return Spell level info structure
	 *
	 */
	const CSpell::LevelInfo& getLevelInfo(const int level) const;
public:
	enum ETargetType {NO_TARGET, CREATURE, OBSTACLE, LOCATION};
	enum ESpellPositiveness {NEGATIVE = -1, NEUTRAL = 0, POSITIVE = 1};

	struct DLL_LINKAGE TargetInfo
	{
		ETargetType type;
		bool smart;
		bool massive;
		bool onlyAlive;
		///no immunity on primary target (mostly spell-like attack)
		bool alwaysHitDirectly;

		bool clearTarget;
		bool clearAffected;

		TargetInfo(const CSpell * spell, const int level);
		TargetInfo(const CSpell * spell, const int level, ECastingMode::ECastingMode mode);

	private:
		void init(const CSpell * spell, const int level);
	};

	SpellID id;
	std::string identifier; //???
	std::string name;

	si32 level;

	std::map<ESpellSchool, bool> school; //todo: use this instead of separate boolean fields

	si32 power; //spell's power

	std::map<TFaction, si32> probabilities; //% chance to gain for castles

	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative

	std::vector<SpellID> counteredSpells; //spells that are removed when effect of this spell is placed on creature (for bless-curse, haste-slow, and similar pairs)

	CSpell();
	~CSpell();

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes = nullptr ) const; //convert range to specific hexes; last optional out parameter is set to true, if spell would cover unavailable hexes (that are not included in ret)
	ETargetType getTargetType() const; //deprecated

	bool isCombatSpell() const;
	bool isAdventureSpell() const;
	bool isCreatureAbility() const;

	bool isPositive() const;
	bool isNegative() const;
	bool isNeutral() const;

	bool isDamageSpell() const;
	bool isHealingSpell() const;
	bool isRisingSpell() const;
	bool isOffensiveSpell() const;

	bool isSpecialSpell() const;

	bool hasEffects() const;
	void getEffects(std::vector<Bonus> &lst, const int level) const;


	///calculate spell damage on stack taking caster`s secondary skills and affectedCreature`s bonuses into account
	ui32 calculateDamage(const ISpellCaster * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower) const;

	///selects from allStacks actually affected stacks
	std::set<const CStack *> getAffectedStacks(const CBattleInfoCallback * cb, ECastingMode::ECastingMode mode, PlayerColor casterColor, int spellLvl, BattleHex destination, const ISpellCaster * caster) const;

	si32 getCost(const int skillLevel) const;

	/**
	 * Returns spell level power, base power ignored
	 */
	si32 getPower(const int skillLevel) const;

	si32 getProbability(const TFaction factionId) const;

	/**
	 * Calls cb for each school this spell belongs to
	 *
	 * Set stop to true to abort looping
	 */
	void forEachSchool(const std::function<void (const SpellSchoolInfo &, bool &)> & cb) const;

	/**
	 * Returns resource name of icon for SPELL_IMMUNITY bonus
	 */
	const std::string& getIconImmune() const;

	const std::string& getCastSound() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & id & name & level & power
		  & probabilities  & attributes & combatSpell & creatureAbility & positiveness & counteredSpells;
		h & isRising & isDamage & isOffensive;
		h & targetType;
		h & immunities & limiters & absoluteImmunities & absoluteLimiters;
		h & iconImmune;
		h & defaultProbability;
		h & isSpecial;
		h & castSound & iconBook & iconEffect & iconScenarioBonus & iconScroll;
		h & levels;
		h & school;
		h & animationInfo;

		if(!h.saving)
			setup();
	}
	friend class CSpellHandler;
	friend class Graphics;
public:
	///internal interface (for callbacks)
	
	///Checks general but spell-specific problems for all casting modes. Use only during battle.
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, PlayerColor player) const;

	///checks for creature immunity / anything that prevent casting *at given hex* - doesn't take into account general problems such as not having spellbook or mana points etc.
	ESpellCastProblem::ESpellCastProblem isImmuneAt(const CBattleInfoCallback * cb, const ISpellCaster * caster, ECastingMode::ECastingMode mode, BattleHex destination) const;
	
	///checks for creature immunity / anything that prevent casting *at given target* - doesn't take into account general problems such as not having spellbook or mana points etc.
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const;
public:
	///Server logic. Has write access to GameState via packets.
	///May be executed on client side by (future) non-cheat-proof scripts.

	bool adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const;
	void battleCast(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const;

public:
	///Client-server logic. Has direct write access to GameState.
	///Shall be called (only) when applying packets on BOTH SIDES

	///implementation of BattleSpellCast applying
	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const;

public://Client logic.
	void prepareBattleLog(const CBattleInfoCallback * cb, const BattleSpellCast * packet, std::vector<std::string> & logLines) const;

public://internal, for use only by Mechanics classes
	///applies caster`s secondary skills and affectedCreature`s to raw damage
	int adjustRawDamage(const ISpellCaster * caster, const CStack * affectedCreature, int rawDamage) const;
	///returns raw damage or healed HP
	int calculateRawEffectValue(int effectLevel, int effectPower) const;		
	///generic immunity calculation
	ESpellCastProblem::ESpellCastProblem internalIsImmune(const ISpellCaster * caster, const CStack *obj) const;

private:
	void setIsOffensive(const bool val);
	void setIsRising(const bool val);

	//call this after load or deserialization. cant be done in constructor.
	void setup();
	void setupMechanics();
private:
	si32 defaultProbability;

	bool isRising;
	bool isDamage;
	bool isOffensive;
	bool isSpecial;

	std::string attributes; //reference only attributes //todo: remove or include in configuration format, currently unused

	ETargetType targetType;

	std::vector<Bonus::BonusType> immunities; //any of these grants immunity
	std::vector<Bonus::BonusType> absoluteImmunities; //any of these grants immunity, can't be negated
	std::vector<Bonus::BonusType> limiters; //all of them are required to be affected
	std::vector<Bonus::BonusType> absoluteLimiters; //all of them are required to be affected, can't be negated

	///graphics related stuff
	std::string iconImmune;
	std::string iconBook;
	std::string iconEffect;
	std::string iconScenarioBonus;
	std::string iconScroll;

	///sound related stuff
	std::string castSound;

	std::vector<LevelInfo> levels;

	ISpellMechanics * mechanics;//(!) do not serialize
};

bool DLL_LINKAGE isInScreenRange(const int3 &center, const int3 &pos); //for spells like Dimension Door

class DLL_LINKAGE CSpellHandler: public CHandlerBase<SpellID, CSpell>
{
public:
	CSpellHandler();
	virtual ~CSpellHandler();

	///IHandler base
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	/**
	 * Gets a list of default allowed spells. OH3 spells are all allowed by default.
	 *
	 * @return a list of allowed spells, the index is the spell id and the value either 0 for not allowed or 1 for allowed
	 */
	std::vector<bool> getDefaultAllowed() const override;

	const std::string getTypeName() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects ;
	}

protected:
	CSpell * loadFromJson(const JsonNode & json) override;
};

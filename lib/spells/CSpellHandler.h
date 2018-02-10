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
#include "../JsonNode.h"
#include "../IHandlerBase.h"
#include "../ConstTransitivePtr.h"
#include "../int3.h"
#include "../GameConstants.h"
#include "../battle/BattleHex.h"
#include "../HeroBonus.h"

namespace spells
{
	class ISpellMechanicsFactory;
	class IBattleCast;
}

class CGObjectInstance;
class CSpell;
class IAdventureSpellMechanics;
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
class AdventureSpellCastParameters;
class SpellCastEnvironment;


namespace spells
{

struct SchoolInfo
{
	ESpellSchool id; //backlink
	Bonus::BonusType damagePremyBonus;
	Bonus::BonusType immunityBonus;
	std::string jsonName;
	SecondarySkill::ESecondarySkill skill;
	Bonus::BonusType knoledgeBonus;
};

}


enum class VerticalPosition : ui8{TOP, CENTER, BOTTOM};

class DLL_LINKAGE CSpell : public spells::Spell
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
			h & minimumAngle;
			h & resourceName;
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
			h & resourceName;
			h & verticalPosition;
			if(version >= 754)
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
			h & projectile;
			h & hit;
			h & cast;
			if(version >= 762)
			{
				h & affect;
			}
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

		//TODO: remove these two when AI will understand special effects
		std::vector<std::shared_ptr<Bonus>> effects; //deprecated
		std::vector<std::shared_ptr<Bonus>> cumulativeEffects; //deprecated

		JsonNode battleEffects;

		LevelInfo();
		~LevelInfo();

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & description;
			h & cost;
			h & power;
			h & AIValue;
			h & smartTarget;
			h & range;

			if(version >= 773)
			{
				h & effects;
				h & cumulativeEffects;
			}
			else
			{
				//all old effects treated as not cumulative, special cases handled by CSpell::serialize
				std::vector<Bonus> old;
				h & old;

				if(!h.saving)
				{
					effects.clear();
					cumulativeEffects.clear();
					for(const Bonus & oldBonus : old)
						effects.push_back(std::make_shared<Bonus>(oldBonus));
				}
			}

			h & clearTarget;
			h & clearAffected;

			if(version >= 780)
				h & battleEffects;
		}
	};

	/** \brief Low level accessor. Don`t use it if absolutely necessary
	 *
	 * \param level. spell school level
	 * \return Spell level info structure
	 *
	 */
	const CSpell::LevelInfo & getLevelInfo(const int level) const;
public:
	enum ESpellPositiveness
	{
		NEGATIVE = -1,
		NEUTRAL = 0,
		POSITIVE = 1
	};

	struct DLL_LINKAGE TargetInfo
	{
		spells::AimType type;
		bool smart;
		bool massive;
		bool clearAffected;
		bool clearTarget;

		TargetInfo(const CSpell * spell, const int level, spells::Mode mode);
	};

	using BTVector = std::vector<Bonus::BonusType>;

	SpellID id;
	std::string identifier;
	std::string name;

	si32 level;

	std::map<ESpellSchool, bool> school;

	si32 power; //spell's power

	std::map<TFaction, si32> probabilities; //% chance to gain for castles

	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative

	std::vector<SpellID> counteredSpells; //spells that are removed when effect of this spell is placed on creature (for bless-curse, haste-slow, and similar pairs)

	JsonNode targetCondition; //custom condition on what spell can affect

	CSpell();
	~CSpell();

	std::vector<BattleHex> rangeInHexes(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster, BattleHex centralHex) const;

	spells::AimType getTargetType() const;

	bool isCombatSpell() const;
	bool isAdventureSpell() const;
	bool isCreatureAbility() const;

	bool isPositive() const;
	bool isNegative() const;
	bool isNeutral() const;

	boost::logic::tribool getPositiveness() const;

	bool isDamageSpell() const;
	bool isRisingSpell() const;
	bool isOffensiveSpell() const;

	bool isSpecialSpell() const;

	bool hasEffects() const;
	void getEffects(std::vector<Bonus> & lst, const int level, const bool cumulative, const si32 duration, boost::optional<si32 *> maxDuration = boost::none) const;

	bool hasBattleEffects() const;
	///calculate spell damage on stack taking caster`s secondary skills and affectedCreature`s bonuses into account
	int64_t calculateDamage(const spells::Caster * caster) const;

	///selects from allStacks actually affected stacks
	std::vector<const CStack *> getAffectedStacks(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster, int spellLvl, const spells::Target & target) const;

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
	void forEachSchool(const std::function<void (const spells::SchoolInfo &, bool &)> & cb) const override;

	int32_t getIndex() const override;

	/**
	 * Returns resource name of icon for SPELL_IMMUNITY bonus
	 */
	const std::string& getIconImmune() const;

	const std::string& getCastSound() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & identifier;
		h & id;
		h & name;
		h & level;
		h & power;
		h & probabilities;
		h & attributes;
		h & combatSpell;
		h & creatureAbility;
		h & positiveness;
		h & counteredSpells;
		h & isRising;
		h & isDamage;
		h & isOffensive;
		h & targetType;

		if(version >= 780)
		{
			h & targetCondition;
		}
		else
		{
			BTVector immunities;
			BTVector absoluteImmunities;
			BTVector limiters;
			BTVector absoluteLimiters;

			h & immunities;
			h & limiters;
			h & absoluteImmunities;
			h & absoluteLimiters;

			if(!h.saving)
				targetCondition = convertTargetCondition(immunities, absoluteImmunities, limiters, absoluteLimiters);
		}

		h & iconImmune;
		h & defaultProbability;
		h & isSpecial;
		h & castSound;
		h & iconBook;
		h & iconEffect;
		h & iconScenarioBonus;
		h & iconScroll;
		h & levels;
		h & school;
		h & animationInfo;

		//backward compatibility
		//can not be added to level structure as level structure does not know spell id
		if(!h.saving && version < 773)
		{
			if(id == SpellID::DISRUPTING_RAY || id == SpellID::ACID_BREATH_DEFENSE)
				for(auto & level : levels)
					std::swap(level.effects, level.cumulativeEffects);
		}

	}
	friend class CSpellHandler;
	friend class Graphics;
public:
	///internal interface (for callbacks)

	///Checks general but spell-specific problems. Use only during battle.
	bool canBeCast(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const;
	bool canBeCast(spells::Problem & problem, const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const;

	///checks for creature immunity / anything that prevent casting *at given hex*
	bool canBeCastAt(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster, BattleHex destination) const; //DEPREACTED
	bool canBeCastAt(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster, const spells::Target & target) const;
public:
	///Server logic. Has write access to GameState via packets.
	///May be executed on client side by (future) non-cheat-proof scripts.

	bool adventureCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;

public://internal, for use only by Mechanics classes
	///applies caster`s secondary skills and affectedCreature`s to raw damage
	int64_t adjustRawDamage(const spells::Caster * caster, const battle::Unit * affectedCreature, int64_t rawDamage) const;

	///returns raw damage or healed HP
	int64_t calculateRawEffectValue(int32_t effectLevel, int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const;

	std::unique_ptr<spells::Mechanics> battleMechanics(const spells::IBattleCast * event) const;
private:
	void setIsOffensive(const bool val);
	void setIsRising(const bool val);

	JsonNode convertTargetCondition(const BTVector & immunity, const BTVector & absImmunity, const BTVector & limit, const BTVector & absLimit) const;

	//call this after load or deserialization. cant be done in constructor.
	void setupMechanics();
private:
	si32 defaultProbability;

	bool isRising;
	bool isDamage;
	bool isOffensive;
	bool isSpecial;

	std::string attributes; //reference only attributes //todo: remove or include in configuration format, currently unused

	spells::AimType targetType;

	///graphics related stuff
	std::string iconImmune;
	std::string iconBook;
	std::string iconEffect;
	std::string iconScenarioBonus;
	std::string iconScroll;

	///sound related stuff
	std::string castSound;

	std::vector<LevelInfo> levels;

	std::unique_ptr<spells::ISpellMechanicsFactory> mechanics;//(!) do not serialize
	std::unique_ptr<IAdventureSpellMechanics> adventureMechanics;//(!) do not serialize
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
	 */
	std::vector<bool> getDefaultAllowed() const override;

	const std::string getTypeName() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
		if(!h.saving && version < 780)
		{
			update780();
		}

		if(!h.saving)
		{
			afterLoadFinalization();
		}
	}

protected:
	CSpell * loadFromJson(const JsonNode & json, const std::string & identifier, size_t index) override;
private:
	void update780();
};

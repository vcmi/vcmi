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

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>
#include <vcmi/spells/Magic.h>
#include "../JsonNode.h"
#include "../IHandlerBase.h"
#include "../ConstTransitivePtr.h"
#include "../int3.h"
#include "../GameConstants.h"
#include "../battle/BattleHex.h"
#include "../HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class CSpell;
class IAdventureSpellMechanics;
class CBattleInfoCallback;
class AdventureSpellCastParameters;
class SpellCastEnvironment;
class JsonSerializeFormat;

namespace test
{
	class CSpellTest;
}

namespace spells
{

class ISpellMechanicsFactory;
class IBattleCast;

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
		std::string effectName;
		VerticalPosition verticalPosition;
		int pause;

		AnimationItem();

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & resourceName;
			if (version > 806)
				h & effectName;
			h & verticalPosition;
			h & pause;
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
			h & affect;
		}

		std::string selectProjectile(const double angle) const;
	} animationInfo;
public:
	struct LevelInfo
	{
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
			h & cost;
			h & power;
			h & AIValue;
			h & smartTarget;
			h & range;
			h & effects;
			h & cumulativeEffects;
			h & clearTarget;
			h & clearAffected;
			h & battleEffects;
		}
	};

	/** \brief Low level accessor. Don`t use it if absolutely necessary
	 *
	 * \param level. spell school level
	 * \return Spell level info structure
	 *
	 */
	const CSpell::LevelInfo & getLevelInfo(const int32_t level) const;

	SpellID id;
	std::string identifier;
	std::string modScope;
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

		TargetInfo(const CSpell * spell, const int32_t level, spells::Mode mode);
	};

	using BTVector = std::vector<Bonus::BonusType>;

	si32 level;

	std::map<ESpellSchool, bool> school;

	si32 power; //spell's power

	std::map<TFaction, si32> probabilities; //% chance to gain for castles

	bool combat; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative

	std::vector<SpellID> counteredSpells; //spells that are removed when effect of this spell is placed on creature (for bless-curse, haste-slow, and similar pairs)

	JsonNode targetCondition; //custom condition on what spell can affect

	CSpell();
	~CSpell();

	int64_t calculateDamage(const spells::Caster * caster) const override;

	/**
	 * Calls cb for each school this spell belongs to
	 *
	 * Set stop to true to abort looping
	 */
	void forEachSchool(const std::function<void(const spells::SchoolInfo &, bool &)> & cb) const override;

	spells::AimType getTargetType() const;

	bool hasEffects() const;
	void getEffects(std::vector<Bonus> & lst, const int level, const bool cumulative, const si32 duration, boost::optional<si32 *> maxDuration = boost::none) const;

	bool hasBattleEffects() const;

	int32_t getCost(const int32_t skillLevel) const override;

	si32 getProbability(const TFaction factionId) const;

	int32_t getBasePower() const override;
	int32_t getLevelPower(const int32_t skillLevel) const override;

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	SpellID getId() const override;

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	std::string getDescriptionTextID(int32_t level) const override;
	std::string getDescriptionTranslated(int32_t level) const override;

	int32_t getLevel() const override;

	boost::logic::tribool getPositiveness() const override;

	bool isPositive() const override;
	bool isNegative() const override;
	bool isNeutral() const override;

	bool isDamage() const override;
	bool isOffensive() const override;

	bool isSpecial() const override;

	bool isAdventure() const override;
	bool isCombat() const override;
	bool isCreatureAbility() const override;

	void registerIcons(const IconRegistar & cb) const override;

	const std::string & getIconImmune() const; ///< Returns resource name of icon for SPELL_IMMUNITY bonus
	const std::string & getIconBook() const;
	const std::string & getIconEffect() const;
	const std::string & getIconScenarioBonus() const;
	const std::string & getIconScroll() const;

	const std::string & getCastSound() const override;

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & identifier;
		h & id;
		h & level;
		h & power;
		h & probabilities;
		h & attributes;
		h & combat;
		h & creatureAbility;
		h & positiveness;
		h & counteredSpells;
		h & rising;
		h & damage;
		h & offensive;
		h & targetType;
		h & targetCondition;
		h & iconImmune;
		h & defaultProbability;
		h & special;
		h & castSound;
		h & iconBook;
		h & iconEffect;
		h & iconScenarioBonus;
		h & iconScroll;
		h & levels;
		h & school;
		h & animationInfo;
	}
	friend class CSpellHandler;
	friend class Graphics;
	friend class test::CSpellTest;
public:
	///internal interface (for callbacks)

	///Checks general but spell-specific problems. Use only during battle.
	bool canBeCast(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const;
	bool canBeCast(spells::Problem & problem, const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const;

public:
	///Server logic. Has write access to GameState via packets.
	///May be executed on client side by (future) non-cheat-proof scripts.

	bool adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;

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

	bool rising;
	bool damage;
	bool offensive;
	bool special;

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

class DLL_LINKAGE CSpellHandler: public CHandlerBase<SpellID, spells::Spell, CSpell, spells::Service>
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

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
		if(!h.saving)
		{
			afterLoadFinalization();
		}
	}

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CSpell * loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END

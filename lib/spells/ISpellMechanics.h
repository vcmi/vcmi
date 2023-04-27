/*
 * ISpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Magic.h>
#include <vcmi/ServerCallback.h>

#include "../battle/Destination.h"
#include "../int3.h"
#include "../GameConstants.h"
#include "../bonuses/HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Query;
class IBattleState;
class CRandomGenerator;
class CMap;
class CGameInfoCallback;
class CBattleInfoCallback;
class IGameInfoCallback;
class JsonNode;
class CStack;
class CGObjectInstance;
class CGHeroInstance;

namespace vstd
{
	class RNG;
}

#if SCRIPTING_ENABLED
namespace scripting
{
	class Service;
}
#endif


///callback to be provided by server
class DLL_LINKAGE SpellCastEnvironment : public ServerCallback
{
public:
	virtual ~SpellCastEnvironment() = default;

	virtual const CMap * getMap() const = 0;
	virtual const CGameInfoCallback * getCb() const = 0;

	virtual bool moveHero(ObjectInstanceID hid, int3 dst, bool teleporting) = 0;	//TODO: remove

	virtual void genericQuery(Query * request, PlayerColor color, std::function<void(const JsonNode &)> callback) = 0;//TODO: type safety on query, use generic query packet when implemented
};

namespace spells
{

class DLL_LINKAGE IBattleCast
{
public:
	using Value = int32_t;
	using Value64 = int64_t;

	using OptionalValue = std::optional<Value>;
	using OptionalValue64 = std::optional<Value64>;

	virtual const CSpell * getSpell() const = 0;
	virtual Mode getMode() const = 0;
	virtual const Caster * getCaster() const = 0;
	virtual const CBattleInfoCallback * getBattle() const = 0;
	virtual const IGameInfoCallback * getGame() const = 0;

	virtual OptionalValue getSpellLevel() const = 0;

	virtual OptionalValue getEffectPower() const = 0;
	virtual OptionalValue getEffectDuration() const = 0;

	virtual OptionalValue64 getEffectValue() const = 0;

	virtual boost::logic::tribool isSmart() const = 0;
	virtual boost::logic::tribool isMassive() const = 0;
};

///all parameters of particular cast event
class DLL_LINKAGE BattleCast : public IBattleCast
{
public:
	boost::logic::tribool smart;
	boost::logic::tribool massive;

	//normal constructor
	BattleCast(const CBattleInfoCallback * cb_, const Caster * caster_, const Mode mode_, const CSpell * spell_);

	//magic mirror constructor
	BattleCast(const BattleCast & orig, const Caster * caster_);

	virtual ~BattleCast();

	///IBattleCast
	const CSpell * getSpell() const override;
	Mode getMode() const override;
	const Caster * getCaster() const override;
	const CBattleInfoCallback * getBattle() const override;
	const IGameInfoCallback * getGame() const override;

	OptionalValue getSpellLevel() const override;

	OptionalValue getEffectPower() const override;
	OptionalValue getEffectDuration() const override;

	OptionalValue64 getEffectValue() const override;

	boost::logic::tribool isSmart() const override;
	boost::logic::tribool isMassive() const override;

	void setSpellLevel(Value value);

	void setEffectPower(Value value);
	void setEffectDuration(Value value);

	void setEffectValue(Value64 value);

	///only apply effects to specified targets
	void applyEffects(ServerCallback * server, const Target & target, bool indirect = false, bool ignoreImmunity = false) const;

	///normal cast
	void cast(ServerCallback * server, Target target);

	///cast evaluation
	void castEval(ServerCallback * server, Target target);

	///cast with silent check for permitted cast
	bool castIfPossible(ServerCallback * server, Target target);

	std::vector<Target> findPotentialTargets() const;

private:
	///spell school level
	OptionalValue magicSkillLevel;

	///actual spell-power affecting effect values
	OptionalValue effectPower;
	///actual spell-power affecting effect duration
	OptionalValue effectDuration;

	///for Archangel-like casting
	OptionalValue64 effectValue;

	Mode mode;
	const CSpell * spell;
	const CBattleInfoCallback * cb;
	const IGameInfoCallback * gameCb;
	const Caster * caster;
};

class DLL_LINKAGE ISpellMechanicsFactory
{
public:
	virtual ~ISpellMechanicsFactory();

	virtual std::unique_ptr<Mechanics> create(const IBattleCast * event) const = 0;

	static std::unique_ptr<ISpellMechanicsFactory> get(const CSpell * s);

protected:
	const CSpell * spell;

	ISpellMechanicsFactory(const CSpell * s);
};

class DLL_LINKAGE Mechanics
{
public:
	virtual ~Mechanics();

	virtual bool adaptProblem(ESpellCastProblem::ESpellCastProblem source, Problem & target) const = 0;
	virtual bool adaptGenericProblem(Problem & target) const = 0;

	virtual std::vector<BattleHex> rangeInHexes(BattleHex centralHex) const = 0;
	virtual std::vector<const CStack *> getAffectedStacks(const Target & target) const = 0;

	virtual bool canBeCast(Problem & problem) const = 0;
	virtual bool canBeCastAt(const Target & target, Problem & problem) const = 0;

	virtual void applyEffects(ServerCallback * server, const Target & targets, bool indirect, bool ignoreImmunity) const = 0;

	virtual void cast(ServerCallback * server, const Target & target) = 0;

	virtual void castEval(ServerCallback * server, const Target & target) = 0;

	virtual bool isReceptive(const battle::Unit * target) const = 0;

	virtual std::vector<AimType> getTargetTypes() const = 0;

	virtual std::vector<Destination> getPossibleDestinations(size_t index, AimType aimType, const Target & current) const = 0;

	virtual const Spell * getSpell() const = 0;

	//Cast event facade

	virtual IBattleCast::Value getEffectLevel() const = 0;
	virtual IBattleCast::Value getRangeLevel() const = 0;

	virtual IBattleCast::Value getEffectPower() const = 0;
	virtual IBattleCast::Value getEffectDuration() const = 0;

	virtual IBattleCast::Value64 getEffectValue() const = 0;

	virtual PlayerColor getCasterColor() const = 0;

	//Spell facade
	virtual int32_t getSpellIndex() const = 0;
	virtual SpellID getSpellId() const = 0;
	virtual std::string getSpellName() const = 0;
	virtual int32_t getSpellLevel() const = 0;

	virtual bool isSmart() const = 0;
	virtual bool isMassive() const = 0;
	virtual bool alwaysHitFirstTarget() const = 0;
	virtual bool requiresClearTiles() const = 0;

	virtual bool isNegativeSpell() const = 0;
	virtual bool isPositiveSpell() const = 0;
	virtual bool isMagicalEffect() const = 0;

	virtual int64_t adjustEffectValue(const battle::Unit * target) const = 0;
	virtual int64_t applySpellBonus(int64_t value, const battle::Unit * target) const = 0;
	virtual int64_t applySpecificSpellBonus(int64_t value) const = 0;
	virtual int64_t calculateRawEffectValue(int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const = 0;

	virtual std::vector<Bonus::BonusType> getElementalImmunity() const = 0;

	//Battle facade
	virtual bool ownerMatches(const battle::Unit * unit) const = 0;
	virtual bool ownerMatches(const battle::Unit * unit, const boost::logic::tribool positivness) const = 0;

	//Global environment facade
	virtual const CreatureService * creatures() const = 0;
#if SCRIPTING_ENABLED
	virtual const scripting::Service * scripts() const = 0;
#endif
	virtual const Service * spells() const = 0;

	virtual const IGameInfoCallback * game() const = 0;
	virtual const CBattleInfoCallback * battle() const = 0;

	const Caster * caster;

	ui8 casterSide;

protected:
	Mechanics();
};

class DLL_LINKAGE BaseMechanics : public Mechanics
{
public:
	virtual ~BaseMechanics();

	bool adaptProblem(ESpellCastProblem::ESpellCastProblem source, Problem & target) const override;
	bool adaptGenericProblem(Problem & target) const override;

	int32_t getSpellIndex() const override;
	SpellID getSpellId() const override;
	std::string getSpellName() const override;
	int32_t getSpellLevel() const override;

	IBattleCast::Value getEffectLevel() const override;
	IBattleCast::Value getRangeLevel() const override;

	IBattleCast::Value getEffectPower() const override;
	IBattleCast::Value getEffectDuration() const override;

	IBattleCast::Value64 getEffectValue() const override;

	PlayerColor getCasterColor() const override;

	bool isSmart() const override;
	bool isMassive() const override;
	bool requiresClearTiles() const override;
	bool alwaysHitFirstTarget() const override;

	bool isNegativeSpell() const override;
	bool isPositiveSpell() const override;
	bool isMagicalEffect() const override;

	int64_t adjustEffectValue(const battle::Unit * target) const override;
	int64_t applySpellBonus(int64_t value, const battle::Unit * target) const override;
	int64_t applySpecificSpellBonus(int64_t value) const override;
	int64_t calculateRawEffectValue(int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const override;

	std::vector<Bonus::BonusType> getElementalImmunity() const override;

	bool ownerMatches(const battle::Unit * unit) const override;
	bool ownerMatches(const battle::Unit * unit, const boost::logic::tribool positivness) const override;

	std::vector<AimType> getTargetTypes() const override;

	const CreatureService * creatures() const override;
#if SCRIPTING_ENABLED
	const scripting::Service * scripts() const override;
#endif
	const Service * spells() const override;

	const IGameInfoCallback * game() const override;
	const CBattleInfoCallback * battle() const override;

protected:
	const CSpell * owner;
	Mode mode;

	BaseMechanics(const IBattleCast * event);

private:
    IBattleCast::Value rangeLevel;
	IBattleCast::Value effectLevel;

	///actual spell-power affecting effect values
	IBattleCast::Value effectPower;
	///actual spell-power affecting effect duration
	IBattleCast::Value effectDuration;

	///raw damage/heal amount
	IBattleCast::Value64 effectValue;

	boost::logic::tribool smart;
	boost::logic::tribool massive;

	const IGameInfoCallback * gameCb;
	const CBattleInfoCallback * cb;
};

class DLL_LINKAGE IReceptiveCheck
{
public:
	virtual ~IReceptiveCheck() = default;

	virtual bool isReceptive(const Mechanics * m, const battle::Unit * target) const = 0;
};

}// namespace spells

class DLL_LINKAGE AdventureSpellCastParameters
{
public:
	const spells::Caster * caster;
	int3 pos;
};

class DLL_LINKAGE IAdventureSpellMechanics
{
public:
	IAdventureSpellMechanics(const CSpell * s);
	virtual ~IAdventureSpellMechanics() = default;

	virtual bool adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const = 0;

	static std::unique_ptr<IAdventureSpellMechanics> createMechanics(const CSpell * s);
protected:
	const CSpell * owner;
};

VCMI_LIB_NAMESPACE_END

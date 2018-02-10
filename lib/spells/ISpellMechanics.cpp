/*
 * ISpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ISpellMechanics.h"

#include "../CRandomGenerator.h"

#include "../HeroBonus.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/IBattleState.h"
#include "../battle/Unit.h"

#include "../mapObjects/CGHeroInstance.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

#include "TargetCondition.h"
#include "Problem.h"

#include "AdventureSpellMechanics.h"
#include "BattleSpellMechanics.h"

#include "effects/Effects.h"
#include "effects/Damage.h"
#include "effects/Timed.h"

#include "CSpellHandler.h"

#include "../CHeroHandler.h"//todo: remove

namespace spells
{

static std::shared_ptr<TargetCondition> makeCondition(const CSpell * s)
{
	std::shared_ptr<TargetCondition> res = std::make_shared<TargetCondition>();

	JsonDeserializer deser(nullptr, s->targetCondition);
	res->serializeJson(deser, TargetConditionItemFactory::getDefault());

	return res;
}

class CustomMechanicsFactory : public ISpellMechanicsFactory
{
public:
	std::unique_ptr<Mechanics> create(const IBattleCast * event) const override
	{
		BattleSpellMechanics * ret = new BattleSpellMechanics(event, effects, targetCondition);
		return std::unique_ptr<Mechanics>(ret);
	}
protected:
	std::shared_ptr<effects::Effects> effects;

	CustomMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s), effects(new effects::Effects)
	{
		targetCondition = makeCondition(s);
	}

	void loadEffects(const JsonNode & config, const int level)
	{
		JsonDeserializer deser(nullptr, config);
		effects->serializeJson(deser, level);
	}
private:
	std::shared_ptr<TargetCondition> targetCondition;
};

class ConfigurableMechanicsFactory : public CustomMechanicsFactory
{
public:
	ConfigurableMechanicsFactory(const CSpell * s)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
			loadEffects(s->getLevelInfo(level).battleEffects, level);
	}
};


//to be used for spells configured with old format
class FallbackMechanicsFactory : public CustomMechanicsFactory
{
public:
	FallbackMechanicsFactory(const CSpell * s)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
		{
			const CSpell::LevelInfo & levelInfo = s->getLevelInfo(level);
			assert(levelInfo.battleEffects.isNull());

			if(s->isOffensiveSpell())
			{
				//default constructed object should be enough
				effects->add("directDamage", std::make_shared<effects::Damage>(), level);
			}

			std::shared_ptr<effects::Effect> effect;

			if(!levelInfo.effects.empty())
			{
				auto timed = new effects::Timed();
				timed->cumulative = false;
				timed->bonus = levelInfo.effects;
				effect.reset(timed);
			}

			if(!levelInfo.cumulativeEffects.empty())
			{
				auto timed = new effects::Timed();
				timed->cumulative = true;
				timed->bonus = levelInfo.cumulativeEffects;
				effect.reset(timed);
			}

			if(effect)
				effects->add("timed", effect, level);
		}
	}
};

BattleStateProxy::BattleStateProxy(const PacketSender * server_)
	: server(server_),
	battleState(nullptr),
	describe(true)
{
}

BattleStateProxy::BattleStateProxy(IBattleState * battleState_)
	: server(nullptr),
	battleState(battleState_),
	describe(false)
{
}

void BattleStateProxy::complain(const std::string & problem) const
{
	if(server)
		server->complain(problem);
	else
		logGlobal->error(problem);
}


BattleCast::BattleCast(const CBattleInfoCallback * cb, const Caster * caster_, const Mode mode_, const CSpell * spell_)
	: spell(spell_),
	cb(cb),
	caster(caster_),
	mode(mode_),
	spellLvl(),
	effectLevel(),
	effectPower(),
	effectDuration(),
	effectValue(),
	smart(boost::logic::indeterminate),
	massive(boost::logic::indeterminate)
{

}

BattleCast::BattleCast(const BattleCast & orig, const Caster * caster_)
	: spell(orig.spell),
	cb(orig.cb),
	caster(caster_),
	mode(Mode::MAGIC_MIRROR),
	spellLvl(orig.spellLvl),
	effectLevel(orig.effectLevel),
	effectPower(orig.effectPower),
	effectDuration(orig.effectDuration),
	effectValue(orig.effectValue),
	smart(true),
	massive(false)
{
}

BattleCast::~BattleCast() = default;

const CSpell * BattleCast::getSpell() const
{
	return spell;
}

Mode BattleCast::getMode() const
{
	return mode;
}

const Caster * BattleCast::getCaster() const
{
	return caster;
}

const CBattleInfoCallback * BattleCast::getBattle() const
{
	return cb;
}

BattleCast::OptionalValue BattleCast::getEffectLevel() const
{
	if(effectLevel)
		return effectLevel;
	else
		return spellLvl;
}

BattleCast::OptionalValue BattleCast::getRangeLevel() const
{
	if(rangeLevel)
		return rangeLevel;
	else
		return spellLvl;
}

BattleCast::OptionalValue BattleCast::getEffectPower() const
{
	return effectPower;
}

BattleCast::OptionalValue BattleCast::getEffectDuration() const
{
	return effectDuration;
}

BattleCast::OptionalValue64 BattleCast::getEffectValue() const
{
	return effectValue;
}

boost::logic::tribool BattleCast::isSmart() const
{
	return smart;
}

boost::logic::tribool BattleCast::isMassive() const
{
	return massive;
}

void BattleCast::setSpellLevel(BattleCast::Value value)
{
	spellLvl = boost::make_optional(value);
}

void BattleCast::setEffectLevel(BattleCast::Value value)
{
	effectLevel = boost::make_optional(value);
}

void BattleCast::setRangeLevel(BattleCast::Value value)
{
	rangeLevel = boost::make_optional(value);
}

void BattleCast::setEffectPower(BattleCast::Value value)
{
	effectPower = boost::make_optional(value);
}

void BattleCast::setEffectDuration(BattleCast::Value value)
{
	effectDuration = boost::make_optional(value);
}

void BattleCast::setEffectValue(BattleCast::Value64 value)
{
	effectValue = boost::make_optional(value);
}

void BattleCast::aimToHex(const BattleHex & destination)
{
	target.push_back(Destination(destination));
}

void BattleCast::aimToUnit(const battle::Unit * destination)
{
	if(nullptr == destination)
		logGlobal->error("BattleCast::aimToUnit: invalid unit.");
	else
		target.push_back(Destination(destination));
}

void BattleCast::applyEffects(const SpellCastEnvironment * env, bool indirect, bool ignoreImmunity) const
{
	auto m = spell->battleMechanics(this);

	BattleStateProxy proxy(env);

	m->applyEffects(&proxy, env->getRandomGenerator(), target, indirect, ignoreImmunity);
}

void BattleCast::cast(const SpellCastEnvironment * env)
{
	if(target.empty())
		aimToHex(BattleHex::INVALID);
	auto m = spell->battleMechanics(this);

	const battle::Unit * mainTarget = nullptr;

	if(target.front().unitValue)
	{
		mainTarget = target.front().unitValue;
	}
	else if(target.front().hexValue.isValid())
	{
		mainTarget = cb->battleGetUnitByPos(target.front().hexValue, true);
	}

	bool tryMagicMirror = (mainTarget != nullptr) && (mode == Mode::HERO || mode == Mode::CREATURE_ACTIVE);//TODO: recheck
	tryMagicMirror = tryMagicMirror && (mainTarget->unitOwner() != caster->getOwner()) && !spell->isPositive();//TODO: recheck

	m->cast(env, env->getRandomGenerator(), target);

	//Magic Mirror effect
	if(tryMagicMirror)
	{
		const std::string magicMirrorCacheStr = "type_MAGIC_MIRROR";
		static const auto magicMirrorSelector = Selector::type(Bonus::MAGIC_MIRROR);

		auto rangeGen = env->getRandomGenerator().getInt64Range(0, 99);

		const int mirrorChance = mainTarget->valOfBonuses(magicMirrorSelector, magicMirrorCacheStr);

		if(rangeGen() < mirrorChance)
		{
			auto mirrorTargets = cb->battleGetUnitsIf([this](const battle::Unit * unit)
			{
				//Get all caster stacks. Magic mirror can reflect to immune creature (with no effect)
				return unit->unitOwner() == caster->getOwner() && unit->isValidTarget(true);
			});


			if(!mirrorTargets.empty())
			{
				auto mirrorTarget = (*RandomGeneratorUtil::nextItem(mirrorTargets, env->getRandomGenerator()));

				BattleCast mirror(*this, mainTarget);
				mirror.aimToUnit(mirrorTarget);
				mirror.cast(env);
			}
		}
	}
}

void BattleCast::cast(IBattleState * battleState, vstd::RNG & rng)
{
	//TODO: make equivalent to normal cast
	if(target.empty())
		aimToHex(BattleHex::INVALID);
	auto m = spell->battleMechanics(this);

	//TODO: reflection
	//TODO: random effects evaluation

	m->cast(battleState, rng, target);
}

bool BattleCast::castIfPossible(const SpellCastEnvironment * env)
{
	if(spell->canBeCast(cb, mode, caster))
	{
		cast(env);
		return true;
	}
	return false;
}

std::vector<Target> BattleCast::findPotentialTargets() const
{
	//TODO: for more than 2 destinations per target much more efficient algorithm is required

	auto m = spell->battleMechanics(this);

	auto targetTypes = m->getTargetTypes();


	if(targetTypes.empty() || targetTypes.size() > 2)
	{
		return std::vector<Target>();
	}
	else
	{
		std::vector<Target> previous;
		std::vector<Target> next;

		for(size_t index = 0; index < targetTypes.size(); index++)
		{
			std::swap(previous, next);
			next.clear();

			std::vector<Destination> destinations;

			if(previous.empty())
			{
				Target empty;
				destinations = m->getPossibleDestinations(index, targetTypes.at(index), empty);

				for(auto & destination : destinations)
				{
					Target target;
					target.emplace_back(destination);
					next.push_back(target);
				}
			}
			else
			{
				for(const Target & current : previous)
				{
					destinations = m->getPossibleDestinations(index, targetTypes.at(index), current);

					for(auto & destination : destinations)
					{
						Target target = current;
						target.emplace_back(destination);
						next.push_back(target);
					}
				}
			}

			if(next.empty())
				break;
		}
		return next;
	}
}

///ISpellMechanicsFactory
ISpellMechanicsFactory::ISpellMechanicsFactory(const CSpell * s)
	: spell(s)
{

}

ISpellMechanicsFactory::~ISpellMechanicsFactory()
{

}

std::unique_ptr<ISpellMechanicsFactory> ISpellMechanicsFactory::get(const CSpell * s)
{
	if(s->hasBattleEffects())
		return make_unique<ConfigurableMechanicsFactory>(s);
	else
		return make_unique<FallbackMechanicsFactory>(s);
}

///Mechanics
Mechanics::Mechanics()
	: cb(nullptr),
	caster(nullptr),
	casterUnit(nullptr),
	casterSide(0)
{

}

Mechanics::~Mechanics() = default;

BaseMechanics::BaseMechanics(const IBattleCast * event)
	: Mechanics(),
	owner(event->getSpell()),
	mode(event->getMode()),
	smart(event->isSmart()),
	massive(event->isMassive())
{
	cb = event->getBattle();
	caster = event->getCaster();

	casterUnit = dynamic_cast<const battle::Unit *>(caster);

	//FIXME: ensure caster and check for valid player and side
	casterSide = 0;
	if(caster)
		casterSide = cb->playerToSide(caster->getOwner()).get();

	{
		auto value = event->getRangeLevel();
		if(value)
			rangeLevel = value.get();
		else
			rangeLevel = caster->getSpellSchoolLevel(owner);
		vstd::abetween(rangeLevel, 0, 3);
	}
	{
		auto value = event->getEffectLevel();
        if(value)
			effectLevel = value.get();
		else
			effectLevel = caster->getEffectLevel(owner);
		vstd::abetween(effectLevel, 0, 3);
	}
	{
		auto value = event->getEffectPower();
		if(value)
			effectPower = value.get();
		else
			effectPower = caster->getEffectPower(owner);
		vstd::amax(effectPower, 0);
	}
	{
		auto value = event->getEffectDuration();
		if(value)
			effectDuration = value.get();
		else
			effectDuration = caster->getEnchantPower(owner);
		vstd::amax(effectDuration, 0); //???
	}
	{
		auto value = event->getEffectValue();
		if(value)
		{
			effectValue = value.get();
		}
		else
		{
			auto casterValue = caster->getEffectValue(owner);
			if(casterValue == 0)
				effectValue = owner->calculateRawEffectValue(effectLevel, effectPower, 1);
			else
				effectValue = casterValue;
		}
		vstd::amax(effectValue, 0);
	}
}

BaseMechanics::~BaseMechanics() = default;

bool BaseMechanics::adaptGenericProblem(Problem & target) const
{
	MetaString text;
	// %s recites the incantations but they seem to have no effect.
	text.addTxt(MetaString::GENERAL_TXT, 541);
	caster->getCasterName(text);

	target.add(std::move(text), spells::Problem::NORMAL);
	return false;
}

bool BaseMechanics::adaptProblem(ESpellCastProblem::ESpellCastProblem source, Problem & target) const
{
	if(source == ESpellCastProblem::OK)
		return true;

	switch(source)
	{
	case ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED:
		{
			MetaString text;
			//TODO: refactor
			auto hero = dynamic_cast<const CGHeroInstance *>(caster);
			if(!hero)
				return adaptGenericProblem(target);

			//Recanter's Cloak or similar effect. Try to retrieve bonus
			const auto b = hero->getBonusLocalFirst(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
			//TODO what about other values and non-artifact sources?
			if(b && b->val == 2 && b->source == Bonus::ARTIFACT)
			{
				//The %s prevents %s from casting 3rd level or higher spells.
				text.addTxt(MetaString::GENERAL_TXT, 536);
				text.addReplacement(MetaString::ART_NAMES, b->sid);
				caster->getCasterName(text);
				target.add(std::move(text), spells::Problem::NORMAL);
			}
			else if(b && b->source == Bonus::TERRAIN_OVERLAY && b->sid == BFieldType::CURSED_GROUND)
			{
				text.addTxt(MetaString::GENERAL_TXT, 537);
				target.add(std::move(text), spells::Problem::NORMAL);
			}
			else
			{
				return adaptGenericProblem(target);
			}
		}
		break;
	case ESpellCastProblem::WRONG_SPELL_TARGET:
	case ESpellCastProblem::STACK_IMMUNE_TO_SPELL:
	case ESpellCastProblem::NO_APPROPRIATE_TARGET:
		{
			MetaString text;
			text.addTxt(MetaString::GENERAL_TXT, 185);
			target.add(std::move(text), spells::Problem::NORMAL);
		}
		break;
	case ESpellCastProblem::INVALID:
		{
			MetaString text;
			text.addReplacement("Internal error during check of spell cast.");
			target.add(std::move(text), spells::Problem::CRITICAL);
		}
		break;
	default:
		return adaptGenericProblem(target);
	}

	return false;
}

int32_t BaseMechanics::getSpellIndex() const
{
	return getSpellId().toEnum();
}

SpellID BaseMechanics::getSpellId() const
{
	return owner->id;
}

std::string BaseMechanics::getSpellName() const
{
	return owner->name;
}

int32_t BaseMechanics::getSpellLevel() const
{
	return owner->level;
}

bool BaseMechanics::isSmart() const
{
	if(boost::logic::indeterminate(smart))
	{
		const CSpell::TargetInfo targetInfo(owner, getRangeLevel(), mode);
		return targetInfo.smart;
	}
	else
	{
		return smart;
	}
}

bool BaseMechanics::isMassive() const
{
	if(boost::logic::indeterminate(massive))
	{
		const CSpell::TargetInfo targetInfo(owner, getRangeLevel(), mode);
		return targetInfo.massive;
	}
	else
	{
		return massive;
	}
}

bool BaseMechanics::requiresClearTiles() const
{
	const CSpell::TargetInfo targetInfo(owner, getRangeLevel(), mode);
	return targetInfo.clearAffected;
}

bool BaseMechanics::alwaysHitFirstTarget() const
{
	return mode == Mode::SPELL_LIKE_ATTACK;
}

bool BaseMechanics::isNegativeSpell() const
{
	return owner->isNegative();
}

bool BaseMechanics::isPositiveSpell() const
{
	return owner->isPositive();
}

int64_t BaseMechanics::adjustEffectValue(const battle::Unit * target) const
{
	return owner->adjustRawDamage(caster, target, getEffectValue());
}

int64_t BaseMechanics::applySpellBonus(int64_t value, const battle::Unit* target) const
{
	return caster->getSpellBonus(owner, value, target);
}

int64_t BaseMechanics::applySpecificSpellBonus(int64_t value) const
{
	return caster->getSpecificSpellBonus(owner, value);
}

int64_t BaseMechanics::calculateRawEffectValue(int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const
{
	return owner->calculateRawEffectValue(getEffectLevel(), basePowerMultiplier, levelPowerMultiplier);
}

std::vector<Bonus::BonusType> BaseMechanics::getElementalImmunity() const
{
	std::vector<Bonus::BonusType> ret;

	owner->forEachSchool([&](const SchoolInfo & cnf, bool & stop)
	{
		ret.push_back(cnf.immunityBonus);
	});

	return ret;
}

bool BaseMechanics::ownerMatches(const battle::Unit * unit) const
{
    return ownerMatches(unit, owner->getPositiveness());
}

bool BaseMechanics::ownerMatches(const battle::Unit * unit, const boost::logic::tribool positivness) const
{
    return cb->battleMatchOwner(caster->getOwner(), unit, positivness);
}

IBattleCast::Value BaseMechanics::getEffectLevel() const
{
	return effectLevel;
}

IBattleCast::Value BaseMechanics::getRangeLevel() const
{
	return rangeLevel;
}

IBattleCast::Value BaseMechanics::getEffectPower() const
{
	return effectPower;
}

IBattleCast::Value BaseMechanics::getEffectDuration() const
{
	return effectDuration;
}

IBattleCast::Value64 BaseMechanics::getEffectValue() const
{
	return effectValue;
}

PlayerColor BaseMechanics::getCasterColor() const
{
	return caster->getOwner();
}

std::vector<AimType> BaseMechanics::getTargetTypes() const
{
	std::vector<AimType> ret;
	detail::ProblemImpl ingored;

	if(canBeCast(ingored))
	{
		auto spellTargetType = owner->getTargetType();

		if(isMassive())
			spellTargetType = AimType::NO_TARGET;
		else if(spellTargetType == AimType::OBSTACLE)
			spellTargetType = AimType::LOCATION;

		ret.push_back(spellTargetType);
	}

	return ret;
}

} //namespace spells

///IAdventureSpellMechanics
IAdventureSpellMechanics::IAdventureSpellMechanics(const CSpell * s)
	: owner(s)
{
}

std::unique_ptr<IAdventureSpellMechanics> IAdventureSpellMechanics::createMechanics(const CSpell * s)
{
	switch (s->id)
	{
	case SpellID::SUMMON_BOAT:
		return make_unique<SummonBoatMechanics>(s);
	case SpellID::SCUTTLE_BOAT:
		return make_unique<ScuttleBoatMechanics>(s);
	case SpellID::DIMENSION_DOOR:
		return make_unique<DimensionDoorMechanics>(s);
	case SpellID::FLY:
	case SpellID::WATER_WALK:
	case SpellID::VISIONS:
	case SpellID::DISGUISE:
		return make_unique<AdventureSpellMechanics>(s); //implemented using bonus system
	case SpellID::TOWN_PORTAL:
		return make_unique<TownPortalMechanics>(s);
	case SpellID::VIEW_EARTH:
		return make_unique<ViewEarthMechanics>(s);
	case SpellID::VIEW_AIR:
		return make_unique<ViewAirMechanics>(s);
	default:
		return std::unique_ptr<IAdventureSpellMechanics>();
	}
}

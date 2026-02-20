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

#include "../GameLibrary.h"

#include "../bonuses/Bonus.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/IBattleState.h"
#include "../battle/Unit.h"

#include "../mapObjects/CGHeroInstance.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

#include "TargetCondition.h"
#include "Problem.h"

#include "adventure/AdventureSpellMechanics.h"

#include "BattleSpellMechanics.h"

#include "effects/Effects.h"
#include "effects/Registry.h"
#include "effects/Damage.h"
#include "effects/Timed.h"

#include "CSpellHandler.h"

#include "../BattleFieldHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

static std::shared_ptr<TargetCondition> makeCondition(const CSpell * s)
{
	auto res = std::make_shared<TargetCondition>();

	JsonDeserializer deser(nullptr, s->targetCondition);
	res->serializeJson(deser, TargetConditionItemFactory::getDefault());

	return res;
}

class CustomMechanicsFactory : public ISpellMechanicsFactory
{
public:
	std::unique_ptr<Mechanics> create(const IBattleCast * event) const override
	{
		auto * ret = new BattleSpellMechanics(event, effects, targetCondition);
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
		effects->serializeJson(LIBRARY->spellEffects(), deser, level);
	}
private:
	std::shared_ptr<IReceptiveCheck> targetCondition;
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

			if(s->isOffensive())
			{
				//default constructed object should be enough
				effects->add("directDamage", std::make_shared<effects::Damage>(), level);
			}

			std::shared_ptr<effects::Effect> effect;

			if(!levelInfo.effects.empty())
			{
				auto * timed = new effects::Timed();
				timed->cumulative = false;
				timed->bonus = levelInfo.effects;
				effect.reset(timed);
			}

			if(!levelInfo.cumulativeEffects.empty())
			{
				auto * timed = new effects::Timed();
				timed->cumulative = true;
				timed->bonus = levelInfo.cumulativeEffects;
				effect.reset(timed);
			}

			if(effect)
				effects->add("timed", effect, level);
		}
	}
};

BattleCast::BattleCast(const CBattleInfoCallback * cb_, const Caster * caster_, const Mode mode_, const CSpell * spell_):
	spell(spell_),
	cb(cb_),
	caster(caster_),
	mode(mode_),
	smart(boost::logic::indeterminate),
	massive(boost::logic::indeterminate)
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

BattleCast::OptionalValue BattleCast::getSpellLevel() const
{
	return magicSkillLevel;
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
	magicSkillLevel = std::make_optional(value);
}

void BattleCast::setEffectPower(BattleCast::Value value)
{
	effectPower = std::make_optional(value);
}

void BattleCast::setEffectDuration(BattleCast::Value value)
{
	effectDuration = std::make_optional(value);
}

void BattleCast::setEffectValue(BattleCast::Value64 value)
{
	effectValue = std::make_optional(value);
}

void BattleCast::applyEffects(ServerCallback * server, const Target & target, bool indirect, bool ignoreImmunity) const
{
	auto m = spell->battleMechanics(this);

	m->applyEffects(server, target, indirect, ignoreImmunity);
}

void BattleCast::cast(ServerCallback * server, Target target)
{
	if(target.empty())
		target.emplace_back();

	auto m = spell->battleMechanics(this);

	m->cast(server, target);
}

void BattleCast::castEval(ServerCallback * server, Target target)
{
	//TODO: make equivalent to normal cast
	if(target.empty())
		target.emplace_back();
	auto m = spell->battleMechanics(this);

	//TODO: reflection
	//TODO: random effects evaluation

	m->castEval(server, target);
}

bool BattleCast::castIfPossible(ServerCallback * server, Target target)
{
	if(spell->canBeCast(cb, mode, caster))
	{
		cast(server, std::move(target));
		return true;
	}
	return false;
}

std::vector<Target> BattleCast::findPotentialTargets(bool fast) const
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
				destinations = m->getPossibleDestinations(index, targetTypes.at(index), empty, fast);

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
					destinations = m->getPossibleDestinations(index, targetTypes.at(index), current, fast);

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

//must be instantiated in .cpp file for access to complete types of all member fields
ISpellMechanicsFactory::~ISpellMechanicsFactory() = default;

std::unique_ptr<ISpellMechanicsFactory> ISpellMechanicsFactory::get(const CSpell * s)
{
	if(s->hasBattleEffects())
		return std::make_unique<ConfigurableMechanicsFactory>(s);
	else
		return std::make_unique<FallbackMechanicsFactory>(s);
}

///Mechanics
Mechanics::Mechanics()
	: caster(nullptr),
	casterSide(BattleSide::NONE)
{

}

Mechanics::~Mechanics() = default;

BaseMechanics::BaseMechanics(const IBattleCast * event):
	owner(event->getSpell()),
	mode(event->getMode()),
	smart(event->isSmart()),
	massive(event->isMassive()),
	cb(event->getBattle())
{
	caster = event->getCaster();

	casterSide = cb->playerToSide(caster->getCasterOwner());

	{
		auto value = event->getSpellLevel();
		rangeLevel = value.value_or(caster->getSpellSchoolLevel(owner));
		vstd::abetween(rangeLevel, 0, 3);
	}
	{
		auto value = event->getSpellLevel();
		effectLevel = value.value_or(caster->getEffectLevel(owner));
		vstd::abetween(effectLevel, 0, 3);
	}
	{
		auto value = event->getEffectPower();
		effectPower = value.value_or(caster->getEffectPower(owner));
		vstd::amax(effectPower, 0);
	}
	{
		auto value = event->getEffectDuration();
		effectDuration = value.value_or(caster->getEnchantPower(owner));
		vstd::amax(effectDuration, 0); //???
	}
	{
		auto value = event->getEffectValue();
		auto casterValue = caster->getEffectValue(owner) ? caster->getEffectValue(owner) : owner->calculateRawEffectValue(effectLevel, effectPower, 1);
		effectValue = value.value_or(casterValue);
		vstd::amax(effectValue, 0);
	}
}

BaseMechanics::~BaseMechanics() = default;

bool BaseMechanics::adaptGenericProblem(Problem & target) const
{
	MetaString text;
	// %s recites the incantations but they seem to have no effect.
	text.appendLocalString(EMetaText::GENERAL_TXT, 541);
	assert(caster);
	caster->getCasterName(text);

	target.add(std::move(text), spells::Problem::NORMAL);
	return false;
}

bool BaseMechanics::adaptProblem(ESpellCastProblem source, Problem & target) const
{
	if(source == ESpellCastProblem::OK)
		return true;

	switch(source)
	{
	case ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED:
		{
			MetaString text;
			//TODO: refactor
			const auto * hero = dynamic_cast<const CGHeroInstance *>(caster);
			if(!hero)
				return adaptGenericProblem(target);

			//Recanter's Cloak or similar effect. Try to retrieve bonus
			const auto b = hero->getFirstBonus(Selector::type()(BonusType::BLOCK_MAGIC_ABOVE));
			//TODO what about other values and non-artifact sources?
			if(b && b->val == 2 && b->source == BonusSource::ARTIFACT)
			{
				//The %s prevents %s from casting 3rd level or higher spells.
				text.appendLocalString(EMetaText::GENERAL_TXT, 536);
				text.replaceName(b->sid.as<ArtifactID>());
				caster->getCasterName(text);
				target.add(std::move(text), spells::Problem::NORMAL);
			}
			else if(b && b->source == BonusSource::TERRAIN_OVERLAY && LIBRARY->battlefields()->getById(b->sid.as<BattleField>())->identifier == "cursed_ground")
			{
				text.appendLocalString(EMetaText::GENERAL_TXT, 537);
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
			text.appendLocalString(EMetaText::GENERAL_TXT, 185);
			target.add(std::move(text), spells::Problem::NORMAL);
		}
		break;
	case ESpellCastProblem::INVALID:
		{
			MetaString text;
			text.appendRawString("Internal error during check of spell cast.");
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
	return owner->getId();
}

std::string BaseMechanics::getSpellName() const
{
	return owner->getNameTranslated();
}

int32_t BaseMechanics::getSpellLevel() const
{
	return owner->getLevel();
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
		return (bool)smart;
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
		return (bool)massive;
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

bool BaseMechanics::isMagicalEffect() const
{
	return owner->isMagical();
}

int64_t BaseMechanics::adjustEffectValue(const battle::Unit * target) const
{
	return owner->adjustRawDamage(caster, target, getEffectValue());
}

int64_t BaseMechanics::applySpellBonus(int64_t value, const battle::Unit * target) const
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

Target BaseMechanics::canonicalizeTarget(const Target & aim) const
{
	return aim;
}

bool BaseMechanics::ownerMatches(const battle::Unit * unit) const
{
	return ownerMatches(unit, owner->getPositiveness());
}

bool BaseMechanics::ownerMatches(const battle::Unit * unit, const boost::logic::tribool positivness) const
{
	return cb->battleMatchOwner(caster->getCasterOwner(), unit, positivness);
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
	return caster->getCasterOwner();
}

std::vector<AimType> BaseMechanics::getTargetTypes() const
{
	std::vector<AimType> ret;
	detail::ProblemImpl ignored;

	if(canBeCast(ignored))
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

const CreatureService * BaseMechanics::creatures() const
{
	return LIBRARY->creatures(); //todo: redirect
}

#if SCRIPTING_ENABLED
const scripting::Service * BaseMechanics::scripts() const
{
	return LIBRARY->scripts(); //todo: redirect
}
#endif

const Service * BaseMechanics::spells() const
{
	return LIBRARY->spells(); //todo: redirect
}

const CBattleInfoCallback * BaseMechanics::battle() const
{
	return cb;
}


} //namespace spells

///IAdventureSpellMechanics
IAdventureSpellMechanics::IAdventureSpellMechanics(const CSpell * s)
	: owner(s)
{
}

std::unique_ptr<IAdventureSpellMechanics> IAdventureSpellMechanics::createMechanics(const CSpell * s)
{
	if (s->isCombat())
		return nullptr;

	return std::make_unique<AdventureSpellMechanics>(s);
}

VCMI_LIB_NAMESPACE_END

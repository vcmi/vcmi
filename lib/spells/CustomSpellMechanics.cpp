/*
 * CustomSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CustomSpellMechanics.h"
#include "CDefaultSpellMechanics.h"
#include "../battle/IBattleState.h"
#include "../battle/CBattleInfoCallback.h"
#include "Problem.h"
#include "CSpellHandler.h"

#include "../CStack.h"

namespace spells
{

CustomSpellMechanics::CustomSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> e)
	: DefaultSpellMechanics(event),
	effects(e)
{}

CustomSpellMechanics::~CustomSpellMechanics() = default;

void CustomSpellMechanics::applyEffects(const SpellCastEnvironment * env, const Target & targets) const
{
	auto callback = [&](const effects::Effect * e, bool & stop)
	{
		EffectTarget target = e->filterTarget(this, targets);
		e->apply(env, env->getRandomGenerator(), this, target);
	};

	effects->forEachEffect(getEffectLevel(), callback);
}

void CustomSpellMechanics::applyIndirectEffects(const SpellCastEnvironment * env, const Target & targets) const
{
	auto callback = [&](const effects::Effect * e, bool & stop)
	{
		if(!e->automatic)
		{
			EffectTarget target = e->filterTarget(this, targets);
			e->apply(env, env->getRandomGenerator(), this, target);
		}
	};

	effects->forEachEffect(getEffectLevel(), callback);
}

void CustomSpellMechanics::applyEffectsForced(const SpellCastEnvironment * env, const Target & targets) const
{
	auto callback = [&](const effects::Effect * e, bool & stop)
	{
		e->apply(env, env->getRandomGenerator(), this, targets);
	};

	effects->forEachEffect(getEffectLevel(), callback);
}

bool CustomSpellMechanics::canBeCast(Problem & problem) const
{
	return effects->applicable(problem, this);
}

bool CustomSpellMechanics::canBeCastAt(BattleHex destination) const
{
	detail::ProblemImpl problem;

	//TODO: add support for secondary targets
	//TODO: send problem to caller (for battle log message in BattleInterface)
	Target tmp;
	tmp.push_back(Destination(destination));

	Target spellTarget = transformSpellTarget(tmp);

    return effects->applicable(problem, this, tmp, spellTarget);
}

std::vector<const CStack *> CustomSpellMechanics::getAffectedStacks(BattleHex destination) const
{
	Target tmp;
	tmp.push_back(Destination(destination));
	Target spellTarget = transformSpellTarget(tmp);

	EffectTarget all;

	effects->forEachEffect(getEffectLevel(), [&all, &tmp, &spellTarget, this](const effects::Effect * e, bool & stop)
	{
		EffectTarget one = e->transformTarget(this, tmp, spellTarget);
		vstd::concatenate(all, one);
	});

	std::set<const CStack *> stacks;

	for(const Destination & dest : all)
	{
		if(dest.unitValue)
		{
			//FIXME: remove and return battle::Unit
			stacks.insert(cb->battleGetStackByID(dest.unitValue->unitId(), false));
		}
	}

	std::vector<const CStack *> res;
	std::copy(stacks.begin(), stacks.end(), std::back_inserter(res));
	return res;
}

void CustomSpellMechanics::beforeCast(vstd::RNG & rng, const Target & target, std::vector<const CStack*> & reflected)
{
	reflected.clear();
	affectedUnits.clear();

	Target spellTarget = transformSpellTarget(target);

	std::vector <const CStack *> resisted;

	auto rangeGen = rng.getInt64Range(0, 99);

	auto filterReflected = [&, this](const CStack * s) -> bool
	{
		const bool tryMagicMirror = mode != Mode::MAGIC_MIRROR && isNegativeSpell() && owner->level && owner->getLevelInfo(0).range == "0";
		if(tryMagicMirror)
		{
			const int mirrorChance = s->valOfBonuses(Bonus::MAGIC_MIRROR);
			if(rangeGen() < mirrorChance)
				return true;
		}
		return false;
	};

	auto filterResisted = [&, this](const CStack * s) -> bool
	{
		if(isNegativeSpell())
		{
			//magic resistance
			const int prob = std::min((s)->magicResistance(), 100); //probability of resistance in %
			if(rangeGen() < prob)
				return true;
		}
		return false;
	};

	auto filterStack = [&](const battle::Unit * st)
	{
		const CStack * s = dynamic_cast<const CStack *>(st);

		if(!s)
			s = cb->battleGetStackByID(st->unitId(), false);

		if(filterResisted(s))
			resisted.push_back(s);
		else if(filterReflected(s))
			reflected.push_back(s);
		else
			affectedUnits.push_back(s);
	};

	//prepare targets
	effectsToApply = effects->prepare(this, target, spellTarget);

	std::set<const battle::Unit *> stacks = collectTargets();

	//process them
	for(auto s : stacks)
		filterStack(s);

	//and update targets
	for(auto & p : effectsToApply)
	{
		vstd::erase_if(p.second, [&](const Destination & d)
		{
			if(!d.unitValue)
				return false;
			return vstd::contains(resisted, d.unitValue) || vstd::contains(reflected, d.unitValue);
		});
	}

	for(auto s : reflected)
		addCustomEffect(s, 3);

	for(auto s : resisted)
		addCustomEffect(s, 78);
}


void CustomSpellMechanics::applyCastEffects(const SpellCastEnvironment * env, const Target & target) const
{
	for(auto & p : effectsToApply)
		p.first->apply(env, env->getRandomGenerator(), this, p.second);
}

void CustomSpellMechanics::cast(IBattleState * battleState, vstd::RNG & rng, const Target & target)
{
	//TODO: evaluate caster updates (mana usage etc.)
	//TODO: evaluate random values

	Target spellTarget = transformSpellTarget(target);

	effectsToApply = effects->prepare(this, target, spellTarget);

	std::set<const battle::Unit *> stacks = collectTargets();

	for(const battle::Unit * one : stacks)
	{
		auto selector = std::bind(&DefaultSpellMechanics::counteringSelector, this, _1);

		std::vector<Bonus> buffer;
		auto bl = one->getBonuses(selector);

		for(auto item : *bl)
			buffer.emplace_back(*item);

		if(!buffer.empty())
			battleState->removeUnitBonus(one->unitId(), buffer);
	}

	for(auto & p : effectsToApply)
		p.first->apply(battleState, rng, this, p.second);
}

std::set<const battle::Unit *> CustomSpellMechanics::collectTargets() const
{
	std::set<const battle::Unit *> result;

	for(const auto & p : effectsToApply)
	{
		for(const Destination & d : p.second)
			if(d.unitValue)
				result.insert(d.unitValue);
	}

	return result;
}

Target CustomSpellMechanics::transformSpellTarget(const Target & aimPoint) const
{
	Target spellTarget;

	if(aimPoint.size() < 1)
	{
		logGlobal->error("Aimed spell cast with no destination.");
	}
	else
	{
		const Destination & primary = aimPoint.at(0);
		BattleHex aimPoint = primary.hexValue;

		//transform primary spell target with spell range (if it`s valid), leave anything else to effects

		if(aimPoint.isValid())
		{
			auto spellRange = spellRangeInHexes(aimPoint);
			for(auto & hex : spellRange)
				spellTarget.push_back(Destination(hex));
		}
	}

	if(spellTarget.empty())
		spellTarget.push_back(Destination(BattleHex::INVALID));

	return std::move(spellTarget);
}

std::vector<AimType> CustomSpellMechanics::getTargetTypes() const
{
	auto ret = DefaultSpellMechanics::getTargetTypes();

	if(!ret.empty())
	{
		effects->forEachEffect(getEffectLevel(), [&](const effects::Effect * e, bool & stop)
		{
			e->adjustTargetTypes(ret);
			stop = ret.empty();
		});
	}

	return ret;
}

std::vector<Destination> CustomSpellMechanics::getPossibleDestinations(size_t index, AimType aimType, const Target & current) const
{
	//TODO: CustomSpellMechanics::getPossibleDestinations

	if(index != 0)
		return std::vector<Destination>();

	std::vector<Destination> ret;

	switch(aimType)
	{
	case AimType::CREATURE:
	case AimType::LOCATION:
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			BattleHex dest(i);
			if(dest.isAvailable())
				if(canBeCastAt(dest))
					ret.emplace_back(dest);
		}
		break;
	case AimType::NO_TARGET:
		ret.emplace_back();
		break;
	default:
		break;
	}

	return ret;
}

std::vector<BattleHex> CustomSpellMechanics::rangeInHexes(BattleHex centralHex, bool * outDroppedHexes) const
{
	if(isMassive() || !centralHex.isValid())
		return std::vector<BattleHex>(1, BattleHex::INVALID);

	Target aimPoint;
	aimPoint.push_back(Destination(centralHex));

	Target spellTarget = transformSpellTarget(aimPoint);

	std::set<BattleHex> effectRange;

	effects->forEachEffect(getEffectLevel(), [&](const effects::Effect * effect, bool & stop)
	{
		if(effect->automatic)
		{
			effect->adjustAffectedHexes(effectRange, this, spellTarget);
		}
	});

	std::vector<BattleHex> ret;
	ret.reserve(effectRange.size());

	std::copy(effectRange.begin(), effectRange.end(), std::back_inserter(ret));

	return ret;
}


} //namespace spells


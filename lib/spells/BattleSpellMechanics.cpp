/*
 * BattleSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleSpellMechanics.h"

#include "Problem.h"
#include "CSpellHandler.h"

#include "../battle/IBattleState.h"
#include "../battle/CBattleInfoCallback.h"

#include "../CStack.h"
#include "../NetPacks.h"

namespace spells
{

namespace SRSLPraserHelpers
{
	static int XYToHex(int x, int y)
	{
		return x + GameConstants::BFIELD_WIDTH * y;
	}

	static int XYToHex(std::pair<int, int> xy)
	{
		return XYToHex(xy.first, xy.second);
	}

	static int hexToY(int battleFieldPosition)
	{
		return battleFieldPosition/GameConstants::BFIELD_WIDTH;
	}

	static int hexToX(int battleFieldPosition)
	{
		int pos = battleFieldPosition - hexToY(battleFieldPosition) * GameConstants::BFIELD_WIDTH;
		return pos;
	}

	static std::pair<int, int> hexToPair(int battleFieldPosition)
	{
		return std::make_pair(hexToX(battleFieldPosition), hexToY(battleFieldPosition));
	}

	//moves hex by one hex in given direction
	//0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left
	static std::pair<int, int> gotoDir(int x, int y, int direction)
	{
		switch(direction)
		{
		case 0: //top left
			return std::make_pair((y%2) ? x-1 : x, y-1);
		case 1: //top right
			return std::make_pair((y%2) ? x : x+1, y-1);
		case 2:  //right
			return std::make_pair(x+1, y);
		case 3: //right bottom
			return std::make_pair((y%2) ? x : x+1, y+1);
		case 4: //left bottom
			return std::make_pair((y%2) ? x-1 : x, y+1);
		case 5: //left
			return std::make_pair(x-1, y);
		default:
			throw std::runtime_error("Disaster: wrong direction in SRSLPraserHelpers::gotoDir!\n");
		}
	}

	static std::pair<int, int> gotoDir(std::pair<int, int> xy, int direction)
	{
		return gotoDir(xy.first, xy.second, direction);
	}

	static bool isGoodHex(std::pair<int, int> xy)
	{
		return xy.first >=0 && xy.first < GameConstants::BFIELD_WIDTH && xy.second >= 0 && xy.second < GameConstants::BFIELD_HEIGHT;
	}

	//helper function for rangeInHexes
	static std::set<ui16> getInRange(unsigned int center, int low, int high)
	{
		std::set<ui16> ret;
		if(low == 0)
		{
			ret.insert(center);
		}

		std::pair<int, int> mainPointForLayer[6]; //A, B, C, D, E, F points
		for(auto & elem : mainPointForLayer)
			elem = hexToPair(center);

		for(int it=1; it<=high; ++it) //it - distance to the center
		{
			for(int b=0; b<6; ++b)
				mainPointForLayer[b] = gotoDir(mainPointForLayer[b], b);

			if(it>=low)
			{
				std::pair<int, int> curHex;

				//adding lines (A-b, B-c, C-d, etc)
				for(int v=0; v<6; ++v)
				{
					curHex = mainPointForLayer[v];
					for(int h=0; h<it; ++h)
					{
						if(isGoodHex(curHex))
							ret.insert(XYToHex(curHex));
						curHex = gotoDir(curHex, (v+2)%6);
					}
				}

			} //if(it>=low)
		}

		return ret;
	}
}


BattleSpellMechanics::BattleSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> effects_, std::shared_ptr<IReceptiveCheck> targetCondition_)
	: BaseMechanics(event),
	effects(effects_),
	targetCondition(targetCondition_)
{}

BattleSpellMechanics::~BattleSpellMechanics() = default;

void BattleSpellMechanics::applyEffects(BattleStateProxy * battleState, vstd::RNG & rng, const Target & targets, bool indirect, bool ignoreImmunity) const
{
	auto callback = [&](const effects::Effect * effect, bool & stop)
	{
		if(indirect == effect->indirect)
		{
			if(ignoreImmunity)
			{
				effect->apply(battleState, rng, this, targets);
			}
			else
			{
				EffectTarget filtered = effect->filterTarget(this, targets);
				effect->apply(battleState, rng, this, filtered);
			}
		}
	};

	effects->forEachEffect(getEffectLevel(), callback);
}

bool BattleSpellMechanics::canBeCast(Problem & problem) const
{
	return effects->applicable(problem, this);
}

bool BattleSpellMechanics::canBeCastAt(const Target & target) const
{
	detail::ProblemImpl problem;

	//TODO: send problem to caller (for battle log message in BattleInterface)

	Target spellTarget = transformSpellTarget(target);

    return effects->applicable(problem, this, target, spellTarget);
}

std::vector<const CStack *> BattleSpellMechanics::getAffectedStacks(const Target & target) const
{
	Target spellTarget = transformSpellTarget(target);

	EffectTarget all;

	effects->forEachEffect(getEffectLevel(), [&all, &target, &spellTarget, this](const effects::Effect * e, bool & stop)
	{
		EffectTarget one = e->transformTarget(this, target, spellTarget);
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

void BattleSpellMechanics::cast(const PacketSender * server, vstd::RNG & rng, const Target & target)
{
	BattleSpellCast sc;

	int spellCost = 0;

	sc.side = casterSide;
	sc.spellID = getSpellId();
	sc.tile = target.at(0).hexValue;

	sc.castByHero = mode == Mode::HERO;
	sc.casterStack = (casterUnit ? casterUnit->unitId() : -1);
	sc.manaGained = 0;

	sc.activeCast = false;
	affectedUnits.clear();

	const CGHeroInstance * otherHero = nullptr;
	{
		//check it there is opponent hero
		const ui8 otherSide = cb->otherSide(casterSide);

		if(cb->battleHasHero(otherSide))
			otherHero = cb->battleGetFightingHero(otherSide);
	}

	//calculate spell cost
	if(mode == Mode::HERO)
	{
		auto casterHero = dynamic_cast<const CGHeroInstance *>(caster);
		spellCost = cb->battleGetSpellCost(owner, casterHero);

		if(nullptr != otherHero) //handle mana channel
		{
			int manaChannel = 0;
			for(const CStack * stack : cb->battleGetAllStacks(true)) //TODO: shouldn't bonus system handle it somehow?
			{
				if(stack->owner == otherHero->tempOwner)
				{
					vstd::amax(manaChannel, stack->valOfBonuses(Bonus::MANA_CHANNELING));
				}
			}
			sc.manaGained = (manaChannel * spellCost) / 100;
		}
		sc.activeCast = true;
	}
	else if(mode == Mode::CREATURE_ACTIVE || mode == Mode::ENCHANTER)
	{
		spellCost = 1;
		sc.activeCast = true;
	}

	beforeCast(sc, rng, target);

	switch (mode)
	{
	case Mode::CREATURE_ACTIVE:
	case Mode::ENCHANTER:
	case Mode::HERO:
	case Mode::PASSIVE:
		{
			MetaString line;
			caster->getCastDescription(owner, affectedUnits, line);
			if(!line.message.empty())
				sc.battleLog.push_back(line);
		}
		break;

	default:
		break;
	}

	doRemoveEffects(server, affectedUnits, std::bind(&BattleSpellMechanics::counteringSelector, this, _1));

	for(auto & unit : affectedUnits)
		sc.affectedCres.insert(unit->unitId());

	server->sendAndApply(&sc);

	{
		BattleStateProxy proxy(server);
		for(auto & p : effectsToApply)
			p.first->apply(&proxy, rng, this, p.second);
	}

//	afterCast();

	if(sc.activeCast)
	{
		caster->spendMana(server, spellCost);

		if(sc.manaGained > 0)
		{
			assert(otherHero);
			otherHero->spendMana(server, -sc.manaGained);
		}
	}
}

void BattleSpellMechanics::beforeCast(BattleSpellCast & sc, vstd::RNG & rng, const Target & target)
{
	affectedUnits.clear();

	Target spellTarget = transformSpellTarget(target);

	std::vector <const battle::Unit *> resisted;

	auto rangeGen = rng.getInt64Range(0, 99);

	auto filterResisted = [&, this](const battle::Unit * unit) -> bool
	{
		if(isNegativeSpell())
		{
			//magic resistance
			const int prob = std::min(unit->magicResistance(), 100); //probability of resistance in %
			if(rangeGen() < prob)
				return true;
		}
		return false;
	};

	auto filterUnit = [&](const battle::Unit * unit)
	{
		if(filterResisted(unit))
			resisted.push_back(unit);
		else
			affectedUnits.push_back(unit);
	};

	//prepare targets
	effectsToApply = effects->prepare(this, target, spellTarget);

	std::set<const battle::Unit *> unitTargets = collectTargets();

	//process them
	for(auto unit : unitTargets)
		filterUnit(unit);

	//and update targets
	for(auto & p : effectsToApply)
	{
		vstd::erase_if(p.second, [&](const Destination & d)
		{
			if(!d.unitValue)
				return false;
			return vstd::contains(resisted, d.unitValue);
		});
	}

	if(mode == Mode::MAGIC_MIRROR)
	{
		if(casterUnit)
		{
			addCustomEffect(sc, casterUnit, 3);
		}
	}

	for(auto unit : resisted)
		addCustomEffect(sc, unit, 78);
}

void BattleSpellMechanics::cast(IBattleState * battleState, vstd::RNG & rng, const Target & target)
{
	//TODO: evaluate caster updates (mana usage etc.)
	//TODO: evaluate random values

	Target spellTarget = transformSpellTarget(target);

	effectsToApply = effects->prepare(this, target, spellTarget);

	std::set<const battle::Unit *> stacks = collectTargets();

	for(const battle::Unit * one : stacks)
	{
		auto selector = std::bind(&BattleSpellMechanics::counteringSelector, this, _1);

		std::vector<Bonus> buffer;
		auto bl = one->getBonuses(selector);

		for(auto item : *bl)
			buffer.emplace_back(*item);

		if(!buffer.empty())
			battleState->removeUnitBonus(one->unitId(), buffer);
	}

	{
		BattleStateProxy proxy(battleState);
		for(auto & p : effectsToApply)
			p.first->apply(&proxy, rng, this, p.second);
	}
}

void BattleSpellMechanics::addCustomEffect(BattleSpellCast & sc, const battle::Unit * target, ui32 effect)
{
	CustomEffectInfo customEffect;
	customEffect.effect = effect;
	customEffect.stack = target->unitId();
	sc.customEffects.push_back(customEffect);
}

std::set<const battle::Unit *> BattleSpellMechanics::collectTargets() const
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

void BattleSpellMechanics::doRemoveEffects(const PacketSender * server, const std::vector<const battle::Unit *> & targets, const CSelector & selector)
{
	SetStackEffect sse;

	for(auto unit : targets)
	{
		std::vector<Bonus> buffer;
		auto bl = unit->getBonuses(selector);

		for(auto item : *bl)
			buffer.emplace_back(*item);

		if(!buffer.empty())
			sse.toRemove.push_back(std::make_pair(unit->unitId(), buffer));
	}

	if(!sse.toRemove.empty())
		server->sendAndApply(&sse);
}

bool BattleSpellMechanics::counteringSelector(const Bonus * bonus) const
{
	if(bonus->source != Bonus::SPELL_EFFECT)
		return false;

	for(const SpellID & id : owner->counteredSpells)
	{
		if(bonus->sid == id.toEnum())
			return true;
	}

	return false;
}

std::set<BattleHex> BattleSpellMechanics::spellRangeInHexes(BattleHex centralHex) const
{
	using namespace SRSLPraserHelpers;

	std::set<BattleHex> ret;
	std::string rng = owner->getLevelInfo(getRangeLevel()).range + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 2 && rng[0] != 'X') //there is at least one hex in range (+artificial comma)
	{
		std::string number1, number2;
		int beg, end;
		bool readingFirst = true;
		for(auto & elem : rng)
		{
			if(std::isdigit(elem) ) //reading number
			{
				if(readingFirst)
					number1 += elem;
				else
					number2 += elem;
			}
			else if(elem == ',') //comma
			{
				//calculating variables
				if(readingFirst)
				{
					beg = atoi(number1.c_str());
					number1 = "";
				}
				else
				{
					end = atoi(number2.c_str());
					number2 = "";
				}
				//obtaining new hexes
				std::set<ui16> curLayer;
				if(readingFirst)
				{
					curLayer = getInRange(centralHex, beg, beg);
				}
				else
				{
					curLayer = getInRange(centralHex, beg, end);
					readingFirst = true;
				}
				//adding obtained hexes
				for(auto & curLayer_it : curLayer)
				{
					ret.insert(curLayer_it);
				}

			}
			else if(elem == '-') //dash
			{
				beg = atoi(number1.c_str());
				number1 = "";
				readingFirst = false;
			}
		}
	}

	return ret;
}

Target BattleSpellMechanics::transformSpellTarget(const Target & aimPoint) const
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

std::vector<AimType> BattleSpellMechanics::getTargetTypes() const
{
	auto ret = BaseMechanics::getTargetTypes();

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

std::vector<Destination> BattleSpellMechanics::getPossibleDestinations(size_t index, AimType aimType, const Target & current) const
{
	//TODO: BattleSpellMechanics::getPossibleDestinations

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
			{
				Target tmp = current;
				tmp.emplace_back(dest);

				if(canBeCastAt(tmp))
					ret.emplace_back(dest);
			}
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

bool BattleSpellMechanics::isReceptive(const battle::Unit * target) const
{
	return targetCondition->isReceptive(this, target);
}

std::vector<BattleHex> BattleSpellMechanics::rangeInHexes(BattleHex centralHex, bool * outDroppedHexes) const
{
	if(isMassive() || !centralHex.isValid())
		return std::vector<BattleHex>(1, BattleHex::INVALID);

	Target aimPoint;
	aimPoint.push_back(Destination(centralHex));

	Target spellTarget = transformSpellTarget(aimPoint);

	std::set<BattleHex> effectRange;

	effects->forEachEffect(getEffectLevel(), [&](const effects::Effect * effect, bool & stop)
	{
		if(!effect->indirect)
		{
			effect->adjustAffectedHexes(effectRange, this, spellTarget);
		}
	});

	std::vector<BattleHex> ret;
	ret.reserve(effectRange.size());

	std::copy(effectRange.begin(), effectRange.end(), std::back_inserter(ret));

	return ret;
}

}


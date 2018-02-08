/*
 * CDefaultSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CDefaultSpellMechanics.h"

#include "CSpellHandler.h"

#include "../CStack.h"
#include "../battle/BattleInfo.h"

#include "../CGeneralTextHandler.h"

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

///DefaultSpellMechanics
DefaultSpellMechanics::DefaultSpellMechanics(const IBattleCast * event)
	: BaseMechanics(event)
{
};

std::vector<BattleHex> DefaultSpellMechanics::rangeInHexes(BattleHex centralHex, bool * outDroppedHexes) const
{
	auto spellRange = spellRangeInHexes(centralHex);

	std::vector<BattleHex> ret;
	ret.reserve(spellRange.size());

	std::copy(spellRange.begin(), spellRange.end(), std::back_inserter(ret));

	return ret;
}

std::set<BattleHex> DefaultSpellMechanics::spellRangeInHexes(BattleHex centralHex) const
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


void DefaultSpellMechanics::addBattleLog(MetaString && line)
{
	sc.battleLog.push_back(line);
}

void DefaultSpellMechanics::addCustomEffect(const battle::Unit * target, ui32 effect)
{
	CustomEffectInfo customEffect;
	customEffect.effect = effect;
	customEffect.stack = target->unitId();
	sc.customEffects.push_back(customEffect);
}


void DefaultSpellMechanics::afterCast() const
{

}

void DefaultSpellMechanics::cast(const SpellCastEnvironment * env, const Target & target, std::vector<const CStack *> & reflected)
{
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

	beforeCast(env->getRandomGenerator(), target, reflected);


	switch (mode)
	{
	case Mode::CREATURE_ACTIVE:
	case Mode::ENCHANTER:
	case Mode::HERO:
	case Mode::PASSIVE:
		{
			MetaString line;
			caster->getCastDescription(owner, affectedUnits, line);
			addBattleLog(std::move(line));
		}
		break;

	default:
		break;
	}

	doRemoveEffects(env, affectedUnits, std::bind(&DefaultSpellMechanics::counteringSelector, this, _1));

	for(auto & unit : affectedUnits)
		sc.affectedCres.insert(unit->unitId());

	env->sendAndApply(&sc);

	applyCastEffects(env, target);

	afterCast();

	if(sc.activeCast)
	{
		caster->spendMana(mode, owner, env, spellCost);
		if(sc.manaGained > 0)
		{
			assert(otherHero);
			otherHero->spendMana(Mode::HERO, owner, env, -sc.manaGained);
		}
	}
}

bool DefaultSpellMechanics::counteringSelector(const Bonus * bonus) const
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

void DefaultSpellMechanics::doRemoveEffects(const SpellCastEnvironment * env, const std::vector<const battle::Unit *> & targets, const CSelector & selector)
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
		env->sendAndApply(&sse);
}

} // namespace spells

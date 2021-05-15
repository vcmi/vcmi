/*
* AINodeStorage.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Actors.h"
#include "../../../CCallback.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapObjects/MapObjects.h"

#include "../Goals/VisitHero.h"

class ExchangeAction : public ISpecialAction
{
private:
	const CGHeroInstance * target;
	const CGHeroInstance * source;

public:
	ExchangeAction(const CGHeroInstance * target, const CGHeroInstance * source)
		:target(target), source(source)
	{ }

	virtual Goals::TSubgoal whatToDo(const HeroPtr & hero) const override
	{
		return Goals::sptr(Goals::VisitHero(target->id.getNum()).sethero(hero));
	}
};

ChainActor::ChainActor(const CGHeroInstance * hero, int chainMask)
	:hero(hero), isMovable(true), chainMask(chainMask), creatureSet(hero), carrierParent(nullptr), otherParent(nullptr)
{
	baseActor = static_cast<HeroActor *>(this);
	initialPosition = hero->visitablePos();
	layer = hero->boat ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND;
	initialMovement = hero->movement;
	initialTurn = 0;
	armyValue = hero->getTotalStrength();
}

ChainActor::ChainActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * heroArmy)
	:hero(carrier->hero), isMovable(true), creatureSet(heroArmy), initialPosition(-1),
	carrierParent(carrier), otherParent(other), chainMask(carrier->chainMask | other->chainMask)
{
	baseActor = static_cast<HeroActor *>(this);
	armyValue = hero->getFightingStrength() * heroArmy->getArmyStrength();
}

HeroActor::HeroActor(const CGHeroInstance * hero, int chainMask)
	:ChainActor(hero, chainMask)
{
	setupSpecialActors();
}

HeroActor::HeroActor(const ChainActor * carrier, const ChainActor * other)
	:ChainActor(
		carrier, 
		other,
		pickBestCreatures(carrier->creatureSet, other->creatureSet))
{
	setupSpecialActors();
	exchangeAction.reset(new ExchangeAction(carrier->hero, other->hero));
}

std::shared_ptr<ISpecialAction> ChainActor::getExchangeAction() const
{ 
	return baseActor->exchangeAction; 
}

void ChainActor::setBaseActor(HeroActor * base)
{
	baseActor = base;
	hero = base->hero;
	layer = base->layer;
	initialMovement = base->initialMovement;
	initialTurn = base->initialTurn;
	armyValue = base->armyValue;
	chainMask = base->chainMask;
	creatureSet = base->creatureSet;
	isMovable = base->isMovable;
}

void HeroActor::setupSpecialActors()
{
	auto allActors = std::vector<ChainActor *>{ this };

	for(ChainActor & specialActor : specialActors)
	{
		specialActor.setBaseActor(this);
		allActors.push_back(&specialActor);
	}

	for(int i = 0; i <= SPECIAL_ACTORS_COUNT; i++)
	{
		ChainActor * actor = allActors[i];

		actor->allowBattle = (i & 1) > 0;
		actor->allowSpellCast = (i & 2) > 0;
		actor->allowUseResources = (i & 4) > 0;
		actor->battleActor = allActors[i | 1];
		actor->castActor = allActors[i | 2];
		actor->resourceActor = allActors[i | 4];
	}
}

ChainActor * ChainActor::exchange(const ChainActor * other) const
{
	return baseActor->exchange(this, other);
}

bool ChainActor::canExchange(const ChainActor * other) const
{
	return baseActor->canExchange(other->baseActor);
}

namespace vstd
{
	template <class M, class Key, class F>
	typename M::mapped_type & getOrCompute(M &m, Key const& k, F f)
	{
		typedef typename M::mapped_type V;

		std::pair<typename M::iterator, bool> r = m.insert(typename M::value_type(k, V()));
		V &v = r.first->second;

		if(r.second)
			f(v);

		return v;
	}
}

bool HeroActor::canExchange(const HeroActor * other)
{
	return vstd::getOrCompute(canExchangeCache, other, [&](bool & result) {
		result = (chainMask & other->chainMask) == 0
			&& howManyReinforcementsCanGet(creatureSet, other->creatureSet) > armyValue / 10;
	});
}

ChainActor * HeroActor::exchange(const ChainActor * specialActor, const ChainActor * other)
{
	HeroActor * result;
	const HeroActor * otherBase = other->getBaseActor();

	if(vstd::contains(exchangeMap, otherBase))
		result = exchangeMap.at(otherBase);
	else 
	{
		// TODO: decide where to release this CCreatureSet and HeroActor. Probably custom ~ctor?
		result = new HeroActor(specialActor, other);
		exchangeMap[otherBase] = result;
	}

	if(specialActor == this)
		return result;

	int index = vstd::find_pos_if(specialActors, [specialActor](const ChainActor & actor) -> bool { 
		return &actor == specialActor;
	});

	return &result->specialActors[index];
}

CCreatureSet * HeroActor::pickBestCreatures(const CCreatureSet * army1, const CCreatureSet * army2) const
{
	CCreatureSet * target = new CCreatureSet();
	const CCreatureSet * armies[] = { army1, army2 };

	//we calculate total strength for each creature type available in armies
	std::map<const CCreature *, int> creToPower;
	for(auto armyPtr : armies)
	{
		for(auto & i : armyPtr->Slots())
		{
			creToPower[i.second->type] += i.second->count;
		}
	}
	//TODO - consider more than just power (ie morale penalty, hero specialty in certain stacks, etc)
	int armySize = creToPower.size();

	vstd::amin(armySize, GameConstants::ARMY_SIZE);

	for(int i = 0; i < armySize && !creToPower.empty(); i++) //pick the creatures from which we can get most power, as many as dest can fit
	{
		typedef const std::pair<const CCreature *, int> & CrePowerPair;
		auto creIt = boost::max_element(creToPower, [](CrePowerPair lhs, CrePowerPair rhs)
		{
			return lhs.first->AIValue * lhs.second < rhs.first->AIValue * rhs.second;
		});

		target->addToSlot(SlotID(i), creIt->first->idNumber, TQuantity(creIt->second));
		creToPower.erase(creIt);
	}

	return target;
}
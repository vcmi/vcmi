/*
 * TavernHeroesPool.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TavernHeroesPool.h"

#include "../mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

TavernHeroesPool::~TavernHeroesPool()
{
	for(auto ptr : heroesPool) // clean hero pool
		delete ptr.second;
}

std::map<HeroTypeID, CGHeroInstance*> TavernHeroesPool::unusedHeroesFromPool() const
{
	std::map<HeroTypeID, CGHeroInstance*> pool = heroesPool;
	for(const auto & player : currentTavern)
		for(auto availableHero : player.second)
			if(availableHero.second)
				pool.erase(HeroTypeID(availableHero.second->subID));

	return pool;
}

void TavernHeroesPool::setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army)
{
	currentTavern[player].erase(slot);

	if (hero == HeroTypeID::NONE)
		return;

	CGHeroInstance * h = heroesPool[hero];

	if (h && army)
		h->setToArmy(army);

	currentTavern[player][slot] = h;
}

bool TavernHeroesPool::isHeroAvailableFor(HeroTypeID hero, PlayerColor color) const
{
	if (pavailable.count(hero))
		return pavailable.at(hero) & (1 << color.getNum());

	return true;
}

std::vector<const CGHeroInstance *> TavernHeroesPool::getHeroesFor(PlayerColor color) const
{
	std::vector<const CGHeroInstance *> result;

	if(!currentTavern.count(color))
		return result;

	for(const auto & hero : currentTavern.at(color))
		result.push_back(hero.second);

	return result;
}

CGHeroInstance * TavernHeroesPool::takeHero(HeroTypeID hero)
{
	assert(heroesPool.count(hero));

	CGHeroInstance * result = heroesPool[hero];
	heroesPool.erase(hero);

	assert(result);
	return result;
}

void TavernHeroesPool::onNewDay()
{
	for(auto & hero : heroesPool)
	{
		assert(hero.second);
		if(!hero.second)
			continue;

		hero.second->setMovementPoints(hero.second->movementPointsLimit(true));
		hero.second->mana = hero.second->manaLimit();
	}
}

void TavernHeroesPool::addHeroToPool(CGHeroInstance * hero)
{
	heroesPool[HeroTypeID(hero->subID)] = hero;
}

void TavernHeroesPool::setAvailability(HeroTypeID hero, PlayerColor::Mask mask)
{
	pavailable[hero] = mask;
}

VCMI_LIB_NAMESPACE_END

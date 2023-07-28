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
#include "../CHeroHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

TavernHeroesPool::~TavernHeroesPool()
{
	for(const auto & ptr : heroesPool) // clean hero pool
		delete ptr.second;
}

std::map<HeroTypeID, CGHeroInstance*> TavernHeroesPool::unusedHeroesFromPool() const
{
	std::map<HeroTypeID, CGHeroInstance*> pool = heroesPool;
	for(const auto & slot : currentTavern)
		pool.erase(HeroTypeID(slot.hero->subID));

	return pool;
}

TavernSlotRole TavernHeroesPool::getSlotRole(HeroTypeID hero) const
{
	for (auto const & slot : currentTavern)
	{
		if (HeroTypeID(slot.hero->subID) == hero)
			return slot.role;
	}
	return TavernSlotRole::NONE;
}

void TavernHeroesPool::setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army, TavernSlotRole role)
{
	vstd::erase_if(currentTavern, [&](const TavernSlot & entry){
		return entry.player == player && entry.slot == slot;
	});

	if (hero == HeroTypeID::NONE)
		return;

	CGHeroInstance * h = heroesPool[hero];

	if (h && army)
		h->setToArmy(army);

	TavernSlot newSlot;
	newSlot.hero = h;
	newSlot.player = player;
	newSlot.role = role;
	newSlot.slot = slot;

	currentTavern.push_back(newSlot);

	boost::range::sort(currentTavern, [](const TavernSlot & left, const TavernSlot & right)
	{
		if (left.slot == right.slot)
			return left.player < right.player;
		else
			return left.slot < right.slot;
	});
}

bool TavernHeroesPool::isHeroAvailableFor(HeroTypeID hero, PlayerColor color) const
{
	if (perPlayerAvailability.count(hero))
		return perPlayerAvailability.at(hero) & (1 << color.getNum());

	return true;
}

std::vector<const CGHeroInstance *> TavernHeroesPool::getHeroesFor(PlayerColor color) const
{
	std::vector<const CGHeroInstance *> result;

	for(const auto & slot : currentTavern)
	{
		if (slot.player == color)
			result.push_back(slot.hero);
	}

	return result;
}

CGHeroInstance * TavernHeroesPool::takeHeroFromPool(HeroTypeID hero)
{
	assert(heroesPool.count(hero));

	CGHeroInstance * result = heroesPool[hero];
	heroesPool.erase(hero);

	vstd::erase_if(currentTavern, [&](const TavernSlot & entry){
		return entry.hero->type->getId() == hero;
	});

	assert(result);
	return result;
}

void TavernHeroesPool::onNewDay()
{
	auto unusedHeroes = unusedHeroesFromPool();

	for(auto & hero : heroesPool)
	{
		assert(hero.second);
		if(!hero.second)
			continue;

		// do not access heroes who are not present in tavern of any players
		if (vstd::contains(unusedHeroes, hero.first))
			continue;

		hero.second->setMovementPoints(hero.second->movementPointsLimit(true));
		hero.second->mana = hero.second->manaLimit();
	}

	for (auto & slot : currentTavern)
	{
		if (slot.role == TavernSlotRole::RETREATED_TODAY)
			slot.role = TavernSlotRole::RETREATED;

		if (slot.role == TavernSlotRole::SURRENDERED_TODAY)
			slot.role = TavernSlotRole::SURRENDERED;
	}
}

void TavernHeroesPool::addHeroToPool(CGHeroInstance * hero)
{
	heroesPool[HeroTypeID(hero->subID)] = hero;
}

void TavernHeroesPool::setAvailability(HeroTypeID hero, PlayerColor::Mask mask)
{
	perPlayerAvailability[hero] = mask;
}

VCMI_LIB_NAMESPACE_END

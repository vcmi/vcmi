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

std::map<HeroTypeID, CGHeroInstance*> TavernHeroesPool::unusedHeroesFromPool() const
{
	std::map<HeroTypeID, CGHeroInstance*> pool;

	for (const auto & hero : heroesPool)
		pool[hero.first] = hero.second.get();

	for(const auto & slot : currentTavern)
		pool.erase(slot.hero->getHeroTypeID());

	return pool;
}

TavernSlotRole TavernHeroesPool::getSlotRole(HeroTypeID hero) const
{
	for (auto const & slot : currentTavern)
	{
		if (slot.hero->getHeroTypeID() == hero)
			return slot.role;
	}
	return TavernSlotRole::NONE;
}

void TavernHeroesPool::setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army, TavernSlotRole role, bool replenishPoints)
{
	vstd::erase_if(currentTavern, [&](const TavernSlot & entry){
		return entry.player == player && entry.slot == slot;
	});

	if (hero == HeroTypeID::NONE)
		return;

	auto h = heroesPool[hero];

	if (h && army)
		h->setToArmy(army);

	if (h && replenishPoints)
	{
		h->setMovementPoints(h->movementPointsLimit(true));
		h->mana = h->manaLimit();
	}

	TavernSlot newSlot;
	newSlot.hero = h.get();
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
		return perPlayerAvailability.at(hero).count(color) != 0;

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

std::shared_ptr<CGHeroInstance> TavernHeroesPool::takeHeroFromPool(HeroTypeID hero)
{
	assert(heroesPool.count(hero));

	auto result = heroesPool[hero];
	heroesPool.erase(hero);

	vstd::erase_if(currentTavern, [&](const TavernSlot & entry){
		return entry.hero->getHeroTypeID() == hero;
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

		hero.second->removeBonusesRecursive(Bonus::OneDay);
		hero.second->reduceBonusDurations(Bonus::NDays);
		hero.second->reduceBonusDurations(Bonus::OneWeek);

		// do not access heroes who are not present in tavern of any players
		if (vstd::contains(unusedHeroes, hero.first))
			continue;

		hero.second->setMovementPoints(hero.second->movementPointsLimit(true));
		hero.second->mana = hero.second->getManaNewTurn();
	}
}

void TavernHeroesPool::addHeroToPool(std::shared_ptr<CGHeroInstance> hero)
{
	heroesPool[hero->getHeroTypeID()] = hero;
}

void TavernHeroesPool::setAvailability(HeroTypeID hero, std::set<PlayerColor> mask)
{
	perPlayerAvailability[hero] = mask;
}

VCMI_LIB_NAMESPACE_END

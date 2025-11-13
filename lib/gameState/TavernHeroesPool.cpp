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

#include "CGameState.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

TavernHeroesPool::TavernHeroesPool(CGameState * owner)
	: owner(owner)
{}

void TavernHeroesPool::setGameState(CGameState * newOwner)
{
	owner = newOwner;
}

std::map<HeroTypeID, CGHeroInstance*> TavernHeroesPool::unusedHeroesFromPool() const
{
	std::map<HeroTypeID, CGHeroInstance*> pool;

	for (const auto & hero : heroesPool)
		pool[hero] = owner->getMap().tryGetFromHeroPool(hero);

	for(const auto & slot : currentTavern)
		pool.erase(slot.hero);

	return pool;
}

TavernSlotRole TavernHeroesPool::getSlotRole(HeroTypeID hero) const
{
	for (auto const & slot : currentTavern)
	{
		if (slot.hero == hero)
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

	auto h = owner->getMap().tryGetFromHeroPool(hero);
	assert(h != nullptr);

	if (army)
		h->setToArmy(army);

	if (replenishPoints)
	{
		h->setMovementPoints(h->movementPointsLimit(true));
		h->mana = h->manaLimit();
	}

	TavernSlot newSlot;
	newSlot.hero = hero;
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
		assert(slot.hero.hasValue());
		if (slot.player == color)
			result.push_back(owner->getMap().tryGetFromHeroPool(slot.hero));
	}

	return result;
}

std::shared_ptr<CGHeroInstance> TavernHeroesPool::takeHeroFromPool(HeroTypeID hero)
{
	assert(vstd::contains(heroesPool, hero));
	vstd::erase(heroesPool, hero);
	vstd::erase_if(currentTavern, [&](const TavernSlot & entry){
		return entry.hero == hero;
	});

	return owner->getMap().tryTakeFromHeroPool(hero);
}

void TavernHeroesPool::onNewDay()
{
	auto unusedHeroes = unusedHeroesFromPool();

	for(auto & heroID : heroesPool)
	{
		auto heroPtr = owner->getMap().tryGetFromHeroPool(heroID);
		assert(heroPtr);

		heroPtr->removeBonusesRecursive(Bonus::OneDay);
		heroPtr->reduceBonusDurations(Bonus::NDays);
		heroPtr->reduceBonusDurations(Bonus::OneWeek);

		// do not access heroes who are not present in tavern of any players
		if (vstd::contains(unusedHeroes, heroID))
			continue;

		heroPtr->setMovementPoints(heroPtr->movementPointsLimit(true));
		heroPtr->mana = heroPtr->getManaNewTurn();
	}
}

void TavernHeroesPool::addHeroToPool(HeroTypeID hero)
{
	assert(!vstd::contains(heroesPool, hero));
	heroesPool.push_back(hero);
}

void TavernHeroesPool::setAvailability(HeroTypeID hero, std::set<PlayerColor> mask)
{
	perPlayerAvailability[hero] = mask;
}

VCMI_LIB_NAMESPACE_END

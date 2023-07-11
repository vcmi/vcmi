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
#include "CPlayerState.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../CHeroHandler.h"

TavernHeroesPool::TavernHeroesPool() = default;

TavernHeroesPool::TavernHeroesPool(CGameState * gameState)
	: gameState(gameState)
{
}

TavernHeroesPool::~TavernHeroesPool()
{
	for(auto ptr : heroesPool) // clean hero pool
		delete ptr.second;
}

std::map<HeroTypeID, CGHeroInstance*> TavernHeroesPool::unusedHeroesFromPool()
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

CGHeroInstance * TavernHeroesPool::pickHeroFor(TavernHeroSlot slot,
													 const PlayerColor & player,
													 const FactionID & factionID,
													 CRandomGenerator & rand,
													 const CHeroClass * bannedClass) const
{
	if(player>=PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Cannot pick hero for player %d. Wrong owner!", player.getStr());
		return nullptr;
	}

	if(slot == TavernHeroSlot::NATIVE)
	{
		std::vector<CGHeroInstance *> pool;

		for(auto & elem : heroesPool)
		{
			//get all available heroes
			bool heroAvailable = isHeroAvailableFor(elem.first, player);
			bool heroClassNative = elem.second->type->heroClass->faction == factionID;

			if(heroAvailable && heroClassNative)
				pool.push_back(elem.second);
		}

		if(!pool.empty())
			return *RandomGeneratorUtil::nextItem(pool, rand);

		logGlobal->error("Cannot pick native hero for %s. Picking any...", player.getStr());
	}

	std::vector<CGHeroInstance *> pool;
	int totalWeight = 0;

	for(auto & elem : heroesPool)
	{
		bool heroAvailable = isHeroAvailableFor(elem.first, player);
		bool heroClassBanned = bannedClass && elem.second->type->heroClass == bannedClass;

		if ( heroAvailable && !heroClassBanned)
		{
			pool.push_back(elem.second);
			totalWeight += elem.second->type->heroClass->selectionProbability[factionID]; //total weight
		}
	}
	if(pool.empty() || totalWeight == 0)
	{
		logGlobal->error("There are no heroes available for player %s!", player.getStr());
		return nullptr;
	}

	int roll = rand.nextInt(totalWeight - 1);
	for (auto & elem : pool)
	{
		roll -= elem->type->heroClass->selectionProbability[factionID];
		if(roll < 0)
		{
			return elem;
		}
	}

	return pool.back();
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

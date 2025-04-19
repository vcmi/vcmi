/*
 * HeroPoolProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HeroPoolProcessor.h"

#include "TurnOrderProcessor.h"
#include "../CGameHandler.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/TavernHeroesPool.h"
#include "../../lib/gameState/TavernSlot.h"
#include "../../lib/IGameSettings.h"

HeroPoolProcessor::HeroPoolProcessor(CGameHandler * gameHandler)
	: gameHandler(gameHandler)
{
}

TavernHeroSlot HeroPoolProcessor::selectSlotForRole(const PlayerColor & player, TavernSlotRole roleID)
{
	const auto & heroesPool = gameHandler->gameState().heroesPool;

	const auto & heroes = heroesPool->getHeroesFor(player);

	// if tavern has empty slot - use it
	if (heroes.size() == 0)
		return TavernHeroSlot::NATIVE;

	if (heroes.size() == 1)
		return TavernHeroSlot::RANDOM;

	// try to find "better" slot to overwrite
	// we want to avoid overwriting retreated heroes when tavern still has slot with random hero
	// as well as avoid overwriting surrendered heroes if we can overwrite retreated hero
	auto roleLeft = heroesPool->getSlotRole(heroes[0]->getHeroTypeID());
	auto roleRight = heroesPool->getSlotRole(heroes[1]->getHeroTypeID());

	if (roleLeft > roleRight)
		return TavernHeroSlot::RANDOM;

	if (roleLeft < roleRight)
		return TavernHeroSlot::NATIVE;

	// both slots are equal in "value", so select randomly
	if (getRandomGenerator(player).nextInt(100) > 50)
		return TavernHeroSlot::RANDOM;
	else
		return TavernHeroSlot::NATIVE;
}

void HeroPoolProcessor::onHeroSurrendered(const PlayerColor & color, const CGHeroInstance * hero)
{
	SetAvailableHero sah;
	sah.roleID = TavernSlotRole::SURRENDERED;

	sah.slotID = selectSlotForRole(color, sah.roleID);
	sah.player = color;
	sah.hid = hero->getHeroTypeID();
	sah.replenishPoints = false;
	gameHandler->sendAndApply(sah);
}

void HeroPoolProcessor::onHeroEscaped(const PlayerColor & color, const CGHeroInstance * hero)
{
	SetAvailableHero sah;
	sah.roleID = TavernSlotRole::RETREATED;

	sah.slotID = selectSlotForRole(color, sah.roleID);
	sah.player = color;
	sah.hid = hero->getHeroTypeID();
	sah.army.clearSlots();
	sah.army.setCreature(SlotID(0), hero->getHeroType()->initialArmy.at(0).creature, 1);
	sah.replenishPoints = false;

	gameHandler->sendAndApply(sah);
}

void HeroPoolProcessor::clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot)
{
	SetAvailableHero sah;
	sah.player = color;
	sah.roleID = TavernSlotRole::NONE;
	sah.slotID = slot;
	sah.hid = HeroTypeID::NONE;
	sah.replenishPoints = false;
	gameHandler->sendAndApply(sah);
}

void HeroPoolProcessor::selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot, bool needNativeHero, bool giveArmy, const HeroTypeID & nextHero)
{
	SetAvailableHero sah;
	sah.player = color;
	sah.slotID = slot;
	sah.replenishPoints = true;

	CGHeroInstance *newHero = nextHero.hasValue()?
		gameHandler->gameState().heroesPool->unusedHeroesFromPool()[nextHero]:
		pickHeroFor(needNativeHero, color);

	if (newHero)
	{
		sah.hid = newHero->getHeroTypeID();

		if (giveArmy)
		{
			sah.roleID = TavernSlotRole::FULL_ARMY;
			newHero->initArmy(getRandomGenerator(color), &sah.army);
		}
		else
		{
			sah.roleID = TavernSlotRole::SINGLE_UNIT;
			sah.army.clearSlots();
			sah.army.setCreature(SlotID(0), newHero->getHeroType()->initialArmy[0].creature, 1);
		}
	}
	else
	{
		sah.hid = HeroTypeID::NONE;
	}

	gameHandler->sendAndApply(sah);
}

void HeroPoolProcessor::onNewWeek(const PlayerColor & color)
{
	clearHeroFromSlot(color, TavernHeroSlot::NATIVE);
	clearHeroFromSlot(color, TavernHeroSlot::RANDOM);
	selectNewHeroForSlot(color, TavernHeroSlot::NATIVE, true, true);
	selectNewHeroForSlot(color, TavernHeroSlot::RANDOM, false, true);
}

bool HeroPoolProcessor::hireHero(const ObjectInstanceID & objectID, const HeroTypeID & heroToRecruit, const PlayerColor & player, const HeroTypeID & nextHero)
{
	const PlayerState * playerState = gameHandler->getPlayerState(player);
	const CGObjectInstance * mapObject = gameHandler->getObj(objectID);
	const CGTownInstance * town = gameHandler->getTown(objectID);
	const auto & heroesPool = gameHandler->gameState().heroesPool;

	if (!mapObject && gameHandler->complain("Invalid map object!"))
		return false;

	if (!playerState && gameHandler->complain("Invalid player!"))
		return false;

	if (playerState->resources[EGameResID::GOLD] < GameConstants::HERO_GOLD_COST && gameHandler->complain("Not enough gold for buying hero!"))
		return false;

	if (gameHandler->getHeroCount(player, false) >= gameHandler->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP) && gameHandler->complain("Cannot hire hero, too many wandering heroes already!"))
		return false;

	if (gameHandler->getHeroCount(player, true) >= gameHandler->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP) && gameHandler->complain("Cannot hire hero, too many heroes garrizoned and wandering already!"))
		return false;

	if (nextHero != HeroTypeID::NONE) // player attempts to invite next hero
	{
		if(!gameHandler->getSettings().getBoolean(EGameSettings::HEROES_TAVERN_INVITE) && gameHandler->complain("Inviting heroes not allowed!"))
			return false;

		if(!heroesPool->unusedHeroesFromPool().count(nextHero) && gameHandler->complain("Cannot invite specified hero!"))
			return false;
		
		if(!heroesPool->isHeroAvailableFor(nextHero, player) && gameHandler->complain("Cannot invite specified hero!"))
			return false;
	}

	if(town) //tavern in town
	{
		if(gameHandler->getPlayerRelations(mapObject->tempOwner, player) == PlayerRelations::ENEMIES && gameHandler->complain("Can't buy hero in enemy town!"))
			return false;

		if(!town->hasBuilt(BuildingID::TAVERN) && gameHandler->complain("No tavern!"))
			return false;

		if(town->getVisitingHero() && gameHandler->complain("There is visiting hero - no place!"))
			return false;
	}

	if(mapObject->ID == Obj::TAVERN)
	{
		const CGHeroInstance * visitor = gameHandler->getVisitingHero(mapObject);

		if (!visitor || visitor->getOwner() != player)
		{
			gameHandler->complain("Can't buy hero in tavern not being visited!");
			return false;
		}

		if(gameHandler->getTile(mapObject->visitablePos())->visitableObjects.back() != mapObject->id && gameHandler->complain("Tavern entry must be unoccupied!"))
			return false;
	}

	auto recruitableHeroes = heroesPool->getHeroesFor(player);

	const CGHeroInstance * recruitedHero = nullptr;

	for(const auto & hero : recruitableHeroes)
	{
		if(hero->getHeroTypeID() == heroToRecruit)
			recruitedHero = hero;
	}

	if(!recruitedHero)
	{
		gameHandler->complain("Hero is not available for hiring!");
		return false;
	}
	const auto targetPos = mapObject->visitablePos();

	HeroRecruited hr;
	hr.tid = mapObject->id;
	hr.hid = recruitedHero->getHeroTypeID();
	hr.player = player;
	hr.tile = recruitedHero->convertFromVisitablePos(targetPos );
	if(gameHandler->getTile(targetPos)->isWater() && !recruitedHero->inBoat())
	{
		//Create a new boat for hero
		gameHandler->createBoat(targetPos, recruitedHero->getBoatType(), player);
		hr.boatId = gameHandler->getTopObj(targetPos)->id;
	}

	// apply netpack -> this will remove hired hero from pool
	gameHandler->sendAndApply(hr);

	if(recruitableHeroes[0] == recruitedHero)
		selectNewHeroForSlot(player, TavernHeroSlot::NATIVE, false, false, nextHero);
	else
		selectNewHeroForSlot(player, TavernHeroSlot::RANDOM, false, false, nextHero);

	gameHandler->giveResource(player, EGameResID::GOLD, -GameConstants::HERO_GOLD_COST);

	if(town)
		gameHandler->objectVisited(town, recruitedHero);

	// If new hero has scouting he might reveal more terrain than we saw before
	gameHandler->changeFogOfWar(recruitedHero->getSightCenter(), recruitedHero->getSightRadius(), player, ETileVisibility::REVEALED);
	return true;
}

std::vector<const CHeroClass *> HeroPoolProcessor::findAvailableClassesFor(const PlayerColor & player) const
{
	std::vector<const CHeroClass *> result;

	const auto & heroesPool = gameHandler->gameState().heroesPool;
	FactionID factionID = gameHandler->getPlayerSettings(player)->castle;

	for(const auto & elem : heroesPool->unusedHeroesFromPool())
	{
		if (vstd::contains(result, elem.second->getHeroClass()))
			continue;

		bool heroAvailable = heroesPool->isHeroAvailableFor(elem.first, player);
		bool heroClassBanned = elem.second->getHeroClass()->tavernProbability(factionID) == 0;

		if(heroAvailable && !heroClassBanned)
			result.push_back(elem.second->getHeroClass());
	}

	return result;
}

std::vector<CGHeroInstance *> HeroPoolProcessor::findAvailableHeroesFor(const PlayerColor & player, const CHeroClass * heroClass) const
{
	std::vector<CGHeroInstance *> result;

	const auto & heroesPool = gameHandler->gameState().heroesPool;

	for(const auto & elem : heroesPool->unusedHeroesFromPool())
	{
		assert(!vstd::contains(result, elem.second));

		bool heroAvailable = heroesPool->isHeroAvailableFor(elem.first, player);
		bool heroClassMatches = elem.second->getHeroClass() == heroClass;

		if(heroAvailable && heroClassMatches)
			result.push_back(elem.second);
	}

	return result;
}

const CHeroClass * HeroPoolProcessor::pickClassFor(bool isNative, const PlayerColor & player)
{
	if(!player.isValidPlayer())
	{
		logGlobal->error("Cannot pick hero for player %d. Wrong owner!", player.toString());
		return nullptr;
	}

	FactionID factionID = gameHandler->getPlayerSettings(player)->castle;
	const auto & heroesPool = gameHandler->gameState().heroesPool;
	const auto & currentTavern = heroesPool->getHeroesFor(player);

	std::vector<const CHeroClass *> potentialClasses = findAvailableClassesFor(player);
	std::vector<const CHeroClass *> possibleClasses;

	if(potentialClasses.empty())
	{
		logGlobal->error("There are no heroes available for player %s!", player.toString());
		return nullptr;
	}

	for(const auto & heroClass : potentialClasses)
	{
		if (isNative && heroClass->faction != factionID)
			continue;

		bool hasSameClass = vstd::contains_if(currentTavern, [&](const CGHeroInstance * hero){
			return hero->getHeroClass() == heroClass;
		});

		if (hasSameClass)
			continue;

		possibleClasses.push_back(heroClass);
	}

	if (possibleClasses.empty())
	{
		logGlobal->error("Cannot pick native hero for %s. Picking any...", player.toString());
		possibleClasses = potentialClasses;
	}

	int totalWeight = 0;
	for(const auto & heroClass : possibleClasses)
		totalWeight += heroClass->tavernProbability(factionID);

	int roll = getRandomGenerator(player).nextInt(totalWeight - 1);

	for(const auto & heroClass : possibleClasses)
	{
		roll -= heroClass->tavernProbability(factionID);
		if(roll < 0)
			return heroClass;
	}

	return *possibleClasses.rbegin();
}

CGHeroInstance * HeroPoolProcessor::pickHeroFor(bool isNative, const PlayerColor & player)
{
	const CHeroClass * heroClass = pickClassFor(isNative, player);

	if(!heroClass)
		return nullptr;

	std::vector<CGHeroInstance *> possibleHeroes = findAvailableHeroesFor(player, heroClass);

	assert(!possibleHeroes.empty());
	if(possibleHeroes.empty())
		return nullptr;

	return *RandomGeneratorUtil::nextItem(possibleHeroes, getRandomGenerator(player));
}

vstd::RNG & HeroPoolProcessor::getHeroSkillsRandomGenerator(const HeroTypeID & hero)
{
	if (heroSeed.count(hero) == 0)
	{
		int seed = gameHandler->getRandomGenerator().nextInt();
		heroSeed.emplace(hero, std::make_unique<CRandomGenerator>(seed));
	}

	return *heroSeed.at(hero);
}

vstd::RNG & HeroPoolProcessor::getRandomGenerator(const PlayerColor & player)
{
	if (playerSeed.count(player) == 0)
	{
		int seed = gameHandler->getRandomGenerator().nextInt();
		playerSeed.emplace(player, std::make_unique<CRandomGenerator>(seed));
	}

	return *playerSeed.at(player);
}

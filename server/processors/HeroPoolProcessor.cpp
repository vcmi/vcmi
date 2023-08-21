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

#include "../CGameHandler.h"

#include "../../lib/CHeroHandler.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/GameSettings.h"
#include "../../lib/NetPacks.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/TavernHeroesPool.h"
#include "../../lib/gameState/TavernSlot.h"

HeroPoolProcessor::HeroPoolProcessor()
	: gameHandler(nullptr)
{
}

HeroPoolProcessor::HeroPoolProcessor(CGameHandler * gameHandler)
	: gameHandler(gameHandler)
{
}

bool HeroPoolProcessor::playerEndedTurn(const PlayerColor & player)
{
	// our player is acting right now and have not ended turn
	if (player == gameHandler->gameState()->currentPlayer)
		return false;

	auto turnOrder = gameHandler->generatePlayerTurnOrder();

	for (auto const & entry : turnOrder)
	{
		// our player is yet to start turn
		if (entry == gameHandler->gameState()->currentPlayer)
			return false;

		// our player have finished turn
		if (entry == player)
			return true;
	}

	assert(false);
	return false;
}

TavernHeroSlot HeroPoolProcessor::selectSlotForRole(const PlayerColor & player, TavernSlotRole roleID)
{
	const auto & heroesPool = gameHandler->gameState()->heroesPool;

	const auto & heroes = heroesPool->getHeroesFor(player);

	// if tavern has empty slot - use it
	if (heroes.size() == 0)
		return TavernHeroSlot::NATIVE;

	if (heroes.size() == 1)
		return TavernHeroSlot::RANDOM;

	// try to find "better" slot to overwrite
	// we want to avoid overwriting retreated heroes when tavern still has slot with random hero
	// as well as avoid overwriting surrendered heroes if we can overwrite retreated hero
	auto roleLeft = heroesPool->getSlotRole(HeroTypeID(heroes[0]->subID));
	auto roleRight = heroesPool->getSlotRole(HeroTypeID(heroes[1]->subID));

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
	if (playerEndedTurn(color))
		sah.roleID = TavernSlotRole::SURRENDERED_TODAY;
	else
		sah.roleID = TavernSlotRole::SURRENDERED;

	sah.slotID = selectSlotForRole(color, sah.roleID);
	sah.player = color;
	sah.hid = hero->subID;
	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::onHeroEscaped(const PlayerColor & color, const CGHeroInstance * hero)
{
	SetAvailableHero sah;
	if (playerEndedTurn(color))
		sah.roleID = TavernSlotRole::RETREATED_TODAY;
	else
		sah.roleID = TavernSlotRole::RETREATED;

	sah.slotID = selectSlotForRole(color, sah.roleID);
	sah.player = color;
	sah.hid = hero->subID;
	sah.army.clearSlots();
	sah.army.setCreature(SlotID(0), hero->type->initialArmy.at(0).creature, 1);

	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot)
{
	SetAvailableHero sah;
	sah.player = color;
	sah.roleID = TavernSlotRole::NONE;
	sah.slotID = slot;
	sah.hid = HeroTypeID::NONE;
	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot, bool needNativeHero, bool giveArmy)
{
	SetAvailableHero sah;
	sah.player = color;
	sah.slotID = slot;

	CGHeroInstance *newHero = pickHeroFor(needNativeHero, color);

	if (newHero)
	{
		sah.hid = newHero->subID;

		if (giveArmy)
		{
			sah.roleID = TavernSlotRole::FULL_ARMY;
			newHero->initArmy(getRandomGenerator(color), &sah.army);
		}
		else
		{
			sah.roleID = TavernSlotRole::SINGLE_UNIT;
			sah.army.clearSlots();
			sah.army.setCreature(SlotID(0), newHero->type->initialArmy[0].creature, 1);
		}
	}
	else
	{
		sah.hid = -1;
	}
	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::onNewWeek(const PlayerColor & color)
{
	const auto & heroesPool = gameHandler->gameState()->heroesPool;
	const auto & heroes = heroesPool->getHeroesFor(color);

	const auto nativeSlotRole = heroes.size() < 1 ? TavernSlotRole::NONE : heroesPool->getSlotRole(heroes[0]->type->getId());
	const auto randomSlotRole = heroes.size() < 2 ? TavernSlotRole::NONE : heroesPool->getSlotRole(heroes[1]->type->getId());

	bool resetNativeSlot = nativeSlotRole != TavernSlotRole::RETREATED_TODAY && nativeSlotRole != TavernSlotRole::SURRENDERED_TODAY;
	bool resetRandomSlot = randomSlotRole != TavernSlotRole::RETREATED_TODAY && randomSlotRole != TavernSlotRole::SURRENDERED_TODAY;

	if (resetNativeSlot)
		clearHeroFromSlot(color, TavernHeroSlot::NATIVE);

	if (resetRandomSlot)
		clearHeroFromSlot(color, TavernHeroSlot::RANDOM);

	if (resetNativeSlot)
		selectNewHeroForSlot(color, TavernHeroSlot::NATIVE, true, true);

	if (resetRandomSlot)
		selectNewHeroForSlot(color, TavernHeroSlot::RANDOM, false, true);
}

bool HeroPoolProcessor::hireHero(const ObjectInstanceID & objectID, const HeroTypeID & heroToRecruit, const PlayerColor & player)
{
	const PlayerState * playerState = gameHandler->getPlayerState(player);
	const CGObjectInstance * mapObject = gameHandler->getObj(objectID);
	const CGTownInstance * town = gameHandler->getTown(objectID);

	if (!mapObject && gameHandler->complain("Invalid map object!"))
		return false;

	if (!playerState && gameHandler->complain("Invalid player!"))
		return false;

	if (playerState->resources[EGameResID::GOLD] < GameConstants::HERO_GOLD_COST && gameHandler->complain("Not enough gold for buying hero!"))
		return false;

	if (gameHandler->getHeroCount(player, false) >= VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP) && gameHandler->complain("Cannot hire hero, too many wandering heroes already!"))
		return false;

	if (gameHandler->getHeroCount(player, true) >= VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP) && gameHandler->complain("Cannot hire hero, too many heroes garrizoned and wandering already!"))
		return false;

	if(town) //tavern in town
	{
		if(gameHandler->getPlayerRelations(mapObject->tempOwner, player) == PlayerRelations::ENEMIES && gameHandler->complain("Can't buy hero in enemy town!"))
			return false;

		if(!town->hasBuilt(BuildingID::TAVERN) && gameHandler->complain("No tavern!"))
			return false;

		if(town->visitingHero && gameHandler->complain("There is visiting hero - no place!"))
			return false;
	}

	if(mapObject->ID == Obj::TAVERN)
	{
		if(gameHandler->getTile(mapObject->visitablePos())->visitableObjects.back() != mapObject && gameHandler->complain("Tavern entry must be unoccupied!"))
			return false;
	}

	auto recruitableHeroes = gameHandler->gameState()->heroesPool->getHeroesFor(player);

	const CGHeroInstance * recruitedHero = nullptr;

	for(const auto & hero : recruitableHeroes)
	{
		if(hero->subID == heroToRecruit)
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
	hr.hid = recruitedHero->subID;
	hr.player = player;
	hr.tile = recruitedHero->convertFromVisitablePos(targetPos );
	if(gameHandler->getTile(targetPos)->isWater() && !recruitedHero->boat)
	{
		//Create a new boat for hero
		gameHandler->createObject(targetPos , Obj::BOAT, recruitedHero->getBoatType().getNum());

		hr.boatId = gameHandler->getTopObj(targetPos)->id;
	}

	// apply netpack -> this will remove hired hero from pool
	gameHandler->sendAndApply(&hr);

	if(recruitableHeroes[0] == recruitedHero)
		selectNewHeroForSlot(player, TavernHeroSlot::NATIVE, false, false);
	else
		selectNewHeroForSlot(player, TavernHeroSlot::RANDOM, false, false);

	gameHandler->giveResource(player, EGameResID::GOLD, -GameConstants::HERO_GOLD_COST);

	if(town)
	{
		gameHandler->visitCastleObjects(town, recruitedHero);
		gameHandler->giveSpells(town, recruitedHero);
	}
	return true;
}

std::vector<const CHeroClass *> HeroPoolProcessor::findAvailableClassesFor(const PlayerColor & player) const
{
	std::vector<const CHeroClass *> result;

	const auto & heroesPool = gameHandler->gameState()->heroesPool;
	FactionID factionID = gameHandler->getPlayerSettings(player)->castle;

	for(auto & elem : heroesPool->unusedHeroesFromPool())
	{
		if (vstd::contains(result, elem.second->type->heroClass))
			continue;

		bool heroAvailable = heroesPool->isHeroAvailableFor(elem.first, player);
		bool heroClassBanned = elem.second->type->heroClass->selectionProbability[factionID] == 0;

		if(heroAvailable && !heroClassBanned)
			result.push_back(elem.second->type->heroClass);
	}

	return result;
}

std::vector<CGHeroInstance *> HeroPoolProcessor::findAvailableHeroesFor(const PlayerColor & player, const CHeroClass * heroClass) const
{
	std::vector<CGHeroInstance *> result;

	const auto & heroesPool = gameHandler->gameState()->heroesPool;

	for(auto & elem : heroesPool->unusedHeroesFromPool())
	{
		assert(!vstd::contains(result, elem.second));

		bool heroAvailable = heroesPool->isHeroAvailableFor(elem.first, player);
		bool heroClassMatches = elem.second->type->heroClass == heroClass;

		if(heroAvailable && heroClassMatches)
			result.push_back(elem.second);
	}

	return result;
}

const CHeroClass * HeroPoolProcessor::pickClassFor(bool isNative, const PlayerColor & player)
{
	if(player >= PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Cannot pick hero for player %d. Wrong owner!", player.getStr());
		return nullptr;
	}

	FactionID factionID = gameHandler->getPlayerSettings(player)->castle;
	const auto & heroesPool = gameHandler->gameState()->heroesPool;
	const auto & currentTavern = heroesPool->getHeroesFor(player);

	std::vector<const CHeroClass *> potentialClasses = findAvailableClassesFor(player);
	std::vector<const CHeroClass *> possibleClasses;

	if(potentialClasses.empty())
	{
		logGlobal->error("There are no heroes available for player %s!", player.getStr());
		return nullptr;
	}

	for(const auto & heroClass : potentialClasses)
	{
		if (isNative && heroClass->faction != factionID)
			continue;

		bool hasSameClass = vstd::contains_if(currentTavern, [&](const CGHeroInstance * hero){
			return hero->type->heroClass == heroClass;
		});

		if (hasSameClass)
			continue;

		possibleClasses.push_back(heroClass);
	}

	if (possibleClasses.empty())
	{
		logGlobal->error("Cannot pick native hero for %s. Picking any...", player.getStr());
		possibleClasses = potentialClasses;
	}

	int totalWeight = 0;
	for(const auto & heroClass : possibleClasses)
		totalWeight += heroClass->selectionProbability.at(factionID);

	int roll = getRandomGenerator(player).nextInt(totalWeight - 1);

	for(const auto & heroClass : possibleClasses)
	{
		roll -= heroClass->selectionProbability.at(factionID);
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

CRandomGenerator & HeroPoolProcessor::getRandomGenerator(const PlayerColor & player)
{
	if (playerSeed.count(player) == 0)
	{
		int seed = gameHandler->getRandomGenerator().nextInt();
		playerSeed.emplace(player, std::make_unique<CRandomGenerator>(seed));
	}

	return *playerSeed.at(player);
}

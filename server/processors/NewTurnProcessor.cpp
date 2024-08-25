/*
 * NewTurnProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NewTurnProcessor.h"

#include "HeroPoolProcessor.h"

#include "../CGameHandler.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/GameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/IOwnableObject.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/pathfinder/TurnInfo.h"

#include <vstd/RNG.h>

NewTurnProcessor::NewTurnProcessor(CGameHandler * gameHandler)
	:gameHandler(gameHandler)
{
}

void NewTurnProcessor::onPlayerTurnStarted(PlayerColor which)
{
	const auto * playerState = gameHandler->gameState()->getPlayerState(which);

	gameHandler->handleTimeEvents(which);
	for (const auto * t : playerState->getTowns())
		gameHandler->handleTownEvents(t);

	for (const auto * t : playerState->getTowns())
	{
		//garrison hero first - consistent with original H3 Mana Vortex and Battle Scholar Academy levelup windows order
		if (t->garrisonHero != nullptr)
			gameHandler->objectVisited(t, t->garrisonHero);

		if (t->visitingHero != nullptr)
			gameHandler->objectVisited(t, t->visitingHero);
	}
}

void NewTurnProcessor::onPlayerTurnEnded(PlayerColor which)
{
	const auto * playerState = gameHandler->gameState()->getPlayerState(which);
	assert(playerState->status == EPlayerStatus::INGAME);

	if (playerState->getTowns().empty())
	{
		DaysWithoutTown pack;
		pack.player = which;
		pack.daysWithoutCastle = playerState->daysWithoutCastle.value_or(0) + 1;
		gameHandler->sendAndApply(&pack);
	}
	else
	{
		if (playerState->daysWithoutCastle.has_value())
		{
			DaysWithoutTown pack;
			pack.player = which;
			pack.daysWithoutCastle = std::nullopt;
			gameHandler->sendAndApply(&pack);
		}
	}

	// check for 7 days without castle
	gameHandler->checkVictoryLossConditionsForPlayer(which);

	bool newWeek = gameHandler->getDate(Date::DAY_OF_WEEK) == 7; // end of 7th day

	if (newWeek) //new heroes in tavern
		gameHandler->heroPool->onNewWeek(which);
}

ResourceSet NewTurnProcessor::generatePlayerIncome(PlayerColor playerID, bool newWeek)
{
	const auto & playerSettings = gameHandler->getPlayerSettings(playerID);
	const PlayerState & state = gameHandler->gameState()->players.at(playerID);
	ResourceSet income;

	for (const auto & town : state.getTowns())
	{
		if (newWeek && town->hasBuilt(BuildingSubID::TREASURY))
		{
			//give 10% of starting gold
			income[EGameResID::GOLD] += state.resources[EGameResID::GOLD] / 10;
		}

		//give resources if there's a Mystic Pond
		if (newWeek && town->hasBuilt(BuildingSubID::MYSTIC_POND))
		{
			static constexpr std::array rareResources = {
				GameResID::MERCURY,
				GameResID::SULFUR,
				GameResID::CRYSTAL,
				GameResID::GEMS
			};

			auto resID = *RandomGeneratorUtil::nextItem(rareResources, gameHandler->getRandomGenerator());
			int resVal = gameHandler->getRandomGenerator().nextInt(1, 4);

			income[resID] += resVal;

			gameHandler->setObjPropertyValue(town->id, ObjProperty::BONUS_VALUE_FIRST, resID);
			gameHandler->setObjPropertyValue(town->id, ObjProperty::BONUS_VALUE_SECOND, resVal);
		}
	}

	for (GameResID k = GameResID::WOOD; k < GameResID::COUNT; k++)
	{
		income += state.valOfBonuses(BonusType::RESOURCES_CONSTANT_BOOST, BonusSubtypeID(k));
		income += state.valOfBonuses(BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST, BonusSubtypeID(k)) * state.getTowns().size();
	}

	if(newWeek) //weekly crystal generation if 1 or more crystal dragons in any hero army or town garrison
	{
		bool hasCrystalGenCreature = false;
		for (const auto & hero : state.getHeroes())
			for(auto stack : hero->stacks)
				if(stack.second->hasBonusOfType(BonusType::SPECIAL_CRYSTAL_GENERATION))
					hasCrystalGenCreature = true;

		for(const auto & town : state.getTowns())
			for(auto stack : town->stacks)
				if(stack.second->hasBonusOfType(BonusType::SPECIAL_CRYSTAL_GENERATION))
					hasCrystalGenCreature = true;

		if(hasCrystalGenCreature)
			income[EGameResID::CRYSTAL] += 3;
	}

	TResources incomeHandicapped = income;
	incomeHandicapped.applyHandicap(playerSettings->handicap.percentIncome);

	for (auto obj :	state.getOwnedObjects())
		incomeHandicapped += obj->asOwnable()->dailyIncome();

	return incomeHandicapped;
}

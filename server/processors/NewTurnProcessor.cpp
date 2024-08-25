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
#include "../../lib/gameState/SThievesGuildInfo.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/IOwnableObject.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/pathfinder/TurnInfo.h"
#include "../../lib/texts/CGeneralTextHandler.h"

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

SetAvailableCreatures NewTurnProcessor::generateTownGrowth(const CGTownInstance * t, EWeekType weekType, CreatureID creatureWeek, bool firstDay)
{
	SetAvailableCreatures sac;
	PlayerColor player = t->tempOwner;

	sac.tid = t->id;
	sac.creatures = t->creatures;

	for (int k=0; k < t->town->creatures.size(); k++)
	{
		if (t->creatures.at(k).second.empty())
			continue;

		uint32_t creaturesBefore = t->creatures.at(k).first;
		uint32_t creatureGrowth = 0;
		const CCreature *cre = t->creatures.at(k).second.back().toCreature();

		if (firstDay)
		{
			creatureGrowth = cre->getGrowth();
		}
		else
		{
			creatureGrowth = t->creatureGrowth(k);

			//Deity of fire week - upgrade both imps and upgrades
			if (weekType == EWeekType::DEITYOFFIRE && vstd::contains(t->creatures.at(k).second, creatureWeek))
				creatureGrowth += 15;

			//bonus week, effect applies only to identical creatures
			if (weekType == EWeekType::BONUS_GROWTH && cre->getId() == creatureWeek)
				creatureGrowth += 5;
		}

		// Neutral towns have halved creature growth
		if (!player.isValidPlayer())
			creatureGrowth /= 2;

		uint32_t resultingCreatures = 0;

		if (weekType == EWeekType::PLAGUE)
			resultingCreatures = creaturesBefore / 2;
		else if (weekType == EWeekType::DOUBLE_GROWTH)
			resultingCreatures = (creaturesBefore + creatureGrowth) * 2;
		else
			resultingCreatures = creaturesBefore + creatureGrowth;

		sac.creatures.at(k).first = resultingCreatures;
	}

	return sac;
}

RumorState NewTurnProcessor::pickNewRumor()
{
	RumorState newRumor;

	static const std::vector<RumorState::ERumorType> rumorTypes = {RumorState::TYPE_MAP, RumorState::TYPE_SPECIAL, RumorState::TYPE_RAND, RumorState::TYPE_RAND};
	std::vector<RumorState::ERumorTypeSpecial> sRumorTypes = {
															  RumorState::RUMOR_OBELISKS, RumorState::RUMOR_ARTIFACTS, RumorState::RUMOR_ARMY, RumorState::RUMOR_INCOME};
	if(gameHandler->gameState()->map->grailPos.valid()) // Grail should always be on map, but I had related crash I didn't manage to reproduce
		sRumorTypes.push_back(RumorState::RUMOR_GRAIL);

	int rumorId = -1;
	int rumorExtra = -1;
	auto & rand = gameHandler->getRandomGenerator();
	newRumor.type = *RandomGeneratorUtil::nextItem(rumorTypes, rand);

	do
	{
		switch(newRumor.type)
		{
			case RumorState::TYPE_SPECIAL:
			{
				SThievesGuildInfo tgi;
				gameHandler->gameState()->obtainPlayersStats(tgi, 20);
				rumorId = *RandomGeneratorUtil::nextItem(sRumorTypes, rand);
				if(rumorId == RumorState::RUMOR_GRAIL)
				{
					rumorExtra = gameHandler->gameState()->getTile(gameHandler->gameState()->map->grailPos)->terType->getIndex();
					break;
				}

				std::vector<PlayerColor> players = {};
				switch(rumorId)
				{
					case RumorState::RUMOR_OBELISKS:
						players = tgi.obelisks[0];
						break;

					case RumorState::RUMOR_ARTIFACTS:
						players = tgi.artifacts[0];
						break;

					case RumorState::RUMOR_ARMY:
						players = tgi.army[0];
						break;

					case RumorState::RUMOR_INCOME:
						players = tgi.income[0];
						break;
				}
				rumorExtra = RandomGeneratorUtil::nextItem(players, rand)->getNum();

				break;
			}
			case RumorState::TYPE_MAP:
				// Makes sure that map rumors only used if there enough rumors too choose from
				if(!gameHandler->gameState()->map->rumors.empty() && (gameHandler->gameState()->map->rumors.size() > 1 || !gameHandler->gameState()->currentRumor.last.count(RumorState::TYPE_MAP)))
				{
					rumorId = rand.nextInt(gameHandler->gameState()->map->rumors.size() - 1);
					break;
				}
				else
					newRumor.type = RumorState::TYPE_RAND;
				[[fallthrough]];

			case RumorState::TYPE_RAND:
				auto vector = VLC->generaltexth->findStringsWithPrefix("core.randtvrn");
				rumorId = rand.nextInt((int)vector.size() - 1);

				break;
		}
	}
	while(!newRumor.update(rumorId, rumorExtra));

	return newRumor;
}

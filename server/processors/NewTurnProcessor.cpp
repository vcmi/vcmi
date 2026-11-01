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
#include "../../lib/IGameSettings.h"
#include "../../lib/MapLayerHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/entities/RumorTypeHandler.h"
#include "../../lib/gameState/CGameState.h"

#include <vcmi/scripting/MapEventDispatcher.h>
#include "../../lib/gameState/SThievesGuildInfo.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/IOwnableObject.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CCastleEvent.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/StackLocation.h"
#include "../../lib/pathfinder/TurnInfo.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include <vstd/RNG.h>

NewTurnProcessor::NewTurnProcessor(CGameHandler * gameHandler)
	:gameHandler(gameHandler)
{
}

void NewTurnProcessor::handleTimeEvents(PlayerColor color)
{
	for (auto const & event : gameHandler->gameState().getMap().events)
	{
		if (!event.occursToday(gameHandler->gameState().day))
			continue;

		if (!event.affectsDifficulty(gameHandler->gameInfo().getStartInfo()->getDifficulty()))
			continue;

		if (!event.affectsPlayer(color, gameHandler->gameInfo().getPlayerState(color)->isHuman()))
			continue;

		if (auto * dispatcher = gameHandler->gameState().getMapEventDispatcher(); dispatcher && !event.scriptHandler.empty())
		{
			gameHandler->runScriptedEvent(*dispatcher, color, {},
				[&](scripting::MapEventDispatcher & d){ return d.onPlayerTurnStart(*gameHandler, event.scriptHandler, color); });
			continue;
		}

		InfoWindow iw;
		iw.player = color;
		iw.text = event.message;
		iw.journalInfo = ScenarioEventJournalInfo{int3(-1, -1, -1)};

		//give resources
		if (!event.resources.empty())
		{
			gameHandler->giveResources(color, event.resources);
			for (GameResID i : LIBRARY->resourceTypeHandler->getAllObjects())
				if (event.resources[i])
					iw.components.emplace_back(ComponentType::RESOURCE, i, event.resources[i]);
		}

		//remove objects specified by event
		for(const ObjectInstanceID objectIdToRemove : event.deletedObjectsInstances)
		{
			const auto * objectInstance = gameHandler->gameInfo().tryGetObj(objectIdToRemove);
			if(objectInstance != nullptr)
				gameHandler->removeObject(objectInstance, PlayerColor::NEUTRAL);
		}
		gameHandler->sendAndApply(iw); //show dialog
	}
}

void NewTurnProcessor::handleTownEvents(const CGTownInstance * town)
{
	for (auto const & event : town->events)
	{
		if (!event.occursToday(gameHandler->gameState().day))
			continue;

		if (!event.affectsDifficulty(gameHandler->gameInfo().getStartInfo()->getDifficulty()))
			continue;

		PlayerColor player = town->getOwner();
		if (!player.isValidPlayer())
		{
			if (event.neutralAffected)
				applyTownEventEffects(town, event, nullptr);
			continue;
		}

		if (!event.affectsPlayer(player, gameHandler->gameInfo().getPlayerState(player)->isHuman()))
			continue;

		if (auto * dispatcher = gameHandler->gameState().getMapEventDispatcher(); dispatcher && !event.scriptHandler.empty())
		{
			gameHandler->runScriptedEvent(*dispatcher, player, {},
				[&](scripting::MapEventDispatcher & d){ return d.onTownTurnStart(*gameHandler, event.scriptHandler, town); });
			continue;
		}

		// dialog
		InfoWindow iw;
		iw.player = player;
		iw.text = event.message;
		iw.journalInfo = ScenarioEventJournalInfo{town->visitablePos()};

		if (event.resources.nonZero())
		{
			gameHandler->giveResources(player, event.resources);

			for (GameResID i : LIBRARY->resourceTypeHandler->getAllObjects())
				if (event.resources[i])
					iw.components.emplace_back(ComponentType::RESOURCE, i, event.resources[i]);
		}

		applyTownEventEffects(town, event, &iw);
		gameHandler->sendAndApply(iw); //show dialog
	}
}

void NewTurnProcessor::applyTownEventEffects(const CGTownInstance * town, const CCastleEvent & event, InfoWindow * iw)
{
	for (auto & i : event.buildings)
	{
		// Only perform action if:
		// 1. Building exists in town (don't attempt to build Lvl 5 guild in Fortress
		// 2. Building was not built yet
		// othervice, silently ignore / skip it
		if (town->getTown()->buildings.count(i) && !town->hasBuilt(i))
		{
			gameHandler->buildStructure(town->id, i, true);
			if (iw)
				iw->components.emplace_back(ComponentType::BUILDING, BuildingTypeUniqueID(town->getFactionID(), i));
		}
	}

	if (!event.creatures.empty())
	{
		SetAvailableCreatures sac;
		sac.tid = town->id;
		sac.creatures = town->creatures;

		for (si32 i=0;i<event.creatures.size();i++) //creature growths
		{
			if (i < town->creatures.size() && !town->creatures.at(i).second.empty() && event.creatures.at(i) > 0)//there is dwelling
			{
				sac.creatures[i].first += event.creatures.at(i);
				if (iw)
					iw->components.emplace_back(ComponentType::CREATURE, town->creatures.at(i).second.back(), event.creatures.at(i));
			}
		}

		gameHandler->sendAndApply(sac);
	}
}

void NewTurnProcessor::handleTurnStartEvents(PlayerColor which)
{
	const auto * playerState = gameHandler->gameState().getPlayerState(which);

	handleTimeEvents(which);
	for (const auto * t : playerState->getTowns())
		handleTownEvents(t);
}

void NewTurnProcessor::onPlayerTurnEnded(PlayerColor which)
{
	const auto * playerState = gameHandler->gameState().getPlayerState(which);
	assert(playerState->status == EPlayerStatus::INGAME);

	if (playerState->getTowns().empty())
	{
		DaysWithoutTown pack;
		pack.player = which;
		pack.daysWithoutCastle = playerState->daysWithoutCastle.value_or(0) + 1;
		gameHandler->sendAndApply(pack);
	}
	else
	{
		if (playerState->daysWithoutCastle.has_value())
		{
			DaysWithoutTown pack;
			pack.player = which;
			pack.daysWithoutCastle = std::nullopt;
			gameHandler->sendAndApply(pack);
		}
	}

	// check for 7 days without castle
	gameHandler->checkVictoryLossConditionsForPlayer(which);

	bool newWeek = gameHandler->gameInfo().getCalendar().nextDay().getDayOfWeek() == 1; // end of 7th day

	if (newWeek) //new heroes in tavern
		gameHandler->heroPool->onNewWeek(which);
}

ResourceSet NewTurnProcessor::generatePlayerIncome(PlayerColor playerID, bool newWeek)
{
	const auto & playerSettings = gameHandler->gameInfo().getPlayerSettings(playerID);
	const PlayerState & state = gameHandler->gameState().players.at(playerID);
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

	for(auto & k : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		income[k] += state.valOfBonuses(BonusType::RESOURCES_CONSTANT_BOOST, BonusSubtypeID(k));
		income[k] += state.valOfBonuses(BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST, BonusSubtypeID(k)) * state.getTowns().size();
	}

	if(newWeek) //weekly crystal generation if 1 or more crystal dragons in any hero army or town garrison
	{
		bool hasCrystalGenCreature = false;
		for (const auto & hero : state.getHeroes())
			for(const auto & stack : hero->stacks)
				if(stack.second->hasBonusOfType(BonusType::SPECIAL_CRYSTAL_GENERATION))
					hasCrystalGenCreature = true;

		for(const auto & town : state.getTowns())
			for(const auto & stack : town->stacks)
				if(stack.second->hasBonusOfType(BonusType::SPECIAL_CRYSTAL_GENERATION))
					hasCrystalGenCreature = true;

		if(hasCrystalGenCreature)
			income[EGameResID::CRYSTAL] += 3;
	}

	TResources incomeHandicapped = income;
	incomeHandicapped.applyHandicap(playerSettings->handicap.percentIncome);

	for (const auto * obj :	state.getOwnedObjects())
		incomeHandicapped += obj->asOwnable()->dailyIncome();

	if (!state.isHuman())
	{
		// Initialize bonuses for different resources
		int difficultyIndex = gameHandler->gameState().getStartInfo()->difficulty;
		const char * difficultyName = GameConstants::DIFFICULTY_NAMES[difficultyIndex];
		const JsonNode & weeklyBonusesConfig = gameHandler->gameState().getSettings().getValue(EGameSettings::RESOURCES_WEEKLY_BONUSES_AI);
		const JsonNode & difficultyConfig = weeklyBonusesConfig[difficultyName];

		// Distribute weekly bonuses over 7 days, depending on the current day of the week
		auto calendar = gameHandler->gameState().getCalendar();
		for (GameResID i : LIBRARY->resourceTypeHandler->getAllObjects())
		{
			const std::string & name = i.toResource()->getJsonKey();
			int64_t weeklyBonus = difficultyConfig[name].Integer();
			int64_t dayOfWeek = calendar.getDayOfWeek();
			int64_t dailyIncome = incomeHandicapped[i];
			int64_t amountTillToday = dailyIncome * weeklyBonus * (dayOfWeek-1) / calendar.getDaysInWeek() / 100;
			int64_t amountAfterToday = dailyIncome * weeklyBonus * dayOfWeek / calendar.getDaysInWeek() / 100;
			int64_t dailyBonusToday = amountAfterToday - amountTillToday;
			int64_t totalIncomeToday = std::min(GameConstants::PLAYER_RESOURCES_CAP, incomeHandicapped[i] + dailyBonusToday);
			incomeHandicapped[i] = totalIncomeToday;
		}
	}

	return incomeHandicapped;
}

SetAvailableCreatures NewTurnProcessor::generateTownGrowth(const CGTownInstance * t, EWeekType weekType, CreatureID creatureWeek, bool firstDay, int additionalGrowth)
{
	SetAvailableCreatures sac;
	PlayerColor player = t->tempOwner;

	sac.tid = t->id;
	sac.creatures = t->creatures;

	for (int k=0; k < t->getTown()->creatures.size(); k++)
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
				creatureGrowth += additionalGrowth;

			//bonus week, effect applies only to identical creatures
			if (weekType == EWeekType::BONUS_GROWTH && cre->getId() == creatureWeek)
				creatureGrowth += additionalGrowth;
		}

		// Neutral towns have halved creature growth
		if (!player.isValidPlayer() && !firstDay)
			creatureGrowth /= 2;

		uint32_t resultingCreatures = 0;

		if (weekType == EWeekType::PLAGUE)
			resultingCreatures = creaturesBefore / 2;
		else if (weekType == EWeekType::DOUBLE_GROWTH && vstd::contains(t->creatures.at(k).second, creatureWeek))
			resultingCreatures = (creaturesBefore + creatureGrowth) * 2;
		else
			resultingCreatures = creaturesBefore + creatureGrowth;

		sac.creatures.at(k).first = resultingCreatures;
	}

	return sac;
}

void NewTurnProcessor::updateNeutralTownGarrison(const CGTownInstance * t, int currentWeek) const
{
	assert(t);
	assert(!t->getOwner().isValidPlayer());

	constexpr int randomRollsCount = 3; // sum of 3 random rolls gives simple bell curve distribution
	constexpr int maxRollValue = 9; // roll range grows each week, but stops growing after 8 weeks
	constexpr int upgradeChance = 5; // Chance for garrison to get an upgrade after growth
	constexpr int growthChanceFort = 80; // Chance for growth to occur in towns with fort built
	constexpr int growthChanceVillage = 40; // Chance for growth to occur in towns without fort
	constexpr std::array<int, 7> maxRollSumForLevel = {2, 5, 9, 14, 18, 21, 27};

	auto & rng = gameHandler->getRandomGenerator();

	int growthChance = t->hasFort() ? growthChanceFort : growthChanceVillage;
	if (rng.nextInt(0, 99) >= growthChance)
		return;

	int rollsSum = 0;
	for(int i = 0; i < randomRollsCount; ++i)
		rollsSum += rng.nextInt(0, std::min(currentWeek, maxRollValue));

	size_t levelToGrow = 0;
	while (rollsSum > maxRollSumForLevel.at(levelToGrow))
		levelToGrow++;

	const auto & townCreatures = t->getTown()->creatures;
	if (levelToGrow >= townCreatures.size() || townCreatures[levelToGrow].empty())
		return;

	const auto & levelCreatures = townCreatures[levelToGrow];
	const CCreature * baseCreature = levelCreatures.front().toCreature();
	const int growthAmount = baseCreature->getGrowth();

	// special creatures may have no growth
	if (growthAmount <= 0)
		return;

	const auto & takeFromAvailable = [this, t, levelToGrow, growthAmount]()
	{
		int creaturesAvailable = t->creatures.at(levelToGrow).first;
		int creaturesLeft = std::max(0, creaturesAvailable - growthAmount);

		if (creaturesLeft != creaturesAvailable)
		{
			SetAvailableCreatures sac;
			sac.tid = t->id;
			sac.creatures = t->creatures;
			sac.creatures.at(levelToGrow).first = creaturesLeft;
			gameHandler->sendAndApply(sac);
		}
	};

	// If garrison already has upgraded creature of this level, new creatures join it
	for(const auto & [slotID, stack] : t->Slots())
	{
		CreatureID creatureID = stack->getCreatureID();
		if (creatureID == baseCreature->getId() || !vstd::contains(levelCreatures, creatureID))
			continue;

		gameHandler->changeStackCount(StackLocation(t->id, slotID), growthAmount, ChangeValueMode::RELATIVE);
		takeFromAvailable();
		return;
	}

	SlotID targetSlot = t->getSlotFor(baseCreature);
	if (!targetSlot.validSlot())
	{
		// All slots are taken - replace weakest stack, but only if it is weaker than new creatures
		SlotID weakestSlot;
		for(const auto & slot : t->Slots())
			if (!weakestSlot.validSlot() || t->getPower(slot.first) < t->getPower(weakestSlot))
				weakestSlot = slot.first;

		uint64_t growthPower = static_cast<uint64_t>(baseCreature->getAIValue()) * growthAmount;
		if (!weakestSlot.validSlot() || t->getPower(weakestSlot) >= growthPower)
			return;

		gameHandler->eraseStack(StackLocation(t->id, weakestSlot), true);
		targetSlot = weakestSlot;
	}

	StackLocation targetLocation(t->id, targetSlot);
	if (t->hasStackAtSlot(targetSlot))
		gameHandler->changeStackCount(targetLocation, growthAmount, ChangeValueMode::RELATIVE);
	else
		gameHandler->insertNewStack(targetLocation, baseCreature, growthAmount);
	takeFromAvailable();

	if (levelCreatures.size() < 2 || rng.nextInt(0, 99) >= upgradeChance)
		return;

	// Upgrade all stacks of newly added creature
	const CCreature * upgradedCreature = levelCreatures.at(rng.nextInt(1, levelCreatures.size() - 1)).toCreature();
	std::vector<SlotID> slotsToUpgrade;
	for(const auto & [slotID, stack] : t->Slots())
		if (stack->getCreatureID() == baseCreature->getId())
			slotsToUpgrade.push_back(slotID);

	for(const auto & slotID : slotsToUpgrade)
		gameHandler->changeStackType(StackLocation(t->id, slotID), upgradedCreature);
}

/// Picks a random item from eligible items that were not used since the pool was last exhausted
template<typename T>
static T pickUnusedItem(const std::vector<T> & eligible, std::set<T> & used, vstd::RNG & rand)
{
	std::vector<T> available;
	for(const auto & item : eligible)
		if(!used.count(item))
			available.push_back(item);

	if(available.empty())
	{
		for(const auto & item : eligible)
			used.erase(item);
		available = eligible;
	}

	T result = *RandomGeneratorUtil::nextItem(available, rand);
	used.insert(result);
	return result;
}

bool NewTurnProcessor::pickGenericRumor(RumorState & rumor)
{
	std::set<FactionID> factionsOnMap;
	for(const auto & townID : gameHandler->gameState().getMap().getAllTowns())
		factionsOnMap.insert(gameHandler->gameState().getTown(townID)->getFactionID());

	std::vector<RumorId> eligible;
	for(const auto & rumorID : LIBRARY->rumorHandler->getAllObjects())
	{
		FactionID faction = rumorID.toEntity(LIBRARY)->getFaction();
		if(faction == FactionID::NONE || factionsOnMap.count(faction))
			eligible.push_back(rumorID);
	}

	if(eligible.empty())
		return false;

	RumorId picked = pickUnusedItem(eligible, rumor.usedRumors, gameHandler->getRandomGenerator());
	rumor.text = MetaString::createFromTextID(picked.toEntity(LIBRARY)->getNameTextID());
	return true;
}

bool NewTurnProcessor::pickMapRumor(RumorState & rumor)
{
	const auto & mapRumors = gameHandler->gameState().getMap().rumors;
	std::vector<int32_t> eligible(mapRumors.size());
	std::iota(eligible.begin(), eligible.end(), 0);

	if(eligible.empty())
		return false;

	int32_t picked = pickUnusedItem(eligible, rumor.usedMapRumors, gameHandler->getRandomGenerator());
	rumor.text = mapRumors.at(picked).text;
	return true;
}

bool NewTurnProcessor::pickStatisticRumor(RumorState & rumor)
{
	SThievesGuildInfo tgi;
	gameHandler->gameState().obtainPlayersStats(tgi, 20);

	std::vector<MetaString> candidates;
	auto addLeader = [&candidates](const std::string & textID, const std::vector<std::vector<PlayerColor>> & ranking)
	{
		// tied players have no leader to tell about
		if(ranking.empty() || ranking.front().size() != 1)
			return;

		MetaString text = MetaString::createFromTextID(textID);
		text.replaceTextID("core.plcolors", ranking.front().front().getNum());
		candidates.push_back(text);
	};

	addLeader("core.genrltxt.208", tgi.obelisks);
	addLeader("core.genrltxt.209", tgi.artifacts);
	addLeader("core.genrltxt.210", tgi.army);
	addLeader("core.genrltxt.211", tgi.income);

	if(candidates.empty())
		return false;

	rumor.text = *RandomGeneratorUtil::nextItem(candidates, gameHandler->getRandomGenerator());
	return true;
}

bool NewTurnProcessor::pickGrailRumor(RumorState & rumor)
{
	// core.arraytxt.148-156 name regions of the map split into 3x3 grid, listed here in row-major order
	static const std::array<int, 9> regionTexts = {
		155, 148, 149,
		154, 156, 150,
		153, 152, 151
	};

	const auto & map = gameHandler->gameState().getMap();
	if(!map.grailPos.isValid() || map.isHoleAt(map.grailPos))
		return false;

	// terrains and map layers from mods may have no text for this rumor
	std::vector<MetaString> candidates;

	const std::string & terrainTextID = map.getTile(map.grailPos).getTerrain()->grailRumorTextID;
	if(!terrainTextID.empty())
	{
		MetaString text = MetaString::createFromTextID("core.genrltxt.212");
		text.replaceTextID(terrainTextID);
		candidates.push_back(text);
	}

	std::string layerTextID = map.mapLayers.at(map.grailPos.z).toEntity(LIBRARY)->getGrailRumorTextID();
	if(!layerTextID.empty())
	{
		int column = map.grailPos.x * 3 / map.width;
		int row = map.grailPos.y * 3 / map.height;
		MetaString text = MetaString::createFromTextID(layerTextID);
		text.replaceTextID("core.arraytxt", regionTexts.at(row * 3 + column));
		candidates.push_back(text);
	}

	if(candidates.empty())
		return false;

	rumor.text = *RandomGeneratorUtil::nextItem(candidates, gameHandler->getRandomGenerator());
	return true;
}

RumorState NewTurnProcessor::pickNewRumor(bool firstTurn)
{
	RumorState rumor = gameHandler->gameState().currentRumor;

	// generic 32%, map 32%, statistic 30%, grail 6%
	int roll = gameHandler->getRandomGenerator().nextInt(99);
	bool picked = false;

	if(roll >= 64 && roll < 94 && !firstTurn)
		picked = pickStatisticRumor(rumor);
	else if(roll >= 64) // statistics of the first turn are the same for all players, grail rumor is told instead
		picked = pickGrailRumor(rumor);
	else if(roll >= 32)
		picked = pickMapRumor(rumor);

	if(!picked)
		picked = pickGenericRumor(rumor);

	if(!picked)
		rumor.text.clear();

	return rumor;
}

std::tuple<EWeekType, CreatureID, int> NewTurnProcessor::pickWeekType(bool newMonth)
{
	// creatures without a map object cannot be spawned as wandering monsters, so exclude them from special weeks
	auto canSpawnAsMapObject = [](const CreatureID & creatureID)
	{
		return LIBRARY->objtypeh->knownSubObjects(Obj::MONSTER).contains(creatureID.getNum());
	};

	std::vector<std::tuple<CreatureID, int>> creaturesWithDeityOfFireBonus;
	for(const auto & bonus : *gameHandler->gameState().globalEffects.getBonusesOfType(BonusType::DEITYOFFIRE))
	{
		CreatureID creatureID = bonus->subtype.as<CreatureID>();
		if(canSpawnAsMapObject(creatureID))
			creaturesWithDeityOfFireBonus.push_back({creatureID, bonus->val});
	}
	if(!creaturesWithDeityOfFireBonus.empty())
	{
		auto item = *RandomGeneratorUtil::nextItem(creaturesWithDeityOfFireBonus, gameHandler->getRandomGenerator());
		return { EWeekType::DEITYOFFIRE, std::get<0>(item), std::get<1>(item)};
	}

	if(!gameHandler->gameInfo().getSettings().getBoolean(EGameSettings::CREATURES_ALLOW_RANDOM_SPECIAL_WEEKS))
		return { EWeekType::NORMAL, CreatureID::NONE, 0};

	int monthType = gameHandler->getRandomGenerator().nextInt(99);
	if (newMonth) //new month
	{
		if (monthType < gameHandler->gameInfo().getSettings().getInteger(EGameSettings::CREATURES_MONTH_DOUBLE_GROWTH_PROBABILITY)) //double growth
		{
			if (gameHandler->gameInfo().getSettings().getBoolean(EGameSettings::CREATURES_ALLOW_ALL_FOR_DOUBLE_MONTH))
			{
				CreatureID creatureID = gameHandler->randomizer->rollCreature();
				return { EWeekType::DOUBLE_GROWTH, creatureID, 0};
			}
			else
			{
				std::vector<CreatureID> spawnableDoubledCreatures;
				for(const auto & creatureID : LIBRARY->creh->doubledCreatures)
					if(canSpawnAsMapObject(creatureID))
						spawnableDoubledCreatures.push_back(creatureID);

				if(!spawnableDoubledCreatures.empty())
				{
					CreatureID creatureID = *RandomGeneratorUtil::nextItem(spawnableDoubledCreatures, gameHandler->getRandomGenerator());
					return { EWeekType::DOUBLE_GROWTH, creatureID, 0};
				}
				else
				{
					gameHandler->complain("Cannot find creature that can be spawned!");
					return { EWeekType::NORMAL, CreatureID::NONE, 0};
				}
			}
		}

		if (monthType < gameHandler->gameInfo().getSettings().getInteger(EGameSettings::CREATURES_MONTH_DOUBLE_GROWTH_PROBABILITY) +
						 gameHandler->gameInfo().getSettings().getInteger(EGameSettings::CREATURES_MONTH_PLAGUE_PROBABILITY)) //plague
			return { EWeekType::PLAGUE, CreatureID::NONE, 0};

		return { EWeekType::NORMAL, CreatureID::NONE, 0};
	}
	else //it's a week, but not full month
	{
		if (monthType < gameHandler->gameInfo().getSettings().getInteger(EGameSettings::CREATURES_WEEK_SPECIAL_PROBABILITY))
		{
			std::pair<int, CreatureID> newMonster(54, CreatureID());
			do
			{
				newMonster.second = gameHandler->randomizer->rollCreature();
			} while (newMonster.second.toEntity(LIBRARY)->getFactionID().toFaction()->town == nullptr); // find first non neutral creature

			return { EWeekType::BONUS_GROWTH, newMonster.second, gameHandler->gameInfo().getSettings().getInteger(EGameSettings::CREATURES_ADDITIONAL_WEEKLY_GROWTH_SPECIAL_WEEK)};
		}
		return { EWeekType::NORMAL, CreatureID::NONE, 0};
	}
}

std::vector<SetMana> NewTurnProcessor::updateHeroesManaPoints()
{
	std::vector<SetMana> result;

	for (const auto & elem : gameHandler->gameState().players)
	{
		for (const CGHeroInstance *h : elem.second.getHeroes())
		{
			int32_t newMana = h->getManaNewTurn();

			if (newMana != h->mana)
				result.emplace_back(h->id, newMana, ChangeValueMode::ABSOLUTE);
		}
	}
	return result;
}

std::vector<SetMovePoints> NewTurnProcessor::updateHeroesMovementPoints()
{
	std::vector<SetMovePoints> result;

	for (const auto & elem : gameHandler->gameState().players)
	{
		for (const CGHeroInstance *h : elem.second.getHeroes())
		{
			auto ti = h->getTurnInfo(1);
			int32_t newMovementPoints = 0;
			if (h->inBoat())
				newMovementPoints = h->movementPointsLimitCached(h->getBoat()->layer, ti.get());
			else
				// NOTE: this code executed when bonuses of previous day not yet updated (this happen in NewTurn::applyGs). See issue 2356
				newMovementPoints = h->movementPointsLimitCached(EPathfindingLayer::LAND, ti.get());

			if (newMovementPoints != h->movementPointsRemaining())
				result.emplace_back(h->id, newMovementPoints);
		}
	}
	return result;
}

InfoWindow NewTurnProcessor::createInfoWindow(EWeekType weekType, CreatureID creatureWeek, bool newMonth, int additionalGrowth)
{
	InfoWindow iw;
	switch (weekType)
	{
		case EWeekType::DOUBLE_GROWTH:
			iw.text.appendTextID("core.arraytxt.131");
			iw.text.replaceNameSingular(creatureWeek);
			iw.text.replaceNameSingular(creatureWeek);
			break;
		case EWeekType::PLAGUE:
			iw.text.appendTextID("core.arraytxt.132");
			break;
		case EWeekType::BONUS_GROWTH:
			iw.text.appendTextID("core.arraytxt.134");
			iw.text.replaceNameSingular(creatureWeek);
			iw.text.replaceNameSingular(creatureWeek);
			break;
		case EWeekType::DEITYOFFIRE:
		{
			const CCreature *base = creatureWeek.toCreature();
			CreatureID upgradedCreature = creatureWeek;
			if(base && !base->upgrades.empty())
				upgradedCreature = *base->upgrades.rbegin();
			iw.text.appendTextID("core.arraytxt.135");
			iw.text.replaceNameSingular(creatureWeek);
			iw.text.replaceNameSingular(creatureWeek);
			iw.text.replacePositiveNumber(additionalGrowth);
			iw.text.replaceNameSingular(upgradedCreature);
			iw.text.replacePositiveNumber(additionalGrowth);
			break;
		}
		default:
			if (newMonth)
			{
				iw.text.appendTextID("core.arraytxt.130");
				iw.text.replaceLocalString(EMetaText::ARRAY_TXT, gameHandler->getRandomGenerator().nextInt(32, 41));
			}
			else
			{
				iw.text.appendTextID("core.arraytxt.133");
				iw.text.replaceLocalString(EMetaText::ARRAY_TXT, gameHandler->getRandomGenerator().nextInt(43, 57));
			}
	}
	return iw;
}

NewTurn NewTurnProcessor::generateNewTurnPack()
{
	NewTurn n;
	n.specialWeek = EWeekType::FIRST_WEEK;
	n.creatureid = CreatureID::NONE;
	n.day = gameHandler->gameState().day + 1;

	auto calendar = gameHandler->gameInfo().getCalendar();
	bool firstTurn = !calendar.getCurrentDay();
	bool newWeek = calendar.nextDay().getDayOfWeek() == 1; //day numbers are confusing, as day was not yet switched
	bool newMonth = calendar.nextDay().getDayOfMonth() == 1;

	int additionalGrowth = 0;

	if (!firstTurn)
	{
		for (const auto & player : gameHandler->gameState().players)
			n.playerIncome[player.first] = generatePlayerIncome(player.first, newWeek);
	}

	if (newWeek && !firstTurn)
	{
		auto [specialWeek, creatureID, addGrowth] = pickWeekType(newMonth);
		n.specialWeek = specialWeek;
		n.creatureid = creatureID;
		additionalGrowth = addGrowth;
	}

	n.heroesMana = updateHeroesManaPoints();
	n.heroesMovement = updateHeroesMovementPoints();

	if (newWeek)
	{
		for (const auto & townID : gameHandler->gameState().getMap().getAllTowns())
		{
			const auto * t = gameHandler->gameState().getTown(townID);
			n.availableCreatures.push_back(generateTownGrowth(t, n.specialWeek, n.creatureid, firstTurn, additionalGrowth));
		}

		n.newRumor = pickNewRumor(firstTurn);

		//new week info popup
		if (n.specialWeek != EWeekType::FIRST_WEEK)
			n.newWeekNotification = createInfoWindow(n.specialWeek, n.creatureid, newMonth, additionalGrowth);
	}

	return n;
}

void NewTurnProcessor::onNewTurn()
{
	NewTurn n = generateNewTurnPack();

	auto calendar = gameHandler->gameInfo().getCalendar();
	bool firstTurn = !calendar.getCurrentDay();
	bool newWeek = calendar.nextDay().getDayOfWeek() == 1; //day numbers are confusing, as day was not yet switched
	bool newMonth = calendar.nextDay().getDayOfMonth() == 1;

	gameHandler->sendAndApply(n);

	// neutral towns have no turn of their own, so their events occur as soon as the day starts
	for (const auto & townID : gameHandler->gameState().getMap().getAllTowns())
	{
		const auto * t = gameHandler->gameState().getTown(townID);
		if (!t->getOwner().isValidPlayer())
			handleTownEvents(t);
	}

	if (newWeek)
	{
		for (const auto & townID : gameHandler->gameState().getMap().getAllTowns())
		{
			const auto * t = gameHandler->gameState().getTown(townID);
			if (t->hasBuilt(BuildingSubID::PORTAL_OF_SUMMONING))
				gameHandler->setPortalDwelling(t, true, n.specialWeek == EWeekType::PLAGUE); //set creatures for Portal of Summoning
		}
	}

	if (newWeek && !firstTurn)
	{
		for (const auto & townID : gameHandler->gameState().getMap().getAllTowns())
		{
			const auto * t = gameHandler->gameState().getTown(townID);
			if (!t->getOwner().isValidPlayer())
				updateNeutralTownGarrison(t, 1 + calendar.getCurrentDay() / calendar.getDaysInWeek());
		}
	}

	//spawn wandering monsters
	if (newMonth && (n.specialWeek == EWeekType::DOUBLE_GROWTH || n.specialWeek == EWeekType::DEITYOFFIRE))
	{
		gameHandler->spawnWanderingMonsters(n.creatureid);
	}

	logGlobal->trace("Info about turn %d has been sent!", n.day);
}

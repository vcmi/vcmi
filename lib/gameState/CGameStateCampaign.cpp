/*
 * CGameStateCampaign.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameStateCampaign.h"

#include "CGameState.h"
#include "QuestInfo.h"

#include "../campaign/CampaignState.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../registerTypes/RegisterTypes.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../StartInfo.h"
#include "../CBuildingHandler.h"
#include "../CHeroHandler.h"
#include "../mapping/CMap.h"
#include "../ArtifactUtils.h"
#include "../CPlayerState.h"
#include "../serializer/CMemorySerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

CampaignHeroReplacement::CampaignHeroReplacement(CGHeroInstance * hero, const ObjectInstanceID & heroPlaceholderId):
	hero(hero),
	heroPlaceholderId(heroPlaceholderId)
{
}

CGameStateCampaign::CGameStateCampaign(CGameState * owner):
	gameState(owner)
{
	assert(gameState->scenarioOps->mode == StartInfo::CAMPAIGN);
	assert(gameState->scenarioOps->campState != nullptr);
}

std::optional<CampaignBonus> CGameStateCampaign::currentBonus() const
{
	auto campaignState = gameState->scenarioOps->campState;
	return campaignState->getBonus(*campaignState->currentScenario());
}

std::optional<CampaignScenarioID> CGameStateCampaign::getHeroesSourceScenario() const
{
	auto campaignState = gameState->scenarioOps->campState;
	auto bonus = currentBonus();

	if(bonus && bonus->type == CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO)
		return static_cast<CampaignScenarioID>(bonus->info2);;

	return campaignState->lastScenario();
}

void CGameStateCampaign::trimCrossoverHeroesParameters(std::vector<CampaignHeroReplacement> & campaignHeroReplacements, const CampaignTravel & travelOptions)
{
	// create heroes list for convenience iterating
	std::vector<CGHeroInstance *> crossoverHeroes;
	crossoverHeroes.reserve(campaignHeroReplacements.size());
	for(auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		crossoverHeroes.push_back(campaignHeroReplacement.hero);
	}

	// TODO this logic (what should be kept) should be part of CScenarioTravel and be exposed via some clean set of methods
	if(!travelOptions.whatHeroKeeps.experience)
	{
		//trimming experience
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->initExp(gameState->getRandomGenerator());
		}
	}

	if(!travelOptions.whatHeroKeeps.primarySkills)
	{
		//trimming prim skills
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
			{
				auto sel = Selector::type()(BonusType::PRIMARY_SKILL)
					.And(Selector::subtype()(g))
					.And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL));

				cgh->getBonusLocalFirst(sel)->val = cgh->type->heroClass->primarySkillInitial[g];
			}
		}
	}

	if(!travelOptions.whatHeroKeeps.secondarySkills)
	{
		//trimming sec skills
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->secSkills = cgh->type->secSkillsInit;
			cgh->recreateSecondarySkillsBonuses();
		}
	}

	if(!travelOptions.whatHeroKeeps.spells)
	{
		for(CGHeroInstance * cgh : crossoverHeroes)
		{
			cgh->removeSpellbook();
		}
	}

	if(!travelOptions.whatHeroKeeps.artifacts)
	{
		//trimming artifacts
		for(CGHeroInstance * hero : crossoverHeroes)
		{
			size_t totalArts = GameConstants::BACKPACK_START + hero->artifactsInBackpack.size();
			for (size_t i = 0; i < totalArts; i++ )
			{
				auto artifactPosition = ArtifactPosition((si32)i);
				if(artifactPosition == ArtifactPosition::SPELLBOOK)
					continue; // do not handle spellbook this way

				const ArtSlotInfo *info = hero->getSlot(artifactPosition);
				if(!info)
					continue;

				// TODO: why would there be nullptr artifacts?
				const CArtifactInstance *art = info->artifact;
				if(!art)
					continue;

				bool takeable = travelOptions.artifactsKeptByHero.count(art->artType->getId());

				ArtifactLocation al(hero, artifactPosition);
				if(!takeable  &&  !al.getSlot()->locked)  //don't try removing locked artifacts -> it crashes #1719
					al.removeArtifact();
			}
		}
	}

	//trimming creatures
	for(CGHeroInstance * cgh : crossoverHeroes)
	{
		auto shouldSlotBeErased = [&](const std::pair<SlotID, CStackInstance *> & j) -> bool
		{
			CreatureID::ECreatureID crid = j.second->getCreatureID().toEnum();
			return !travelOptions.monstersKeptByHero.count(crid);
		};

		auto stacksCopy = cgh->stacks; //copy of the map, so we can iterate iover it and remove stacks
		for(auto &slotPair : stacksCopy)
			if(shouldSlotBeErased(slotPair))
				cgh->eraseStack(slotPair.first);
	}

	// Removing short-term bonuses
	for(CGHeroInstance * cgh : crossoverHeroes)
	{
		cgh->removeBonusesRecursive(CSelector(Bonus::OneDay)
			.Or(CSelector(Bonus::OneWeek))
			.Or(CSelector(Bonus::NTurns))
			.Or(CSelector(Bonus::NDays))
			.Or(CSelector(Bonus::OneBattle)));
	}
}

void CGameStateCampaign::placeCampaignHeroes()
{
	// place bonus hero
	auto campaignState = gameState->scenarioOps->campState;
	auto campaignBonus = campaignState->getBonus(*campaignState->currentScenario());
	bool campaignGiveHero = campaignBonus && campaignBonus->type == CampaignBonusType::HERO;

	if(campaignGiveHero)
	{
		auto playerColor = PlayerColor(campaignBonus->info1);
		auto it = gameState->scenarioOps->playerInfos.find(playerColor);
		if(it != gameState->scenarioOps->playerInfos.end())
		{
			auto heroTypeId = campaignBonus->info2;
			if(heroTypeId == 0xffff) // random bonus hero
			{
				heroTypeId = gameState->pickUnusedHeroTypeRandomly(playerColor);
			}

			gameState->placeStartingHero(playerColor, HeroTypeID(heroTypeId), gameState->map->players[playerColor.getNum()].posOfMainTown);
		}
	}

	logGlobal->debug("\tGenerate list of hero placeholders");
	auto campaignHeroReplacements = generateCampaignHeroesToReplace();

	logGlobal->debug("\tPrepare crossover heroes");
	trimCrossoverHeroesParameters(campaignHeroReplacements, campaignState->scenario(*campaignState->currentScenario()).travelOptions);

	// remove same heroes on the map which will be added through crossover heroes
	// INFO: we will remove heroes because later it may be possible that the API doesn't allow having heroes
	// with the same hero type id
	std::vector<CGHeroInstance *> removedHeroes;

	std::set<HeroTypeID> heroesToRemove = campaignState->getReservedHeroes();

	for(auto & campaignHeroReplacement : campaignHeroReplacements)
		heroesToRemove.insert(HeroTypeID(campaignHeroReplacement.hero->subID));

	for(auto & heroID : heroesToRemove)
	{
		auto * hero = gameState->getUsedHero(heroID);
		if(hero)
		{
			removedHeroes.push_back(hero);
			gameState->map->heroesOnMap -= hero;
			gameState->map->objects[hero->id.getNum()] = nullptr;
			gameState->map->removeBlockVisTiles(hero, true);
		}
	}

	logGlobal->debug("\tReplace placeholders with heroes");
	replaceHeroesPlaceholders(campaignHeroReplacements);

	// now add removed heroes again with unused type ID
	for(auto * hero : removedHeroes)
	{
		si32 heroTypeId = 0;
		if(hero->ID == Obj::HERO)
		{
			heroTypeId = gameState->pickUnusedHeroTypeRandomly(hero->tempOwner);
		}
		else if(hero->ID == Obj::PRISON)
		{
			auto unusedHeroTypeIds = gameState->getUnusedAllowedHeroes();
			if(!unusedHeroTypeIds.empty())
			{
				heroTypeId = (*RandomGeneratorUtil::nextItem(unusedHeroTypeIds, gameState->getRandomGenerator())).getNum();
			}
			else
			{
				logGlobal->error("No free hero type ID found to replace prison.");
				assert(0);
			}
		}
		else
		{
			assert(0); // should not happen
		}

		hero->subID = heroTypeId;
		hero->portrait = hero->subID;
		gameState->map->getEditManager()->insertObject(hero);
	}
}

void CGameStateCampaign::giveCampaignBonusToHero(CGHeroInstance * hero)
{
	auto curBonus = currentBonus();
	if(!curBonus)
		return;

	assert(curBonus->isBonusForHero());

	//apply bonus
	switch(curBonus->type)
	{
		case CampaignBonusType::SPELL:
		{
			hero->addSpellToSpellbook(SpellID(curBonus->info2));
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
			{
				if(hero->slotEmpty(SlotID(i)))
				{
					hero->addToSlot(SlotID(i), CreatureID(curBonus->info2), curBonus->info3);
					break;
				}
			}
			break;
		}
		case CampaignBonusType::ARTIFACT:
		{
			if(!gameState->giveHeroArtifact(hero, static_cast<ArtifactID>(curBonus->info2)))
				logGlobal->error("Cannot give starting artifact - no free slots!");
			break;
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			CArtifactInstance * scroll = ArtifactUtils::createScroll(SpellID(curBonus->info2));
			const auto slot = ArtifactUtils::getArtAnyPosition(hero, scroll->getTypeId());
			if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
				scroll->putAt(ArtifactLocation(hero, slot));
			else
				logGlobal->error("Cannot give starting scroll - no free slots!");
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const ui8 * ptr = reinterpret_cast<const ui8 *>(&curBonus->info2);
			for(int g = 0; g < GameConstants::PRIMARY_SKILLS; ++g)
			{
				int val = ptr[g];
				if(val == 0)
				{
					continue;
				}
				auto bb = std::make_shared<Bonus>(
					BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CAMPAIGN_BONUS, val, static_cast<int>(*gameState->scenarioOps->campState->currentScenario()), g
				);
				hero->addNewBonus(bb);
			}
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			hero->setSecSkillLevel(SecondarySkill(curBonus->info2), curBonus->info3, true);
			break;
		}
	}
}

void CGameStateCampaign::replaceHeroesPlaceholders(const std::vector<CampaignHeroReplacement> & campaignHeroReplacements)
{
	for(const auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(gameState->getObjInstance(campaignHeroReplacement.heroPlaceholderId));

		CGHeroInstance *heroToPlace = campaignHeroReplacement.hero;
		heroToPlace->id = campaignHeroReplacement.heroPlaceholderId;
		heroToPlace->tempOwner = heroPlaceholder->tempOwner;
		heroToPlace->pos = heroPlaceholder->pos;
		heroToPlace->type = VLC->heroh->objects[heroToPlace->subID];
		heroToPlace->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, heroToPlace->type->heroClass->getIndex())->getTemplates().front();

		gameState->map->removeBlockVisTiles(heroPlaceholder, true);
		gameState->map->objects[heroPlaceholder->id.getNum()] = nullptr;
		gameState->map->instanceNames.erase(heroPlaceholder->instanceName);

		gameState->map->heroesOnMap.emplace_back(heroToPlace);
		gameState->map->objects[heroToPlace->id.getNum()] = heroToPlace;
		gameState->map->addBlockVisTiles(heroToPlace);
		gameState->map->instanceNames[heroToPlace->instanceName] = heroToPlace;

		delete heroPlaceholder;
	}
}

std::vector<CampaignHeroReplacement> CGameStateCampaign::generateCampaignHeroesToReplace()
{
	auto campaignState = gameState->scenarioOps->campState;

	std::vector<CampaignHeroReplacement> campaignHeroReplacements;
	std::vector<CGHeroPlaceholder *> placeholdersByPower;
	std::vector<CGHeroPlaceholder *> placeholdersByType;

	// find all placeholders on map
	for(auto obj : gameState->map->objects)
	{
		if(!obj)
			continue;

		if (obj->ID != Obj::HERO_PLACEHOLDER)
			continue;

		auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());

		// only 1 field must be set
		assert(heroPlaceholder->powerRank != heroPlaceholder->heroType);

		if(heroPlaceholder->powerRank)
			placeholdersByPower.push_back(heroPlaceholder);

		if(heroPlaceholder->heroType)
			placeholdersByType.push_back(heroPlaceholder);
	}

	//selecting heroes by type
	for (auto const * placeholder : placeholdersByType)
	{
		auto const & node = campaignState->getHeroByType(*placeholder->heroType);
		if (node.isNull())
		{
			logGlobal->info("Hero crossover: Unable to replace placeholder for %d (%s)!", placeholder->heroType->getNum(), VLC->heroTypes()->getById(*placeholder->heroType)->getNameTranslated());
			continue;
		}

		CGHeroInstance * hero = CampaignState::crossoverDeserialize(node, gameState->map);

		logGlobal->info("Hero crossover: Loading placeholder for %d (%s)", hero->subID, hero->getNameTranslated());

		campaignHeroReplacements.emplace_back(hero, placeholder->id);
	}

	auto lastScenario = getHeroesSourceScenario();

	if (!placeholdersByPower.empty() && lastScenario)
	{
		// sort hero placeholders descending power
		boost::range::sort(placeholdersByPower, [](const CGHeroPlaceholder * a, const CGHeroPlaceholder * b)
		{
			return *a->powerRank > *b->powerRank;
		});

		auto const & nodeList = campaignState->getHeroesByPower(lastScenario.value());
		auto nodeListIter = nodeList.begin();

		for (auto const * placeholder : placeholdersByPower)
		{
			if (nodeListIter == nodeList.end())
				break;

			CGHeroInstance * hero = CampaignState::crossoverDeserialize(*nodeListIter, gameState->map);
			nodeListIter++;

			logGlobal->info("Hero crossover: Loading placeholder as %d (%s)", hero->subID, hero->getNameTranslated());

			campaignHeroReplacements.emplace_back(hero, placeholder->id);
		}
	}
	return campaignHeroReplacements;
}

void CGameStateCampaign::initHeroes()
{
	auto chosenBonus = currentBonus();
	if (chosenBonus && chosenBonus->isBonusForHero() && chosenBonus->info1 != 0xFFFE) //exclude generated heroes
	{
		//find human player
		PlayerColor humanPlayer=PlayerColor::NEUTRAL;
		for (auto & elem : gameState->players)
		{
			if(elem.second.human)
			{
				humanPlayer = elem.first;
				break;
			}
		}
		assert(humanPlayer != PlayerColor::NEUTRAL);

		std::vector<ConstTransitivePtr<CGHeroInstance> > & heroes = gameState->players[humanPlayer].heroes;

		if (chosenBonus->info1 == 0xFFFD) //most powerful
		{
			int maxB = -1;
			for (int b=0; b<heroes.size(); ++b)
			{
				if (maxB == -1 || heroes[b]->getTotalStrength() > heroes[maxB]->getTotalStrength())
				{
					maxB = b;
				}
			}
			if(maxB < 0)
				logGlobal->warn("Cannot give bonus to hero cause there are no heroes!");
			else
				giveCampaignBonusToHero(heroes[maxB]);
		}
		else //specific hero
		{
			for (auto & heroe : heroes)
			{
				if (heroe->subID == chosenBonus->info1)
				{
					giveCampaignBonusToHero(heroe);
					break;
				}
			}
		}
	}
}

void CGameStateCampaign::initStartingResources()
{
	auto getHumanPlayerInfo = [&]() -> std::vector<const PlayerSettings *>
	{
		std::vector<const PlayerSettings *> ret;
		for(const auto & playerInfo : gameState->scenarioOps->playerInfos)
		{
			if(playerInfo.second.isControlledByHuman())
				ret.push_back(&playerInfo.second);
		}

		return ret;
	};

	auto chosenBonus = currentBonus();
	if(chosenBonus && chosenBonus->type == CampaignBonusType::RESOURCE)
	{
		std::vector<const PlayerSettings *> people = getHumanPlayerInfo(); //players we will give resource bonus
		for(const PlayerSettings *ps : people)
		{
			std::vector<int> res; //resources we will give
			switch (chosenBonus->info1)
			{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6:
					res.push_back(chosenBonus->info1);
					break;
				case 0xFD: //wood+ore
					res.push_back(GameResID(EGameResID::WOOD));
					res.push_back(GameResID(EGameResID::ORE));
					break;
				case 0xFE:  //rare
					res.push_back(GameResID(EGameResID::MERCURY));
					res.push_back(GameResID(EGameResID::SULFUR));
					res.push_back(GameResID(EGameResID::CRYSTAL));
					res.push_back(GameResID(EGameResID::GEMS));
					break;
				default:
					assert(0);
					break;
			}
			//increasing resource quantity
			for (auto & re : res)
			{
				gameState->players[ps->color].resources[re] += chosenBonus->info2;
			}
		}
	}
}

void CGameStateCampaign::initTowns()
{
	auto chosenBonus = currentBonus();

	if (!chosenBonus)
		return;

	if (chosenBonus->type != CampaignBonusType::BUILDING)
		return;

	for (int g=0; g<gameState->map->towns.size(); ++g)
	{
		CGTownInstance * town = gameState->map->towns[g];

		PlayerState * owner = gameState->getPlayerState(town->getOwner());
		if (!owner)
			continue;

		PlayerInfo & pi = gameState->map->players[owner->color.getNum()];

		if (!owner->human)
			continue;

		if (town->pos != pi.posOfMainTown)
			continue;

		BuildingID newBuilding;
		if(gameState->scenarioOps->campState->formatVCMI())
			newBuilding = BuildingID(chosenBonus->info1);
		else
			newBuilding = CBuildingHandler::campToERMU(chosenBonus->info1, town->subID, town->builtBuildings);

		// Build granted building & all prerequisites - e.g. Mages Guild Lvl 3 should also give Mages Guild Lvl 1 & 2
		while(true)
		{
			if (newBuilding == BuildingID::NONE)
				break;

			if (town->builtBuildings.count(newBuilding) != 0)
				break;

			town->builtBuildings.insert(newBuilding);

			auto building = town->town->buildings.at(newBuilding);
			newBuilding = building->upgrade;
		}
		break;
	}
}

bool CGameStateCampaign::playerHasStartingHero(PlayerColor playerColor) const
{
	auto campaignBonus = currentBonus();

	if (!campaignBonus)
		return false;

	if(campaignBonus->type == CampaignBonusType::HERO && playerColor == PlayerColor(campaignBonus->info1))
		return true;
	return false;
}

std::unique_ptr<CMap> CGameStateCampaign::getCurrentMap() const
{
	return gameState->scenarioOps->campState->getMap(CampaignScenarioID::NONE);
}

VCMI_LIB_NAMESPACE_END

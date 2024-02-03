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
#include "../mapObjects/CGTownInstance.h"
#include "../networkPacks/ArtifactLocation.h"
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
	assert(gameState->scenarioOps->mode == EStartMode::CAMPAIGN);
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
		return static_cast<CampaignScenarioID>(bonus->info2);

	return campaignState->lastScenario();
}

void CGameStateCampaign::trimCrossoverHeroesParameters(const CampaignTravel & travelOptions)
{
	// TODO this logic (what should be kept) should be part of CScenarioTravel and be exposed via some clean set of methods
	if(!travelOptions.whatHeroKeeps.experience)
	{
		//trimming experience
		for(auto & hero : campaignHeroReplacements)
		{
			hero.hero->initExp(gameState->getRandomGenerator());
		}
	}

	if(!travelOptions.whatHeroKeeps.primarySkills)
	{
		//trimming prim skills
		for(auto & hero : campaignHeroReplacements)
		{
			for(auto g = PrimarySkill::BEGIN; g < PrimarySkill::END; ++g)
			{
				auto sel = Selector::type()(BonusType::PRIMARY_SKILL)
					.And(Selector::subtype()(BonusSubtypeID(g)))
					.And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL));

				hero.hero->getLocalBonus(sel)->val = hero.hero->type->heroClass->primarySkillInitial[g.getNum()];
			}
		}
	}

	if(!travelOptions.whatHeroKeeps.secondarySkills)
	{
		//trimming sec skills
		for(auto & hero : campaignHeroReplacements)
		{
			hero.hero->secSkills = hero.hero->type->secSkillsInit;
			hero.hero->recreateSecondarySkillsBonuses();
		}
	}

	if(!travelOptions.whatHeroKeeps.spells)
	{
		for(auto & hero : campaignHeroReplacements)
		{
			hero.hero->removeSpellbook();
		}
	}

	if(!travelOptions.whatHeroKeeps.artifacts)
	{
		//trimming artifacts
		for(auto & hero : campaignHeroReplacements)
		{
			const auto & checkAndRemoveArtifact = [&](const ArtifactPosition & artifactPosition)
			{
				if(artifactPosition == ArtifactPosition::SPELLBOOK)
					return; // do not handle spellbook this way

				const ArtSlotInfo *info = hero.hero->getSlot(artifactPosition);
				if(!info)
					return;

				// TODO: why would there be nullptr artifacts?
				const CArtifactInstance *art = info->artifact;
				if(!art)
					return;

				bool takeable = travelOptions.artifactsKeptByHero.count(art->artType->getId());

				if (takeable)
					hero.transferrableArtifacts.push_back(artifactPosition);

				ArtifactLocation al(hero.hero->id, artifactPosition);
				if(!takeable && !hero.hero->getSlot(al.slot)->locked)  //don't try removing locked artifacts -> it crashes #1719
					hero.hero->getArt(al.slot)->removeFrom(*hero.hero, al.slot);
			};

			// process on copy - removal of artifact will invalidate container
			auto artifactsWorn = hero.hero->artifactsWorn;
			for(const auto & art : artifactsWorn)
				checkAndRemoveArtifact(art.first);

			// process in reverse - removal of artifact will shift all artifacts after this one
			for(int slotNumber = hero.hero->artifactsInBackpack.size() - 1; slotNumber >= 0; slotNumber--)
				checkAndRemoveArtifact(ArtifactPosition::BACKPACK_START + slotNumber);
		}
	}

	//trimming creatures
	for(auto & hero : campaignHeroReplacements)
	{
		auto shouldSlotBeErased = [&](const std::pair<SlotID, CStackInstance *> & j) -> bool
		{
			CreatureID crid = j.second->getCreatureID();
			return !travelOptions.monstersKeptByHero.count(crid);
		};

		auto stacksCopy = hero.hero->stacks; //copy of the map, so we can iterate iover it and remove stacks
		for(auto &slotPair : stacksCopy)
			if(shouldSlotBeErased(slotPair))
				hero.hero->eraseStack(slotPair.first);
	}

	// Removing short-term bonuses
	for(auto & hero : campaignHeroReplacements)
	{
		hero.hero->removeBonusesRecursive(CSelector(Bonus::OneDay)
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
			HeroTypeID heroTypeId = HeroTypeID(campaignBonus->info2);
			if(heroTypeId.getNum() == 0xffff) // random bonus hero
			{
				heroTypeId = gameState->pickUnusedHeroTypeRandomly(playerColor);
			}

			gameState->placeStartingHero(playerColor, HeroTypeID(heroTypeId), gameState->map->players[playerColor.getNum()].posOfMainTown);
		}
	}

	logGlobal->debug("\tGenerate list of hero placeholders");
	generateCampaignHeroesToReplace();

	logGlobal->debug("\tPrepare crossover heroes");
	trimCrossoverHeroesParameters(campaignState->scenario(*campaignState->currentScenario()).travelOptions);

	// remove same heroes on the map which will be added through crossover heroes
	// INFO: we will remove heroes because later it may be possible that the API doesn't allow having heroes
	// with the same hero type id
	std::vector<CGHeroInstance *> removedHeroes;

	std::set<HeroTypeID> reservedHeroes = campaignState->getReservedHeroes();
	std::set<HeroTypeID> heroesToRemove;

	for (auto const & heroID : reservedHeroes )
	{
		// Do not replace reserved heroes initially, e.g. in 1st campaign scenario in which they appear
		if (!campaignState->getHeroByType(heroID).isNull())
			heroesToRemove.insert(heroID);
	}

	for(auto & replacement : campaignHeroReplacements)
		if (replacement.heroPlaceholderId.hasValue())
			heroesToRemove.insert(replacement.hero->getHeroType());

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
	replaceHeroesPlaceholders();

	// now add removed heroes again with unused type ID
	for(auto * hero : removedHeroes)
	{
		HeroTypeID heroTypeId;
		if(hero->ID == Obj::HERO)
		{
			heroTypeId = gameState->pickUnusedHeroTypeRandomly(hero->tempOwner);
		}
		else if(hero->ID == Obj::PRISON)
		{
			auto unusedHeroTypeIds = gameState->getUnusedAllowedHeroes();
			if(!unusedHeroTypeIds.empty())
			{
				heroTypeId = (*RandomGeneratorUtil::nextItem(unusedHeroTypeIds, gameState->getRandomGenerator()));
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

		hero->setHeroType(heroTypeId);
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
				scroll->putAt(*hero, slot);
			else
				logGlobal->error("Cannot give starting scroll - no free slots!");
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const ui8 * ptr = reinterpret_cast<const ui8 *>(&curBonus->info2);
			for(auto g = PrimarySkill::BEGIN; g < PrimarySkill::END; ++g)
			{
				int val = ptr[g.getNum()];
				if(val == 0)
					continue;

				auto currentScenario = *gameState->scenarioOps->campState->currentScenario();
				auto bb = std::make_shared<Bonus>( BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CAMPAIGN_BONUS, val, BonusSourceID(currentScenario), BonusSubtypeID(g) );
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

void CGameStateCampaign::replaceHeroesPlaceholders()
{
	for(const auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		if (!campaignHeroReplacement.heroPlaceholderId.hasValue())
			continue;

		auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(gameState->getObjInstance(campaignHeroReplacement.heroPlaceholderId));

		CGHeroInstance *heroToPlace = campaignHeroReplacement.hero;
		heroToPlace->id = campaignHeroReplacement.heroPlaceholderId;
		if(heroPlaceholder->tempOwner.isValidPlayer())
			heroToPlace->tempOwner = heroPlaceholder->tempOwner;
		heroToPlace->pos = heroPlaceholder->pos;
		heroToPlace->type = heroToPlace->getHeroType().toHeroType();
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

void CGameStateCampaign::transferMissingArtifacts(const CampaignTravel & travelOptions)
{
	CGHeroInstance * receiver = nullptr;

	for(auto obj : gameState->map->objects)
	{
		if (!obj)
			continue;

		if (obj->ID != Obj::HERO)
			continue;

		auto * hero = dynamic_cast<CGHeroInstance *>(obj.get());

		if (gameState->getPlayerState(hero->getOwner())->isHuman())
		{
			receiver = hero;
			break;
		}
	}
	assert(receiver);

	for(const auto & campaignHeroReplacement : campaignHeroReplacements)
	{
		if (campaignHeroReplacement.heroPlaceholderId.hasValue())
			continue;

		auto * donorHero = campaignHeroReplacement.hero;

		for (auto const & artLocation : campaignHeroReplacement.transferrableArtifacts)
		{
			auto * artifact = donorHero->getArt(artLocation);
			artifact->removeFrom(*donorHero, artLocation);

			if (receiver)
			{
				const auto slot = ArtifactUtils::getArtAnyPosition(receiver, artifact->getTypeId());
				if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
					artifact->putAt(*receiver, slot);
				else
					logGlobal->error("Cannot transfer artifact - no free slots!");
			}
			else
				logGlobal->error("Cannot transfer artifact - no receiver hero!");
		}

		delete donorHero;
	}
}

void CGameStateCampaign::generateCampaignHeroesToReplace()
{
	auto campaignState = gameState->scenarioOps->campState;

	std::vector<CGHeroPlaceholder *> placeholdersByPower;
	std::vector<CGHeroPlaceholder *> placeholdersByType;

	campaignHeroReplacements.clear();

	// find all placeholders on map
	for(auto obj : gameState->map->objects)
	{
		if(!obj)
			continue;

		if (obj->ID != Obj::HERO_PLACEHOLDER)
			continue;

		auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());

		// only 1 field must be set
		assert(heroPlaceholder->powerRank.has_value() != heroPlaceholder->heroType.has_value());

		if(heroPlaceholder->powerRank)
			placeholdersByPower.push_back(heroPlaceholder);

		if(heroPlaceholder->heroType)
			placeholdersByType.push_back(heroPlaceholder);
	}

	//selecting heroes by type
	for(const auto * placeholder : placeholdersByType)
	{
		const auto & node = campaignState->getHeroByType(*placeholder->heroType);
		if (node.isNull())
		{
			logGlobal->info("Hero crossover: Unable to replace placeholder for %d (%s)!", placeholder->heroType->getNum(), VLC->heroTypes()->getById(*placeholder->heroType)->getNameTranslated());
			continue;
		}

		CGHeroInstance * hero = campaignState->crossoverDeserialize(node, gameState->map);

		logGlobal->info("Hero crossover: Loading placeholder for %d (%s)", hero->getHeroType(), hero->getNameTranslated());

		campaignHeroReplacements.emplace_back(hero, placeholder->id);
	}

	auto lastScenario = getHeroesSourceScenario();

	if (lastScenario)
	{
		// sort hero placeholders descending power
		boost::range::sort(placeholdersByPower, [](const CGHeroPlaceholder * a, const CGHeroPlaceholder * b)
		{
			return *a->powerRank > *b->powerRank;
		});

		const auto & nodeList = campaignState->getHeroesByPower(lastScenario.value());
		auto nodeListIter = nodeList.begin();

		for(const auto * placeholder : placeholdersByPower)
		{
			if (nodeListIter == nodeList.end())
				break;

			CGHeroInstance * hero = campaignState->crossoverDeserialize(*nodeListIter, gameState->map);
			nodeListIter++;

			logGlobal->info("Hero crossover: Loading placeholder as %d (%s)", hero->getHeroType(), hero->getNameTranslated());

			campaignHeroReplacements.emplace_back(hero, placeholder->id);
		}

		// Add remaining heroes without placeholders - to transfer their artifacts to placed heroes
		for (;nodeListIter != nodeList.end(); ++nodeListIter)
		{
			CGHeroInstance * hero = campaignState->crossoverDeserialize(*nodeListIter, gameState->map);
			campaignHeroReplacements.emplace_back(hero, ObjectInstanceID::NONE);
		}
	}
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
				if (heroe->getHeroType().getNum() == chosenBonus->info1)
				{
					giveCampaignBonusToHero(heroe);
					break;
				}
			}
		}
	}

	auto campaignState = gameState->scenarioOps->campState;
	auto * yog = gameState->getUsedHero(HeroTypeID::SOLMYR);
	if (yog && boost::starts_with(campaignState->getFilename(), "DATA/YOG") && campaignState->currentScenario()->getNum() == 2)
	{
		assert(yog->isCampaignYog());
		gameState->giveHeroArtifact(yog, ArtifactID::ANGELIC_ALLIANCE);
	}

	transferMissingArtifacts(campaignState->scenario(*campaignState->currentScenario()).travelOptions);
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
			std::vector<GameResID> res; //resources we will give
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
			newBuilding = CBuildingHandler::campToERMU(chosenBonus->info1, town->getFaction(), town->builtBuildings);

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

std::unique_ptr<CMap> CGameStateCampaign::getCurrentMap()
{
	return gameState->scenarioOps->campState->getMap(CampaignScenarioID::NONE, gameState->callback);
}

VCMI_LIB_NAMESPACE_END

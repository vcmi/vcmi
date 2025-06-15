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
#include "../entities/artifact/ArtifactUtils.h"
#include "../entities/artifact/CArtifact.h"
#include "../entities/building/CBuilding.h"
#include "../entities/hero/CHeroClass.h"
#include "../entities/hero/CHero.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../networkPacks/ArtifactLocation.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../StartInfo.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"
#include "mapping/MapFormatSettings.h"

#include <vstd/RNG.h>
#include <vcmi/HeroTypeService.h>

VCMI_LIB_NAMESPACE_BEGIN

CampaignHeroReplacement::CampaignHeroReplacement(std::shared_ptr<CGHeroInstance> hero, const ObjectInstanceID & heroPlaceholderId):
	hero(hero),
	heroPlaceholderId(heroPlaceholderId)
{
}

CGameStateCampaign::CGameStateCampaign() = default;

CGameStateCampaign::CGameStateCampaign(CGameState * owner):
	gameState(owner)
{
	assert(gameState->scenarioOps->mode == EStartMode::CAMPAIGN);
	assert(gameState->scenarioOps->campState != nullptr);
}

void CGameStateCampaign::setGamestate(CGameState * owner)
{
	gameState = owner;
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

	if(bonus && bonus->getType() == CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO)
		return bonus->getValue<CampaignBonusHeroesFromScenario>().scenario;

	return campaignState->lastScenario();
}

void CGameStateCampaign::trimCrossoverHeroesParameters(vstd::RNG & randomGenerator, const CampaignTravel & travelOptions)
{
	// TODO this logic (what should be kept) should be part of CScenarioTravel and be exposed via some clean set of methods
	if(!travelOptions.whatHeroKeeps.experience)
	{
		//trimming experience
		for(auto & hero : campaignHeroReplacements)
		{
			hero.hero->initExp(randomGenerator);
		}
	}

	if(!travelOptions.whatHeroKeeps.primarySkills)
	{
		//trimming prim skills
		for(auto & hero : campaignHeroReplacements)
		{
			for(auto skill : PrimarySkill::ALL_SKILLS())
			{
				auto sel = Selector::type()(BonusType::PRIMARY_SKILL)
					.And(Selector::subtype()(BonusSubtypeID(skill)))
					.And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL));

				hero.hero->getLocalBonus(sel)->val = hero.hero->getHeroClass()->primarySkillInitial[skill.getNum()];
			}
		}
	}

	if(!travelOptions.whatHeroKeeps.secondarySkills)
	{
		//trimming sec skills
		for(auto & hero : campaignHeroReplacements)
		{
			hero.hero->secSkills = hero.hero->getHeroType()->secSkillsInit;
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
			const auto & checkAndRemoveArtifact = [&](const ArtifactPosition & artifactPosition) -> bool
			{
				if(artifactPosition == ArtifactPosition::SPELLBOOK)
					return false; // do not handle spellbook this way

				const ArtSlotInfo *info = hero.hero->getSlot(artifactPosition);
				if(!info)
					return false;

				// FIXME: double-check how H3 handles case of transferring components of a combined artifact if entire combined artifact is not transferrable
				// For example, what happens if hero has assembled Angelic Alliance, AA is not marked is transferrable, but Sandals can be transferred? Should artifact be disassembled?
				if (info->locked)
					return false;

				// TODO: why would there be nullptr artifacts?
				const CArtifactInstance *art = info->getArt();
				if(!art)
					return false;

				ArtifactLocation al(hero.hero->id, artifactPosition);

				bool takeable = travelOptions.artifactsKeptByHero.count(art->getTypeId());
				bool locked = hero.hero->getSlot(al.slot)->locked;

				if (!locked && takeable)
				{
					logGlobal->debug("Artifact %s from slot %d of hero %s will be transferred to next scenario", art->getType()->getJsonKey(), al.slot.getNum(), hero.hero->getHeroTypeName());
					hero.transferrableArtifacts.push_back(artifactPosition);
				}

				if (!locked && !takeable)
				{
					logGlobal->debug("Removing artifact %s from slot %d of hero %s", art->getType()->getJsonKey(), al.slot.getNum(), hero.hero->getHeroTypeName());
					gameState->map->removeArtifactInstance(*hero.hero, al.slot);
					return true;
				}
				return false;
			};

			// process on copy - removal of artifact will invalidate container
			auto artifactsWorn = hero.hero->artifactsWorn;
			for(const auto & art : artifactsWorn)
				checkAndRemoveArtifact(art.first);

			for (int slotNumber = 0; slotNumber < hero.hero->artifactsInBackpack.size();)
			{
				if (checkAndRemoveArtifact(ArtifactPosition::BACKPACK_START + slotNumber))
					continue; // artifact was removed and backpack slots were shifted -> test this slot again
				else
					slotNumber++; // artifact was kept for transfer -> test next slot
			};
		}
	}

	//trimming creatures
	for(auto & hero : campaignHeroReplacements)
	{
		auto shouldSlotBeErased = [&](CStackInstance & j) -> bool
		{
			CreatureID crid = j.getCreatureID();
			return !travelOptions.monstersKeptByHero.count(crid);
		};

		//generate list of slots without removing anything first to avoid iterator invalidation
		std::vector<SlotID> slotsToErase;

		for(auto &slotPair : hero.hero->Slots())
			if(shouldSlotBeErased(*slotPair.second))
				slotsToErase.push_back(slotPair.first);

		for (const auto slotID : slotsToErase)
			hero.hero->eraseStack(slotID);
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

void CGameStateCampaign::placeCampaignHeroes(vstd::RNG & randomGenerator)
{
	// place bonus hero
	const auto & campaignState = gameState->scenarioOps->campState;
	const auto & campaignBonus = campaignState->getBonus(*campaignState->currentScenario());
	bool campaignGiveHero = campaignBonus && campaignBonus->getType() == CampaignBonusType::HERO;

	if(campaignGiveHero)
	{
		const auto & campaignBonusValue = campaignBonus->getValue<CampaignBonusStartingHero>();
		const auto & playerColor = campaignBonusValue.startingPlayer;
		const auto & it = gameState->scenarioOps->playerInfos.find(playerColor);
		if(it != gameState->scenarioOps->playerInfos.end())
		{
			HeroTypeID heroTypeId = campaignBonusValue.hero;
			if(heroTypeId == HeroTypeID::CAMP_RANDOM) // random bonus hero
			{
				heroTypeId = gameState->pickUnusedHeroTypeRandomly(randomGenerator, playerColor);
			}

			gameState->placeStartingHero(playerColor, HeroTypeID(heroTypeId), gameState->map->players[playerColor.getNum()].posOfMainTown);
		}
	}

	logGlobal->debug("\tGenerate list of hero placeholders");
	generateCampaignHeroesToReplace();

	logGlobal->debug("\tPrepare crossover heroes");
	trimCrossoverHeroesParameters(randomGenerator, campaignState->scenario(*campaignState->currentScenario()).travelOptions);

	// remove same heroes on the map which will be added through crossover heroes
	// INFO: we will remove heroes because later it may be possible that the API doesn't allow having heroes
	// with the same hero type id
	std::vector<std::shared_ptr<CGObjectInstance>> removedHeroes;

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
			heroesToRemove.insert(replacement.hero->getHeroTypeID());

	for(auto & heroID : heroesToRemove)
	{
		auto * hero = gameState->getUsedHero(heroID);
		if(hero)
		{
			removedHeroes.push_back(gameState->map->eraseObject(hero->id));
		}
	}

	logGlobal->debug("\tReplace placeholders with heroes");
	replaceHeroesPlaceholders();

	// now add removed heroes again with unused type ID
	for(auto object : removedHeroes)
	{
		auto hero = dynamic_cast<CGHeroInstance*>(object.get());
		HeroTypeID heroTypeId;
		if(hero->ID == Obj::HERO)
		{
			heroTypeId = gameState->pickUnusedHeroTypeRandomly(randomGenerator, hero->tempOwner);
		}
		else if(hero->ID == Obj::PRISON)
		{
			auto unusedHeroTypeIds = gameState->getUnusedAllowedHeroes();
			if(!unusedHeroTypeIds.empty())
			{
				heroTypeId = (*RandomGeneratorUtil::nextItem(unusedHeroTypeIds, randomGenerator));
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
		gameState->map->getEditManager()->insertObject(object);
	}
}

void CGameStateCampaign::giveCampaignBonusToHero(CGHeroInstance * hero)
{
	auto curBonus = currentBonus();
	if(!curBonus)
		return;

	assert(curBonus->isBonusForHero());

	//apply bonus
	switch(curBonus->getType())
	{
		case CampaignBonusType::SPELL:
		{
			const auto & bonusValue = curBonus->getValue<CampaignBonusSpell>();
			hero->addSpellToSpellbook(bonusValue.spell);
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			const auto & bonusValue = curBonus->getValue<CampaignBonusCreatures>();
			for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
			{
				if(hero->slotEmpty(SlotID(i)))
				{
					hero->addToSlot(SlotID(i), bonusValue.creature, bonusValue.amount);
					break;
				}
			}
			break;
		}
		case CampaignBonusType::ARTIFACT:
		{
			const auto & bonusValue = curBonus->getValue<CampaignBonusArtifact>();
			if(!gameState->giveHeroArtifact(hero, bonusValue.artifact))
				logGlobal->error("Cannot give starting artifact - no free slots!");
			break;
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			const auto & bonusValue = curBonus->getValue<CampaignBonusSpellScroll>();
			const auto scroll = gameState->createScroll(bonusValue.spell);
			const auto slot = ArtifactUtils::getArtAnyPosition(hero, scroll->getTypeId());
			if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
				gameState->map->putArtifactInstance(*hero, scroll->getId(), slot);
			else
				logGlobal->error("Cannot give starting scroll - no free slots!");
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const auto & bonusValue = curBonus->getValue<CampaignBonusPrimarySkill>();
			for(auto skill : PrimarySkill::ALL_SKILLS())
			{
				int val = bonusValue.amounts[skill.getNum()];
				if(val == 0)
					continue;

				auto currentScenario = *gameState->scenarioOps->campState->currentScenario();
				auto bb = std::make_shared<Bonus>( BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CAMPAIGN_BONUS, val, BonusSourceID(currentScenario), BonusSubtypeID(skill) );
				hero->addNewBonus(bb);
			}
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			const auto & bonusValue = curBonus->getValue<CampaignBonusSecondarySkill>();
			hero->setSecSkillLevel(bonusValue.skill, bonusValue.mastery, ChangeValueMode::ABSOLUTE);
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

		auto heroPlaceholder = gameState->map->getObject(campaignHeroReplacement.heroPlaceholderId);
		auto heroToPlace = campaignHeroReplacement.hero;

		if(heroPlaceholder->tempOwner.isValidPlayer())
			heroToPlace->tempOwner = heroPlaceholder->tempOwner;

		heroToPlace->setAnchorPos(heroPlaceholder->anchorPos());
		heroToPlace->setHeroType(heroToPlace->getHeroTypeID());
		heroToPlace->appearance = heroToPlace->getObjectHandler()->getTemplates().front();

		gameState->map->replaceObject(campaignHeroReplacement.heroPlaceholderId, heroToPlace);
	}
}

void CGameStateCampaign::transferMissingArtifacts(const CampaignTravel & travelOptions)
{
	CGHeroInstance * receiver = nullptr;

	for(auto hero : gameState->map->getObjects<CGHeroInstance>())
	{
		if (!hero->getOwner().isValidPlayer())
			continue; // prisons

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

		auto donorHero = campaignHeroReplacement.hero;

		if (!donorHero)
			throw std::runtime_error("Failed to find hero to take artifacts from! Scenario: " + gameState->map->name.toString());

		// process in reverse - 2nd artifact from a backpack must be processed before 1st one to avoid invalidation of artifact positions
		for (auto const & artLocation : boost::adaptors::reverse(campaignHeroReplacement.transferrableArtifacts))
		{
			auto * artifact = donorHero->getArt(artLocation);

			logGlobal->debug("Removing artifact %s from slot %d of hero %s for transfer", artifact->getType()->getJsonKey(), artLocation.getNum(), donorHero->getHeroTypeName());
			gameState->map->removeArtifactInstance(*donorHero, artLocation);

			if (receiver)
			{
				logGlobal->debug("Granting artifact %s to hero %s for transfer", artifact->getType()->getJsonKey(), receiver->getHeroTypeName());

				const auto slot = ArtifactUtils::getArtAnyPosition(receiver, artifact->getTypeId());
				if(ArtifactUtils::isSlotEquipment(slot) || ArtifactUtils::isSlotBackpack(slot))
					gameState->map->putArtifactInstance(*receiver, artifact->getId(), slot);
				else
					logGlobal->error("Cannot transfer artifact - no free slots!");
			}
			else
				logGlobal->error("Cannot transfer artifact - no receiver hero!");
		}
	}
}

void CGameStateCampaign::generateCampaignHeroesToReplace()
{
	auto campaignState = gameState->scenarioOps->campState;

	std::vector<CGHeroPlaceholder *> placeholdersByPower;
	std::vector<CGHeroPlaceholder *> placeholdersByType;

	campaignHeroReplacements.clear();

	// find all placeholders on map
	for(auto heroPlaceholder : gameState->map->getObjects<CGHeroPlaceholder>())
	{
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
			logGlobal->info("Hero crossover: Unable to replace placeholder for %d (%s)!", placeholder->heroType->getNum(), LIBRARY->heroTypes()->getById(*placeholder->heroType)->getNameTranslated());
			continue;
		}

		auto hero = campaignState->crossoverDeserialize(node, gameState->map.get());

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

			if (!gameState->players.count(placeholder->getOwner()))
				continue; // illegal?

			// It looks like heroes placeholder by power can only be replaced for human player
			// Example where this is important: Spoils of War -> Greed
			// Meanwhile, placeholders by hero ID can be replaced for AI as well
			// Example: Armageddon's Blade -> To Kill A Hero
			if (!gameState->players.at(placeholder->getOwner()).isHuman())
				continue;

			auto hero = campaignState->crossoverDeserialize(*nodeListIter, gameState->map.get());
			nodeListIter++;

			logGlobal->info("Hero crossover: Loading placeholder as %d (%s)", hero->getHeroType(), hero->getNameTranslated());

			campaignHeroReplacements.emplace_back(hero, placeholder->id);
		}

		// Add remaining heroes without placeholders - to transfer their artifacts to placed heroes
		for (;nodeListIter != nodeList.end(); ++nodeListIter)
		{
			auto hero = campaignState->crossoverDeserialize(*nodeListIter, gameState->map.get());
			campaignHeroReplacements.emplace_back(hero, ObjectInstanceID::NONE);
		}
	}
}

void CGameStateCampaign::initHeroes()
{
	auto chosenBonus = currentBonus();
	if (chosenBonus && chosenBonus->isBonusForHero() && chosenBonus->getTargetedHero() != HeroTypeID::CAMP_GENERATED.getNum()) //exclude generated heroes
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

		const auto & heroes = gameState->players.at(humanPlayer).getHeroes();

		if (chosenBonus->getTargetedHero() == HeroTypeID::CAMP_STRONGEST.getNum()) //most powerful
		{
			int maxB = -1;
			for (int b=0; b<heroes.size(); ++b)
			{
				if(maxB == -1 || CGHeroInstance::compareCampaignValue(heroes[b], heroes[maxB]))
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
			for (auto & hero : heroes)
			{
				if (hero->getHeroTypeID().getNum() == chosenBonus->getTargetedHero())
				{
					giveCampaignBonusToHero(hero);
					break;
				}
			}
		}
	}

	auto campaignState = gameState->scenarioOps->campState;
	if (campaignState->getYogWizardID().hasValue() && boost::starts_with(campaignState->getFilename(), "DATA/YOG") && campaignState->currentScenario()->getNum() == 2)
	{
		auto * yog = gameState->getUsedHero(campaignState->getYogWizardID());
		assert(yog);
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

	const auto & chosenBonus = currentBonus();
	if(chosenBonus && chosenBonus->getType() == CampaignBonusType::RESOURCE)
	{
		const auto & bonusValue = chosenBonus->getValue<CampaignBonusStartingResources>();

		std::vector<const PlayerSettings *> people = getHumanPlayerInfo(); //players we will give resource bonus
		for(const PlayerSettings *ps : people)
		{
			std::vector<GameResID> res; //resources we will give
			switch (bonusValue.resource.toEnum())
			{
				case EGameResID::COMMON: //wood+ore
					res.push_back(GameResID(EGameResID::WOOD));
					res.push_back(GameResID(EGameResID::ORE));
					break;
				case EGameResID::RARE:  //rare
					res.push_back(GameResID(EGameResID::MERCURY));
					res.push_back(GameResID(EGameResID::SULFUR));
					res.push_back(GameResID(EGameResID::CRYSTAL));
					res.push_back(GameResID(EGameResID::GEMS));
					break;
				default:
					res.push_back(bonusValue.resource);
					break;
			}

			for (auto & re : res)
				gameState->players.at(ps->color).resources[re] += bonusValue.amount;
		}
	}
}

void CGameStateCampaign::initTowns()
{
	auto chosenBonus = currentBonus();

	if (!chosenBonus)
		return;

	if (chosenBonus->getType() != CampaignBonusType::BUILDING)
		return;

	const auto & bonusValue = chosenBonus->getValue<CampaignBonusBuilding>();

	for (const auto & townID : gameState->map->getAllTowns())
	{
		auto town = gameState->getTown(townID);

		PlayerState * owner = gameState->getPlayerState(town->getOwner());
		if (!owner)
			continue;

		PlayerInfo & pi = gameState->map->players[owner->color.getNum()];

		if (!owner->human)
			continue;

		if (town->anchorPos() != pi.posOfMainTown)
			continue;

		BuildingID newBuilding = bonusValue.buildingDecoded;

		if (bonusValue.buildingH3M.hasValue())
		{
			auto mapping = LIBRARY->mapFormat->getMapping(gameState->scenarioOps->campState->getFormat());
			newBuilding = mapping.remapBuilding(town->getFactionID(), bonusValue.buildingH3M);
		}

		// Build granted building & all prerequisites - e.g. Mages Guild Lvl 3 should also give Mages Guild Lvl 1 & 2
		while(true)
		{
			if (newBuilding == BuildingID::NONE)
				break;

			if(town->hasBuilt(newBuilding))
				break;

			town->addBuilding(newBuilding);

			const auto & building = town->getTown()->buildings.at(newBuilding);
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

	if(campaignBonus->getType() == CampaignBonusType::HERO && playerColor == PlayerColor(campaignBonus->getValue<CampaignBonusStartingHero>().startingPlayer))
		return true;
	return false;
}

std::unique_ptr<CMap> CGameStateCampaign::getCurrentMap()
{
	return gameState->scenarioOps->campState->getMap(CampaignScenarioID::NONE, gameState);
}

VCMI_LIB_NAMESPACE_END

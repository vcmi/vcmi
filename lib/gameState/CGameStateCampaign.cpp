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

void CrossoverHeroesList::addHeroToBothLists(CGHeroInstance * hero)
{
	heroesFromPreviousScenario.push_back(hero);
	heroesFromAnyPreviousScenarios.push_back(hero);
}

void CrossoverHeroesList::removeHeroFromBothLists(CGHeroInstance * hero)
{
	heroesFromPreviousScenario -= hero;
	heroesFromAnyPreviousScenarios -= hero;
}

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

CrossoverHeroesList CGameStateCampaign::getCrossoverHeroesFromPreviousScenarios() const
{
	CrossoverHeroesList crossoverHeroes;

	auto campaignState = gameState->scenarioOps->campState;
	auto bonus = campaignState->getBonusForCurrentMap();
	if(bonus && bonus->type == CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO)
	{
		auto scenarioID = static_cast<CampaignScenarioID>(bonus->info2);

		std::vector<CGHeroInstance *> heroes;
		for(auto & node : campaignState->getCrossoverHeroes(scenarioID))
		{
			auto * h = CampaignState::crossoverDeserialize(node);
			heroes.push_back(h);
		}
		crossoverHeroes.heroesFromAnyPreviousScenarios = heroes;
		crossoverHeroes.heroesFromPreviousScenario = heroes;

		return crossoverHeroes;
	}

	if(!campaignState->lastScenario())
		return crossoverHeroes;

	for(auto mapNr : campaignState->conqueredScenarios())
	{
		// create a list of deleted heroes
		auto lostCrossoverHeroes = campaignState->getLostCrossoverHeroes(mapNr);

		// remove heroes which didn't reached the end of the scenario, but were available at the start
		for(auto * hero : lostCrossoverHeroes)
		{
			//					auto hero = CCampaignState::crossoverDeserialize(node);
			vstd::erase_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [hero](CGHeroInstance * h)
			{
				return hero->subID == h->subID;
			});
		}

		// now add heroes which completed the scenario
		for(auto node : campaignState->getCrossoverHeroes(mapNr))
		{
			auto * hero = CampaignState::crossoverDeserialize(node);
			// add new heroes and replace old heroes with newer ones
			auto it = range::find_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [hero](CGHeroInstance * h)
			{
				return hero->subID == h->subID;
			});

			if(it != crossoverHeroes.heroesFromAnyPreviousScenarios.end())
			{
				// replace old hero with newer one
				crossoverHeroes.heroesFromAnyPreviousScenarios[it - crossoverHeroes.heroesFromAnyPreviousScenarios.begin()] = hero;
			}
			else
			{
				// add new hero
				crossoverHeroes.heroesFromAnyPreviousScenarios.push_back(hero);
			}

			if(mapNr == campaignState->lastScenario())
			{
				crossoverHeroes.heroesFromPreviousScenario.push_back(hero);
			}
		}
	}
	return crossoverHeroes;
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
				if(artifactPosition == ArtifactPosition::SPELLBOOK) continue; // do not handle spellbook this way

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
	// WARNING: CURRENT CODE IS LIKELY INCORRECT AND LEADS TO MULTIPLE ISSUES WITH HERO TRANSFER
	// Approximate behavior according to testing H3 game logic.
	// 1) definitions:
	// - 'reserved heroes' are heroes that have fixed placeholder in unfinished maps. See CMapHeader::reservedCampaignHeroes
	// - 'campaign pool' are serialized heroes and is unique to a campaign
	// - 'scenario pool' are serialized heroes and is unique to a scenario
	//
	// 2) scenario end logic:
	// - at end of scenario, all 'reserved heroes' of a player go to 'campaign pool'
	// - at end of scenario, rest of player's heroes go to 'scenario pool'
	//
	// 3) scenario start logic
	// - at scenario start, all heroes that are placed on map but already exist in 'campaign pool' and are still 'reserved heroes' are replaced with other, randomly selected heroes (and probably marked as unavailable in map)
	// - at scenario start, all fixed placeholders are replaced with heroes from 'campaign pool', if exist
	// - at scenario start, all power placeholders owned by player are replaced by heroes from 'scenario pool' of last complete map, if exist
	// - exception: if starting bonus is 'select player' then power placeholders are taken from 'scenario pool' of linked map

	// place bonus hero
	auto campaignBonus = gameState->scenarioOps->campState->getBonusForCurrentMap();
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

	// replace heroes placeholders
	auto crossoverHeroes = getCrossoverHeroesFromPreviousScenarios();

	if(!crossoverHeroes.heroesFromAnyPreviousScenarios.empty())
	{
		logGlobal->debug("\tGenerate list of hero placeholders");
		auto campaignHeroReplacements = generateCampaignHeroesToReplace(crossoverHeroes);

		logGlobal->debug("\tPrepare crossover heroes");
		trimCrossoverHeroesParameters(campaignHeroReplacements, gameState->scenarioOps->campState->getCurrentScenario().travelOptions);

		// remove same heroes on the map which will be added through crossover heroes
		// INFO: we will remove heroes because later it may be possible that the API doesn't allow having heroes
		// with the same hero type id
		std::vector<CGHeroInstance *> removedHeroes;

		for(auto & campaignHeroReplacement : campaignHeroReplacements)
		{
			auto * hero = gameState->getUsedHero(HeroTypeID(campaignHeroReplacement.hero->subID));
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
}

void CGameStateCampaign::giveCampaignBonusToHero(CGHeroInstance * hero)
{
	const std::optional<CampaignBonus> & curBonus = gameState->scenarioOps->campState->getBonusForCurrentMap();
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
		heroToPlace->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO,
															   heroToPlace->type->heroClass->getIndex())->getTemplates().front();

		for(auto &&i : heroToPlace->stacks)
			i.second->type = VLC->creh->objects[i.second->getCreatureID()];

		auto fixArtifact = [&](CArtifactInstance * art)
		{
			art->artType = VLC->arth->objects[art->artType->getId()];
			gameState->map->artInstances.emplace_back(art);
			art->id = ArtifactInstanceID((si32)gameState->map->artInstances.size() - 1);
		};

		for(auto &&i : heroToPlace->artifactsWorn)
			fixArtifact(i.second.artifact);
		for(auto &&i : heroToPlace->artifactsInBackpack)
			fixArtifact(i.artifact);

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

std::vector<CampaignHeroReplacement> CGameStateCampaign::generateCampaignHeroesToReplace(CrossoverHeroesList & crossoverHeroes)
{
	std::vector<CampaignHeroReplacement> campaignHeroReplacements;

	//selecting heroes by type
	for(auto obj : gameState->map->objects)
	{
		if(obj && obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(heroPlaceholder->subID != 0xFF) //select by type
			{
				auto it = range::find_if(crossoverHeroes.heroesFromAnyPreviousScenarios, [heroPlaceholder](CGHeroInstance * hero)
				{
					return hero->subID == heroPlaceholder->subID;
				});

				if(it != crossoverHeroes.heroesFromAnyPreviousScenarios.end())
				{
					auto * hero = *it;
					crossoverHeroes.removeHeroFromBothLists(hero);
					campaignHeroReplacements.emplace_back(CMemorySerializer::deepCopy(*hero).release(), heroPlaceholder->id);
				}
			}
		}
	}

	//selecting heroes by power
	range::sort(crossoverHeroes.heroesFromPreviousScenario, [](const CGHeroInstance * a, const CGHeroInstance * b)
	{
		return a->getHeroStrength() > b->getHeroStrength();
	}); //sort, descending strength

	// sort hero placeholders descending power
	std::vector<CGHeroPlaceholder *> heroPlaceholders;
	for(auto obj : gameState->map->objects)
	{
		if(obj && obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto * heroPlaceholder = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(heroPlaceholder->subID == 0xFF) //select by power
			{
				heroPlaceholders.push_back(heroPlaceholder);
			}
		}
	}
	range::sort(heroPlaceholders, [](const CGHeroPlaceholder * a, const CGHeroPlaceholder * b)
	{
		return a->power > b->power;
	});

	for(int i = 0; i < heroPlaceholders.size(); ++i)
	{
		auto * heroPlaceholder = heroPlaceholders[i];
		if(crossoverHeroes.heroesFromPreviousScenario.size() > i)
		{
			auto * hero = crossoverHeroes.heroesFromPreviousScenario[i];
			campaignHeroReplacements.emplace_back(CMemorySerializer::deepCopy(*hero).release(), heroPlaceholder->id);
		}
	}

	return campaignHeroReplacements;
}

void CGameStateCampaign::initHeroes()
{
	auto chosenBonus = gameState->scenarioOps->campState->getBonusForCurrentMap();
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

	auto chosenBonus = gameState->scenarioOps->campState->getBonusForCurrentMap();
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
	auto chosenBonus = gameState->scenarioOps->campState->getBonusForCurrentMap();

	if (chosenBonus && chosenBonus->type == CampaignBonusType::BUILDING)
	{
		for (int g=0; g<gameState->map->towns.size(); ++g)
		{
			PlayerState * owner = gameState->getPlayerState(gameState->map->towns[g]->getOwner());
			if (owner)
			{
				PlayerInfo & pi = gameState->map->players[owner->color.getNum()];

				if (owner->human && //human-owned
					gameState->map->towns[g]->pos == pi.posOfMainTown)
				{
					BuildingID buildingId;
					if(gameState->scenarioOps->campState->getHeader().version == CampaignVersion::VCMI)
						buildingId = BuildingID(chosenBonus->info1);
					else
						buildingId = CBuildingHandler::campToERMU(chosenBonus->info1, gameState->map->towns[g]->subID, gameState->map->towns[g]->builtBuildings);

					gameState->map->towns[g]->builtBuildings.insert(buildingId);
					break;
				}
			}
		}
	}
}

bool CGameStateCampaign::playerHasStartingHero(PlayerColor playerColor) const
{
	auto campaignBonus = gameState->scenarioOps->campState->getBonusForCurrentMap();

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

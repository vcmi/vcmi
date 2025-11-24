/*
 * TreasurePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TreasurePlacer.h"
#include "../CRmgTemplate.h"
#include "../CMapGenerator.h"
#include "../Functions.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "ConnectionsPlacer.h"
#include "../RmgMap.h"
#include "../TileInfo.h"
#include "../CZonePlacer.h"
#include "PrisonHeroPlacer.h"
#include "QuestArtifactPlacer.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjectConstructors/DwellingInstanceConstructor.h"
#include "../../rewardable/Info.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGPandoraBox.h"
#include "../../mapObjects/CQuest.h"
#include "../../mapObjects/MiscObjects.h"
#include "../../CCreatureHandler.h"
#include "../../spells/CSpellHandler.h" //for choosing random spells
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void TreasurePlacer::process()
{
	if (zone.getMaxTreasureValue() == 0)
	{
		//No treasures at all
		return;
	}

	tierValues = generator.getConfig().pandoraCreatureValues;
	// Add all native creatures
	for(auto const & cre : LIBRARY->creh->objects)
	{
		if(!cre->special && cre->getFactionID() == zone.getTownType())
		{
			creatures.push_back(cre.get());
		}
	}

	// Get default objects
	addAllPossibleObjects();
	// Override with custom objects
	objects.patchWithZoneConfig(zone, this);

	auto * m = zone.getModificator<ObjectManager>();
	if(m)
		createTreasures(*m);
}

void TreasurePlacer::init()
{
	maxPrisons = 0; //Should be in the constructor, but we use macro for that
	DEPENDENCY(ObjectManager);
	DEPENDENCY(ConnectionsPlacer);
	DEPENDENCY_ALL(PrisonHeroPlacer);
	DEPENDENCY(RoadPlacer);
}

void TreasurePlacer::addObjectToRandomPool(const ObjectInfo& oi)
{
	if (oi.templates.empty())
	{
		logGlobal->error("Attempt to add ObjectInfo with no templates! Value: %d", oi.value);
		return;
	}
	if (!oi.generateObject)
	{
		logGlobal->error("Attempt to add ObjectInfo with no generateObject function! Value: %d", oi.value);
		return;
	}
	if (!oi.maxPerZone)
	{
		logGlobal->warn("Attempt to add ObjectInfo with 0 maxPerZone! Value: %d", oi.value);
		return;
	}
	RecursiveLock lock(externalAccessMutex);
	objects.addObject(oi);
}

void TreasurePlacer::addAllPossibleObjects()
{
	addCommonObjects();
	addDwellings();
	addPandoraBoxes();
	addSeerHuts();
	addPrisons();
	addScrolls();
}

void TreasurePlacer::addCommonObjects()
{
	for(auto primaryID : LIBRARY->objtypeh->knownObjects())
	{
		for(auto secondaryID : LIBRARY->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(!handler->isStaticObject() && handler->getRMGInfo().value)
			{
				auto rmgInfo = handler->getRMGInfo();
				if (rmgInfo.mapLimit || rmgInfo.value > zone.getMaxTreasureValue())
				{
					//Skip objects with per-map limit here
					continue;
				}
				ObjectInfo oi(primaryID, secondaryID);
				setBasicProperties(oi, CompoundMapObjectID(primaryID, secondaryID));

				oi.value = rmgInfo.value;
				oi.probability = rmgInfo.rarity;
				oi.maxPerZone = rmgInfo.zoneLimit;

				if(!oi.templates.empty())
					addObjectToRandomPool(oi);
			}
		}
	}
}

void TreasurePlacer::setBasicProperties(ObjectInfo & oi, CompoundMapObjectID objid) const
{
	oi.generateObject = [this, objid]() -> std::shared_ptr<CGObjectInstance>
	{
		return LIBRARY->objtypeh->getHandlerFor(objid)->create(map.mapInstance->cb, nullptr);
	};
	oi.setTemplates(objid.primaryID, objid.secondaryID, zone.getTerrainType());
}

void TreasurePlacer::addPrisons()
{
	//Generate Prison on water only if it has a template
	auto prisonTemplates = LIBRARY->objtypeh->getHandlerFor(Obj::PRISON, 0)->getTemplates(zone.getTerrainType());
	if (!prisonTemplates.empty())
	{
		PrisonHeroPlacer * prisonHeroPlacer = nullptr;
		for(auto & z : map.getZones())
		{
			prisonHeroPlacer = z.second->getModificator<PrisonHeroPlacer>();
		 	if (prisonHeroPlacer)
			{
				break;
			}
		}

		//prisons
		//levels 1, 5, 10, 20, 30
		static const int prisonsLevels = std::min(generator.getConfig().prisonExperience.size(), generator.getConfig().prisonValues.size());

		size_t prisonsLeft = getMaxPrisons();
		for (int i = prisonsLevels - 1; i >= 0; i--)
		{
			ObjectInfo oi(Obj::PRISON, 0); // Create new instance which will hold destructor operation

			oi.value = generator.getConfig().prisonValues[i];
			if (oi.value > zone.getMaxTreasureValue())
			{
				continue;
			}

			oi.generateObject = [i, this, prisonHeroPlacer]() -> std::shared_ptr<CGObjectInstance>
			{
				HeroTypeID hid = prisonHeroPlacer->drawRandomHero();
				auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PRISON, 0);
				auto obj = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(map.mapInstance->cb, nullptr));

				obj->setHeroType(hid); //will be initialized later
				obj->exp = generator.getConfig().prisonExperience[i];
				obj->setOwner(PlayerColor::NEUTRAL);

				return obj;
			};
			oi.destroyObject = [prisonHeroPlacer](CGObjectInstance& obj)
			{
				// Hero can be used again
				auto & hero = dynamic_cast<CGHeroInstance&>(obj);
				prisonHeroPlacer->restoreDrawnHero(hero.getHeroTypeID());
			};

			oi.setTemplates(Obj::PRISON, 0, zone.getTerrainType());
			oi.value = generator.getConfig().prisonValues[i];
			oi.probability = 30;

			//Distribute all allowed prisons, starting from the most valuable
			oi.maxPerZone = (std::ceil((float)prisonsLeft / (i + 1)));
			prisonsLeft -= oi.maxPerZone;
			if(!oi.templates.empty())
				addObjectToRandomPool(oi);
		}
	}
}

void TreasurePlacer::addDwellings()
{
	if(zone.getType() == ETemplateZoneType::WATER)
		return;

	//dwellings
	auto dwellingTypes = {Obj::CREATURE_GENERATOR1, Obj::CREATURE_GENERATOR4};
	
	for(auto dwellingType : dwellingTypes)
	{
		auto subObjects = LIBRARY->objtypeh->knownSubObjects(dwellingType);
		
		if(dwellingType == Obj::CREATURE_GENERATOR1)
		{
			//don't spawn original "neutral" dwellings that got replaced by Conflux dwellings in AB
			static const MapObjectSubID elementalConfluxROE[] = {7, 13, 16, 47};
			for(auto const & i : elementalConfluxROE)
				vstd::erase_if_present(subObjects, i);
		}
		
		for(auto secondaryID : subObjects)
		{
			const auto * dwellingHandler = dynamic_cast<const DwellingInstanceConstructor *>(LIBRARY->objtypeh->getHandlerFor(dwellingType, secondaryID).get());
			auto creatures = dwellingHandler->getProducedCreatures();
			if(creatures.empty())
				continue;

			const auto * cre = creatures.front();
			if(cre->getFactionID() == zone.getTownType())
			{
				auto nativeZonesCount = static_cast<float>(map.getZoneCount(cre->getFactionID()));
				ObjectInfo oi(dwellingType, secondaryID);
				setBasicProperties(oi, CompoundMapObjectID(dwellingType, secondaryID));

				oi.value = static_cast<ui32>(cre->getAIValue() * cre->getGrowth() * (1 + (nativeZonesCount / map.getTotalZoneCount()) + (nativeZonesCount / 2)));
				oi.probability = 40;
				
				oi.generateObject = [this, secondaryID, dwellingType]() -> std::shared_ptr<CGObjectInstance>
				{
					auto obj = LIBRARY->objtypeh->getHandlerFor(dwellingType, secondaryID)->create(map.mapInstance->cb, nullptr);
					obj->tempOwner = PlayerColor::NEUTRAL;
					return obj;
				};
				if(!oi.templates.empty())
					addObjectToRandomPool(oi);
			}
		}
	}
}

void TreasurePlacer::addScrolls()
{
	if(zone.getType() == ETemplateZoneType::WATER)
		return;

	ObjectInfo oi(Obj::SPELL_SCROLL, 0);

	for(int i = 0; i < generator.getConfig().scrollValues.size(); i++)
	{
		oi.generateObject = [i, this]() -> std::shared_ptr<CGObjectInstance>
		{
			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::SPELL_SCROLL, 0);
			auto obj = std::dynamic_pointer_cast<CGArtifact>(factory->create(map.mapInstance->cb, nullptr));
			std::vector<SpellID> out;
			
			for(auto spellID : LIBRARY->spellh->getDefaultAllowed())
			{
				if(map.isAllowedSpell(spellID) && spellID.toSpell()->getLevel() == i + 1)
					out.push_back(spellID);
			}
			auto * a = map.mapInstance->createScroll(*RandomGeneratorUtil::nextItem(out, zone.getRand()));
			obj->setArtifactInstance(a);
			return obj;
		};
		oi.setTemplates(Obj::SPELL_SCROLL, 0, zone.getTerrainType());
		oi.value = generator.getConfig().scrollValues[i];
		oi.probability = 30;
		if(!oi.templates.empty())
			addObjectToRandomPool(oi);
	}
	
}

void TreasurePlacer::addPandoraBoxes()
{
	if(zone.getType() == ETemplateZoneType::WATER)
		return;

	addPandoraBoxesWithGold();
	addPandoraBoxesWithExperience();
	addPandoraBoxesWithCreatures();
	addPandoraBoxesWithSpells();
}

void TreasurePlacer::addPandoraBoxesWithGold()
{
	ObjectInfo oi(Obj::PANDORAS_BOX, 0);
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [this, i]() -> std::shared_ptr<CGObjectInstance>
		{
			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory->create(map.mapInstance->cb, nullptr));
			
			Rewardable::VisitInfo reward;
			reward.reward.resources[EGameResID::GOLD] = i * 5000;
			reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			obj->configuration.info.push_back(reward);
			
			return obj;
		};
		oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = i * generator.getConfig().pandoraMultiplierGold;
		oi.probability = 5;
		if(!oi.templates.empty())
			addObjectToRandomPool(oi);
	}
}

void TreasurePlacer::addPandoraBoxesWithExperience()
{
	ObjectInfo oi(Obj::PANDORAS_BOX, 0);
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [this, i]() -> std::shared_ptr<CGObjectInstance>
		{
			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory->create(map.mapInstance->cb, nullptr));
			
			Rewardable::VisitInfo reward;
			reward.reward.heroExperience = i * 5000;
			reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			obj->configuration.info.push_back(reward);
			
			return obj;
		};
		oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = i * generator.getConfig().pandoraMultiplierExperience;
		oi.probability = 20;
		if(!oi.templates.empty())
			addObjectToRandomPool(oi);
	}
}

void TreasurePlacer::addPandoraBoxesWithCreatures()
{
	for(auto * creature : creatures)
	{
		int creaturesAmount = creatureToCount(creature);
		if(!creaturesAmount)
			continue;

		ObjectInfo oi(Obj::PANDORAS_BOX, 0);
		
		oi.generateObject = [this, creature, creaturesAmount]() -> std::shared_ptr<CGObjectInstance>
		{
			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory->create(map.mapInstance->cb, nullptr));
			
			Rewardable::VisitInfo reward;
			reward.reward.creatures.emplace_back(creature, creaturesAmount);
			reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			obj->configuration.info.push_back(reward);
			
			return obj;
		};
		oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = static_cast<ui32>(creature->getAIValue() * creaturesAmount * (1 + static_cast<float>(map.getZoneCount(creature->getFactionID())) / map.getTotalZoneCount()));
		oi.probability = 3;
		if(!oi.templates.empty())
			addObjectToRandomPool(oi);
	}
}

void TreasurePlacer::addPandoraBoxesWithSpells()
{
	ObjectInfo oi(Obj::PANDORAS_BOX, 0);
	//Pandora with 12 spells of certain level
	for(int i = 1; i <= GameConstants::SPELL_LEVELS; i++)
	{
		oi.generateObject = [i, this]() -> std::shared_ptr<CGObjectInstance>
		{
			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory->create(map.mapInstance->cb, nullptr));

			std::vector <const CSpell *> spells;
			for(auto spellID : LIBRARY->spellh->getDefaultAllowed())
			{
				if(map.isAllowedSpell(spellID) && spellID.toSpell()->getLevel() == i)
					spells.push_back(spellID.toSpell());
			}
			
			RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
			Rewardable::VisitInfo reward;
			for(int j = 0; j < std::min(12, static_cast<int>(spells.size())); j++)
			{
				reward.reward.spells.push_back(spells[j]->id);
			}
			reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			obj->configuration.info.push_back(reward);
			
			return obj;
		};
		oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = (i + 1) * generator.getConfig().pandoraMultiplierSpells; //5000 - 15000
		oi.probability = 2;
		if(!oi.templates.empty())
			addObjectToRandomPool(oi);
	}
	
	//Pandora with 15 spells of certain school
	for(int i = 0; i < 4; i++)
	{
		oi.generateObject = [i, this]() -> std::shared_ptr<CGObjectInstance>
		{
			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory->create(map.mapInstance->cb, nullptr));

			std::vector <const CSpell *> spells;
			for(auto spellID : LIBRARY->spellh->getDefaultAllowed())
			{
				if(map.isAllowedSpell(spellID) && spellID.toSpell()->hasSchool(SpellSchool(i)))
					spells.push_back(spellID.toSpell());
			}
			
			RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
			Rewardable::VisitInfo reward;
			for(int j = 0; j < std::min(15, static_cast<int>(spells.size())); j++)
			{
				reward.reward.spells.push_back(spells[j]->id);
			}
			reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			obj->configuration.info.push_back(reward);
			
			return obj;
		};
		oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = generator.getConfig().pandoraSpellSchool;
		oi.probability = 2;
		if(!oi.templates.empty())
			addObjectToRandomPool(oi);
	}
	
	// Pandora box with 60 random spells
	
	oi.generateObject = [this]() -> std::shared_ptr<CGObjectInstance>
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
		auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory->create(map.mapInstance->cb, nullptr));

		std::vector <const CSpell *> spells;
		for(auto spellID : LIBRARY->spellh->getDefaultAllowed())
		{
			if(map.isAllowedSpell(spellID))
				spells.push_back(spellID.toSpell());
		}
		
		RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
		Rewardable::VisitInfo reward;
		for(int j = 0; j < std::min(60, static_cast<int>(spells.size())); j++)
		{
			reward.reward.spells.push_back(spells[j]->id);
		}
		reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		obj->configuration.info.push_back(reward);
		
		return obj;
	};
	oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
	oi.value = generator.getConfig().pandoraSpell60;
	oi.probability = 2;
	if(!oi.templates.empty())
		addObjectToRandomPool(oi);
}

void TreasurePlacer::addSeerHuts()
{
	//Seer huts with creatures or generic rewards

	ObjectInfo oi(Obj::SEER_HUT, 0);

	if(zone.getConnectedZoneIds().size()) //Unlikely, but...
	{
		auto * qap = zone.getModificator<QuestArtifactPlacer>();
		if(!qap)
		{
			return; //TODO: throw?
		}
		
		const int questArtsRemaining = qap->getMaxQuestArtifactCount();
		if (!questArtsRemaining)
		{
			return;
		}
		
		//Generate Seer Hut one by one. Duplicated oi possible and should work fine.
		oi.maxPerZone = 1;

		std::vector<ObjectInfo> possibleSeerHuts;
		//14 creatures per town + 4 for each of gold / exp reward
		possibleSeerHuts.reserve(14 + 4 + 4);
		
		RandomGeneratorUtil::randomShuffle(creatures, zone.getRand());

		auto setRandomArtifact = [qap](CGSeerHut * obj)
		{
			ArtifactID artid = qap->drawRandomArtifact();
			obj->getQuest().mission.artifacts.push_back(artid);
			qap->addQuestArtifact(artid);
		};
		auto destroyObject = [qap](CGObjectInstance & obj)
		{
			auto & seer = dynamic_cast<CGSeerHut &>(obj);
			// Artifact can be used again
			ArtifactID artid = seer.getQuest().mission.artifacts.front();
			qap->addRandomArtifact(artid);
			qap->removeQuestArtifact(artid);
		};

		for(int i = 0; i < static_cast<int>(creatures.size()); i++)
		{
			auto * creature = creatures[i];
			int creaturesAmount = creatureToCount(creature);
			
			if(!creaturesAmount)
				continue;
			
			int randomAppearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
			
			// FIXME: Remove duplicated code for gold, exp and creaure reward
			oi.generateObject = [cb=map.mapInstance->cb, creature, creaturesAmount, randomAppearance, setRandomArtifact]() -> std::shared_ptr<CGObjectInstance>
			{
				auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = std::dynamic_pointer_cast<CGSeerHut>(factory->create(cb, nullptr));
				
				Rewardable::VisitInfo reward;
				reward.reward.creatures.emplace_back(creature->getId(), creaturesAmount);
				reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
				obj->configuration.info.push_back(reward);
				setRandomArtifact(obj.get());
				
				return obj;
			};
			oi.destroyObject = destroyObject;
			oi.probability = 3;
			oi.setTemplates(Obj::SEER_HUT, randomAppearance, zone.getTerrainType());
			oi.value = static_cast<ui32>(((2 * (creature->getAIValue()) * creaturesAmount * (1 + static_cast<float>(map.getZoneCount(creature->getFactionID())) / map.getTotalZoneCount())) - 4000) / 3);
			if (oi.value > zone.getMaxTreasureValue())
			{
				continue;
			}
			else
			{
				if(!oi.templates.empty())
					possibleSeerHuts.push_back(oi);
			}
		}
		
		static const int seerLevels = std::min(generator.getConfig().questValues.size(), generator.getConfig().questRewardValues.size());
		for(int i = 0; i < seerLevels; i++) //seems that code for exp and gold reward is similar
		{
			int randomAppearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
			
			oi.setTemplates(Obj::SEER_HUT, randomAppearance, zone.getTerrainType());
			oi.value = generator.getConfig().questValues[i];
			if (oi.value > zone.getMaxTreasureValue())
			{
				//Both variants have same value
				continue;
			}

			oi.probability = 10;
			oi.maxPerZone = 1;
			
			oi.generateObject = [i, randomAppearance, this, setRandomArtifact]() -> std::shared_ptr<CGObjectInstance>
			{
				auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = std::dynamic_pointer_cast<CGSeerHut>(factory->create(map.mapInstance->cb, nullptr));
				
				Rewardable::VisitInfo reward;
				reward.reward.heroExperience = generator.getConfig().questRewardValues[i];
				reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
				obj->configuration.info.push_back(reward);
				setRandomArtifact(obj.get());

				return obj;
			};
			oi.destroyObject = destroyObject;
			
			if(!oi.templates.empty())
				possibleSeerHuts.push_back(oi);
			
			oi.generateObject = [i, randomAppearance, this, setRandomArtifact]() -> std::shared_ptr<CGObjectInstance>
			{
				auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = std::dynamic_pointer_cast<CGSeerHut>(factory->create(map.mapInstance->cb, nullptr));
				
				Rewardable::VisitInfo reward;
				reward.reward.resources[EGameResID::GOLD] = generator.getConfig().questRewardValues[i];
				reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
				obj->configuration.info.push_back(reward);
				
				setRandomArtifact(obj.get());
				
				return obj;
			};
			oi.destroyObject = destroyObject;
			
			if(!oi.templates.empty())
				possibleSeerHuts.push_back(oi);
		}

		if (possibleSeerHuts.empty())
		{
			return;
		}
		for (size_t i = 0; i < questArtsRemaining; i++)
		{
			addObjectToRandomPool(*RandomGeneratorUtil::nextItem(possibleSeerHuts, zone.getRand()));
		}
	}
}

void TreasurePlacer::setMaxPrisons(size_t count)
{
	RecursiveLock lock(externalAccessMutex);
	maxPrisons = count;
}

size_t TreasurePlacer::getMaxPrisons() const
{
	RecursiveLock lock(externalAccessMutex);
	return maxPrisons;
}

int TreasurePlacer::creatureToCount(const CCreature * creature) const
{
	if(!creature->getAIValue() || tierValues.empty()) //bug #2681
		return 0; //this box won't be generated
	
	//Follow the rules from https://heroes.thelazy.net/index.php/Pandora%27s_Box

	int actualTier = creature->getLevel() > tierValues.size() ?
		tierValues.size() - 1 :
		creature->getLevel() - 1;
	float creaturesAmount = std::floor((static_cast<float>(tierValues[actualTier])) / creature->getAIValue());
	if (creaturesAmount < 1)
	{
		return 0;
	}
	else if(creaturesAmount <= 5)
	{
		//No change
	}
	else if(creaturesAmount <= 12)
	{
		creaturesAmount = std::ceil(creaturesAmount / 2) * 2;
	}
	else if(creaturesAmount <= 50)
	{
		creaturesAmount = std::round(creaturesAmount / 5) * 5;
	}
	else
	{
		creaturesAmount = std::round(creaturesAmount / 10) * 10;
	}
	return static_cast<int>(creaturesAmount);
};

bool TreasurePlacer::isGuardNeededForTreasure(int value)
{// no guard in a zone with "monsters: none" and for small treasures; water zones cen get monster strength ZONE_NONE elsewhere if needed
	return zone.monsterStrength != EMonsterStrength::ZONE_NONE && value > minGuardedValue;
}

std::vector<ObjectInfo*> TreasurePlacer::prepareTreasurePile(const CTreasureInfo& treasureInfo)
{
	std::vector<ObjectInfo*> objectInfos;
	int maxValue = treasureInfo.max;
	int minValue = treasureInfo.min;
	
	const ui32 desiredValue = zone.getRand().nextInt(minValue, maxValue);
	
	int currentValue = 0;
	bool hasLargeObject = false;
	while(currentValue <= static_cast<int>(desiredValue) - 100) //no objects with value below 100 are available
	{
		// FIXME: Pointer might be invalidated after this
		auto * oi = getRandomObject(desiredValue, currentValue, !hasLargeObject);
		if(!oi) //fail
			break;
		
		bool visitableFromTop = true;
		for(auto & t : oi->templates)
			if(!t->isVisitableFromTop())
				visitableFromTop = false;
		
		if(visitableFromTop)
		{
			objectInfos.push_back(oi);
		}
		else
		{
			objectInfos.insert(objectInfos.begin(), oi); //large object shall at first place
			hasLargeObject = true;
		}
		
		//remove from possible objects
		assert(oi->maxPerZone);
		oi->maxPerZone--;
		
		currentValue += oi->value;

		if (currentValue >= minValue)
		{
			// 50% chance to end right here
			if (zone.getRand().nextInt(0, 1) == 1)
				break;
		}
	}
	
	return objectInfos;
}

rmg::Object TreasurePlacer::constructTreasurePile(const std::vector<ObjectInfo*> & treasureInfos, bool densePlacement)
{
	rmg::Object rmgObject;
	for(const auto & oi : treasureInfos)
	{
		auto blockedArea = rmgObject.getArea();
		auto entrableArea = rmgObject.getEntrableArea();
		auto accessibleArea = rmgObject.getAccessibleArea();
		
		if(rmgObject.instances().empty())
		{
			rmgObject.setValue(0);
			accessibleArea.add(int3());
		}
		
		std::shared_ptr<CGObjectInstance> object = nullptr;
		if (oi->generateObject)
		{
			object = oi->generateObject();
			if(oi->templates.empty())
			{
				logGlobal->warn("Deleting randomized object with no templates: %s", object->getObjectName());
				oi->destroyObject(*object);
				continue;
			}
		}
		else
		{
			logGlobal->error("ObjectInfo has no generateObject function! Templates: %d", oi->templates.size());
			continue;
		}
		
		auto templates = object->getObjectHandler()->getMostSpecificTemplates(zone.getTerrainType());

		if (templates.empty())
		{
			throw rmgException(boost::str(boost::format("Did not find template for object (%d,%d) at %s") % object->ID.getNum() % object->subID.getNum() % zone.getTerrainType().encode(zone.getTerrainType().getNum())));
		}

		object->appearance = *RandomGeneratorUtil::nextItem(templates, zone.getRand());

		//Put object in accessible area next to entrable area (excluding blockvis tiles)
		if (!entrableArea.empty())
		{
			auto entrableBorder = entrableArea.getBorderOutside();
			accessibleArea.erase_if([&](const int3 & tile)
			{
				return !entrableBorder.count(tile);
			});
		}

		auto & instance = rmgObject.addInstance(object);
		rmgObject.setValue(rmgObject.getValue() + oi->value);
		instance.onCleared = oi->destroyObject;

		do
		{
			if(accessibleArea.empty())
			{
				//fail - fallback
				rmgObject.clear();
				return rmgObject;
			}
			
			std::vector<int3> bestPositions;
			if(densePlacement && !entrableArea.empty())
			{
				// Choose positon which has access to as many entrable tiles as possible
				int bestPositionsWeight = std::numeric_limits<int>::max();
				for(const auto & t : accessibleArea.getTilesVector())
				{
					instance.setPosition(t);

					auto currentAccessibleArea = rmgObject.getAccessibleArea();
					auto currentEntrableBorder = rmgObject.getEntrableArea().getBorderOutside();
					currentAccessibleArea.erase_if([&](const int3 & tile)
					{
						return !currentEntrableBorder.count(tile);
					});

					size_t w = currentAccessibleArea.getTilesVector().size();

					if(w > bestPositionsWeight)
					{
						// Minimum 1 position must be entrable
						bestPositions.clear();
						bestPositions.push_back(t);
						bestPositionsWeight = w;
					}
					else if(w == bestPositionsWeight)
					{
						bestPositions.push_back(t);
					}
				}
			}

			if (bestPositions.empty())
			{
				bestPositions = accessibleArea.getTilesVector();
			}
			
			int3 nextPos = *RandomGeneratorUtil::nextItem(bestPositions, zone.getRand());
			instance.setPosition(nextPos - rmgObject.getPosition());
			
			auto instanceAccessibleArea = instance.getAccessibleArea();
			if(instance.getBlockedArea().getTilesVector().size() == 1)
			{
				if(instance.object().appearance->isVisitableFromTop() && !instance.object().isBlockedVisitable())
					instanceAccessibleArea.add(instance.getVisitablePosition());
			}
			
			//Do not clean up after first object
			if(rmgObject.instances().size() == 1)
				break;

			if(!blockedArea.overlap(instance.getBlockedArea()) && accessibleArea.overlap(instanceAccessibleArea))
				break;

			//fail - new position
			accessibleArea.erase(nextPos);
		} while(true);
	}
	return rmgObject;
}

ObjectInfo * TreasurePlacer::getRandomObject(ui32 desiredValue, ui32 currentValue, bool allowLargeObjects)
{
	std::vector<std::pair<ui32, ObjectInfo*>> thresholds; //handle complex object via pointer
	ui32 total = 0;
	
	//calculate actual treasure value range based on remaining value
	ui32 maxVal = desiredValue - currentValue;
	ui32 minValue = static_cast<ui32>(0.25f * (desiredValue - currentValue));
	
	for(ObjectInfo & oi : objects.getPossibleObjects()) //copy constructor turned out to be costly
	{
		if(oi.value > maxVal)
			break; //this assumes values are sorted in ascending order
		
		bool visitableFromTop = true;
		for(auto & t : oi.templates)
			if(!t->isVisitableFromTop())
				visitableFromTop = false;
		
		if(!visitableFromTop && !allowLargeObjects)
			continue;
		
		if(oi.value >= minValue && oi.maxPerZone > 0)
		{
			total += oi.probability;
			thresholds.emplace_back(total, &oi);
		}
	}
	
	if(thresholds.empty())
	{
		return nullptr;
	}
	else
	{
		int r = zone.getRand().nextInt(1, total);
		auto sorter = [](const std::pair<ui32, ObjectInfo *> & rhs, const ui32 lhs) -> bool 
		{
			return static_cast<int>(rhs.first) < lhs; 
		};

		//binary search = fastest
		auto it = std::lower_bound(thresholds.begin(), thresholds.end(), r, sorter);
		return it->second;
	}
}

void TreasurePlacer::createTreasures(ObjectManager& manager)
{
	const int maxAttempts = 2;

	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zone.monsterStrength == EMonsterStrength::ZONE_NONE ? 0 : zone.monsterStrength + mapMonsterStrength - 1); //array index from 0 to 4; pick any correct value for ZONE_NONE, minGuardedValue won't be used in this case anyway
	static const int minGuardedValues[] = { 6500, 4167, 3000, 1833, 1333 };
	minGuardedValue = minGuardedValues[monsterStrength];
	const auto blockingGuardMaxValue = zone.getMaxTreasureValue() / 3;

	auto valueComparator = [](const CTreasureInfo& lhs, const CTreasureInfo& rhs) -> bool
	{
		return lhs.max > rhs.max;
	};

	auto restoreZoneLimits = [](const std::vector<ObjectInfo*>& treasurePile)
	{
		for (auto* oi : treasurePile)
		{
			oi->maxPerZone++;
		}
	};

	rmg::Area roads;
	auto rp = zone.getModificator<RoadPlacer>();
	if (rp)
	{
		roads = rp->getRoads();
	}
	rmg::Area nextToRoad(roads.getBorderOutside());

	//place biggest treasures first at large distance, place smaller ones inbetween
	auto treasureInfo = zone.getTreasureInfo();
	boost::sort(treasureInfo, valueComparator);

	//sort treasures by ascending value so we can stop checking treasures with too high value
	objects.sortPossibleObjects();

	const size_t size = zone.area()->getTilesVector().size();

	int totalDensity = 0;

	// FIXME: No need to use iterator here
	for (auto t  = treasureInfo.begin(); t != treasureInfo.end(); t++)
	{
		std::vector<rmg::Object> treasures;

		//discard objects with too high value to be ever placed
		objects.discardObjectsAboveValue(t->max);

		totalDensity += t->density;

		const int DENSITY_CONSTANT = 400;
		size_t count = (size * t->density) / DENSITY_CONSTANT;

		const float minDistance = std::max<float>(std::sqrt(std::min<ui32>(t->min, 30000) / 10.0f / totalDensity), 1.0f);

		size_t emergencyLoopFinish = 0;
		while(treasures.size() < count && emergencyLoopFinish < count)
		{
			auto treasurePileInfos = prepareTreasurePile(*t);
			if (treasurePileInfos.empty())
			{
				emergencyLoopFinish++; //Exit potentially infinite loop for bad settings
				continue;
			}

			int value = std::accumulate(treasurePileInfos.begin(), treasurePileInfos.end(), 0,
				[](int v, const ObjectInfo* oi)
			{
				return v + oi->value;
			});

			const ui32 maxPileGenerationAttempts = 2;
			for (ui32 attempt = 0; attempt < maxPileGenerationAttempts; attempt++)
			{
				auto rmgObject = constructTreasurePile(treasurePileInfos, attempt == maxAttempts);

				if (rmgObject.instances().empty())
				{
					// Restore once if all attempts failed
					if (attempt == (maxPileGenerationAttempts - 1))
					{
						restoreZoneLimits(treasurePileInfos);
					}
					continue;
				}

				//guard treasure pile
				bool guarded = isGuardNeededForTreasure(value);
				if (guarded)
					guarded = manager.addGuard(rmgObject, value);

				treasures.push_back(rmgObject);
				break;
			}
		}

		for (auto& rmgObject : treasures)
		{
			const bool guarded = rmgObject.isGuarded();

			auto path = rmg::Path::invalid();

			{
				Zone::Lock lock(zone.areaMutex); //We are going to subtract this area

				auto searchArea = zone.areaPossible().get();
				searchArea.erase_if([this, &minDistance](const int3& tile) -> bool
				{
					auto ti = map.getTileInfo(tile);
					return (ti.getNearestObjectDistance() < minDistance);
				});

				if (guarded)
				{
					searchArea.subtract(roads);

					path = manager.placeAndConnectObject(searchArea, rmgObject, [this, &rmgObject, &minDistance, &manager, blockingGuardMaxValue, &roads, &nextToRoad](const int3& tile)
						{
							float bestDistance = 10e9;
							for (const auto& t : rmgObject.getArea().getTilesVector())
							{
								float distance = map.getTileInfo(t).getNearestObjectDistance();
								if (distance < minDistance)
									return -1.f;
								else
									vstd::amin(bestDistance, distance);
							}

							// Guard cannot be adjacent to road, but blocked side of an object could be
							if (rmgObject.getValue() > blockingGuardMaxValue && nextToRoad.contains(rmgObject.getGuardPos()))
							{
								return -1.f;
							}

							const auto & guardedArea = rmgObject.instances().back()->getAccessibleArea();
							const auto areaToBlock = rmgObject.getAccessibleArea(true) - guardedArea;

							if (zone.freePaths()->overlap(areaToBlock) || roads.overlap(areaToBlock) || manager.getVisitableArea().overlap(areaToBlock))
								return -1.f;

							// Add huge penalty for objects hiding roads
							if (rmgObject.getBorderAbove().overlap(roads))
								bestDistance /= 10.0f;

							return bestDistance;
						}, guarded, false, ObjectManager::OptimizeType::BOTH);
				}
				else
				{
					// Do not place non-removable objects on roads
					if (!rmgObject.getRemovableArea().contains(rmgObject.getArea()))
					{
						searchArea.subtract(roads);
					}

					path = manager.placeAndConnectObject(searchArea, rmgObject, minDistance, guarded, false, ObjectManager::OptimizeType::DISTANCE);
				}
			}

			if (path.valid())
			{
#ifdef TREASURE_PLACER_LOG
				treasureArea.unite(rmgObject.getArea());
				if (guarded)
				{
					guards.unite(rmgObject.instances().back()->getBlockedArea());
					auto guardedArea = rmgObject.instances().back()->getAccessibleArea();
					auto areaToBlock = rmgObject.getAccessibleArea(true) - guardedArea;
					treasureBlockArea.unite(areaToBlock);
				}
#endif
				zone.connectPath(path);
				manager.placeObject(rmgObject, guarded, true);
			}
		}
	}
}

char TreasurePlacer::dump(const int3 & t)
{
	if(guards.contains(t))
		return '!';
	if(treasureArea.contains(t))
		return '$';
	if(treasureBlockArea.contains(t))
		return '*';
	
	return Modificator::dump(t);
}

void TreasurePlacer::ObjectPool::addObject(const ObjectInfo & info)
{
	possibleObjects.push_back(info);
}

void TreasurePlacer::ObjectPool::updateObject(MapObjectID id, MapObjectSubID subid, ObjectInfo info)
{
	/*
	Handle separately:
		- Dwellings
		- Prisons
		- Seer huts (quests)
		- Pandora Boxes
	*/
	// FIXME: This will drop all templates
	customObjects.insert(std::make_pair(CompoundMapObjectID(id, subid), info));
}

void TreasurePlacer::ObjectPool::patchWithZoneConfig(const Zone & zone, TreasurePlacer * tp)
{
	// FIXME: Wycina wszystkie obiekty poza pandorami i dwellami :?

	// Copy standard objects if they are not already modified
	/*
	for (const auto & object : possibleObjects)
	{
		for (const auto & templ : object.templates)
		{
			// FIXME: Objects with same temmplates (Pandora boxes) are not added
			CompoundMapObjectID key(templ->id, templ->subid);
			if (!vstd::contains(customObjects, key))
			{
				customObjects[key] = object;
			}
		}
	}
	*/
	auto bannedObjectCategories = zone.getBannedObjectCategories();
	auto categoriesSet = std::unordered_set<ObjectConfig::EObjectCategory>(bannedObjectCategories.begin(), bannedObjectCategories.end());

	if (categoriesSet.count(ObjectConfig::EObjectCategory::ALL))
	{
		possibleObjects.clear();
	}
	else
	{
		vstd::erase_if(possibleObjects, [this, &categoriesSet](const ObjectInfo & oi) -> bool

		{
			auto category = getObjectCategory(oi.getCompoundID());
			if (categoriesSet.count(category))
			{
				logGlobal->info("Removing object %s from possible objects", oi.templates.front()->stringID);
				return true;
			}
			return false;
		});

		auto bannedObjects = zone.getBannedObjects();
		auto bannedObjectsSet = std::set<CompoundMapObjectID>(bannedObjects.begin(), bannedObjects.end());
		vstd::erase_if(possibleObjects, [&bannedObjectsSet](const ObjectInfo & object)
		{
			for (const auto & templ : object.templates)
			{
				CompoundMapObjectID key = object.getCompoundID();
				CompoundMapObjectID keyGroup( key.primaryID, -1);
				if (bannedObjectsSet.count(key) || bannedObjectsSet.count(keyGroup))
				{
					// FIXME: Stopped working, nothing is banned
					logGlobal->info("Banning object %s from possible objects", templ->stringID);
					return true;
				}
			}
			return false;
		});
	}

	auto configuredObjects = zone.getConfiguredObjects();

	// FIXME: Access TreasurePlacer from ObjectPool
	for (auto & object : configuredObjects)
	{
		tp->setBasicProperties(object, object.getCompoundID());
		addObject(object);
		logGlobal->info("Added custom object of type %d.%d", object.primaryID, object.secondaryID);
	}
	// TODO: Overwrite or add to possibleObjects

	// FIXME: Protect with mutex as well?
	/*
	for (const auto & customObject : customObjects)
	{
		addObject(customObject.second);
	}
	*/
	// TODO: Consider adding custom Pandora boxes with arbitrary content
}

std::vector<ObjectInfo> & TreasurePlacer::ObjectPool::getPossibleObjects()
{
	return possibleObjects;
}

void TreasurePlacer::ObjectPool::sortPossibleObjects()
{
	boost::sort(possibleObjects, [](const ObjectInfo& oi1, const ObjectInfo& oi2) -> bool
	{
		return oi1.value < oi2.value;
	});
}

void TreasurePlacer::ObjectPool::discardObjectsAboveValue(ui32 value)
{
	vstd::erase_if(possibleObjects, [value](const ObjectInfo& oi) -> bool
	{
		return oi.value > value;
	});
}

ObjectConfig::EObjectCategory TreasurePlacer::ObjectPool::getObjectCategory(CompoundMapObjectID id)
{
	auto name = LIBRARY->objtypeh->getObjectHandlerName(id.primaryID);

	if (name == "configurable")
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(id.primaryID, id.secondaryID);
		if (!handler)
		{
			return ObjectConfig::EObjectCategory::NONE;
		}

		auto temp = handler->getTemplates().front();
		auto info = handler->getObjectInfo(temp);

		if (info->hasGuards())
		{
			return ObjectConfig::EObjectCategory::CREATURE_BANK;
		}
		else if (info->givesResources())
		{
			return ObjectConfig::EObjectCategory::RESOURCE;
		}
		else if (info->givesArtifacts())
		{
			return ObjectConfig::EObjectCategory::RANDOM_ARTIFACT;
		}
		else if (info->givesBonuses())
		{
			return ObjectConfig::EObjectCategory::BONUS;
		}

		return ObjectConfig::EObjectCategory::OTHER;
	}
	else if (name == "dwelling" || name == "randomDwelling")
	{
		// TODO: Special handling for different tiers
		return ObjectConfig::EObjectCategory::DWELLING;
	}
	else if (name == "bank")
		return ObjectConfig::EObjectCategory::CREATURE_BANK;
	else if (name == "market")
		return ObjectConfig::EObjectCategory::OTHER;
	else if (name == "hillFort")
		return ObjectConfig::EObjectCategory::OTHER;
	else if (name == "resource" || name == "randomResource")
		return ObjectConfig::EObjectCategory::RESOURCE;
	else if (name == "randomArtifact") //"artifact"
		return ObjectConfig::EObjectCategory::RANDOM_ARTIFACT;
	else if (name == "artifact")
	{
		if (id.primaryID == Obj::SPELL_SCROLL ) // randomArtifactTreasure
		{
			return ObjectConfig::EObjectCategory::SPELL_SCROLL;
		}
		else
		{
			return ObjectConfig::EObjectCategory::QUEST_ARTIFACT;
		}
	}
	else if (name == "denOfThieves")
		return ObjectConfig::EObjectCategory::OTHER;
	else if (name == "lighthouse")
	{
		return ObjectConfig::EObjectCategory::BONUS;
	}
	else if (name == "magi")
	{
		// TODO: By default, both eye and hut are banned in every zone
		return ObjectConfig::EObjectCategory::OTHER;
	}
	else if (name == "mine")
		return ObjectConfig::EObjectCategory::RESOURCE_GENERATOR;
	else if (name == "pandora")
		return ObjectConfig::EObjectCategory::PANDORAS_BOX;
	else if (name == "prison")
	{
		// TODO: Prisons should be configurable
		return ObjectConfig::EObjectCategory::OTHER;
	}
	else if (name == "questArtifact")
	{
		// TODO: There are no dedicated quest artifacts, needs extra logic
		return ObjectConfig::EObjectCategory::QUEST_ARTIFACT;
	}
	else if (name == "seerHut")
	{
		return ObjectConfig::EObjectCategory::SEER_HUT;
	}
	else if (name == "siren")
		return ObjectConfig::EObjectCategory::BONUS;
	else if (name == "obelisk")
		return ObjectConfig::EObjectCategory::OTHER;

	// TODO: ObjectConfig::EObjectCategory::SPELL_SCROLL

	// Not interesting for us
	return ObjectConfig::EObjectCategory::NONE;
}

VCMI_LIB_NAMESPACE_END

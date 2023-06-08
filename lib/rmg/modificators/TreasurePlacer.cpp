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
#include "../CMapGenerator.h"
#include "../Functions.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "ConnectionsPlacer.h"
#include "../RmgMap.h"
#include "../TileInfo.h"
#include "../CZonePlacer.h"
#include "QuestArtifactPlacer.h"
#include "../../ArtifactUtils.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjectConstructors/CommonConstructors.h"
#include "../../mapObjects/CGPandoraBox.h"
#include "../../CCreatureHandler.h"
#include "../../spells/CSpellHandler.h" //for choosing random spells
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"

VCMI_LIB_NAMESPACE_BEGIN

void TreasurePlacer::process()
{
	addAllPossibleObjects();
	auto * m = zone.getModificator<ObjectManager>();
	if(m)
		createTreasures(*m);
}

void TreasurePlacer::init()
{
	DEPENDENCY(ObjectManager);
	DEPENDENCY(ConnectionsPlacer);
	POSTFUNCTION(RoadPlacer);
}

void TreasurePlacer::addObjectToRandomPool(const ObjectInfo& oi)
{
	RecursiveLock lock(externalAccessMutex);
	possibleObjects.push_back(oi);
}

void TreasurePlacer::addAllPossibleObjects()
{
	ObjectInfo oi;
	
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(!handler->isStaticObject() && handler->getRMGInfo().value)
			{
				auto rmgInfo = handler->getRMGInfo();
				if (rmgInfo.mapLimit || rmgInfo.value > zone.getMaxTreasureValue())
				{
					//Skip objects with per-map limit here
					continue;
				}

				auto templates = handler->getTemplates(zone.getTerrainType());
				if (templates.empty())
					continue;

				//TODO: Reuse chooseRandomAppearance (eg. WoG treasure chests)
				//Assume the template with fewest terrains is the most suitable
				auto temp = *boost::min_element(templates, [](std::shared_ptr<const ObjectTemplate> lhs, std::shared_ptr<const ObjectTemplate> rhs) -> bool
				{
					return lhs->getAllowedTerrains().size() < rhs->getAllowedTerrains().size();
				});

				oi.generateObject = [temp]() -> CGObjectInstance *
				{
					return VLC->objtypeh->getHandlerFor(temp->id, temp->subid)->create(temp);
				};
				oi.value = rmgInfo.value;
				oi.probability = rmgInfo.rarity;
				oi.templ = temp;
				oi.maxPerZone = rmgInfo.zoneLimit;
				addObjectToRandomPool(oi);
			}
		}
	}
	
	if(zone.getType() == ETemplateZoneType::WATER)
		return;
	
	//prisons
	//levels 1, 5, 10, 20, 30
	static int prisonsLevels = std::min(generator.getConfig().prisonExperience.size(), generator.getConfig().prisonValues.size());
	
	size_t prisonsLeft = getMaxPrisons();
	for(int i = prisonsLevels - 1; i >= 0 ;i--)
	{
		oi.value = generator.getConfig().prisonValues[i];
		if (oi.value > zone.getMaxTreasureValue())
		{
			continue;
		}

		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto possibleHeroes = generator.getAllPossibleHeroes();
			HeroTypeID hid = *RandomGeneratorUtil::nextItem(possibleHeroes, zone.getRand());

			auto factory = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0);
			auto * obj = dynamic_cast<CGHeroInstance *>(factory->create());

			obj->subID = hid; //will be initialized later
			obj->exp = generator.getConfig().prisonExperience[i];
			obj->setOwner(PlayerColor::NEUTRAL);
			generator.banHero(hid);
			obj->appearance = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0)->getTemplates(zone.getTerrainType()).front(); //can't init template with hero subID
			
			return obj;
		};
		oi.setTemplate(Obj::PRISON, 0, zone.getTerrainType());
		oi.value = generator.getConfig().prisonValues[i];
		oi.probability = 30;
		
		//Distribute all allowed prisons, starting from the most valuable
		oi.maxPerZone = (std::ceil((float)prisonsLeft / (i + 1)));
		prisonsLeft -= oi.maxPerZone;
		addObjectToRandomPool(oi);
	}
	
	//all following objects are unlimited
	oi.maxPerZone = std::numeric_limits<ui32>::max();

	std::vector<CCreature *> creatures; //native creatures for this zone
	for(auto cre : VLC->creh->objects)
	{
		if(!cre->special && cre->getFaction() == zone.getTownType())
		{
			creatures.push_back(cre);
		}
	}
	
	//dwellings
	auto dwellingTypes = {Obj::CREATURE_GENERATOR1, Obj::CREATURE_GENERATOR4};
	
	for(auto dwellingType : dwellingTypes)
	{
		auto subObjects = VLC->objtypeh->knownSubObjects(dwellingType);
		
		if(dwellingType == Obj::CREATURE_GENERATOR1)
		{
			//don't spawn original "neutral" dwellings that got replaced by Conflux dwellings in AB
			static int elementalConfluxROE[] = {7, 13, 16, 47};
			for(int & i : elementalConfluxROE)
				vstd::erase_if_present(subObjects, i);
		}
		
		for(auto secondaryID : subObjects)
		{
			const auto * dwellingHandler = dynamic_cast<const CDwellingInstanceConstructor *>(VLC->objtypeh->getHandlerFor(dwellingType, secondaryID).get());
			auto creatures = dwellingHandler->getProducedCreatures();
			if(creatures.empty())
				continue;

			const auto * cre = creatures.front();
			if(cre->getFaction() == zone.getTownType())
			{
				auto nativeZonesCount = static_cast<float>(map.getZoneCount(cre->getFaction()));
				oi.value = static_cast<ui32>(cre->getAIValue() * cre->getGrowth() * (1 + (nativeZonesCount / map.getTotalZoneCount()) + (nativeZonesCount / 2)));
				oi.probability = 40;

				for(const auto & tmplate : dwellingHandler->getTemplates())
				{
					if(tmplate->canBePlacedAt(zone.getTerrainType()))
					{
						oi.generateObject = [tmplate, secondaryID, dwellingType]() -> CGObjectInstance *
						{
							auto * obj = VLC->objtypeh->getHandlerFor(dwellingType, secondaryID)->create(tmplate);
							obj->tempOwner = PlayerColor::NEUTRAL;
							return obj;
						};

						oi.templ = tmplate;
						addObjectToRandomPool(oi);
					}
				}
			}
		}
	}
	
	for(int i = 0; i < generator.getConfig().scrollValues.size(); i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::SPELL_SCROLL, 0);
			auto * obj = dynamic_cast<CGArtifact *>(factory->create());
			std::vector<SpellID> out;
			
			for(auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
			{
				if(map.isAllowedSpell(spell->id) && spell->level == i + 1)
				{
					out.push_back(spell->id);
				}
			}
			auto * a = ArtifactUtils::createScroll(*RandomGeneratorUtil::nextItem(out, zone.getRand()));
			obj->storedArtifact = a;
			return obj;
		};
		oi.setTemplate(Obj::SPELL_SCROLL, 0, zone.getTerrainType());
		oi.value = generator.getConfig().scrollValues[i];
		oi.probability = 30;
		addObjectToRandomPool(oi);
	}
	
	//pandora box with gold
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto * obj = dynamic_cast<CGPandoraBox *>(factory->create());
			obj->resources[EGameResID::GOLD] = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = i * generator.getConfig().pandoraMultiplierGold;
		oi.probability = 5;
		addObjectToRandomPool(oi);
	}
	
	//pandora box with experience
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto * obj = dynamic_cast<CGPandoraBox *>(factory->create());
			obj->gainedExp = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = i * generator.getConfig().pandoraMultiplierExperience;
		oi.probability = 20;
		addObjectToRandomPool(oi);
	}
	
	//pandora box with creatures
	const std::vector<int> & tierValues = generator.getConfig().pandoraCreatureValues;
	
	auto creatureToCount = [tierValues](CCreature * creature) -> int
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

	for(auto * creature : creatures)
	{
		int creaturesAmount = creatureToCount(creature);
		if(!creaturesAmount)
			continue;
		
		oi.generateObject = [creature, creaturesAmount]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto * obj = dynamic_cast<CGPandoraBox *>(factory->create());
			auto * stack = new CStackInstance(creature, creaturesAmount);
			obj->creatures.putStack(SlotID(0), stack);
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = static_cast<ui32>((2 * (creature->getAIValue()) * creaturesAmount * (1 + static_cast<float>(map.getZoneCount(creature->getFaction())) / map.getTotalZoneCount())) / 3);
		oi.probability = 3;
		addObjectToRandomPool(oi);
	}
	
	//Pandora with 12 spells of certain level
	for(int i = 1; i <= GameConstants::SPELL_LEVELS; i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto * obj = dynamic_cast<CGPandoraBox *>(factory->create());

			std::vector <CSpell *> spells;
			for(auto spell : VLC->spellh->objects)
			{
				if(map.isAllowedSpell(spell->id) && spell->level == i)
					spells.push_back(spell);
			}
			
			RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
			for(int j = 0; j < std::min(12, static_cast<int>(spells.size())); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}
			
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = (i + 1) * generator.getConfig().pandoraMultiplierSpells; //5000 - 15000
		oi.probability = 2;
		addObjectToRandomPool(oi);
	}
	
	//Pandora with 15 spells of certain school
	for(int i = 0; i < 4; i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto * obj = dynamic_cast<CGPandoraBox *>(factory->create());

			std::vector <CSpell *> spells;
			for(auto spell : VLC->spellh->objects)
			{
				if(map.isAllowedSpell(spell->id) && spell->school[SpellSchool(i)])
					spells.push_back(spell);
			}
			
			RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
			for(int j = 0; j < std::min(15, static_cast<int>(spells.size())); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}
			
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = generator.getConfig().pandoraSpellSchool;
		oi.probability = 2;
		addObjectToRandomPool(oi);
	}
	
	// Pandora box with 60 random spells
	
	oi.generateObject = [this]() -> CGObjectInstance *
	{
		auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
		auto * obj = dynamic_cast<CGPandoraBox *>(factory->create());

		std::vector <CSpell *> spells;
		for(auto spell : VLC->spellh->objects)
		{
			if(map.isAllowedSpell(spell->id))
				spells.push_back(spell);
		}
		
		RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
		for(int j = 0; j < std::min(60, static_cast<int>(spells.size())); j++)
		{
			obj->spells.push_back(spells[j]->id);
		}
		
		return obj;
	};
	oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
	oi.value = generator.getConfig().pandoraSpell60;
	oi.probability = 2;
	addObjectToRandomPool(oi);
	
	//Seer huts with creatures or generic rewards

	if(zone.getConnections().size()) //Unlikely, but...
	{
		auto * qap = zone.getModificator<QuestArtifactPlacer>();
		if(!qap)
		{
			return; //TODO: throw?
		}
		
		const int questArtsRemaining = qap->getMaxQuestArtifactCount();
		
		//Generate Seer Hut one by one. Duplicated oi possible and should work fine.
		oi.maxPerZone = 1;

		std::vector<ObjectInfo> possibleSeerHuts;
		//14 creatures per town + 4 for each of gold / exp reward
		possibleSeerHuts.reserve(14 + 4 + 4);
		
		RandomGeneratorUtil::randomShuffle(creatures, zone.getRand());

		for(int i = 0; i < static_cast<int>(creatures.size()); i++)
		{
			auto * creature = creatures[i];
			int creaturesAmount = creatureToCount(creature);
			
			if(!creaturesAmount)
				continue;
			
			int randomAppearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
			
			oi.generateObject = [creature, creaturesAmount, randomAppearance, this, qap]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto * obj = dynamic_cast<CGSeerHut *>(factory->create());
				obj->rewardType = CGSeerHut::CREATURE;
				obj->rID = creature->getId();
				obj->rVal = creaturesAmount;
				
				obj->quest->missionType = CQuest::MISSION_ART;
				
				ArtifactID artid = qap->drawRandomArtifact();
				obj->quest->addArtifactID(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;
				
				generator.banQuestArt(artid);
				zone.getModificator<QuestArtifactPlacer>()->addQuestArtifact(artid);
				
				return obj;
			};
			oi.probability = 3;
			oi.setTemplate(Obj::SEER_HUT, randomAppearance, zone.getTerrainType());
			oi.value = static_cast<ui32>(((2 * (creature->getAIValue()) * creaturesAmount * (1 + static_cast<float>(map.getZoneCount(creature->getFaction())) / map.getTotalZoneCount())) - 4000) / 3);
			if (oi.value > zone.getMaxTreasureValue())
			{
				continue;
			}
			else
			{
				possibleSeerHuts.push_back(oi);
			}
		}
		
		static int seerLevels = std::min(generator.getConfig().questValues.size(), generator.getConfig().questRewardValues.size());
		for(int i = 0; i < seerLevels; i++) //seems that code for exp and gold reward is similiar
		{
			int randomAppearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
			
			oi.setTemplate(Obj::SEER_HUT, randomAppearance, zone.getTerrainType());
			oi.value = generator.getConfig().questValues[i];
			if (oi.value > zone.getMaxTreasureValue())
			{
				//Both variants have same value
				continue;
			}

			oi.probability = 10;
			
			oi.generateObject = [i, randomAppearance, this, qap]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto * obj = dynamic_cast<CGSeerHut *>(factory->create());

				obj->rewardType = CGSeerHut::EXPERIENCE;
				obj->rID = 0; //unitialized?
				obj->rVal = generator.getConfig().questRewardValues[i];
				
				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = qap->drawRandomArtifact();
				obj->quest->addArtifactID(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;
				
				generator.banQuestArt(artid);
				zone.getModificator<QuestArtifactPlacer>()->addQuestArtifact(artid);
				
				return obj;
			};
			
			possibleSeerHuts.push_back(oi);
			
			oi.generateObject = [i, randomAppearance, this, qap]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto * obj = dynamic_cast<CGSeerHut *>(factory->create());
				obj->rewardType = CGSeerHut::RESOURCES;
				obj->rID = GameResID(EGameResID::GOLD);
				obj->rVal = generator.getConfig().questRewardValues[i];
				
				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = qap->drawRandomArtifact();
				obj->quest->addArtifactID(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;
				
				generator.banQuestArt(artid);
				zone.getModificator<QuestArtifactPlacer>()->addQuestArtifact(artid);
				
				return obj;
			};
			
			possibleSeerHuts.push_back(oi);
		}

		for (size_t i = 0; i < questArtsRemaining; i++)
		{
			addObjectToRandomPool(*RandomGeneratorUtil::nextItem(possibleSeerHuts, zone.getRand()));
		}
	}
}

size_t TreasurePlacer::getPossibleObjectsSize() const
{
	RecursiveLock lock(externalAccessMutex);
	return possibleObjects.size();
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

bool TreasurePlacer::isGuardNeededForTreasure(int value)
{// no guard in a zone with "monsters: none" and for small treasures; water zones cen get monster strength ZONE_NONE elsewhere if needed
	return zone.monsterStrength != EMonsterStrength::ZONE_NONE && value > minGuardedValue;
}

std::vector<ObjectInfo*> TreasurePlacer::prepareTreasurePile(const CTreasureInfo& treasureInfo)
{
	std::vector<ObjectInfo*> objectInfos;
	int maxValue = treasureInfo.max;
	int minValue = treasureInfo.min;
	
	const ui32 desiredValue =zone.getRand().nextInt(minValue, maxValue);
	
	int currentValue = 0;
	bool hasLargeObject = false;
	while(currentValue <= static_cast<int>(desiredValue) - 100) //no objects with value below 100 are available
	{
		auto * oi = getRandomObject(desiredValue, currentValue, !hasLargeObject);
		if(!oi) //fail
			break;
		
		if(oi->templ->isVisitableFromTop())
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
	}
	
	return objectInfos;
}

rmg::Object TreasurePlacer::constructTreasurePile(const std::vector<ObjectInfo*> & treasureInfos, bool densePlacement)
{
	rmg::Object rmgObject;
	for(const auto & oi : treasureInfos)
	{
		auto blockedArea = rmgObject.getArea();
		auto accessibleArea = rmgObject.getAccessibleArea();
		if(rmgObject.instances().empty())
			accessibleArea.add(int3());
		
		auto * object = oi->generateObject();
		object->appearance = oi->templ;
		auto & instance = rmgObject.addInstance(*object);

		do
		{
			if(accessibleArea.empty())
			{
				//fail - fallback
				rmgObject.clear();
				return rmgObject;
			}
			
			std::vector<int3> bestPositions;
			if(densePlacement)
			{
				int bestPositionsWeight = std::numeric_limits<int>::max();
				for(const auto & t : accessibleArea.getTilesVector())
				{
					instance.setPosition(t);
					int w = rmgObject.getAccessibleArea().getTilesVector().size();
					if(w < bestPositionsWeight)
					{
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
			else
			{
				bestPositions = accessibleArea.getTilesVector();
			}
			
			int3 nextPos = *RandomGeneratorUtil::nextItem(bestPositions, zone.getRand());
			instance.setPosition(nextPos - rmgObject.getPosition());
			
			auto instanceAccessibleArea = instance.getAccessibleArea();
			if(instance.getBlockedArea().getTilesVector().size() == 1)
			{
				if(instance.object().appearance->isVisitableFromTop() && instance.object().ID != Obj::CORPSE)
					instanceAccessibleArea.add(instance.getVisitablePosition());
			}
			
			//first object is good
			if(rmgObject.instances().size() == 1)
				break;
			
			//condition for good position
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
	
	for(ObjectInfo & oi : possibleObjects) //copy constructor turned out to be costly
	{
		if(oi.value > maxVal)
			break; //this assumes values are sorted in ascending order
		
		if(!oi.templ->isVisitableFromTop() && !allowLargeObjects)
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

void TreasurePlacer::createTreasures(ObjectManager & manager)
{
	const int maxAttempts = 2;
	
	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zone.monsterStrength == EMonsterStrength::ZONE_NONE ? 0 : zone.monsterStrength  + mapMonsterStrength - 1); //array index from 0 to 4; pick any correct value for ZONE_NONE, minGuardedValue won't be used in this case anyway
	static int minGuardedValues[] = { 6500, 4167, 3000, 1833, 1333 };
	minGuardedValue = minGuardedValues[monsterStrength];
	
	auto valueComparator = [](const CTreasureInfo & lhs, const CTreasureInfo & rhs) -> bool
	{
		return lhs.max > rhs.max;
	};
	
	auto restoreZoneLimits = [](const std::vector<ObjectInfo*> & treasurePile)
	{
		for(auto * oi : treasurePile)
		{
			oi->maxPerZone++;
		}
	};
	
	//place biggest treasures first at large distance, place smaller ones inbetween
	auto treasureInfo = zone.getTreasureInfo();
	boost::sort(treasureInfo, valueComparator);
	
	//sort treasures by ascending value so we can stop checking treasures with too high value
	boost::sort(possibleObjects, [](const ObjectInfo& oi1, const ObjectInfo& oi2) -> bool
	{
		return oi1.value < oi2.value;
	});
	
	int totalDensity = 0;
	for (auto t : treasureInfo)
	{
		//discard objects with too high value to be ever placed
		vstd::erase_if(possibleObjects, [t](const ObjectInfo& oi) -> bool
		{
			return oi.value > t.max;
		});
		
		totalDensity += t.density;
		
		//treasure density is inversely proportional to zone size but must be scaled back to map size
		//also, normalize it to zone count - higher count means relatively smaller zones
		
		//this is squared distance for optimization purposes
		const float minDistance = std::max<float>((125.f / totalDensity), 2.0f);
		//distance lower than 2 causes objects to overlap and crash
		
		for(int attempt = 0; attempt <= maxAttempts;)
		{
			auto treasurePileInfos = prepareTreasurePile(t);
			if(treasurePileInfos.empty())
			{
				++attempt;
				continue;
			}
			
			int value = std::accumulate(treasurePileInfos.begin(), treasurePileInfos.end(), 0, [](int v, const ObjectInfo * oi){return v + oi->value;});
			
			auto rmgObject = constructTreasurePile(treasurePileInfos, attempt == maxAttempts);
			if(rmgObject.instances().empty()) //handle incorrect placement
			{
				restoreZoneLimits(treasurePileInfos);
				continue;
			}
			
			//guard treasure pile
			bool guarded = isGuardNeededForTreasure(value);
			if(guarded)
				guarded = manager.addGuard(rmgObject, value);
			
			Zone::Lock lock(zone.areaMutex); //We are going to subtract this area
			//TODO: Don't place 
			auto possibleArea = zone.areaPossible();
			
			auto path = rmg::Path::invalid();
			if(guarded)
			{
				path = manager.placeAndConnectObject(possibleArea, rmgObject, [this, &rmgObject, &minDistance, &manager](const int3 & tile)
				{
					auto ti = map.getTileInfo(tile);
					if(ti.getNearestObjectDistance() < minDistance)
						return -1.f;

					for(const auto & t : rmgObject.getArea().getTilesVector())
					{
						if(map.getTileInfo(t).getNearestObjectDistance() < minDistance)
							return -1.f;
					}
					
					auto guardedArea = rmgObject.instances().back()->getAccessibleArea();
					auto areaToBlock = rmgObject.getAccessibleArea(true);
					areaToBlock.subtract(guardedArea);
					if(areaToBlock.overlap(zone.freePaths()) || areaToBlock.overlap(manager.getVisitableArea()))
						return -1.f;
					
					return ti.getNearestObjectDistance();
				}, guarded, false, ObjectManager::OptimizeType::DISTANCE);
			}
			else
			{
				path = manager.placeAndConnectObject(possibleArea, rmgObject, minDistance, guarded, false, ObjectManager::OptimizeType::DISTANCE);
			}
			
			if(path.valid())
			{
				//debug purposes
				treasureArea.unite(rmgObject.getArea());
				if(guarded)
				{
					guards.unite(rmgObject.instances().back()->getBlockedArea());
					auto guardedArea = rmgObject.instances().back()->getAccessibleArea();
					auto areaToBlock = rmgObject.getAccessibleArea(true);
					areaToBlock.subtract(guardedArea);
					treasureBlockArea.unite(areaToBlock);
				}
				zone.connectPath(path);
				manager.placeObject(rmgObject, guarded, true);
				attempt = 0;
			}
			else
			{
				restoreZoneLimits(treasurePileInfos);
				rmgObject.clear();
				++attempt;
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

void ObjectInfo::setTemplate(si32 type, si32 subtype, TerrainId terrainType)
{
	auto templHandler = VLC->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	auto templates = templHandler->getTemplates(terrainType);
	if(templates.empty())
		return;
	
	templ = templates.front();
}

VCMI_LIB_NAMESPACE_END

//
//  TreasurePlacer.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "TreasurePlacer.h"
#include "CMapGenerator.h"
#include "Functions.h"
#include "ObjectManager.h"
#include "RmgMap.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../CCreatureHandler.h"
#include "../spells/CSpellHandler.h" //for choosing random spells
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"

TreasurePlacer::TreasurePlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator) : zone(zone), map(map), generator(generator), questArtZone(nullptr)
{
}

void TreasurePlacer::process()
{
	auto * m = zone.getModificator<ObjectManager>();
	if(m)
		createTreasures(*m);
}

void TreasurePlacer::setQuestArtZone(Zone * otherZone)
{
	questArtZone = otherZone;
}

void TreasurePlacer::addAllPossibleObjects(CMapGenerator & gen)
{
	ObjectInfo oi;
	
	int numZones = static_cast<int>(map.getZones().size());
	
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(!handler->isStaticObject() && handler->getRMGInfo().value)
			{
				for(auto temp : handler->getTemplates())
				{
					if(temp.canBePlacedAt(zone.getTerrainType()))
					{
						oi.generateObject = [temp]() -> CGObjectInstance *
						{
							return VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
						};
						auto rmgInfo = handler->getRMGInfo();
						oi.value = rmgInfo.value;
						oi.probability = rmgInfo.rarity;
						oi.templ = temp;
						oi.maxPerZone = rmgInfo.zoneLimit;
						vstd::amin(oi.maxPerZone, rmgInfo.mapLimit / numZones); //simple, but should distribute objects evenly on large maps
						possibleObjects.push_back(oi);
					}
				}
			}
		}
	}
	
	if(zone.getType() == ETemplateZoneType::WATER)
		return;
	
	//prisons
	//levels 1, 5, 10, 20, 30
	static int prisonsLevels = std::min(gen.getConfig().prisonExperience.size(), gen.getConfig().prisonValues.size());
	for(int i = 0; i < prisonsLevels; i++)
	{
		oi.generateObject = [&gen, i, this]() -> CGObjectInstance *
		{
			std::vector<ui32> possibleHeroes;
			for(int j = 0; j < map.map().allowedHeroes.size(); j++)
			{
				if(map.map().allowedHeroes[j])
					possibleHeroes.push_back(j);
			}
			
			auto hid = *RandomGeneratorUtil::nextItem(possibleHeroes, generator);
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0);
			auto obj = (CGHeroInstance *) factory->create(ObjectTemplate());
			
			
			obj->subID = hid; //will be initialized later
			obj->exp = gen.getConfig().prisonExperience[i];
			obj->setOwner(PlayerColor::NEUTRAL);
			map.map().allowedHeroes[hid] = false; //ban this hero
			gen.decreasePrisonsRemaining();
			obj->appearance = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0)->getTemplates(zone.getTerrainType()).front(); //can't init template with hero subID
			
			return obj;
		};
		oi.setTemplate(Obj::PRISON, 0, zone.getTerrainType());
		oi.value = gen.getConfig().prisonValues[i];
		oi.probability = 30;
		oi.maxPerZone = gen.getPrisonsRemaning() / 5; //probably not perfect, but we can't generate more prisons than hereos.
		possibleObjects.push_back(oi);
	}
	
	//all following objects are unlimited
	oi.maxPerZone = std::numeric_limits<ui32>().max();
	
	std::vector<CCreature *> creatures; //native creatures for this zone
	for(auto cre : VLC->creh->objects)
	{
		if(!cre->special && cre->faction == zone.getTownType())
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
			for(int i = 0; i < 4; i++)
				vstd::erase_if_present(subObjects, elementalConfluxROE[i]);
		}
		
		for(auto secondaryID : subObjects)
		{
			auto dwellingHandler = dynamic_cast<const CDwellingInstanceConstructor *>(VLC->objtypeh->getHandlerFor(dwellingType, secondaryID).get());
			auto creatures = dwellingHandler->getProducedCreatures();
			if(creatures.empty())
				continue;
			
			auto cre = creatures.front();
			if(cre->faction == zone.getTownType())
			{
				float nativeZonesCount = static_cast<float>(map.getZoneCount(cre->faction));
				oi.value = static_cast<ui32>(cre->AIValue * cre->growth * (1 + (nativeZonesCount / map.getTotalZoneCount()) + (nativeZonesCount / 2)));
				oi.probability = 40;
				
				for(auto tmplate : dwellingHandler->getTemplates())
				{
					if(tmplate.canBePlacedAt(zone.getTerrainType()))
					{
						oi.generateObject = [tmplate, secondaryID, dwellingType]() -> CGObjectInstance *
						{
							auto obj = VLC->objtypeh->getHandlerFor(dwellingType, secondaryID)->create(tmplate);
							obj->tempOwner = PlayerColor::NEUTRAL;
							return obj;
						};
						
						oi.templ = tmplate;
						possibleObjects.push_back(oi);
					}
				}
			}
		}
	}
	
	for(int i = 0; i < gen.getConfig().scrollValues.size(); i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::SPELL_SCROLL, 0);
			auto obj = (CGArtifact *) factory->create(ObjectTemplate());
			std::vector<SpellID> out;
			
			for(auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
			{
				if(map.isAllowedSpell(spell->id) && spell->level == i + 1)
				{
					out.push_back(spell->id);
				}
			}
			auto a = CArtifactInstance::createScroll(*RandomGeneratorUtil::nextItem(out, generator));
			obj->storedArtifact = a;
			return obj;
		};
		oi.setTemplate(Obj::SPELL_SCROLL, 0, zone.getTerrainType());
		oi.value = gen.getConfig().scrollValues[i];
		oi.probability = 30;
		possibleObjects.push_back(oi);
	}
	
	//pandora box with gold
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			obj->resources[Res::GOLD] = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = i * gen.getConfig().pandoraMultiplierGold;
		oi.probability = 5;
		possibleObjects.push_back(oi);
	}
	
	//pandora box with experience
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			obj->gainedExp = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = i * gen.getConfig().pandoraMultiplierExperience;
		oi.probability = 20;
		possibleObjects.push_back(oi);
	}
	
	//pandora box with creatures
	const std::vector<int> & tierValues = gen.getConfig().pandoraCreatureValues;
	
	auto creatureToCount = [&tierValues](CCreature * creature) -> int
	{
		if(!creature->AIValue) //bug #2681
			return 0; //this box won't be generated
		
		int actualTier = creature->level > tierValues.size() ?
		tierValues.size() - 1 :
		creature->level - 1;
		float creaturesAmount = ((float)tierValues[actualTier]) / creature->AIValue;
		if(creaturesAmount <= 5)
		{
			creaturesAmount = boost::math::round(creaturesAmount); //allow single monsters
			if(creaturesAmount < 1)
				return 0;
		}
		else if(creaturesAmount <= 12)
		{
			(creaturesAmount /= 2) *= 2;
		}
		else if(creaturesAmount <= 50)
		{
			creaturesAmount = boost::math::round(creaturesAmount / 5) * 5;
		}
		else
		{
			creaturesAmount = boost::math::round(creaturesAmount / 10) * 10;
		}
		return static_cast<int>(creaturesAmount);
	};
	
	for(auto creature : creatures)
	{
		int creaturesAmount = creatureToCount(creature);
		if(!creaturesAmount)
			continue;
		
		oi.generateObject = [creature, creaturesAmount]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			auto stack = new CStackInstance(creature, creaturesAmount);
			obj->creatures.putStack(SlotID(0), stack);
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = static_cast<ui32>((2 * (creature->AIValue) * creaturesAmount * (1 + (float)(map.getZoneCount(creature->faction)) / map.getTotalZoneCount())) / 3);
		oi.probability = 3;
		possibleObjects.push_back(oi);
	}
	
	//Pandora with 12 spells of certain level
	for(int i = 1; i <= GameConstants::SPELL_LEVELS; i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			
			std::vector <CSpell *> spells;
			for(auto spell : VLC->spellh->objects)
			{
				if(map.isAllowedSpell(spell->id) && spell->level == i)
					spells.push_back(spell);
			}
			
			RandomGeneratorUtil::randomShuffle(spells, generator);
			for(int j = 0; j < std::min(12, (int)spells.size()); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}
			
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = (i + 1) * gen.getConfig().pandoraMultiplierSpells; //5000 - 15000
		oi.probability = 2;
		possibleObjects.push_back(oi);
	}
	
	//Pandora with 15 spells of certain school
	for(int i = 0; i < 4; i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			
			std::vector <CSpell *> spells;
			for(auto spell : VLC->spellh->objects)
			{
				if(map.isAllowedSpell(spell->id) && spell->school[(ESpellSchool)i])
					spells.push_back(spell);
			}
			
			RandomGeneratorUtil::randomShuffle(spells, generator);
			for(int j = 0; j < std::min(15, (int)spells.size()); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}
			
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
		oi.value = gen.getConfig().pandoraSpellSchool;
		oi.probability = 2;
		possibleObjects.push_back(oi);
	}
	
	// Pandora box with 60 random spells
	
	oi.generateObject = [this]() -> CGObjectInstance *
	{
		auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
		auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
		
		std::vector <CSpell *> spells;
		for(auto spell : VLC->spellh->objects)
		{
			if(map.isAllowedSpell(spell->id))
				spells.push_back(spell);
		}
		
		RandomGeneratorUtil::randomShuffle(spells, generator);
		for(int j = 0; j < std::min(60, (int)spells.size()); j++)
		{
			obj->spells.push_back(spells[j]->id);
		}
		
		return obj;
	};
	oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
	oi.value = gen.getConfig().pandoraSpell60;
	oi.probability = 2;
	possibleObjects.push_back(oi);
	
	//seer huts with creatures or generic rewards
	
	if(questArtZone) //we won't be placing seer huts if there is no zone left to place arties
	{
		static const int genericSeerHuts = 8;
		int seerHutsPerType = 0;
		const int questArtsRemaining = static_cast<int>(gen.getQuestArtsRemaning().size());
		
		//general issue is that not many artifact types are available for quests
		
		if(questArtsRemaining >= genericSeerHuts + (int)creatures.size())
		{
			seerHutsPerType = questArtsRemaining / (genericSeerHuts + (int)creatures.size());
		}
		else if(questArtsRemaining >= genericSeerHuts)
		{
			seerHutsPerType = 1;
		}
		oi.maxPerZone = seerHutsPerType;
		
		RandomGeneratorUtil::randomShuffle(creatures, gen.rand);
		
		auto generateArtInfo = [this](ArtifactID id) -> ObjectInfo
		{
			ObjectInfo artInfo;
			artInfo.probability = std::numeric_limits<ui16>::max(); //99,9% to spawn that art in first treasure pile
			artInfo.maxPerZone = 1;
			artInfo.value = 2000; //treasure art
			artInfo.setTemplate(Obj::ARTIFACT, id, this->zone.getTerrainType());
			artInfo.generateObject = [id]() -> CGObjectInstance *
			{
				auto handler = VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, id);
				return handler->create(handler->getTemplates().front());
			};
			return artInfo;
		};
		
		for(int i = 0; i < std::min((int)creatures.size(), questArtsRemaining - genericSeerHuts); i++)
		{
			auto creature = creatures[i];
			int creaturesAmount = creatureToCount(creature);
			
			if(!creaturesAmount)
				continue;
			
			int randomAppearance = chooseRandomAppearance(generator, Obj::SEER_HUT, zone.getTerrainType());
			
			oi.generateObject = [&gen, creature, creaturesAmount, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());
				obj->rewardType = CGSeerHut::CREATURE;
				obj->rID = creature->idNumber;
				obj->rVal = creaturesAmount;
				
				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen.getQuestArtsRemaning(), generator);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;
				
				gen.banQuestArt(artid);
				
				
				this->questArtZone->getModificator<TreasurePlacer>()->possibleObjects.push_back(generateArtInfo(artid));
				
				return obj;
			};
			oi.setTemplate(Obj::SEER_HUT, randomAppearance, zone.getTerrainType());
			oi.value = static_cast<ui32>(((2 * (creature->AIValue) * creaturesAmount * (1 + (float)(map.getZoneCount(creature->faction)) / map.getTotalZoneCount())) - 4000) / 3);
			oi.probability = 3;
			possibleObjects.push_back(oi);
		}
		
		static int seerLevels = std::min(gen.getConfig().questValues.size(), gen.getConfig().questRewardValues.size());
		for(int i = 0; i < seerLevels; i++) //seems that code for exp and gold reward is similiar
		{
			int randomAppearance = chooseRandomAppearance(gen.rand, Obj::SEER_HUT, zone.getTerrainType());
			
			oi.setTemplate(Obj::SEER_HUT, randomAppearance, zone.getTerrainType());
			oi.value = gen.getConfig().questValues[i];
			oi.probability = 10;
			
			oi.generateObject = [&gen, i, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());
				
				obj->rewardType = CGSeerHut::EXPERIENCE;
				obj->rID = 0; //unitialized?
				obj->rVal = gen.getConfig().questRewardValues[i];
				
				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen.getQuestArtsRemaning(), generator);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;
				
				gen.banQuestArt(artid);
				
				this->questArtZone->getModificator<TreasurePlacer>()->possibleObjects.push_back(generateArtInfo(artid));
				
				return obj;
			};
			
			possibleObjects.push_back(oi);
			
			oi.generateObject = [&gen, i, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());
				obj->rewardType = CGSeerHut::RESOURCES;
				obj->rID = Res::GOLD;
				obj->rVal = gen.getConfig().questRewardValues[i];
				
				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen.getQuestArtsRemaning(), generator);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;
				
				gen.banQuestArt(artid);
				
				this->questArtZone->getModificator<TreasurePlacer>()->possibleObjects.push_back(generateArtInfo(artid));
				
				return obj;
			};
			
			possibleObjects.push_back(oi);
		}
	}
}

bool TreasurePlacer::isGuardNeededForTreasure(int value)
{
	return zone.getType() != ETemplateZoneType::WATER && value > minGuardedValue;
}

std::vector<ObjectInfo> TreasurePlacer::prepareTreasurePile(const CTreasureInfo& treasureInfo)
{
	std::vector<ObjectInfo> objectInfos, tempObjectInfos;
	int maxValue = treasureInfo.max;
	int minValue = treasureInfo.min;
	
	const ui32 desiredValue = generator.nextInt(minValue, maxValue);
	
	int currentValue = 0;
	bool hasLargeObject = false;
	while(currentValue <= (int)desiredValue - 100) //no objects with value below 100 are available
	{
		auto oi = getRandomObject(desiredValue, currentValue, !hasLargeObject);
		if(oi.value == 0) //fail
			break;
		
		if(oi.templ.isVisitableFromTop())
		{
			tempObjectInfos.push_back(oi);
		}
		else
		{
			if(hasLargeObject)
			{
				//only one large object per treasure pile
				continue;
			}
			else
			{
				tempObjectInfos.insert(tempObjectInfos.begin(), oi); //large object shall at first place
				hasLargeObject = true;
			}
		}
		
		auto rmgTemp = constuctTreasurePile(tempObjectInfos);
		if(rmgTemp.instances().empty()) //handle incorrect placement
		{
			tempObjectInfos = objectInfos;
			continue;
		}
		rmgTemp.clear();
		
		//remove from possible objects
		auto oiptr = std::find(possibleObjects.begin(), possibleObjects.end(), oi);
		oiptr->maxPerZone--;
		
		currentValue += oi.value;
		
		objectInfos = tempObjectInfos;
	}
	
	return objectInfos;
}

Rmg::Object TreasurePlacer::constuctTreasurePile(const std::vector<ObjectInfo> & treasureInfos)
{
	Rmg::Object rmgObject;
	for(auto & oi : treasureInfos)
	{
		auto blockedArea = rmgObject.getArea();
		auto accessibleArea = rmgObject.getAccessibleArea();
		if(accessibleArea.empty())
			accessibleArea.add(int3());
		
		auto * object = oi.generateObject();
		object->appearance = oi.templ;
		auto & instance = rmgObject.addInstance(*object);

		do
		{
			if(accessibleArea.empty())
			{
				//fail - fallback
				rmgObject.clear();
				return rmgObject;
			}
			
			int3 nextPos = *RandomGeneratorUtil::nextItem(accessibleArea.getTiles(), generator);
			instance.setPosition(nextPos - rmgObject.getPosition());
			
			auto instanceAccessibleArea = instance.getAccessibleArea();
			if(instance.getBlockedArea().getTiles().size() == 1)
				instanceAccessibleArea.add(instance.getVisitablePosition());
			
			//condition for good position
			if(!blockedArea.overlap(instance.getBlockedArea()) && accessibleArea.overlap(instanceAccessibleArea))
				break;
			
			//fail - new position
			accessibleArea.erase(nextPos);
		} while(true);
	}
	return rmgObject;
}

ObjectInfo TreasurePlacer::getRandomObject(ui32 desiredValue, ui32 currentValue, bool allowLargeObjects)
{
	std::vector<std::pair<ui32, ObjectInfo*>> thresholds; //handle complex object via pointer
	ui32 total = 0;
	
	//calculate actual treasure value range based on remaining value
	ui32 maxVal = desiredValue - currentValue;
	ui32 minValue = static_cast<ui32>(0.25f * (desiredValue - currentValue));
	
	for(ObjectInfo & oi : possibleObjects) //copy constructor turned out to be costly
	{
		if(oi.value > maxVal)
			break; //this assumes values are sorted in ascending order TODO: do we need continue or break?
		
		if(!oi.templ.isVisitableFromTop() && !allowLargeObjects)
			continue;
		
		if(oi.value >= minValue && oi.maxPerZone > 0)
		{
			total += oi.probability;
			thresholds.push_back(std::make_pair(total, &oi));
		}
	}
	
	if(thresholds.empty())
	{
		ObjectInfo oi;
		//Generate pandora Box with gold if the value is extremely high
		if(minValue > 30000) //we don't have object valuable enough TODO: use gen.getConfig().treasureValueLimit
		{
			oi.generateObject = [minValue]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
				auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
				obj->resources[Res::GOLD] = minValue;
				return obj;
			};
			oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
			oi.value = minValue;
			oi.probability = 0;
		}
		else //generate empty object with 0 value if the value if we can't spawn anything*/
		{
			oi.generateObject = []() -> CGObjectInstance *
			{
				return nullptr;
			};
			oi.setTemplate(Obj::PANDORAS_BOX, 0, zone.getTerrainType()); //TODO: null template or something? should be never used, but hell knows
			oi.value = 0; // this field is checked to determine no object
			oi.probability = 0;
		}
		return oi;
	}
	else
	{
		int r = generator.nextInt(1, total);
		
		//binary search = fastest
		auto it = std::lower_bound(thresholds.begin(), thresholds.end(), r,
								   [](const std::pair<ui32, ObjectInfo*> &rhs, const int lhs)->bool
								   {
			return (int)rhs.first < lhs;
		});
		return *(it->second);
	}
}

void TreasurePlacer::createTreasures(ObjectManager & manager)
{
	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = zone.zoneMonsterStrength + mapMonsterStrength - 1; //array index from 0 to 4
	
	static int minGuardedValues[] = { 6500, 4167, 3000, 1833, 1333 };
	minGuardedValue = minGuardedValues[monsterStrength];
	
	auto valueComparator = [](const CTreasureInfo & lhs, const CTreasureInfo & rhs) -> bool
	{
		return lhs.max > rhs.max;
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
		
		bool stop = false;
		do
		{
			auto treasurePileInfos = prepareTreasurePile(t);
			if(treasurePileInfos.empty())
				break;
			
			int value = std::accumulate(treasurePileInfos.begin(), treasurePileInfos.end(), 0, [](int v, const ObjectInfo & oi){return v + oi.value;});
			
			auto rmgObject = constuctTreasurePile(treasurePileInfos);
			if(rmgObject.instances().empty()) //handle incorrect placement
				continue;
			
			//guard treasure pile
			bool guarded = isGuardNeededForTreasure(value);
			if(guarded)
				guarded = manager.addGuard(rmgObject, value);
			
			int3 pos;
			auto possibleArea = zone.areaPossible();
			while(true)
			{
				pos = manager.findPlaceForObject(possibleArea, rmgObject, minDistance);
				if(!pos.valid())
				{
					stop = true;
					break; //we placed all objects
				}
				possibleArea.erase(pos); //do not place again at this point
				auto possibleAreaTemp = zone.areaPossible();
				zone.areaPossible().subtract(rmgObject.getArea());
				if(zone.connectPath(rmgObject.getAccessibleArea(), false))
				{
					manager.placeObject(rmgObject, guarded, true);
					break;
				}
				zone.areaPossible() = possibleAreaTemp;
			}
		} while(!stop);
	}
}

ObjectInfo::ObjectInfo()
: templ(), value(0), probability(0), maxPerZone(1)
{
	
}

void ObjectInfo::setTemplate(si32 type, si32 subtype, Terrain terrainType)
{
	auto templHandler = VLC->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	auto templates = templHandler->getTemplates(terrainType);
	if(templates.empty())
		return;
	
	templ = templates.front();
}

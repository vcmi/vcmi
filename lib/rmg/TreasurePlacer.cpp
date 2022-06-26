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

bool TreasurePlacer::findPlaceForTreasurePile(float min_dist, int3 &pos, int value)
{
	float best_distance = 0;
	bool result = false;
	
	bool needsGuard = isGuardNeededForTreasure(value);
	
	//logGlobal->info("Min dist for density %f is %d", density, min_dist);
	for(auto tile : zone.areaPossible().getTiles())
	{
		auto dist = map.getNearestObjectDistance(tile);
		
		if((dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			map.foreach_neighbour(tile, [this, &allTilesAvailable, needsGuard](int3 neighbour)
			{
				if(!(map.isPossible(neighbour) || map.shouldBeBlocked(neighbour) || map.getZoneID(neighbour)==zone.getId() || (!needsGuard && map.isFree(neighbour))))
				{
					allTilesAvailable = false; //all present tiles must be already blocked or ready for new objects
				}
			});
			if(allTilesAvailable)
			{
				best_distance = dist;
				pos = tile;
				result = true;
			}
		}
	}
	if(result)
	{
		map.setOccupied(pos, ETileType::BLOCKED); //block that tile //FIXME: why?
	}
	return result;
}

bool TreasurePlacer::createTreasurePile(ObjectManager & manager, int3 &pos, float minDistance, const CTreasureInfo& treasureInfo)
{
	CTreasurePileInfo info;
	
	std::map<int3, CGObjectInstance *> treasures;
	std::set<int3> boundary;
	int3 guardPos (-1,-1,-1);
	info.nextTreasurePos = pos;
	
	int maxValue = treasureInfo.max;
	int minValue = treasureInfo.min;
	
	ui32 desiredValue = (generator.nextInt(minValue, maxValue));
	
	int currentValue = 0;
	CGObjectInstance * object = nullptr;
	while (currentValue <= (int)desiredValue - 100) //no objects with value below 100 are available
	{
		treasures[info.nextTreasurePos] = nullptr;
		
		for(auto treasurePos : treasures)
		{
			map.foreach_neighbour(treasurePos.first, [&boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}
		for(auto treasurePos : treasures)
		{
			//leaving only boundary around objects
			vstd::erase_if_present(boundary, treasurePos.first);
		}
		
		for(auto tile : boundary)
		{
			//we can't extend boundary anymore
			if(!(map.isBlocked(tile) || map.isPossible(tile)))
				break;
		}
		
		ObjectInfo oi = getRandomObject(manager, info, desiredValue, maxValue, currentValue);
		if(!oi.value) //0 value indicates no object
		{
			vstd::erase_if_present(treasures, info.nextTreasurePos);
			break;
		}
		else
		{
			object = oi.generateObject();
			object->appearance = oi.templ;
			
			//remove from possible objects
			auto oiptr = std::find(possibleObjects.begin(), possibleObjects.end(), oi);
			assert (oiptr != possibleObjects.end());
			oiptr->maxPerZone--;
			if(!oiptr->maxPerZone)
				possibleObjects.erase(oiptr);
			
			//update treasure pile area
			int3 visitablePos = info.nextTreasurePos;
			
			if(oi.templ.isVisitableFromTop())
				info.visitableFromTopPositions.insert(visitablePos); //can be accessed from any direction
			else
				info.visitableFromBottomPositions.insert(visitablePos); //can be accessed only from bottom or side
			
			for(auto blockedOffset : oi.templ.getBlockedOffsets())
			{
				int3 blockPos = info.nextTreasurePos + blockedOffset + oi.templ.getVisitableOffset(); //object will be moved to align vistable pos to treasure pos
				info.occupiedPositions.insert(blockPos);
				info.blockedPositions.insert(blockPos);
			}
			info.occupiedPositions.insert(visitablePos + oi.templ.getVisitableOffset());
			
			currentValue += oi.value;
			
			treasures[info.nextTreasurePos] = object;
			
			//now find place for next object
			int3 placeFound(-1,-1,-1);
			
			//randomize next position from among possible ones
			std::vector<int3> boundaryCopy (boundary.begin(), boundary.end());
			//RandomGeneratorUtil::randomShuffle(boundaryCopy, gen.rand);
			auto chooseTopTile = [](const int3 & lhs, const int3 & rhs) -> bool
			{
				return lhs.y < rhs.y;
			};
			boost::sort(boundaryCopy, chooseTopTile); //start from top tiles to allow objects accessible from bottom
			
			for(auto tile : boundaryCopy)
			{
				if(map.isPossible(tile) && map.getZoneID(tile) == zone.getId()) //we can place new treasure only on possible tile
				{
					bool here = true;
					map.foreach_neighbour (tile, [this, &here, minDistance](int3 pos)
					{
						if(!(map.isBlocked(pos) || map.isPossible(pos)) || map.getZoneID(pos) != zone.getId() || map.getNearestObjectDistance(pos) < minDistance)
							here = false;
					});
					if(here)
					{
						placeFound = tile;
						break;
					}
				}
			}
			if(placeFound.valid())
				info.nextTreasurePos = placeFound;
			else
				break; //no more place to add any objects
		}
	}
	
	if(treasures.size())
	{
		//find object closest to free path, then connect it to the middle of the zone
		
		int3 closestTile = int3(-1,-1,-1);
		float minTreasureDistance = 1e10;
		
		for(auto visitablePos : info.visitableFromBottomPositions) //objects that are not visitable from top must be accessible from bottom or side
		{
			int3 closestFreeTile = zone.freePaths().nearest(visitablePos);
			if(closestFreeTile.dist2d(visitablePos) < minTreasureDistance)
			{
				closestTile = visitablePos + int3 (0, 1, 0); //start below object (y+1), possibly even outside the map, to not make path up through it
				minTreasureDistance = static_cast<float>(closestFreeTile.dist2d(visitablePos));
			}
		}
		for(auto visitablePos : info.visitableFromTopPositions) //all objects are accessible from any direction
		{
			int3 closestFreeTile = zone.freePaths().nearest(visitablePos);
			if(closestFreeTile.dist2d(visitablePos) < minTreasureDistance)
			{
				closestTile = visitablePos;
				minTreasureDistance = static_cast<float>(closestFreeTile.dist2d(visitablePos));
			}
		}
		assert (closestTile.valid());
		
		for(auto tile : info.occupiedPositions)
		{
			if(map.isOnMap(tile) && map.isPossible(tile) && map.getZoneID(tile) == zone.getId()) //pile boundary may reach map border
				map.setOccupied(tile, ETileType::BLOCKED); //so that crunch path doesn't cut through objects
		}
		
		if(!zone.connectPath(closestTile, false)) //this place is sealed off, need to find new position
		{
			return false;
		}
		
		//update boundary around our objects, including knowledge about objects visitable from bottom
		boundary.clear();
		
		for(auto tile : info.visitableFromBottomPositions)
		{
			map.foreach_neighbour(tile, [tile, &boundary](int3 pos)
			{
				if(pos.y >= tile.y) //don't block these objects from above
					boundary.insert(pos);
			});
		}
		for(auto tile : info.visitableFromTopPositions)
		{
			map.foreach_neighbour(tile, [&boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}
		
		bool isPileGuarded = isGuardNeededForTreasure(currentValue);
		
		for(auto tile : boundary) //guard must be standing there
		{
			if(map.isFree(tile)) //this tile could be already blocked, don't place a monster here
			{
				guardPos = tile;
				break;
			}
		}
		
		if(guardPos.valid())
		{
			for(auto treasure : treasures)
			{
				int3 visitableOffset = treasure.second->getVisitableOffset();
				manager.placeObject(treasure.second, treasure.first + visitableOffset);
			}
			if(isPileGuarded && manager.addMonster(guardPos, currentValue, false))
			{//block only if the object is guarded
				for(auto tile : boundary)
				{
					if(map.isPossible(tile))
						map.setOccupied(tile, ETileType::BLOCKED);
				}
				//do not spawn anything near monster
				map.foreach_neighbour(guardPos, [this](int3 pos)
				{
					if(map.isPossible(pos))
						map.setOccupied(pos, ETileType::FREE);
				});
			}
		}
		else if(isPileGuarded)//we couldn't make a connection to this location, block it
		{
			for(auto treasure : treasures)
			{
				if(map.isPossible(treasure.first))
					map.setOccupied(treasure.first, ETileType::BLOCKED);
				
				delete treasure.second;
			}
		}
		
		return true;
	}
	else //we did not place eveyrthing successfully
	{
		if(map.isPossible(pos))
			map.setOccupied(pos, ETileType::BLOCKED); //TODO: refactor stop condition
		zone.areaPossible().erase(pos);
		return false;
	}
}

ObjectInfo TreasurePlacer::getRandomObject(ui32 desiredValue, ui32 currentValue)
{
	std::vector<std::pair<ui32, ObjectInfo*>> thresholds; //handle complex object via pointer
	ui32 total = 0;
	
	//calculate actual treasure value range based on remaining value
	ui32 maxVal = desiredValue - currentValue;
	ui32 minValue = static_cast<ui32>(0.25f * (desiredValue - currentValue));
	
	for(ObjectInfo & oi : possibleObjects) //copy constructor turned out to be costly
	{
		if(oi.value > maxVal)
			continue; //this assumes values are sorted in ascending order TODO: do we need continue or break?
		
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

ObjectInfo TreasurePlacer::getRandomObject(const ObjectManager & manager, CTreasurePileInfo &info, ui32 desiredValue, ui32 maxValue, ui32 currentValue)
{
	//int objectsVisitableFromBottom = 0; //for debug
	
	std::vector<std::pair<ui32, ObjectInfo*>> thresholds; //handle complex object via pointer
	ui32 total = 0;
	
	//calculate actual treasure value range based on remaining value
	ui32 maxVal = desiredValue - currentValue;
	ui32 minValue = static_cast<ui32>(0.25f * (desiredValue - currentValue));
	
	//roulette wheel
	for(ObjectInfo &oi : possibleObjects) //copy constructor turned out to be costly
	{
		if(oi.value > maxVal)
			break; //this assumes values are sorted in ascending order
		if(oi.value >= minValue && oi.maxPerZone > 0)
		{
			int3 newVisitableOffset = oi.templ.getVisitableOffset(); //visitablePos assumes object will be shifter by visitableOffset
			int3 newVisitablePos = info.nextTreasurePos;
			
			if(!oi.templ.isVisitableFromTop())
			{
				//objectsVisitableFromBottom++;
				//there must be free tiles under object
				auto blockedOffsets = oi.templ.getBlockedOffsets();
				if(!manager.isAccessibleFromSomewhere(oi.templ, newVisitablePos))
					continue;
			}
			
			//NOTE: y coordinate grows downwards
			if(info.visitableFromBottomPositions.size() + info.visitableFromTopPositions.size()) //do not try to match first object in zone
			{
				bool fitsHere = false;
				
				if(oi.templ.isVisitableFromTop()) //new can be accessed from any direction
				{
					for(auto tile : info.visitableFromTopPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if(newVisitablePos.areNeighbours(actualTile)) //we access other removable object from any position
						{
							fitsHere = true;
							break;
						}
					}
					for(auto tile : info.visitableFromBottomPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if(newVisitablePos.areNeighbours(actualTile) && newVisitablePos.y >= actualTile.y) //we access existing static object from side or bottom only
						{
							fitsHere = true;
							break;
						}
					}
				}
				else //if new object is not visitable from top, it must be accessible from below or side
				{
					for(auto tile : info.visitableFromTopPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if(newVisitablePos.areNeighbours(actualTile) && newVisitablePos.y <= actualTile.y) //we access existing removable object from top or side only
						{
							fitsHere = true;
							break;
						}
					}
					for(auto tile : info.visitableFromBottomPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if(newVisitablePos.areNeighbours(actualTile) && newVisitablePos.y == actualTile.y) //we access other static object from side only
						{
							fitsHere = true;
							break;
						}
					}
				}
				if(!fitsHere)
					continue;
			}
			
			//now check blockmap, including our already reserved pile area
			
			bool fitsBlockmap = true;
			
			std::set<int3> blockedOffsets = oi.templ.getBlockedOffsets();
			blockedOffsets.insert (newVisitableOffset);
			for(auto blockingTile : blockedOffsets)
			{
				int3 t = info.nextTreasurePos + newVisitableOffset + blockingTile;
				if(!map.isOnMap(t) || vstd::contains(info.occupiedPositions, t))
				{
					fitsBlockmap = false; //if at least one tile is not possible, object can't be placed here
					break;
				}
				if(!(map.isPossible(t) || map.isBlocked(t))) //blocked tiles of object may cover blocked tiles, but not used or free tiles
				{
					fitsBlockmap = false;
					break;
				}
			}
			if(!fitsBlockmap)
				continue;
			
			total += oi.probability;
			
			thresholds.push_back(std::make_pair (total, &oi));
		}
	}
	
	if(thresholds.empty())
	{
		ObjectInfo oi;
		//Generate pandora Box with gold if the value is extremely high
		/*if(minValue > gen.getConfig().treasureValueLimit) //we don't have object valuable enough
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
		do {
			//optimization - don't check tiles which are not allowed
			std::vector<int3> possibleTilesToRemove;
			for(auto tile : zone.areaPossible().getTiles())
			{
				//for water area we sholdn't place treasures close to coast
				/*for(auto & lake : lakes)
					if(vstd::contains(lake.distance, tile) && lake.distance[tile] < 2)
						return true;*/
				
				if(!map.isPossible(tile) || map.getZoneID(tile) != zone.getId())
					possibleTilesToRemove.push_back(tile);
			}
			for(auto tile : possibleTilesToRemove)
			{
				zone.areaPossible().erase(tile);
			}
			
			int3 treasureTilePos;
			//If we are able to place at least one object with value lower than minGuardedValue, it's ok
			do
			{
				if (!findPlaceForTreasurePile(minDistance, treasureTilePos, t.min))
				{
					stop = true;
					break;
				}
			}
			while (!createTreasurePile(manager, treasureTilePos, minDistance, t)); //failed creation - position was wrong, cannot connect it
			
		} while (!stop);
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

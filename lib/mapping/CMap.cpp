/*
 * CMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMap.h"

#include "../CArtHandler.h"
#include "../VCMI_Lib.h"
#include "../CCreatureHandler.h"
#include "../GameSettings.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../texts/CGeneralTextHandler.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "CMapEditManager.h"
#include "CMapOperation.h"
#include "../serializer/JsonSerializeFormat.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void Rumor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeStruct("text", text);
}

DisposedHero::DisposedHero() : heroId(0), portrait(255)
{

}

CMapEvent::CMapEvent()
	: humanAffected(false)
	, computerAffected(false)
	, firstOccurrence(0)
	, nextOccurrence(0)
{

}

bool CMapEvent::occursToday(int currentDay) const
{
	if (currentDay == firstOccurrence + 1)
		return true;

	if (nextOccurrence == 0)
		return false;

	if (currentDay < firstOccurrence)
		return false;

	return (currentDay - firstOccurrence - 1) % nextOccurrence == 0;
}

bool CMapEvent::affectsPlayer(PlayerColor color, bool isHuman) const
{
	if (players.count(color) == 0)
		return false;

	if (!isHuman && !computerAffected)
		return false;

	if (isHuman && !humanAffected)
		return false;

	return true;
}

void CMapEvent::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeStruct("message", message);
	if (!handler.saving && handler.getCurrent()["players"].isNumber())
	{
		// compatibility for old maps
		int playersMask = 0;
		handler.serializeInt("players", playersMask);
		for (int i = 0; i < 8; ++i)
			if ((playersMask & (1 << i)) != 0)
				players.insert(PlayerColor(i));
	}
	else
	{
		handler.serializeIdArray("players", players);
	}
	handler.serializeInt("humanAffected", humanAffected);
	handler.serializeInt("computerAffected", computerAffected);
	handler.serializeInt("firstOccurrence", firstOccurrence);
	handler.serializeInt("nextOccurrence", nextOccurrence);
	resources.serializeJson(handler, "resources");
}

void CCastleEvent::serializeJson(JsonSerializeFormat & handler)
{
	CMapEvent::serializeJson(handler);

	// TODO: handler.serializeIdArray("buildings", buildings);
	{
		std::vector<BuildingID> temp(buildings.begin(), buildings.end());
		auto a = handler.enterArray("buildings");
		a.syncSize(temp);
		for(int i = 0; i < temp.size(); ++i)
		{
			int buildingID = temp[i].getNum();
			a.serializeInt(i, buildingID);
			buildings.insert(buildingID);
		}
	}

	{
		auto a = handler.enterArray("creatures");
		a.syncSize(creatures);
		for(int i = 0; i < creatures.size(); ++i)
			a.serializeInt(i, creatures[i]);
	}
}

TerrainTile::TerrainTile():
	terType(nullptr),
	riverType(VLC->riverTypeHandler->getById(River::NO_RIVER)),
	roadType(VLC->roadTypeHandler->getById(Road::NO_ROAD)),
	terView(0),
	riverDir(0),
	roadDir(0),
	extTileFlags(0),
	visitable(false),
	blocked(false)
{
}

bool TerrainTile::entrableTerrain(const TerrainTile * from) const
{
	return entrableTerrain(from ? from->terType->isLand() : true, from ? from->terType->isWater() : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
	return terType->isPassable()
			&& ((allowSea && terType->isWater())  ||  (allowLand && terType->isLand()));
}

bool TerrainTile::isClear(const TerrainTile * from) const
{
	return entrableTerrain(from) && !blocked;
}

Obj TerrainTile::topVisitableId(bool excludeTop) const
{
	return topVisitableObj(excludeTop) ? topVisitableObj(excludeTop)->ID : Obj(Obj::NO_OBJ);
}

CGObjectInstance * TerrainTile::topVisitableObj(bool excludeTop) const
{
	if(visitableObjects.empty() || (excludeTop && visitableObjects.size() == 1))
		return nullptr;

	if(excludeTop)
		return visitableObjects[visitableObjects.size()-2];

	return visitableObjects.back();
}

EDiggingStatus TerrainTile::getDiggingStatus(const bool excludeTop) const
{
	if(terType->isWater() || !terType->isPassable())
		return EDiggingStatus::WRONG_TERRAIN;

	int allowedBlocked = excludeTop ? 1 : 0;
	if(blockingObjects.size() > allowedBlocked || topVisitableObj(excludeTop))
		return EDiggingStatus::TILE_OCCUPIED;
	else
		return EDiggingStatus::CAN_DIG;
}

bool TerrainTile::hasFavorableWinds() const
{
	return extTileFlags & 128;
}

bool TerrainTile::isWater() const
{
	return terType->isWater();
}

CMap::CMap(IGameCallback * cb)
	: GameCallbackHolder(cb)
	, checksum(0)
	, grailPos(-1, -1, -1)
	, grailRadius(0)
	, waterMap(false)
	, uidCounter(0)
{
	allHeroes.resize(VLC->heroh->size());
	allowedAbilities = VLC->skillh->getDefaultAllowed();
	allowedArtifact = VLC->arth->getDefaultAllowed();
	allowedSpells = VLC->spellh->getDefaultAllowed();

	gameSettings = std::make_unique<GameSettings>();
	gameSettings->loadBase(VLC->settingsHandler->getFullConfig());
}

CMap::~CMap()
{
	getEditManager()->getUndoManager().clearAll();

	for(auto obj : objects)
		obj.dellNull();

	for(auto quest : quests)
		quest.dellNull();

	for(auto artInstance : artInstances)
		artInstance.dellNull();

	resetStaticData();
}

void CMap::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	const int zVal = obj->anchorPos().z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->anchorPos().x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->anchorPos().y - fy;
			if(xVal>=0 && xVal < width && yVal>=0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(total || obj->visitableAt(int3(xVal, yVal, zVal)))
				{
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(total || obj->blockingAt(int3(xVal, yVal, zVal)))
				{
					curt.blockingObjects -= obj;
					curt.blocked = curt.blockingObjects.size();
				}
			}
		}
	}
}

void CMap::addBlockVisTiles(CGObjectInstance * obj)
{
	const int zVal = obj->anchorPos().z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->anchorPos().x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->anchorPos().y - fy;
			if(xVal>=0 && xVal < width && yVal >= 0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(obj->visitableAt(int3(xVal, yVal, zVal)))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(obj->blockingAt(int3(xVal, yVal, zVal)))
				{
					curt.blockingObjects.push_back(obj);
					curt.blocked = true;
				}
			}
		}
	}
}

void CMap::calculateGuardingGreaturePositions()
{
	int levels = twoLevel ? 2 : 1;
	for(int z = 0; z < levels; z++)
	{
		for(int x = 0; x < width; x++)
		{
			for(int y = 0; y < height; y++)
			{
				guardingCreaturePositions[z][x][y] = guardingCreaturePosition(int3(x, y, z));
			}
		}
	}
}

CGHeroInstance * CMap::getHero(HeroTypeID heroID)
{
	for(auto & elem : heroesOnMap)
		if(elem->getHeroTypeID() == heroID)
			return elem;
	return nullptr;
}

bool CMap::isCoastalTile(const int3 & pos) const
{
	//todo: refactoring: extract neighbor tile iterator and use it in GameState
	static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	if(!isInTheMap(pos))
	{
		logGlobal->error("Coastal check outside of map: %s", pos.toString());
		return false;
	}

	if(isWaterTile(pos))
		return false;

	for(const auto & dir : dirs)
	{
		const int3 hlp = pos + dir;

		if(!isInTheMap(hlp))
			continue;
		const TerrainTile &hlpt = getTile(hlp);
		if(hlpt.isWater())
			return true;
	}

	return false;
}

TerrainTile & CMap::getTile(const int3 & tile)
{
	assert(isInTheMap(tile));
	return terrain[tile.z][tile.x][tile.y];
}

const TerrainTile & CMap::getTile(const int3 & tile) const
{
	assert(isInTheMap(tile));
	return terrain[tile.z][tile.x][tile.y];
}

bool CMap::isWaterTile(const int3 &pos) const
{
	return isInTheMap(pos) && getTile(pos).isWater();
}
bool CMap::canMoveBetween(const int3 &src, const int3 &dst) const
{
	const TerrainTile * dstTile = &getTile(dst);
	const TerrainTile * srcTile = &getTile(src);
	return checkForVisitableDir(src, dstTile, dst) && checkForVisitableDir(dst, srcTile, src);
}

bool CMap::checkForVisitableDir(const int3 & src, const TerrainTile * pom, const int3 & dst) const
{
	if (!pom->entrableTerrain()) //rock is never accessible
		return false;
	for(auto * obj : pom->visitableObjects) //checking destination tile
	{
		if(!vstd::contains(pom->blockingObjects, obj)) //this visitable object is not blocking, ignore
			continue;

		if (!obj->appearance->isVisitableFrom(src.x - dst.x, src.y - dst.y))
			return false;
	}
	return true;
}

int3 CMap::guardingCreaturePosition (int3 pos) const
{
	const int3 originalPos = pos;
	// Give monster at position priority.
	if (!isInTheMap(pos))
		return int3(-1, -1, -1);
	const TerrainTile &posTile = getTile(pos);
	if (posTile.visitable)
	{
		for (CGObjectInstance* obj : posTile.visitableObjects)
		{
			if (obj->ID == Obj::MONSTER)
				return pos;
		}
	}

	// See if there are any monsters adjacent.
	bool water = posTile.isWater();

	pos -= int3(1, 1, 0); // Start with top left.
	for (int dx = 0; dx < 3; dx++)
	{
		for (int dy = 0; dy < 3; dy++)
		{
			if (isInTheMap(pos))
			{
				const auto & tile = getTile(pos);
                if (tile.visitable && (tile.isWater() == water))
				{
					for (CGObjectInstance* obj : tile.visitableObjects)
					{
						if (obj->ID == Obj::MONSTER  &&  checkForVisitableDir(pos, &posTile, originalPos)) // Monster being able to attack investigated tile
						{
							return pos;
						}
					}
				}
			}

			pos.y++;
		}
		pos.y -= 3;
		pos.x++;
	}

	return int3(-1, -1, -1);
}

const CGObjectInstance * CMap::getObjectiveObjectFrom(const int3 & pos, Obj type)
{
	for (CGObjectInstance * object : getTile(pos).visitableObjects)
	{
		if (object->ID == type)
			return object;
	}
	// There is weird bug because of which sometimes heroes will not be found properly despite having correct position
	// Try to workaround that and find closest object that we can use

	logGlobal->error("Failed to find object of type %d at %s", type.getNum(), pos.toString());
	logGlobal->error("Will try to find closest matching object");

	CGObjectInstance * bestMatch = nullptr;
	for (CGObjectInstance * object : objects)
	{
		if (object && object->ID == type)
		{
			if (bestMatch == nullptr)
				bestMatch = object;
			else
			{
				if (object->anchorPos().dist2dSQ(pos) < bestMatch->anchorPos().dist2dSQ(pos))
					bestMatch = object;// closer than one we already found
			}
		}
	}
	assert(bestMatch != nullptr); // if this happens - victory conditions or map itself is very, very broken

	logGlobal->error("Will use %s from %s", bestMatch->getObjectName(), bestMatch->anchorPos().toString());
	return bestMatch;
}

void CMap::checkForObjectives()
{
	// NOTE: probably should be moved to MapFormatH3M.cpp
	for (TriggeredEvent & event : triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			switch (cond.condition)
			{
				case EventCondition::HAVE_ARTIFACT:
					event.onFulfill.replaceTextID(cond.objectType.as<ArtifactID>().toEntity(VLC)->getNameTextID());
					break;

				case EventCondition::HAVE_CREATURES:
					event.onFulfill.replaceTextID(cond.objectType.as<CreatureID>().toEntity(VLC)->getNameSingularTextID());
					event.onFulfill.replaceNumber(cond.value);
					break;

				case EventCondition::HAVE_RESOURCES:
					event.onFulfill.replaceName(cond.objectType.as<GameResID>());
					event.onFulfill.replaceNumber(cond.value);
					break;

				case EventCondition::HAVE_BUILDING:
					if (isInTheMap(cond.position))
						cond.objectID = getObjectiveObjectFrom(cond.position, Obj::TOWN)->id;
					break;

				case EventCondition::CONTROL:
					if (isInTheMap(cond.position))
						cond.objectID = getObjectiveObjectFrom(cond.position, cond.objectType.as<MapObjectID>())->id;

					if (cond.objectID != ObjectInstanceID::NONE)
					{
						const auto * town = dynamic_cast<const CGTownInstance *>(objects[cond.objectID].get());
						if (town)
							event.onFulfill.replaceRawString(town->getNameTranslated());
						const auto * hero = dynamic_cast<const CGHeroInstance *>(objects[cond.objectID].get());
						if (hero)
							event.onFulfill.replaceRawString(hero->getNameTranslated());
					}
					break;

				case EventCondition::DESTROY:
					if (isInTheMap(cond.position))
						cond.objectID = getObjectiveObjectFrom(cond.position, cond.objectType.as<MapObjectID>())->id;

					if (cond.objectID != ObjectInstanceID::NONE)
					{
						const auto * hero = dynamic_cast<const CGHeroInstance *>(objects[cond.objectID].get());
						if (hero)
							event.onFulfill.replaceRawString(hero->getNameTranslated());
					}
					break;
				case EventCondition::TRANSPORT:
					cond.objectID = getObjectiveObjectFrom(cond.position, Obj::TOWN)->id;
					break;
				//break; case EventCondition::DAYS_PASSED:
				//break; case EventCondition::IS_HUMAN:
				//break; case EventCondition::DAYS_WITHOUT_TOWN:
				//break; case EventCondition::STANDARD_WIN:
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}
}

void CMap::addNewArtifactInstance(CArtifactSet & artSet)
{
	for(const auto & [slot, slotInfo] : artSet.artifactsWorn)
	{
		if(!slotInfo.locked && slotInfo.getArt())
			addNewArtifactInstance(slotInfo.artifact);
	}
	for(const auto & slotInfo : artSet.artifactsInBackpack)
		addNewArtifactInstance(slotInfo.artifact);
}

void CMap::addNewArtifactInstance(ConstTransitivePtr<CArtifactInstance> art)
{
	assert(art);
	assert(art->getId() == -1);
	art->setId(static_cast<ArtifactInstanceID>(artInstances.size()));
	artInstances.emplace_back(art);
		
	for(const auto & partInfo : art->getPartsInfo())
		addNewArtifactInstance(partInfo.art);
}

void CMap::eraseArtifactInstance(CArtifactInstance * art)
{
	//TODO: handle for artifacts removed in map editor
	assert(artInstances[art->getId().getNum()] == art);
	artInstances[art->getId().getNum()].dellNull();
}

void CMap::moveArtifactInstance(
	CArtifactSet & srcSet, const ArtifactPosition & srcSlot,
	CArtifactSet & dstSet, const ArtifactPosition & dstSlot)
{
	auto art = srcSet.getArt(srcSlot);
	removeArtifactInstance(srcSet, srcSlot);
	putArtifactInstance(dstSet, art, dstSlot);
}

void CMap::putArtifactInstance(CArtifactSet & set, CArtifactInstance * art, const ArtifactPosition & slot)
{
	art->addPlacementMap(set.putArtifact(slot, art));
}

void CMap::removeArtifactInstance(CArtifactSet & set, const ArtifactPosition & slot)
{
	auto art = set.getArt(slot);
	assert(art);
	set.removeArtifact(slot);
	CArtifactSet::ArtPlacementMap partsMap;
	for(auto & part : art->getPartsInfo())
	{
		if(part.slot != ArtifactPosition::PRE_FIRST)
			partsMap.try_emplace(part.art, ArtifactPosition::PRE_FIRST);
	}
	art->addPlacementMap(partsMap);
}

void CMap::addNewQuestInstance(CQuest* quest)
{
	quest->qid = static_cast<si32>(quests.size());
	quests.emplace_back(quest);
}

void CMap::removeQuestInstance(CQuest * quest)

{
	//TODO: should be called only by map editor.
	//During game, completed quests or quests from removed objects stay forever

	//Shift indexes
	auto iter = std::next(quests.begin(), quest->qid);
	iter = quests.erase(iter);
	for (int i = quest->qid; iter != quests.end(); ++i, ++iter)
	{
		(*iter)->qid = i;
	}
}

void CMap::setUniqueInstanceName(CGObjectInstance * obj)
{
	//this gives object unique name even if objects are removed later

	auto uid = uidCounter++;

	boost::format fmt("%s_%d");
	fmt % obj->getTypeName() % uid;
	obj->instanceName = fmt.str();
}

void CMap::addNewObject(CGObjectInstance * obj)
{
	if(obj->id != ObjectInstanceID(static_cast<si32>(objects.size())))
		throw std::runtime_error("Invalid object instance id");

	if(obj->instanceName.empty())
		throw std::runtime_error("Object instance name missing");

	if (vstd::contains(instanceNames, obj->instanceName))
		throw std::runtime_error("Object instance name duplicated: "+obj->instanceName);

	objects.emplace_back(obj);
	instanceNames[obj->instanceName] = obj;
	addBlockVisTiles(obj);

	//TODO: how about defeated heroes recruited again?

	obj->afterAddToMap(this);
}

void CMap::moveObject(CGObjectInstance * obj, const int3 & pos)
{
	removeBlockVisTiles(obj);
	obj->setAnchorPos(pos);
	addBlockVisTiles(obj);
}

void CMap::removeObject(CGObjectInstance * obj)
{
	removeBlockVisTiles(obj);
	instanceNames.erase(obj->instanceName);

	//update indices

	auto iter = std::next(objects.begin(), obj->id.getNum());
	iter = objects.erase(iter);
	for(int i = obj->id.getNum(); iter != objects.end(); ++i, ++iter)
	{
		(*iter)->id = ObjectInstanceID(i);
	}

	obj->afterRemoveFromMap(this);

	//TODO: Clean artifact instances (mostly worn by hero?) and quests related to this object
	//This causes crash with undo/redo in editor
}

bool CMap::isWaterMap() const
{
	return waterMap;
}

bool CMap::calculateWaterContent()
{
	size_t totalTiles = height * width * levels();
	size_t waterTiles = 0;

	for(auto tile = terrain.origin(); tile < (terrain.origin() + terrain.num_elements()); ++tile) 
	{
		if (tile->isWater())
		{
			waterTiles++;
		}
	}

	if (waterTiles >= totalTiles / 100) //At least 1% of area is water
	{
		waterMap = true;
	}
	else
	{
		waterMap = false;
	}

	return waterMap;
}

void CMap::banWaterContent()
{
	banWaterHeroes();
	banWaterArtifacts();
	banWaterSpells();
	banWaterSkills();
}

void CMap::banWaterSpells()
{
	vstd::erase_if(allowedSpells, [&](SpellID spell)
	{
		return spell.toSpell()->onlyOnWaterMap && !isWaterMap();
	});
}

void CMap::banWaterArtifacts()
{
	vstd::erase_if(allowedArtifact, [&](ArtifactID artifact)
	{
		return artifact.toArtifact()->onlyOnWaterMap && !isWaterMap();
	});
}

void CMap::banWaterSkills()
{
	vstd::erase_if(allowedAbilities, [&](SecondarySkill skill)
	{
		return skill.toSkill()->onlyOnWaterMap && !isWaterMap();
	});
}

void CMap::banWaterHeroes()
{
	vstd::erase_if(allowedHeroes, [&](HeroTypeID hero)
	{
		return hero.toHeroType()->onlyOnWaterMap && !isWaterMap();
	});

	vstd::erase_if(allowedHeroes, [&](HeroTypeID hero)
	{
		return hero.toHeroType()->onlyOnMapWithoutWater && isWaterMap();
	});
}

void CMap::banHero(const HeroTypeID & id)
{
	if (!vstd::contains(allowedHeroes, id))
		logGlobal->warn("Attempt to ban hero %s, who is already not allowed", id.encode(id));
	allowedHeroes.erase(id);
}

void CMap::unbanHero(const HeroTypeID & id)
{
	if (vstd::contains(allowedHeroes, id))
		logGlobal->warn("Attempt to unban hero %s, who is already allowed", id.encode(id));
	allowedHeroes.insert(id);
}

void CMap::initTerrain()
{
	terrain.resize(boost::extents[levels()][width][height]);
	guardingCreaturePositions.resize(boost::extents[levels()][width][height]);
}

CMapEditManager * CMap::getEditManager()
{
	if(!editManager) editManager = std::make_unique<CMapEditManager>(this);
	return editManager.get();
}

void CMap::resetStaticData()
{
	obeliskCount = 0;
	obelisksVisited.clear();
	townMerchantArtifacts.clear();
	townUniversitySkills.clear();
}

void CMap::resolveQuestIdentifiers()
{
	//FIXME: move to CMapLoaderH3M
	for (auto & quest : quests)
	{
		if (quest && quest->killTarget != ObjectInstanceID::NONE)
			quest->killTarget = questIdentifierToId[quest->killTarget.getNum()];
	}
	questIdentifierToId.clear();
}

void CMap::reindexObjects()
{
	// Only reindex at editor / RMG operations

	std::sort(objects.begin(), objects.end(), [](const CGObjectInstance * lhs, const CGObjectInstance * rhs)
	{
		// Obstacles first, then visitable, at the end - removable

		if (!lhs->isVisitable() && rhs->isVisitable())
			return true;
		if (lhs->isVisitable() && !rhs->isVisitable())
			return false;

		// Special case for Windomill - draw on top of other objects
		if (lhs->ID != Obj::WINDMILL && rhs->ID == Obj::WINDMILL)
			return true;
		if (lhs->ID == Obj::WINDMILL && rhs->ID != Obj::WINDMILL)
			return false;

		if (!lhs->isRemovable() && rhs->isRemovable())
			return true;
		if (lhs->isRemovable() && !rhs->isRemovable())
			return false;

		return lhs->anchorPos().y < rhs->anchorPos().y;
	});

	// instanceNames don't change
	for (size_t i = 0; i < objects.size(); ++i)
	{
		objects[i]->id = ObjectInstanceID(i);
	}
}

const IGameSettings & CMap::getSettings() const
{
	return *gameSettings;
}

void CMap::overrideGameSetting(EGameSettings option, const JsonNode & input)
{
	return gameSettings->addOverride(option, input);
}

void CMap::overrideGameSettings(const JsonNode & input)
{
	return gameSettings->loadOverrides(input);
}

VCMI_LIB_NAMESPACE_END

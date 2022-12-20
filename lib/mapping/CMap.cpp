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
#include "../CTownHandler.h"
#include "../CHeroHandler.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../CGeneralTextHandler.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "CMapEditManager.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

SHeroName::SHeroName() : heroId(-1)
{

}

PlayerInfo::PlayerInfo(): canHumanPlay(false), canComputerPlay(false),
	aiTactic(EAiTactic::RANDOM), isFactionRandom(false), hasRandomHero(false), mainCustomHeroPortrait(-1), mainCustomHeroId(-1), hasMainTown(false),
	generateHeroAtMainTown(false), posOfMainTown(-1), team(TeamID::NO_TEAM), /* following are unused */ generateHero(false), p7(0), powerPlaceholders(-1)
{
	allowedFactions = VLC->townh->getAllowedFactions();
}

si8 PlayerInfo::defaultCastle() const
{
	//if random allowed set it as default
	if(isFactionRandom)
		return -1;

	if(!allowedFactions.empty())
		return *allowedFactions.begin();

	// fall back to random
	return -1;
}

si8 PlayerInfo::defaultHero() const
{
	// we will generate hero in front of main town
	if((generateHeroAtMainTown && hasMainTown) || hasRandomHero)
	{
		//random hero
		return -1;
	}

	return -2;
}

bool PlayerInfo::canAnyonePlay() const
{
	return canHumanPlay || canComputerPlay;
}

bool PlayerInfo::hasCustomMainHero() const
{
	return !mainCustomHeroName.empty() && mainCustomHeroPortrait != -1;
}

EventCondition::EventCondition(EWinLoseType condition):
	object(nullptr),
	metaType(EMetaclass::INVALID),
	value(-1),
	objectType(-1),
	objectSubtype(-1),
	position(-1, -1, -1),
	condition(condition)
{
}

EventCondition::EventCondition(EWinLoseType condition, si32 value, si32 objectType, int3 position):
	object(nullptr),
	metaType(EMetaclass::INVALID),
	value(value),
	objectType(objectType),
	objectSubtype(-1),
	position(position),
	condition(condition)
{}

void Rumor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeString("text", text);
}

DisposedHero::DisposedHero() : heroId(0), portrait(255), players(0)
{

}

CMapEvent::CMapEvent() : players(0), humanAffected(0), computerAffected(0),
	firstOccurence(0), nextOccurence(0)
{

}

bool CMapEvent::earlierThan(const CMapEvent & other) const
{
	return firstOccurence < other.firstOccurence;
}

bool CMapEvent::earlierThanOrEqual(const CMapEvent & other) const
{
	return firstOccurence <= other.firstOccurence;
}

CCastleEvent::CCastleEvent() : town(nullptr)
{

}

TerrainTile::TerrainTile():
	terType(nullptr),
	terView(0),
	riverType(nullptr),
	riverDir(0),
	roadType(nullptr),
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

void CMapHeader::setupEvents()
{
	EventCondition victoryCondition(EventCondition::STANDARD_WIN);
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	//Victory condition - defeat all
	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[5];
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill = VLC->generaltexth->allTexts[659];
	standardVictory.trigger = EventExpression(victoryCondition);

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage = VLC->generaltexth->allTexts[8];
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill = VLC->generaltexth->allTexts[7];
	standardDefeat.trigger = EventExpression(defeatCondition);

	triggeredEvents.push_back(standardVictory);
	triggeredEvents.push_back(standardDefeat);

	victoryIconIndex = 11;
	victoryMessage = VLC->generaltexth->victoryConditions[0];

	defeatIconIndex = 3;
	defeatMessage = VLC->generaltexth->lossCondtions[0];
}

CMapHeader::CMapHeader() : version(EMapFormat::SOD), height(72), width(72),
	twoLevel(true), difficulty(1), levelLimit(0), howManyTeams(0), areAnyPlayers(false)
{
	setupEvents();
	allowedHeroes = VLC->heroh->getDefaultAllowed();
	players.resize(PlayerColor::PLAYER_LIMIT_I);
}

CMapHeader::~CMapHeader()
{
}

ui8 CMapHeader::levels() const
{
	return (twoLevel ? 2 : 1);
}

CMap::CMap()
	: checksum(0), grailPos(-1, -1, -1), grailRadius(0), terrain(nullptr),
	guardingCreaturePositions(nullptr),
	uidCounter(0)
{
	allHeroes.resize(allowedHeroes.size());
	allowedAbilities = VLC->skillh->getDefaultAllowed();
	allowedArtifact = VLC->arth->getDefaultAllowed();
	allowedSpell = VLC->spellh->getDefaultAllowed();
}

CMap::~CMap()
{
	getEditManager()->getUndoManager().clearAll();
	
	if(terrain)
	{
		for(int z = 0; z < levels(); z++)
		{
			for(int x = 0; x < width; x++)
			{
				delete[] terrain[z][x];
				delete[] guardingCreaturePositions[z][x];
			}
			delete[] terrain[z];
			delete[] guardingCreaturePositions[z];
		}
		delete [] terrain;
		delete [] guardingCreaturePositions;
	}

	for(auto obj : objects)
		obj.dellNull();

	for(auto quest : quests)
		quest.dellNull();

	resetStaticData();
}

void CMap::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	const int zVal = obj->pos.z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->pos.x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->pos.y - fy;
			if(xVal>=0 && xVal < width && yVal>=0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(total || obj->visitableAt(xVal, yVal))
				{
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(total || obj->blockingAt(xVal, yVal))
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
	const int zVal = obj->pos.z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->pos.x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->pos.y - fy;
			if(xVal>=0 && xVal < width && yVal >= 0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(obj->visitableAt(xVal, yVal))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(obj->blockingAt(xVal, yVal))
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

CGHeroInstance * CMap::getHero(int heroID)
{
	for(auto & elem : heroesOnMap)
		if(elem->subID == heroID)
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

	for (auto & dir : dirs)
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

bool CMap::isInTheMap(const int3 & pos) const
{
	if(pos.x < 0 || pos.y < 0 || pos.z < 0 || pos.x >= width || pos.y >= height
			|| pos.z > (twoLevel ? 1 : 0))
	{
		return false;
	}
	else
	{
		return true;
	}
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

bool CMap::checkForVisitableDir(const int3 & src, const TerrainTile *pom, const int3 & dst ) const
{
	if (!pom->entrableTerrain()) //rock is never accessible
		return false;
	for (auto obj : pom->visitableObjects) //checking destination tile
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
			if(obj->blockVisit)
			{
				if (obj->ID == Obj::MONSTER) // Monster
					return pos;
				else
					return int3(-1, -1, -1); //blockvis objects are not guarded by neighbouring creatures
			}
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

const CGObjectInstance * CMap::getObjectiveObjectFrom(int3 pos, Obj::EObj type)
{
	for (CGObjectInstance * object : getTile(pos).visitableObjects)
	{
		if (object->ID == type)
			return object;
	}
	// There is weird bug because of which sometimes heroes will not be found properly despite having correct position
	// Try to workaround that and find closest object that we can use

	logGlobal->error("Failed to find object of type %d at %s", int(type), pos.toString());
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
				if (object->pos.dist2dSQ(pos) < bestMatch->pos.dist2dSQ(pos))
					bestMatch = object;// closer than one we already found
			}
		}
	}
	assert(bestMatch != nullptr); // if this happens - victory conditions or map itself is very, very broken

	logGlobal->error("Will use %s from %s", bestMatch->getObjectName(), bestMatch->pos.toString());
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
					boost::algorithm::replace_first(event.onFulfill, "%s", VLC->arth->objects[cond.objectType]->getName());
					break;

				case EventCondition::HAVE_CREATURES:
					boost::algorithm::replace_first(event.onFulfill, "%s", VLC->creh->objects[cond.objectType]->nameSing);
					boost::algorithm::replace_first(event.onFulfill, "%d", boost::lexical_cast<std::string>(cond.value));
					break;

				case EventCondition::HAVE_RESOURCES:
					boost::algorithm::replace_first(event.onFulfill, "%s", VLC->generaltexth->restypes[cond.objectType]);
					boost::algorithm::replace_first(event.onFulfill, "%d", boost::lexical_cast<std::string>(cond.value));
					break;

				case EventCondition::HAVE_BUILDING:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, Obj::TOWN);
					break;

				case EventCondition::CONTROL:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, Obj::EObj(cond.objectType));

					if (cond.object)
					{
						const CGTownInstance *town = dynamic_cast<const CGTownInstance*>(cond.object);
						if (town)
							boost::algorithm::replace_first(event.onFulfill, "%s", town->name);
						const CGHeroInstance *hero = dynamic_cast<const CGHeroInstance*>(cond.object);
						if (hero)
							boost::algorithm::replace_first(event.onFulfill, "%s", hero->name);
					}
					break;

				case EventCondition::DESTROY:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, Obj::EObj(cond.objectType));

					if (cond.object)
					{
						const CGHeroInstance *hero = dynamic_cast<const CGHeroInstance*>(cond.object);
						if (hero)
							boost::algorithm::replace_first(event.onFulfill, "%s", hero->name);
					}
					break;
				case EventCondition::TRANSPORT:
					cond.object = getObjectiveObjectFrom(cond.position, Obj::TOWN);
					break;
				//break; case EventCondition::DAYS_PASSED:
				//break; case EventCondition::IS_HUMAN:
				//break; case EventCondition::DAYS_WITHOUT_TOWN:
				//break; case EventCondition::STANDARD_WIN:

				//TODO: support new condition format
				case EventCondition::HAVE_0:
					break;
				case EventCondition::DESTROY_0:
					break;
				case EventCondition::HAVE_BUILDING_0:
					break;
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}
}

void CMap::addNewArtifactInstance(CArtifactInstance * art)
{
	art->id = ArtifactInstanceID((si32)artInstances.size());
	artInstances.push_back(art);
}

void CMap::eraseArtifactInstance(CArtifactInstance * art)
{
	//TODO: handle for artifacts removed in map editor
	assert(artInstances[art->id.getNum()] == art);
	artInstances[art->id.getNum()].dellNull();
}

void CMap::addNewQuestInstance(CQuest* quest)
{
	quest->qid = static_cast<si32>(quests.size());
	quests.push_back(quest);
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
	fmt % obj->typeName % uid;
	obj->instanceName = fmt.str();
}

void CMap::addNewObject(CGObjectInstance * obj)
{
	if(obj->id != ObjectInstanceID((si32)objects.size()))
		throw std::runtime_error("Invalid object instance id");

	if(obj->instanceName.empty())
		throw std::runtime_error("Object instance name missing");

	if (vstd::contains(instanceNames, obj->instanceName))
		throw std::runtime_error("Object instance name duplicated: "+obj->instanceName);

	objects.push_back(obj);
	instanceNames[obj->instanceName] = obj;
	addBlockVisTiles(obj);

	//TODO: how about defeated heroes recruited again?

	obj->afterAddToMap(this);
}

void CMap::moveObject(CGObjectInstance * obj, const int3 & pos)
{
	removeBlockVisTiles(obj);
	obj->pos = pos;
	addBlockVisTiles(obj);
}

void CMap::removeObject(CGObjectInstance * obj)
{
	removeBlockVisTiles(obj);
	instanceNames.erase(obj->instanceName);

	//update indeces
	auto iter = std::next(objects.begin(), obj->id.getNum());
	iter = objects.erase(iter);
	for(int i = obj->id.getNum(); iter != objects.end(); ++i, ++iter)
	{
		(*iter)->id = ObjectInstanceID(i);
	}
	
	obj->afterRemoveFromMap(this);

	//TOOD: Clean artifact instances (mostly worn by hero?) and quests related to this object
}

void CMap::initTerrain()
{
	int level = levels();
	terrain = new TerrainTile**[level];
	guardingCreaturePositions = new int3**[level];
	for(int z = 0; z < level; ++z)
	{
		terrain[z] = new TerrainTile*[width];
		guardingCreaturePositions[z] = new int3*[width];
		for(int x = 0; x < width; ++x)
		{
			terrain[z][x] = new TerrainTile[height];
			guardingCreaturePositions[z][x] = new int3[height];
		}
	}
}

CMapEditManager * CMap::getEditManager()
{
	if(!editManager) editManager = std::make_unique<CMapEditManager>(this);
	return editManager.get();
}

void CMap::resetStaticData()
{
	CGKeys::reset();
	CGMagi::reset();
	CGObelisk::reset();
	CGTownInstance::reset();
}

VCMI_LIB_NAMESPACE_END

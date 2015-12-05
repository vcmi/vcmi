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
#include "CMapEditManager.h"

SHeroName::SHeroName() : heroId(-1)
{

}

PlayerInfo::PlayerInfo(): canHumanPlay(false), canComputerPlay(false),
	aiTactic(EAiTactic::RANDOM), isFactionRandom(false), mainCustomHeroPortrait(-1), mainCustomHeroId(-1), hasMainTown(false),
	generateHeroAtMainTown(false), team(255), hasRandomHero(false), /* following are unused */ generateHero(false), p7(0), powerPlaceholders(-1)
{
	allowedFactions = VLC->townh->getAllowedFactions();
}

si8 PlayerInfo::defaultCastle() const
{
	if(allowedFactions.size() == 1 || !isFactionRandom)
	{
		// faction can't be chosen - set to first that is marked as allowed
		assert(!allowedFactions.empty());
		return *allowedFactions.begin();
	}

	// set to random
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
	value(-1),
	objectType(-1),
	position(-1, -1, -1),
	condition(condition)
{
}

EventCondition::EventCondition(EWinLoseType condition, si32 value, si32 objectType, int3 position):
	object(nullptr),
	value(value),
	objectType(objectType),
	position(position),
	condition(condition)
{}

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

TerrainTile::TerrainTile() : terType(ETerrainType::BORDER), terView(0), riverType(ERiverType::NO_RIVER),
	riverDir(0), roadType(ERoadType::NO_ROAD), roadDir(0), extTileFlags(0), visitable(false),
	blocked(false)
{

}

bool TerrainTile::entrableTerrain(const TerrainTile * from /*= nullptr*/) const
{
	return entrableTerrain(from ? from->terType != ETerrainType::WATER : true, from ? from->terType == ETerrainType::WATER : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
	return terType != ETerrainType::ROCK
			&& ((allowSea && terType == ETerrainType::WATER)  ||  (allowLand && terType != ETerrainType::WATER));
}

bool TerrainTile::isClear(const TerrainTile *from /*= nullptr*/) const
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

bool TerrainTile::isCoastal() const
{
	return extTileFlags & 64;
}

EDiggingStatus TerrainTile::getDiggingStatus(const bool excludeTop) const
{
	if(terType == ETerrainType::WATER || terType == ETerrainType::ROCK)
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
	return terType == ETerrainType::WATER;
}

const int CMapHeader::MAP_SIZE_SMALL = 36;
const int CMapHeader::MAP_SIZE_MIDDLE = 72;
const int CMapHeader::MAP_SIZE_LARGE = 108;
const int CMapHeader::MAP_SIZE_XLARGE = 144;

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
	standardVictory.description = ""; // TODO: display in quest window
	standardVictory.onFulfill = VLC->generaltexth->allTexts[659];
	standardVictory.trigger = EventExpression(victoryCondition);

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage = VLC->generaltexth->allTexts[8];
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description = ""; // TODO: display in quest window
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

CMap::CMap() : checksum(0), grailPos(-1, -1, -1), grailRadious(0), terrain(nullptr)
{
	allHeroes.resize(allowedHeroes.size());
	allowedAbilities = VLC->heroh->getDefaultAllowedAbilities();
	allowedArtifact = VLC->arth->getDefaultAllowed();
	allowedSpell = VLC->spellh->getDefaultAllowed();
}

CMap::~CMap()
{
	if(terrain)
	{
		for (int i=0; i<width; i++)
		{
			for(int j=0; j<height; j++)
			{
				delete [] terrain[i][j];
				delete [] guardingCreaturePositions[i][j];
			}
			delete [] terrain[i];
			delete [] guardingCreaturePositions[i];
		}
		delete [] terrain;
		delete [] guardingCreaturePositions;
	}
}

void CMap::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	for(int fx=0; fx<obj->getWidth(); ++fx)
	{
		for(int fy=0; fy<obj->getHeight(); ++fy)
		{
			int xVal = obj->pos.x - fx;
			int yVal = obj->pos.y - fy;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
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
	for(int fx=0; fx<obj->getWidth(); ++fx)
	{
		for(int fy=0; fy<obj->getHeight(); ++fy)
		{
			int xVal = obj->pos.x - fx;
			int yVal = obj->pos.y - fy;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if( obj->visitableAt(xVal, yVal))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if( obj->blockingAt(xVal, yVal))
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
	for (int i=0; i<width; i++)
	{
		for(int j=0; j<height; j++)
		{
			for (int k = 0; k < levels; k++)
				guardingCreaturePositions[i][j][k] = guardingCreaturePosition(int3(i,j,k));
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
	return terrain[tile.x][tile.y][tile.z];
}

const TerrainTile & CMap::getTile(const int3 & tile) const
{
	assert(isInTheMap(tile));
	return terrain[tile.x][tile.y][tile.z];
}

bool CMap::isWaterTile(const int3 &pos) const
{
	return isInTheMap(pos) && getTile(pos).terType == ETerrainType::WATER;
}

bool CMap::checkForVisitableDir(const int3 & src, const TerrainTile *pom, const int3 & dst ) const
{
	if (!pom->entrableTerrain()) //rock is never accessible
		return false;
	for (auto obj : pom->visitableObjects) //checking destination tile
	{
		if(!vstd::contains(pom->blockingObjects, obj)) //this visitable object is not blocking, ignore
			continue;

		if (!obj->appearance.isVisitableFrom(src.x - dst.x, src.y - dst.y))
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

	logGlobal->errorStream() << "Failed to find object of type " << int(type) << " at " << pos;
	logGlobal->errorStream() << "Will try to find closest matching object";

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

	logGlobal->errorStream() << "Will use " << bestMatch->getObjectName() << " from " << bestMatch->pos;
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
				break; case EventCondition::HAVE_ARTIFACT:
					boost::algorithm::replace_first(event.onFulfill, "%s", VLC->arth->artifacts[cond.objectType]->Name());

				break; case EventCondition::HAVE_CREATURES:
					boost::algorithm::replace_first(event.onFulfill, "%s", VLC->creh->creatures[cond.objectType]->nameSing);
					boost::algorithm::replace_first(event.onFulfill, "%d", boost::lexical_cast<std::string>(cond.value));

				break; case EventCondition::HAVE_RESOURCES:
					boost::algorithm::replace_first(event.onFulfill, "%s", VLC->generaltexth->restypes[cond.objectType]);
					boost::algorithm::replace_first(event.onFulfill, "%d", boost::lexical_cast<std::string>(cond.value));

				break; case EventCondition::HAVE_BUILDING:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, Obj::TOWN);

				break; case EventCondition::CONTROL:
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

				break; case EventCondition::DESTROY:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, Obj::EObj(cond.objectType));

					if (cond.object)
					{
						const CGHeroInstance *hero = dynamic_cast<const CGHeroInstance*>(cond.object);
						if (hero)
							boost::algorithm::replace_first(event.onFulfill, "%s", hero->name);
					}
				break; case EventCondition::TRANSPORT:
					cond.object = getObjectiveObjectFrom(cond.position, Obj::TOWN);
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

void CMap::addNewArtifactInstance(CArtifactInstance * art)
{
	art->id = ArtifactInstanceID(artInstances.size());
	artInstances.push_back(art);
}

void CMap::eraseArtifactInstance(CArtifactInstance * art)
{
	assert(artInstances[art->id.getNum()] == art);
	artInstances[art->id.getNum()].dellNull();
}

void CMap::addQuest(CGObjectInstance * quest)
{
	auto q = dynamic_cast<IQuestObject *>(quest);
	q->quest->qid = quests.size();
	quests.push_back(q->quest);
}

void CMap::initTerrain()
{
	int level = twoLevel ? 2 : 1;
	terrain = new TerrainTile**[width];
	guardingCreaturePositions = new int3**[width];
	for (int i = 0; i < width; ++i)
	{
		terrain[i] = new TerrainTile*[height];
		guardingCreaturePositions[i] = new int3*[height];
		for (int j = 0; j < height; ++j)
		{
			terrain[i][j] = new TerrainTile[level];
			guardingCreaturePositions[i][j] = new int3[level];
		}
	}
}

CMapEditManager * CMap::getEditManager()
{
	if(!editManager) editManager = make_unique<CMapEditManager>(this);
	return editManager.get();
}

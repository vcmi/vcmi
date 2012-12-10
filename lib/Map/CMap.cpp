#include "StdInc.h"
#include "CMap.h"

#include "../CArtHandler.h"
#include "../VCMI_Lib.h"
#include "../CTownHandler.h"
#include "../CHeroHandler.h"
#include "../CDefObjInfoHandler.h"

SHeroName::SHeroName() : heroId(-1)
{

}

PlayerInfo::PlayerInfo(): canHumanPlay(false), canComputerPlay(false),
	aiTactic(EAiTactic::RANDOM), isFactionRandom(false), mainHeroPortrait(-1), hasMainTown(true),
	generateHeroAtMainTown(true), team(255), generateHero(false), p7(0), hasHero(false), customHeroID(-1), powerPlaceholders(-1)
{
	allowedFactions = VLC->townh->getDefaultAllowedFactions();
}

si8 PlayerInfo::defaultCastle() const
{
	if(allowedFactions.size() == 1)
	{
		// only one faction is available - pick it
		return *allowedFactions.begin();
	}

	// set to random
	return -1;
}

si8 PlayerInfo::defaultHero() const
{
	// we will generate hero in front of main town
	if((generateHeroAtMainTown && hasMainTown) || hasHero)
	{
		//random hero
		return -1;
	}

	return -2;
}

LossCondition::LossCondition() : typeOfLossCon(ELossConditionType::LOSSSTANDARD),
	pos(int3(-1, -1, -1)), timeLimit(-1), obj(nullptr)
{

}

VictoryCondition::VictoryCondition() : condition(EVictoryConditionType::WINSTANDARD),
	allowNormalVictory(false), appliesToAI(false), pos(int3(-1, -1, -1)), objectId(0),
	count(0), obj(nullptr)
{

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

TerrainTile::TerrainTile() : terType(ETerrainType::BORDER), terView(0), riverType(ERiverType::NO_RIVER),
	riverDir(0), roadType(ERoadType::NO_ROAD), roadDir(0), extTileFlags(0), visitable(false),
	blocked(false)
{

}

bool TerrainTile::entrableTerrain(const TerrainTile * from /*= NULL*/) const
{
	return entrableTerrain(from ? from->terType != ETerrainType::WATER : true, from ? from->terType == ETerrainType::WATER : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
	return terType != ETerrainType::ROCK
		&& ((allowSea && terType == ETerrainType::WATER)  ||  (allowLand && terType != ETerrainType::WATER));
}

bool TerrainTile::isClear(const TerrainTile *from /*= NULL*/) const
{
	return entrableTerrain(from) && !blocked;
}

int TerrainTile::topVisitableId() const
{
	return visitableObjects.size() ? visitableObjects.back()->ID : -1;
}

bool TerrainTile::isCoastal() const
{
	return extTileFlags & 64;
}

bool TerrainTile::hasFavourableWinds() const
{
	return extTileFlags & 128;
}

bool TerrainTile::isWater() const
{
	return terType == ETerrainType::WATER;
}

CMapHeader::CMapHeader() : version(EMapFormat::SOD), height(72), width(72),
	twoLevel(true), difficulty(1), levelLimit(0), howManyTeams(0), areAnyPlayers(false)
{
	allowedHeroes = VLC->heroh->getDefaultAllowedHeroes();
}

CMapHeader::~CMapHeader()
{

}

CMap::CMap() : checksum(0), terrain(nullptr), grailRadious(0)
{

}

CMap::~CMap()
{
	if(terrain)
	{
		for(int ii=0;ii<width;ii++)
		{
			for(int jj=0;jj<height;jj++)
				delete [] terrain[ii][jj];
			delete [] terrain[ii];
		}
		delete [] terrain;
	}

	for(std::list<ConstTransitivePtr<CMapEvent> >::iterator i = events.begin(); i != events.end(); i++)
	{
		i->dellNull();
	}
}

void CMap::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if(total || ((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(total || !((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
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
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if(((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(!((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
				{
					curt.blockingObjects.push_back(obj);
					curt.blocked = true;
				}
			}
		}
	}
}

CGHeroInstance * CMap::getHero(int heroID)
{
	for(ui32 i=0; i<heroes.size();i++)
		if(heroes[i]->subID == heroID)
			return heroes[i];
	return nullptr;
}

bool CMap::isInTheMap(const int3 &pos) const
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

TerrainTile & CMap::getTile( const int3 & tile )
{
	return terrain[tile.x][tile.y][tile.z];
}

const TerrainTile & CMap::getTile( const int3 & tile ) const
{
	return terrain[tile.x][tile.y][tile.z];
}

bool CMap::isWaterTile(const int3 &pos) const
{
	return isInTheMap(pos) && getTile(pos).terType == ETerrainType::WATER;
}

const CGObjectInstance * CMap::getObjectiveObjectFrom(int3 pos, bool lookForHero)
{
	const std::vector<CGObjectInstance *> & objs = getTile(pos).visitableObjects;
	assert(objs.size());
	if(objs.size() > 1 && lookForHero && objs.front()->ID != Obj::HERO)
	{
		assert(objs.back()->ID == Obj::HERO);
		return objs.back();
	}
	else
		return objs.front();
}

void CMap::checkForObjectives()
{
	if(isInTheMap(victoryCondition.pos))
	{
		victoryCondition.obj = getObjectiveObjectFrom(victoryCondition.pos, victoryCondition.condition == EVictoryConditionType::BEATHERO);
	}

	if(isInTheMap(lossCondition.pos))
	{
		lossCondition.obj = getObjectiveObjectFrom(lossCondition.pos, lossCondition.typeOfLossCon == ELossConditionType::LOSSHERO);
	}
}

void CMap::addNewArtifactInstance(CArtifactInstance * art)
{
	art->id = artInstances.size();
	artInstances.push_back(art);
}

void CMap::eraseArtifactInstance(CArtifactInstance * art)
{
	assert(artInstances[art->id] == art);
	artInstances[art->id].dellNull();
}

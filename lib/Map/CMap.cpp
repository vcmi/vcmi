#include "StdInc.h"
#include "CMap.h"

#include "../CObjectHandler.h"
#include "../CArtHandler.h"
#include "../CDefObjInfoHandler.h"

PlayerInfo::PlayerInfo(): p7(0), p8(0), p9(0), canHumanPlay(0), canComputerPlay(0),
    AITactic(0), isFactionRandom(0),
    mainHeroPortrait(0), hasMainTown(0), generateHeroAtMainTown(0),
    team(255), generateHero(0)
{

}

si8 PlayerInfo::defaultCastle() const
{
    assert(!allowedFactions.empty()); // impossible?

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
    if((generateHeroAtMainTown && hasMainTown) || p8)
    {
        //random hero
        return -1;
    }

    return -2;
}

CMapHeader::CMapHeader() : version(EMapFormat::INVALID)
{
    areAnyPLayers = difficulty = levelLimit = howManyTeams = 0;
    height = width = twoLevel = -1;
}

CMapHeader::~CMapHeader()
{

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

CMap::CMap() : terrain(nullptr)
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

CGHeroInstance * CMap::getHero(int heroID)
{
    for(ui32 i=0; i<heroes.size();i++)
        if(heroes[i]->subID == heroID)
            return heroes[i];
    return nullptr;
}

bool CMap::isInTheMap(const int3 &pos) const
{
    if(pos.x<0 || pos.y<0 || pos.z<0 || pos.x >= width || pos.y >= height || pos.z > twoLevel)
        return false;
    else return true;
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
    return isInTheMap(pos) && getTile(pos).tertype == ETerrainType::WATER;
}

const CGObjectInstance *CMap::getObjectiveObjectFrom(int3 pos, bool lookForHero)
{
    const std::vector <CGObjectInstance *> & objs = getTile(pos).visitableObjects;
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
        victoryCondition.obj = getObjectiveObjectFrom(victoryCondition.pos, victoryCondition.condition == EVictoryConditionType::BEATHERO);

    if(isInTheMap(lossCondition.pos))
        lossCondition.obj = getObjectiveObjectFrom(lossCondition.pos, lossCondition.typeOfLossCon == ELossConditionType::LOSSHERO);
}

void CMap::addNewArtifactInstance( CArtifactInstance *art )
{
    art->id = artInstances.size();
    artInstances.push_back(art);
}

void CMap::eraseArtifactInstance(CArtifactInstance *art)
{
    assert(artInstances[art->id] == art);
    artInstances[art->id].dellNull();
}

LossCondition::LossCondition()
{
    obj = NULL;
    timeLimit = -1;
    pos = int3(-1,-1,-1);
}

VictoryCondition::VictoryCondition()
{
    pos = int3(-1,-1,-1);
    obj = NULL;
    ID = allowNormalVictory = appliesToAI = count = 0;
}

bool TerrainTile::entrableTerrain(const TerrainTile * from /*= NULL*/) const
{
    return entrableTerrain(from ? from->tertype != ETerrainType::WATER : true, from ? from->tertype == ETerrainType::WATER : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
    return tertype != ETerrainType::ROCK
        && ((allowSea && tertype == ETerrainType::WATER)  ||  (allowLand && tertype != ETerrainType::WATER));
}

bool TerrainTile::isClear(const TerrainTile *from /*= NULL*/) const
{
    return entrableTerrain(from) && !blocked;
}

int TerrainTile::topVisitableID() const
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
    return tertype == ETerrainType::WATER;
}

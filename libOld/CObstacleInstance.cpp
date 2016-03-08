#include "StdInc.h"
#include "CObstacleInstance.h"
#include "CHeroHandler.h"
#include "CTownHandler.h"
#include "VCMI_Lib.h"
#include "spells/CSpellHandler.h"

/*
 * CObstacleInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CObstacleInstance::CObstacleInstance()
{
	obstacleType = USUAL;
}

CObstacleInstance::~CObstacleInstance()
{

}

const CObstacleInfo & CObstacleInstance::getInfo() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
		return VLC->heroh->absoluteObstacles[ID];
	case USUAL:
		return VLC->heroh->obstacles[ID];
	case MOAT:
		assert(0);
	default:
		assert(0);
	}
	throw std::runtime_error("Unknown obstacle type in CObstacleInstance::getInfo()");
}

std::vector<BattleHex> CObstacleInstance::getBlockedTiles() const
{
	if(blocksTiles())
		return getAffectedTiles();
	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getStoppingTile() const
{
	if(stopsMovement())
		return getAffectedTiles();
	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getAffectedTiles() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
	case USUAL:
		return getInfo().getBlocked(pos);
	default:
		assert(0);
		return std::vector<BattleHex>();
	}
}

// bool CObstacleInstance::spellGenerated() const
// {
// 	if(obstacleType == USUAL  ||  obstacleType == ABSOLUTE_OBSTACLE)
// 		return false;
//
// 	return true;
// }

bool CObstacleInstance::visibleForSide(ui8 side, bool hasNativeStack) const
{
	//by default obstacle is visible for everyone
	return true;
}

bool CObstacleInstance::stopsMovement() const
{
	return obstacleType == QUICKSAND  ||  obstacleType == MOAT;
}

bool CObstacleInstance::blocksTiles() const
{
	return obstacleType == USUAL  ||  obstacleType == ABSOLUTE_OBSTACLE  ||  obstacleType == FORCE_FIELD;
}

SpellCreatedObstacle::SpellCreatedObstacle()
{
	casterSide = -1;
	spellLevel = -1;
	casterSpellPower = -1;
	turnsRemaining = -1;
	visibleForAnotherSide = -1;
}

bool SpellCreatedObstacle::visibleForSide(ui8 side, bool hasNativeStack) const
{
	switch(obstacleType)
	{
	case FIRE_WALL:
	case FORCE_FIELD:
		//these are always visible
		return true;
	case QUICKSAND:
	case LAND_MINE:
		//we hide mines and not discovered quicksands
		//quicksands are visible to the caster or if owned unit stepped into that particular patch
		//additionally if side has a native unit, mines/quicksands will be visible
		return casterSide == side  ||  visibleForAnotherSide  ||  hasNativeStack;
	default:
		assert(0);
		return false;
	}
}

std::vector<BattleHex> SpellCreatedObstacle::getAffectedTiles() const
{
	switch(obstacleType)
	{
	case QUICKSAND:
	case LAND_MINE:
	case FIRE_WALL:
		return std::vector<BattleHex>(1, pos);
	case FORCE_FIELD:
		return SpellID(SpellID::FORCE_FIELD).toSpell()->rangeInHexes(pos, spellLevel, casterSide);
		//TODO Fire Wall
	default:
		assert(0);
		return std::vector<BattleHex>();
	}
}

void SpellCreatedObstacle::battleTurnPassed()
{
	if(turnsRemaining > 0)
		turnsRemaining--;
}

std::vector<BattleHex> MoatObstacle::getAffectedTiles() const
{
	return VLC->townh->factions[ID]->town->moatHexes;
}

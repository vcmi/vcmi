/*
 * IObjectInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "IObjectInterface.h"

#include "CGTownInstance.h"
#include "MiscObjects.h"

#include "../TerrainHandler.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapping/TerrainTile.h"
#include "../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

void IObjectInterface::showInfoDialog(IGameEventCallback & gameEvents, const ui32 txtID, const ui16 soundID, EInfoWindowMode mode) const
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = getOwner();
	iw.type = mode;
	iw.text.appendLocalString(EMetaText::ADVOB_TXT,txtID);
	gameEvents.sendAndApply(iw);
}

///IObjectInterface
void IObjectInterface::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{}

void IObjectInterface::onHeroLeave(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{}

void IObjectInterface::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{}

void IObjectInterface::initObj(IGameRandomizer & gameRandomizer)
{}

void IObjectInterface::pickRandomObject(IGameRandomizer & gameRandomizer)
{}

void IObjectInterface::setProperty(ObjProperty what, ObjPropertyID identifier)
{}

bool IObjectInterface::wasVisited (PlayerColor player) const
{
	return false;
}
bool IObjectInterface::wasVisited (const CGHeroInstance * h) const
{
	return false;
}

void IObjectInterface::postInit()
{}

void IObjectInterface::preInit()
{}

void IObjectInterface::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{}

void IObjectInterface::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{}

void IObjectInterface::garrisonDialogClosed(IGameEventCallback & gameEvents, const CGHeroInstance *hero) const
{}

void IObjectInterface::heroLevelUpDone(IGameEventCallback & gameEvents, const CGHeroInstance *hero) const
{}

int3 IBoatGenerator::bestLocation() const
{
	std::vector<int3> offsets;
	getOutOffsets(offsets);

	for (auto & offset : offsets)
	{
		int3 targetTile = getObject()->visitablePos() + offset;
		const TerrainTile *tile = getObject()->cb->getTile(targetTile, false);

		if(!tile)
			continue; // tile not visible / outside the map

		if(!tile->isWater())
			continue;

		if (tile->blocked())
			continue;

		if (tile->visitable())
		{
			bool hasBoat = false;
			for (auto const & objectID : tile->visitableObjects)
			{
				const auto * object = getObject()->cb->getObj(objectID);
				if (object->ID == Obj::BOAT || object->ID == Obj::HERO)
					hasBoat = true;
			}

			if (!hasBoat)
				continue; // tile is blocked, but not by boat -> check next potential position
		}
		return targetTile;
	}
	return int3 (-1,-1,-1);
}

IBoatGenerator::EGeneratorState IBoatGenerator::shipyardStatus() const
{
	int3 tile = bestLocation();

	if(!tile.isValid())
		return TILE_BLOCKED; //no available water

	const TerrainTile *t = getObject()->cb->getTile(tile);
	if(!t)
		return TILE_BLOCKED; //no available water

	if(t->blockingObjects.empty())
		return GOOD; //OK

	auto blockerObject = getObject()->cb->getObjInstance(t->blockingObjects.front());

	if(blockerObject->ID == Obj::BOAT || blockerObject->ID == Obj::HERO)
		return BOAT_ALREADY_BUILT; //blocked with boat

	return TILE_BLOCKED; //blocked
}

void IBoatGenerator::getProblemText(MetaString &out, const CGHeroInstance *visitor) const
{
	switch(shipyardStatus())
	{
	case BOAT_ALREADY_BUILT:
		out.appendLocalString(EMetaText::GENERAL_TXT, 51);
		break;
	case TILE_BLOCKED:
		if(visitor)
		{
			out.appendLocalString(EMetaText::GENERAL_TXT, 134);
			out.replaceRawString(visitor->getNameTranslated());
		}
		else
			out.appendLocalString(EMetaText::ADVOB_TXT, 189);
		break;
	case NO_WATER:
		logGlobal->error("Shipyard without water at tile %s! ", getObject()->anchorPos().toString());
		return;
	}
}

void IShipyard::getBoatCost(TResources & cost) const
{
	cost[EGameResID::WOOD] = 10;
	cost[EGameResID::GOLD] = 1000;
}

VCMI_LIB_NAMESPACE_END

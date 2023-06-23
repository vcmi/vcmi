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

#include "../NetPacks.h"
#include "../IGameCallback.h"
#include "../TerrainHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

IGameCallback * IObjectInterface::cb = nullptr;

///helpers
void IObjectInterface::openWindow(const EOpenWindowMode type, const int id1, const int id2)
{
	OpenWindow ow;
	ow.window = type;
	ow.id1 = id1;
	ow.id2 = id2;
	IObjectInterface::cb->sendAndApply(&ow);
}

void IObjectInterface::showInfoDialog(const ui32 txtID, const ui16 soundID, EInfoWindowMode mode) const
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = getOwner();
	iw.type = mode;
	iw.text.appendLocalString(EMetaText::ADVOB_TXT,txtID);
	IObjectInterface::cb->sendAndApply(&iw);
}

///IObjectInterface
void IObjectInterface::onHeroVisit(const CGHeroInstance * h) const
{}

void IObjectInterface::onHeroLeave(const CGHeroInstance * h) const
{}

void IObjectInterface::newTurn(CRandomGenerator & rand) const
{}

void IObjectInterface::initObj(CRandomGenerator & rand)
{}

void IObjectInterface::setProperty( ui8 what, ui32 val )
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

void IObjectInterface::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{}

void IObjectInterface::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{}

void IObjectInterface::garrisonDialogClosed(const CGHeroInstance *hero) const
{}

void IObjectInterface::heroLevelUpDone(const CGHeroInstance *hero) const
{}

int3 IBoatGenerator::bestLocation() const
{
	std::vector<int3> offsets;
	getOutOffsets(offsets);

	for (auto & offset : offsets)
	{
		if(const TerrainTile *tile = getObject()->cb->getTile(getObject()->getPosition() + offset, false)) //tile is in the map
		{
			if(tile->terType->isWater()  &&  (!tile->blocked || tile->blockingObjects.front()->ID == Obj::BOAT)) //and is water and is not blocked or is blocked by boat
				return getObject()->getPosition() + offset;
		}
	}
	return int3 (-1,-1,-1);
}

IBoatGenerator::EGeneratorState IBoatGenerator::shipyardStatus() const
{
	int3 tile = bestLocation();
	const TerrainTile *t = IObjectInterface::cb->getTile(tile);
	if(!t)
		return TILE_BLOCKED; //no available water

	if(t->blockingObjects.empty())
		return GOOD; //OK

	if(t->blockingObjects.front()->ID == Obj::BOAT)
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
		logGlobal->error("Shipyard without water at tile %s! ", getObject()->getPosition().toString());
		return;
	}
}

void IShipyard::getBoatCost(TResources & cost) const
{
	cost[EGameResID::WOOD] = 10;
	cost[EGameResID::GOLD] = 1000;
}

const IShipyard * IShipyard::castFrom( const CGObjectInstance *obj )
{
	return dynamic_cast<const IShipyard *>(obj);
}

VCMI_LIB_NAMESPACE_END

/*
* ShipyardInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ShipyardInstanceConstructor.h"

#include "../mapObjects/MiscObjects.h"
#include "../CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void ShipyardInstanceConstructor::initTypeData(const JsonNode & config)
{
	parameters = config;
}

void ShipyardInstanceConstructor::initializeObject(CGShipyard * shipyard) const
{
	shipyard->createdBoat = BoatId(*VLC->modh->identifiers.getIdentifier("core:boat", parameters["boat"]));
}

VCMI_LIB_NAMESPACE_END

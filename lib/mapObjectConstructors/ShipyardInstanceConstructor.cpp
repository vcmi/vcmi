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
#include "IObjectInfo.h"
#include "../CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void ShipyardInstanceConstructor::initTypeData(const JsonNode & config)
{
	parameters = config;
}

CGObjectInstance * ShipyardInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	CGShipyard * shipyard = new CGShipyard;

	preInitObject(shipyard);

	if(tmpl)
		shipyard->appearance = tmpl;

	shipyard->createdBoat = BoatId(*VLC->modh->identifiers.getIdentifier("core:boat", parameters["boat"]));

	return shipyard;
}

void ShipyardInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
}

std::unique_ptr<IObjectInfo> ShipyardInstanceConstructor::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return nullptr;
}

VCMI_LIB_NAMESPACE_END

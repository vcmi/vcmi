/*
* ShipyardInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"
#include "../json/JsonNode.h"
#include "../mapObjects/MiscObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGShipyard;

class ShipyardInstanceConstructor final : public CDefaultObjectTypeHandler<CGShipyard>
{
	JsonNode parameters;

protected:
	void initTypeData(const JsonNode & config) override;
	void initializeObject(CGShipyard * object) const override;

public:
};

VCMI_LIB_NAMESPACE_END

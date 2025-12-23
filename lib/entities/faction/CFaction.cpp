/*
 * CFaction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CFaction.h"

#include "CTown.h"

#include "../../GameLibrary.h"
#include "../../texts/CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

CFaction::CFaction() = default;
CFaction::~CFaction() = default;

int32_t CFaction::getIndex() const
{
	return index.getNum();
}

int32_t CFaction::getIconIndex() const
{
	return index.getNum(); //???
}

std::string CFaction::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CFaction::getModScope() const
{
	return modScope;
}

void CFaction::registerIcons(const IconRegistar & cb) const
{
	if(town)
	{
		auto & info = town->clientInfo;
		cb(info.icons[0][0], 0, "ITPT", info.iconLarge[0][0]);
		cb(info.icons[0][1], 0, "ITPT", info.iconLarge[0][1]);
		cb(info.icons[1][0], 0, "ITPT", info.iconLarge[1][0]);
		cb(info.icons[1][1], 0, "ITPT", info.iconLarge[1][1]);

		cb(info.icons[0][0] + 2, 0, "ITPA", info.iconSmall[0][0]);
		cb(info.icons[0][1] + 2, 0, "ITPA", info.iconSmall[0][1]);
		cb(info.icons[1][0] + 2, 0, "ITPA", info.iconSmall[1][0]);
		cb(info.icons[1][1] + 2, 0, "ITPA", info.iconSmall[1][1]);

		cb(index.getNum(), 1, "CPRSMALL", info.towerIconSmall);
		cb(index.getNum(), 1, "TWCRPORT", info.towerIconLarge);

	}
}

std::string CFaction::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CFaction::getNameTextID() const
{
	return TextIdentifier("faction", modScope, identifier, "name").get();
}

std::string CFaction::getDescriptionTranslated() const
{
	return LIBRARY->generaltexth->translate(getDescriptionTextID());
}

std::string CFaction::getDescriptionTextID() const
{
	return TextIdentifier("faction", modScope, identifier, "description").get();
}

FactionID CFaction::getId() const
{
	return FactionID(index);
}

FactionID CFaction::getFactionID() const
{
	return FactionID(index);
}

bool CFaction::hasTown() const
{
	return town != nullptr;
}

EAlignment CFaction::getAlignment() const
{
	return alignment;
}

BoatId CFaction::getBoatType() const
{
	return boatType;
}

TerrainId CFaction::getNativeTerrain() const
{
	return nativeTerrain;
}

void CFaction::updateFrom(const JsonNode & data)
{

}

void CFaction::serializeJson(JsonSerializeFormat & handler)
{

}

VCMI_LIB_NAMESPACE_END

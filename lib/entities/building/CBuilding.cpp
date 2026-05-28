/*
 * CBuilding.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBuilding.h"

#include "../../GameLibrary.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../faction/CFaction.h"
#include "../faction/CTown.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::map<std::string, CBuilding::EBuildMode> CBuilding::MODES =
	{
		{ "normal", CBuilding::BUILD_NORMAL },
		{ "auto", CBuilding::BUILD_AUTO },
		{ "special", CBuilding::BUILD_SPECIAL },
		{ "grail", CBuilding::BUILD_GRAIL }
};

BuildingTypeUniqueID CBuilding::getUniqueTypeID() const
{
	return BuildingTypeUniqueID(town->faction->getId(), bid);
}

std::string CBuilding::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CBuilding::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CBuilding::getDescriptionTranslated() const
{
	return LIBRARY->generaltexth->translate(getDescriptionTextID());
}

std::string CBuilding::getBaseTextID() const
{
	return TextIdentifier("building", modScope, town->faction->identifier, identifier).get();
}

std::string CBuilding::getNameTextID() const
{
	return TextIdentifier(getBaseTextID(), "name").get();
}

std::string CBuilding::getDescriptionTextID() const
{
	return TextIdentifier(getBaseTextID(), "description").get();
}

BuildingID CBuilding::getBase() const
{
	const CBuilding * build = this;
	while (build->upgrade != BuildingID::NONE)
	{
		build = build->town->buildings.at(build->upgrade).get();
	}

	return build->bid;
}

si32 CBuilding::getDistance(const BuildingID & buildID) const
{
	const CBuilding * build = town->buildings.at(buildID).get();
	int distance = 0;
	while (build->upgrade != BuildingID::NONE && build != this)
	{
		build = build->town->buildings.at(build->upgrade).get();
		distance++;
	}
	if (build == this)
		return distance;
	return -1;
}

void CBuilding::addNewBonus(const std::shared_ptr<Bonus> & b, BonusList & bonusList) const
{
	bonusList.push_back(b);
}

VCMI_LIB_NAMESPACE_END

/*
* HillFortInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "HillFortInstanceConstructor.h"

#include "../GameLibrary.h"
#include "../modding/LibraryLoadContext.h"
#include "../mapObjects/MiscObjects.h"
#include "../texts/CGeneralTextHandler.h"

void HillFortInstanceConstructor::initTypeData(LibraryLoadContext & context, const JsonNode & config)
{
	parameters = config;
	if(!parameters["unavailableUpgradeMessage"].isNull())
		context.lib.generaltexth->registerString(parameters.getModScope(), TextIdentifier(getBaseTextID(), "unavailableUpgradeMessage"), parameters["unavailableUpgradeMessage"].String());
	
	context.lib.generaltexth->registerString(parameters.getModScope(), TextIdentifier(getBaseTextID(), "description"), parameters["description"].String());
}

void HillFortInstanceConstructor::initializeObject(HillFort * fort) const
{
	fort->upgradeCostPercentage = parameters["upgradeCostFactor"].convertTo<std::vector<int>>();
}

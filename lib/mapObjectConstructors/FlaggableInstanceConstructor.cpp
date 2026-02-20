/*
* FlaggableInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "FlaggableInstanceConstructor.h"

#include "../CConfigHandler.h"
#include "../GameLibrary.h"
#include "../json/JsonBonus.h"
#include "../json/JsonUtils.h"
#include "../texts/CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void FlaggableInstanceConstructor::initTypeData(const JsonNode & config)
{
	if (settings["mods"]["validation"].String() != "off")
		JsonUtils::validate(config, "vcmi:flaggable", getJsonKey());

	for (const auto & bonusJson : config["bonuses"].Struct())
		providedBonuses.push_back(JsonUtils::parseBonus(bonusJson.second));

	if (!config["message"].isNull())
	{
		std::string message = config["message"].String();
		if (!message.empty() && message.at(0) == '@')
		{
			visitMessageTextID = message.substr(1);
		}
		else
		{
			visitMessageTextID = TextIdentifier(getBaseTextID(), "onVisit").get();
			LIBRARY->generaltexth->registerString( config.getModScope(), visitMessageTextID, config["message"]);
		}
	}

	dailyIncome.resolveFromJson(config["dailyIncome"]);
}

void FlaggableInstanceConstructor::initializeObject(FlaggableMapObject * flaggable) const
{
}

const std::string & FlaggableInstanceConstructor::getVisitMessageTextID() const
{
	return visitMessageTextID;
}

const std::vector<std::shared_ptr<Bonus>> & FlaggableInstanceConstructor::getProvidedBonuses() const
{
	return providedBonuses;
}

const ResourceSet & FlaggableInstanceConstructor::getDailyIncome() const
{
	return dailyIncome;
}

VCMI_LIB_NAMESPACE_END

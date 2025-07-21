/*
* MarketInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "MarketInstanceConstructor.h"

#include "../CConfigHandler.h"
#include "../GameLibrary.h"
#include "../constants/StringConstants.h"
#include "../json/JsonRandom.h"
#include "../json/JsonUtils.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

void MarketInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (settings["mods"]["validation"].String() != "off")
		JsonUtils::validate(input, "vcmi:market", getJsonKey());

	if (!input["description"].isNull())
	{
		std::string description = input["description"].String();
		if (!description.empty() && description.at(0) == '@')
		{
			descriptionTextID = description.substr(1);
		}
		else
		{
			descriptionTextID = TextIdentifier(getBaseTextID(), "description").get();
			LIBRARY->generaltexth->registerString( input.getModScope(), descriptionTextID, input["description"]);
		}
	}

	if (!input["speech"].isNull())
	{
		std::string speech = input["speech"].String();
		if (!speech.empty() && speech.at(0) == '@')
		{
			speechTextID = speech.substr(1);
		}
		else
		{
			speechTextID = TextIdentifier(getBaseTextID(), "speech").get();
			LIBRARY->generaltexth->registerString( input.getModScope(), speechTextID, input["speech"]);
		}
	}

	for(auto & element : input["modes"].Vector())
	{
		if(MappedKeys::MARKET_NAMES_TO_TYPES.count(element.String()))
			marketModes.insert(MappedKeys::MARKET_NAMES_TO_TYPES.at(element.String()));
	}

	marketEfficiency = input["efficiency"].isNull() ? 5 : input["efficiency"].Integer();
	predefinedOffer = input["offer"];
}

bool MarketInstanceConstructor::hasDescription() const
{
	return !descriptionTextID.empty();
}

std::string MarketInstanceConstructor::getDescriptionTextID() const
{
	return descriptionTextID;
}

std::shared_ptr<CGMarket> MarketInstanceConstructor::createObject(IGameInfoCallback * cb) const
{
	if(marketModes.size() == 1)
	{
		switch(*marketModes.begin())
		{
			case EMarketMode::ARTIFACT_RESOURCE:
			case EMarketMode::RESOURCE_ARTIFACT:
				return std::make_shared<CGBlackMarket>(cb);

			case EMarketMode::RESOURCE_SKILL:
				return std::make_shared<CGUniversity>(cb);
		}
	}
	return std::make_shared<CGMarket>(cb);
}

const std::set<EMarketMode> & MarketInstanceConstructor::availableModes() const
{
	return marketModes;
}

void MarketInstanceConstructor::randomizeObject(CGMarket * object, IGameRandomizer & gameRandomizer) const
{
	JsonRandom randomizer(object->cb, gameRandomizer);
	JsonRandom::Variables emptyVariables;

	if(auto * university = dynamic_cast<CGUniversity *>(object))
	{
		for(auto skill : randomizer.loadSecondaries(predefinedOffer, emptyVariables))
			university->skills.push_back(skill.first);
	}
}

std::string MarketInstanceConstructor::getSpeechTranslated() const
{
	assert(marketModes.count(EMarketMode::RESOURCE_SKILL));
	return LIBRARY->generaltexth->translate(speechTextID);
}

int MarketInstanceConstructor::getMarketEfficiency() const
{
	return marketEfficiency;
}

VCMI_LIB_NAMESPACE_END

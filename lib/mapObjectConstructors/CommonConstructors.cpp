/*
* CommonConstructors.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "CommonConstructors.h"

#include "../texts/CGeneralTextHandler.h"
#include "../json/JsonRandom.h"
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../GameLibrary.h"

#include "../CConfigHandler.h"
#include "../callback/IGameInfoCallback.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/hero/CHeroClass.h"
#include "../json/JsonUtils.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGMarket.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapping/TerrainTile.h"
#include "../modding/IdentifierStorage.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CObstacleConstructor::isStaticObject()
{
	return true;
}

bool CreatureInstanceConstructor::hasNameTextID() const
{
	return true;
}

std::string CreatureInstanceConstructor::getNameTextID() const
{
	return LIBRARY->creatures()->getByIndex(getSubIndex())->getNamePluralTextID();
}

void ResourceInstanceConstructor::initTypeData(const JsonNode & input)
{
	config = input;

	resourceType = GameResID::GOLD; //set up fallback
	LIBRARY->identifiers()->requestIdentifierIfNotNull("resource", input["resource"], [&](si32 index)
	{
		resourceType = GameResID(index);
	});
}

bool ResourceInstanceConstructor::hasNameTextID() const
{
	return true;
}

std::string ResourceInstanceConstructor::getNameTextID() const
{
	return TextIdentifier("core", "restypes", resourceType.getNum()).get();
}

GameResID ResourceInstanceConstructor::getResourceType() const
{
	return resourceType;
}

int ResourceInstanceConstructor::getAmountMultiplier() const
{
	if (config["amountMultiplier"].isNull())
		return 1;
	return config["amountMultiplier"].Integer();
}

void ResourceInstanceConstructor::randomizeObject(CGResource * object, IGameRandomizer & gameRandomizer) const
{
	if (object->amount != CGResource::RANDOM_AMOUNT)
		return;

	JsonRandom randomizer(object->cb, gameRandomizer);
	JsonRandom::Variables dummy;

	if (!config["amounts"].isNull())
		object->amount = randomizer.loadValue(config["amounts"], dummy, 0) * getAmountMultiplier();
	else
		object->amount = 5 * getAmountMultiplier();
}

void CTownInstanceConstructor::initTypeData(const JsonNode & input)
{
	LIBRARY->identifiers()->requestIdentifier("faction", input["faction"], [&](si32 index)
	{
		faction = (*LIBRARY->townh)[index];
	});

	filtersJson = input["filters"];

	// change scope of "filters" to scope of object that is being loaded
	// since this filters require to resolve building ID's
	filtersJson.setModScope(input["faction"].getModScope());
}

void CTownInstanceConstructor::afterLoadFinalization()
{
	assert(faction);
	for(const auto & entry : filtersJson.Struct())
	{
		filters[entry.first] = LogicalExpression<BuildingID>(entry.second, [this](const JsonNode & node)
		{
			return BuildingID(LIBRARY->identifiers()->getIdentifier("building." + faction->getJsonKey(), node.Vector()[0]).value_or(-1));
		});
	}
}

bool CTownInstanceConstructor::objectFilter(const CGObjectInstance * object, std::shared_ptr<const ObjectTemplate> templ) const
{
	const auto * town = dynamic_cast<const CGTownInstance *>(object);

	auto buildTest = [&](const BuildingID & id)
	{
		return town->hasBuilt(id);
	};

	return filters.count(templ->stringID) != 0 && filters.at(templ->stringID).test(buildTest);
}

void CTownInstanceConstructor::initializeObject(CGTownInstance * obj) const
{
	obj->tempOwner = PlayerColor::NEUTRAL;
}

void CTownInstanceConstructor::randomizeObject(CGTownInstance * object, IGameRandomizer & gameRandomizer) const
{
	auto templ = getOverride(object->cb->getTile(object->pos)->getTerrainID(), object);
	if(templ)
		object->appearance = templ;
}

bool CTownInstanceConstructor::hasNameTextID() const
{
	return true;
}

std::string CTownInstanceConstructor::getNameTextID() const
{
	return faction->getNameTextID();
}

void CHeroInstanceConstructor::initTypeData(const JsonNode & input)
{
	LIBRARY->identifiers()->requestIdentifier(
		"heroClass",
		input["heroClass"],
		[&](si32 index) { heroClass = HeroClassID(index).toHeroClass(); });

	for (const auto & [name, config] : input["filters"].Struct())
	{
		HeroFilter filter;
		filter.allowFemale =  config["female"].Bool();
		filter.allowMale =  config["male"].Bool();
		filters[name] = filter;

		if (!config["hero"].isNull())
		{
			LIBRARY->identifiers()->requestIdentifier( "hero", config["hero"], [this, templateName = name](si32 index) {
				filters.at(templateName).fixedHero = HeroTypeID(index);
			});
		}
	}
}

std::shared_ptr<const ObjectTemplate> CHeroInstanceConstructor::getOverride(TerrainId terrainType, const CGObjectInstance * object) const
{
	const auto * hero = dynamic_cast<const CGHeroInstance *>(object);

	std::vector<std::shared_ptr<const ObjectTemplate>> allTemplates = getTemplates();
	std::shared_ptr<const ObjectTemplate> candidateFullMatch;
	std::shared_ptr<const ObjectTemplate> candidateGenderMatch;
	std::shared_ptr<const ObjectTemplate> candidateBase;

	assert(hero->gender != EHeroGender::DEFAULT);

	for (const auto & templ : allTemplates)
	{
		if (filters.count(templ->stringID))
		{
			const auto & filter = filters.at(templ->stringID);
			if (filter.fixedHero.hasValue())
			{
				if (filter.fixedHero == hero->getHeroTypeID())
					candidateFullMatch = templ;
			}
			else if (filter.allowMale)
			{
				if (hero->gender == EHeroGender::MALE)
					candidateGenderMatch = templ;
			}
			else if (filter.allowFemale)
			{
				if (hero->gender == EHeroGender::FEMALE)
					candidateGenderMatch = templ;
			}
			else
			{
				candidateBase = templ;
			}
		}
		else
		{
			candidateBase = templ;
		}
	}

	if (candidateFullMatch)
		return candidateFullMatch;

	if (candidateGenderMatch)
		return candidateGenderMatch;

	return candidateBase;
}

void CHeroInstanceConstructor::randomizeObject(CGHeroInstance * object, IGameRandomizer & gameRandomizer) const
{

}

bool CHeroInstanceConstructor::hasNameTextID() const
{
	return true;
}

std::string CHeroInstanceConstructor::getNameTextID() const
{
	return heroClass->getNameTextID();
}

void BoatInstanceConstructor::initTypeData(const JsonNode & input)
{
	layer = EPathfindingLayer::SAIL;
	int pos = vstd::find_pos(NPathfindingLayer::names, input["layer"].String());
	if(pos != -1)
		layer = EPathfindingLayer(pos);
	else
		logMod->error("Unknown layer %s found in boat!", input["layer"].String());

	onboardAssaultAllowed = input["onboardAssaultAllowed"].Bool();
	onboardVisitAllowed = input["onboardVisitAllowed"].Bool();
	actualAnimation = AnimationPath::fromJson(input["actualAnimation"]);
	overlayAnimation = AnimationPath::fromJson(input["overlayAnimation"]);
	for(int i = 0; i < flagAnimations.size() && i < input["flagAnimations"].Vector().size(); ++i)
		flagAnimations[i] = AnimationPath::fromJson(input["flagAnimations"].Vector()[i]);
	bonuses = JsonRandom::loadBonuses(input["bonuses"]);
}

void BoatInstanceConstructor::initializeObject(CGBoat * boat) const
{
	boat->layer = layer;
	boat->actualAnimation = actualAnimation;
	boat->overlayAnimation = overlayAnimation;
	boat->flagAnimations = flagAnimations;
	boat->onboardAssaultAllowed = onboardAssaultAllowed;
	boat->onboardVisitAllowed = onboardVisitAllowed;
	for(auto & b : bonuses)
		boat->addNewBonus(b);
}

AnimationPath BoatInstanceConstructor::getBoatAnimationName() const
{
	return actualAnimation;
}

void MarketInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (settings["mods"]["validation"].String() != "off")
		JsonUtils::validate(input, "vcmi:market", getJsonKey());

	if (!input["description"].isNull())
	{
		std::string description = input["description"].String();
		descriptionTextID = TextIdentifier(getBaseTextID(), "description").get();
		LIBRARY->generaltexth->registerString( input.getModScope(), descriptionTextID, input["description"]);
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

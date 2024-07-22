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
#include "../CHeroHandler.h"
#include "../IGameCallback.h"
#include "../json/JsonRandom.h"
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../VCMI_Lib.h"

#include "../entities/faction/CTownHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGMarket.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/ObjectTemplate.h"

#include "../modding/IdentifierStorage.h"

#include "../mapping/CMapDefines.h"

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
	return VLC->creatures()->getByIndex(getSubIndex())->getNamePluralTextID();
}

bool ResourceInstanceConstructor::hasNameTextID() const
{
	return true;
}

std::string ResourceInstanceConstructor::getNameTextID() const
{
	return TextIdentifier("core", "restypes", getSubIndex()).get();
}

void CTownInstanceConstructor::initTypeData(const JsonNode & input)
{
	VLC->identifiers()->requestIdentifier("faction", input["faction"], [&](si32 index)
	{
		faction = (*VLC->townh)[index];
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
			return BuildingID(VLC->identifiers()->getIdentifier("building." + faction->getJsonKey(), node.Vector()[0]).value_or(-1));
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
	obj->town = faction->town;
	obj->tempOwner = PlayerColor::NEUTRAL;
}

void CTownInstanceConstructor::randomizeObject(CGTownInstance * object, vstd::RNG & rng) const
{
	auto templ = getOverride(object->cb->getTile(object->pos)->terType->getId(), object);
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
	VLC->identifiers()->requestIdentifier(
		"heroClass",
		input["heroClass"],
		[&](si32 index) { heroClass = HeroClassID(index).toHeroClass(); });

	filtersJson = input["filters"];
}

void CHeroInstanceConstructor::afterLoadFinalization()
{
	for(const auto & entry : filtersJson.Struct())
	{
		filters[entry.first] = LogicalExpression<HeroTypeID>(entry.second, [](const JsonNode & node)
		{
			return HeroTypeID(VLC->identifiers()->getIdentifier("hero", node.Vector()[0]).value_or(-1));
		});
	}
}

bool CHeroInstanceConstructor::objectFilter(const CGObjectInstance * object, std::shared_ptr<const ObjectTemplate> templ) const
{
	const auto * hero = dynamic_cast<const CGHeroInstance *>(object);

	auto heroTest = [&](const HeroTypeID & id)
	{
		return hero->type->getId() == id;
	};

	if(filters.count(templ->stringID))
	{
		return filters.at(templ->stringID).test(heroTest);
	}
	return false;
}

void CHeroInstanceConstructor::initializeObject(CGHeroInstance * obj) const
{
	obj->type = nullptr; //FIXME: set to valid value. somehow.
}

void CHeroInstanceConstructor::randomizeObject(CGHeroInstance * object, vstd::RNG & rng) const
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
		boat->addNewBonus(std::make_shared<Bonus>(b));
}

AnimationPath BoatInstanceConstructor::getBoatAnimationName() const
{
	return actualAnimation;
}

void MarketInstanceConstructor::initTypeData(const JsonNode & input)
{
	for(auto & element : input["modes"].Vector())
	{
		if(MappedKeys::MARKET_NAMES_TO_TYPES.count(element.String()))
			marketModes.insert(MappedKeys::MARKET_NAMES_TO_TYPES.at(element.String()));
	}
	
	marketEfficiency = input["efficiency"].isNull() ? 5 : input["efficiency"].Integer();
	predefinedOffer = input["offer"];
	
	title = input["title"].String();
	speech = input["speech"].String();
}

CGMarket * MarketInstanceConstructor::createObject(IGameCallback * cb) const
{
	if(marketModes.size() == 1)
	{
		switch(*marketModes.begin())
		{
			case EMarketMode::ARTIFACT_RESOURCE:
			case EMarketMode::RESOURCE_ARTIFACT:
				return new CGBlackMarket(cb);

			case EMarketMode::RESOURCE_SKILL:
				return new CGUniversity(cb);
		}
	}
	else if(marketModes.size() == 2)
	{
		if(vstd::contains(marketModes, EMarketMode::ARTIFACT_EXP))
			return new CGArtifactsAltar(cb);
	}
	return new CGMarket(cb);
}

void MarketInstanceConstructor::initializeObject(CGMarket * market) const
{
	market->marketModes = marketModes;
	market->marketEfficiency = marketEfficiency;
	
	market->title = market->getObjectName();
	if(!title.empty())
		market->title = VLC->generaltexth->translate(title);
	
	if (!speech.empty())
		market->speech = VLC->generaltexth->translate(speech);
}

void MarketInstanceConstructor::randomizeObject(CGMarket * object, vstd::RNG & rng) const
{
	JsonRandom randomizer(object->cb);
	JsonRandom::Variables emptyVariables;

	if(auto * university = dynamic_cast<CGUniversity *>(object))
	{
		for(auto skill : randomizer.loadSecondaries(predefinedOffer, rng, emptyVariables))
			university->skills.push_back(skill.first);
	}
}

VCMI_LIB_NAMESPACE_END

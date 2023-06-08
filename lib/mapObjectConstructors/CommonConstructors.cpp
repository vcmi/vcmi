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

#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CModHandler.h"
#include "../CTownHandler.h"
#include "../IGameCallback.h"
#include "../JsonRandom.h"
#include "../StringConstants.h"
#include "../TerrainHandler.h"
#include "../VCMI_Lib.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGMarket.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/ObjectTemplate.h"

#include "../mapping/CMapDefines.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CObstacleConstructor::isStaticObject()
{
	return true;
}

void CTownInstanceConstructor::initTypeData(const JsonNode & input)
{
	VLC->modh->identifiers.requestIdentifier("faction", input["faction"], [&](si32 index)
	{
		faction = (*VLC->townh)[index];
	});

	filtersJson = input["filters"];

	// change scope of "filters" to scope of object that is being loaded
	// since this filters require to resolve building ID's
	filtersJson.setMeta(input["faction"].meta);
}

void CTownInstanceConstructor::afterLoadFinalization()
{
	assert(faction);
	for(const auto & entry : filtersJson.Struct())
	{
		filters[entry.first] = LogicalExpression<BuildingID>(entry.second, [this](const JsonNode & node)
		{
			return BuildingID(VLC->modh->identifiers.getIdentifier("building." + faction->getJsonKey(), node.Vector()[0]).value());
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

void CTownInstanceConstructor::randomizeObject(CGTownInstance * object, CRandomGenerator & rng) const
{
	auto templ = getOverride(CGObjectInstance::cb->getTile(object->pos)->terType->getId(), object);
	if(templ)
		object->appearance = templ;
}

void CHeroInstanceConstructor::initTypeData(const JsonNode & input)
{
	VLC->modh->identifiers.requestIdentifier(
		"heroClass",
		input["heroClass"],
		[&](si32 index) { heroClass = VLC->heroh->classes[index]; });

	filtersJson = input["filters"];
}

void CHeroInstanceConstructor::afterLoadFinalization()
{
	for(const auto & entry : filtersJson.Struct())
	{
		filters[entry.first] = LogicalExpression<HeroTypeID>(entry.second, [](const JsonNode & node)
		{
			return HeroTypeID(VLC->modh->identifiers.getIdentifier("hero", node.Vector()[0]).value());
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

void CHeroInstanceConstructor::randomizeObject(CGHeroInstance * object, CRandomGenerator & rng) const
{

}

void BoatInstanceConstructor::initTypeData(const JsonNode & input)
{
	layer = EPathfindingLayer::SAIL;
	int pos = vstd::find_pos(NPathfindingLayer::names, input["layer"].String());
	if(pos != -1)
		layer = EPathfindingLayer(pos);
	onboardAssaultAllowed = input["onboardAssaultAllowed"].Bool();
	onboardVisitAllowed = input["onboardVisitAllowed"].Bool();
	actualAnimation = input["actualAnimation"].String();
	overlayAnimation = input["overlayAnimation"].String();
	for(int i = 0; i < flagAnimations.size() && i < input["flagAnimations"].Vector().size(); ++i)
		flagAnimations[i] = input["flagAnimations"].Vector()[i].String();
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

std::string BoatInstanceConstructor::getBoatAnimationName() const
{
	return actualAnimation;
}

void BoatInstanceConstructor::afterLoadFinalization()
{
	if (layer == EPathfindingLayer::SAIL)
	{
		if (getTemplates(TerrainId(ETerrainId::WATER)).empty())
			logMod->warn("Boat of type %s has no templates suitable for water!", getJsonKey());
	}
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

CGMarket * MarketInstanceConstructor::createObject() const
{
	if(marketModes.size() == 1)
	{
		switch(*marketModes.begin())
		{
			case EMarketMode::ARTIFACT_RESOURCE:
			case EMarketMode::RESOURCE_ARTIFACT:
				return new CGBlackMarket;

			case EMarketMode::RESOURCE_SKILL:
				return new CGUniversity;
		}
	}
	return new CGMarket;
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

void MarketInstanceConstructor::randomizeObject(CGMarket * object, CRandomGenerator & rng) const
{
	if(auto * university = dynamic_cast<CGUniversity *>(object))
	{
		for(auto skill : JsonRandom::loadSecondary(predefinedOffer, rng))
			university->skills.push_back(skill.first.getNum());
	}
}

VCMI_LIB_NAMESPACE_END

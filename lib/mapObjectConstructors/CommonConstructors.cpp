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

#include "../CCreatureHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CModHandler.h"
#include "../IGameCallback.h"
#include "../StringConstants.h"
#include "../TerrainHandler.h"
#include "../mapObjects/CBank.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapping/CMapDefines.h"
#include "../JsonRandom.h"

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

CGObjectInstance * CTownInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	CGTownInstance * obj = createTyped(tmpl);
	obj->town = faction->town;
	obj->tempOwner = PlayerColor::NEUTRAL;
	return obj;
}

void CTownInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
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

CGObjectInstance * CHeroInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	CGHeroInstance * obj = createTyped(tmpl);
	obj->type = nullptr; //FIXME: set to valid value. somehow.
	return obj;
}

void CHeroInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{

}

bool CDwellingInstanceConstructor::hasNameTextID() const
{
	return true;
}

void CDwellingInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (input.Struct().count("name") == 0)
		logMod->warn("Dwelling %s missing name!", getJsonKey());

	VLC->generaltexth->registerString( input.meta, getNameTextID(), input["name"].String());

	const JsonVector & levels = input["creatures"].Vector();
	const auto totalLevels = levels.size();

	availableCreatures.resize(totalLevels);
	for(auto currentLevel = 0; currentLevel < totalLevels; currentLevel++)
	{
		const JsonVector & creaturesOnLevel = levels[currentLevel].Vector();
		const auto creaturesNumber = creaturesOnLevel.size();
		availableCreatures[currentLevel].resize(creaturesNumber);

		for(auto currentCreature = 0; currentCreature < creaturesNumber; currentCreature++)
		{
			VLC->modh->identifiers.requestIdentifier("creature", creaturesOnLevel[currentCreature], [=] (si32 index)
			{
				availableCreatures[currentLevel][currentCreature] = VLC->creh->objects[index];
			});
		}
		assert(!availableCreatures[currentLevel].empty());
	}
	guards = input["guards"];
}

bool CDwellingInstanceConstructor::objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return false;
}

CGObjectInstance * CDwellingInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	CGDwelling * obj = createTyped(tmpl);

	obj->creatures.resize(availableCreatures.size());
	for(const auto & entry : availableCreatures)
	{
		for(const CCreature * cre : entry)
			obj->creatures.back().second.push_back(cre->getId());
	}
	return obj;
}

void CDwellingInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator &rng) const
{
	auto * dwelling = dynamic_cast<CGDwelling *>(object);

	dwelling->creatures.clear();
	dwelling->creatures.reserve(availableCreatures.size());

	for(const auto & entry : availableCreatures)
	{
		dwelling->creatures.resize(dwelling->creatures.size() + 1);
		for(const CCreature * cre : entry)
			dwelling->creatures.back().second.push_back(cre->getId());
	}

	bool guarded = false; //TODO: serialize for sanity

	if(guards.getType() == JsonNode::JsonType::DATA_BOOL) //simple switch
	{
		if(guards.Bool())
		{
			guarded = true;
		}
	}
	else if(guards.getType() == JsonNode::JsonType::DATA_VECTOR) //custom guards (eg. Elemental Conflux)
	{
		for(auto & stack : JsonRandom::loadCreatures(guards, rng))
		{
			dwelling->putStack(SlotID(dwelling->stacksCount()), new CStackInstance(stack.type->getId(), stack.count));
		}
	}
	else //default condition - creatures are of level 5 or higher
	{
		for(auto creatureEntry : availableCreatures)
		{
			if(creatureEntry.at(0)->getLevel() >= 5)
			{
				guarded = true;
				break;
			}
		}
	}

	if(guarded)
	{
		for(auto creatureEntry : availableCreatures)
		{
			const CCreature * crea = creatureEntry.at(0);
			dwelling->putStack(SlotID(dwelling->stacksCount()), new CStackInstance(crea->getId(), crea->getGrowth() * 3));
		}
	}
}

bool CDwellingInstanceConstructor::producesCreature(const CCreature * crea) const
{
	for(const auto & entry : availableCreatures)
	{
		for(const CCreature * cre : entry)
			if(crea == cre)
				return true;
	}
	return false;
}

std::vector<const CCreature *> CDwellingInstanceConstructor::getProducedCreatures() const
{
	std::vector<const CCreature *> creatures; //no idea why it's 2D, to be honest
	for(const auto & entry : availableCreatures)
	{
		for(const CCreature * cre : entry)
			creatures.push_back(cre);
	}
	return creatures;
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

CGObjectInstance * BoatInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	CGBoat * boat = createTyped(tmpl);
	boat->layer = layer;
	boat->actualAnimation = actualAnimation;
	boat->overlayAnimation = overlayAnimation;
	boat->flagAnimations = flagAnimations;
	boat->onboardAssaultAllowed = onboardAssaultAllowed;
	boat->onboardVisitAllowed = onboardVisitAllowed;
	for(auto & b : bonuses)
		boat->addNewBonus(std::make_shared<Bonus>(b));
	
	return boat;
}

std::string BoatInstanceConstructor::getBoatAnimationName() const
{
	return actualAnimation;
}

void BoatInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{

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

CGObjectInstance * MarketInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	CGMarket * market = nullptr;
	if(marketModes.size() == 1)
	{
		switch(*marketModes.begin())
		{
			case EMarketMode::ARTIFACT_RESOURCE:
			case EMarketMode::RESOURCE_ARTIFACT:
				market = new CGBlackMarket;
				break;
				
			case EMarketMode::RESOURCE_SKILL:
				market = new CGUniversity;
				break;
		}
	}
	
	if(!market)
		market = new CGMarket;
	
	preInitObject(market);

	if(tmpl)
		market->appearance = tmpl;
	market->marketModes = marketModes;
	market->marketEfficiency = marketEfficiency;
	
	market->title = market->getObjectName();
	if(!title.empty())
		market->title = VLC->generaltexth->translate(title);
	
	if (!speech.empty())
		market->speech = VLC->generaltexth->translate(speech);
	
	return market;
}

void MarketInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	if(auto * university = dynamic_cast<CGUniversity *>(object))
	{
		for(auto skill : JsonRandom::loadSecondary(predefinedOffer, rng))
			university->skills.push_back(skill.first.getNum());
	}
}

VCMI_LIB_NAMESPACE_END

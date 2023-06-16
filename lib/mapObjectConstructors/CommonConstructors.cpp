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
#include "JsonRandom.h"

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

void BoatInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{

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


bool CBankInstanceConstructor::hasNameTextID() const
{
	return true;
}

void CBankInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (input.Struct().count("name") == 0)
		logMod->warn("Bank %s missing name!", getJsonKey());

	VLC->generaltexth->registerString(input.meta, getNameTextID(), input["name"].String());

	levels = input["levels"].Vector();
	bankResetDuration = static_cast<si32>(input["resetDuration"].Float());
}

CGObjectInstance *CBankInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return createTyped(tmpl);
}

BankConfig CBankInstanceConstructor::generateConfig(const JsonNode & level, CRandomGenerator & rng) const
{
	BankConfig bc;

	bc.chance = static_cast<ui32>(level["chance"].Float());

	bc.guards = JsonRandom::loadCreatures(level["guards"], rng);
	bc.upgradeChance = static_cast<ui32>(level["upgrade_chance"].Float());
	bc.combatValue = static_cast<ui32>(level["combat_value"].Float());

	std::vector<SpellID> spells;
	for (size_t i=0; i<6; i++)
		IObjectInterface::cb->getAllowedSpells(spells, static_cast<ui16>(i));

	bc.resources = ResourceSet(level["reward"]["resources"]);
	bc.creatures = JsonRandom::loadCreatures(level["reward"]["creatures"], rng);
	bc.artifacts = JsonRandom::loadArtifacts(level["reward"]["artifacts"], rng);
	bc.spells = JsonRandom::loadSpells(level["reward"]["spells"], rng, spells);

	bc.value = static_cast<ui32>(level["value"].Float());

	return bc;
}

void CBankInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	auto * bank = dynamic_cast<CBank *>(object);

	bank->resetDuration = bankResetDuration;

	si32 totalChance = 0;
	for(const auto & node : levels)
		totalChance += static_cast<si32>(node["chance"].Float());

	assert(totalChance != 0);

	si32 selectedChance = rng.nextInt(totalChance - 1);

	int cumulativeChance = 0;
	for(const auto & node : levels)
	{
		cumulativeChance += static_cast<int>(node["chance"].Float());
		if(selectedChance < cumulativeChance)
		{
			bank->setConfig(generateConfig(node, rng));
			break;
		}
	}
}

CBankInfo::CBankInfo(const JsonVector & Config) :
	config(Config)
{
	assert(!Config.empty());
}

static void addStackToArmy(IObjectInfo::CArmyStructure & army, const CCreature * crea, si32 amount)
{
	army.totalStrength += crea->getFightValue() * amount;

	bool walker = true;
	if(crea->hasBonusOfType(BonusType::SHOOTER))
	{
		army.shootersStrength += crea->getFightValue() * amount;
		walker = false;
	}
	if(crea->hasBonusOfType(BonusType::FLYING))
	{
		army.flyersStrength += crea->getFightValue() * amount;
		walker = false;
	}
	if(walker)
		army.walkersStrength += crea->getFightValue() * amount;
}

IObjectInfo::CArmyStructure CBankInfo::minGuards() const
{
	std::vector<IObjectInfo::CArmyStructure> armies;
	for(auto configEntry : config)
	{
		auto stacks = JsonRandom::evaluateCreatures(configEntry["guards"]);
		IObjectInfo::CArmyStructure army;
		for(auto & stack : stacks)
		{
			assert(!stack.allowedCreatures.empty());
			auto weakest = boost::range::min_element(stack.allowedCreatures, [](const CCreature * a, const CCreature * b)
			{
				return a->getFightValue() < b->getFightValue();
			});
			addStackToArmy(army, *weakest, stack.minAmount);
		}
		armies.push_back(army);
	}
	return *boost::range::min_element(armies);
}

IObjectInfo::CArmyStructure CBankInfo::maxGuards() const
{
	std::vector<IObjectInfo::CArmyStructure> armies;
	for(auto configEntry : config)
	{
		auto stacks = JsonRandom::evaluateCreatures(configEntry["guards"]);
		IObjectInfo::CArmyStructure army;
		for(auto & stack : stacks)
		{
			assert(!stack.allowedCreatures.empty());
			auto strongest = boost::range::max_element(stack.allowedCreatures, [](const CCreature * a, const CCreature * b)
			{
				return a->getFightValue() < b->getFightValue();
			});
			addStackToArmy(army, *strongest, stack.maxAmount);
		}
		armies.push_back(army);
	}
	return *boost::range::max_element(armies);
}

TPossibleGuards CBankInfo::getPossibleGuards() const
{
	TPossibleGuards out;

	for(const JsonNode & configEntry : config)
	{
		const JsonNode & guardsInfo = configEntry["guards"];
		auto stacks = JsonRandom::evaluateCreatures(guardsInfo);
		IObjectInfo::CArmyStructure army;


		for(auto stack : stacks)
		{
			army.totalStrength += stack.allowedCreatures.front()->getAIValue() * (stack.minAmount + stack.maxAmount) / 2;
			//TODO: add fields for flyers, walkers etc...
		}

		ui8 chance = static_cast<ui8>(configEntry["chance"].Float());
		out.push_back(std::make_pair(chance, army));
	}
	return out;
}

std::vector<PossibleReward<TResources>> CBankInfo::getPossibleResourcesReward() const
{
	std::vector<PossibleReward<TResources>> result;

	for(const JsonNode & configEntry : config)
	{
		const JsonNode & resourcesInfo = configEntry["reward"]["resources"];

		if(!resourcesInfo.isNull())
		{
			result.emplace_back(configEntry["chance"].Integer(), TResources(resourcesInfo));
		}
	}

	return result;
}

std::vector<PossibleReward<CStackBasicDescriptor>> CBankInfo::getPossibleCreaturesReward() const
{
	std::vector<PossibleReward<CStackBasicDescriptor>> aproximateReward;

	for(const JsonNode & configEntry : config)
	{
		const JsonNode & guardsInfo = configEntry["reward"]["creatures"];
		auto stacks = JsonRandom::evaluateCreatures(guardsInfo);

		for(auto stack : stacks)
		{
			const auto * creature = stack.allowedCreatures.front();

			aproximateReward.emplace_back(configEntry["chance"].Integer(), CStackBasicDescriptor(creature, (stack.minAmount + stack.maxAmount) / 2));
		}
	}

	return aproximateReward;
}

bool CBankInfo::givesResources() const
{
	for(const JsonNode & node : config)
		if(!node["reward"]["resources"].isNull())
			return true;
	return false;
}

bool CBankInfo::givesArtifacts() const
{
	for(const JsonNode & node : config)
		if(!node["reward"]["artifacts"].isNull())
			return true;
	return false;
}

bool CBankInfo::givesCreatures() const
{
	for(const JsonNode & node : config)
		if(!node["reward"]["creatures"].isNull())
			return true;
	return false;
}

bool CBankInfo::givesSpells() const
{
	for(const JsonNode & node : config)
		if(!node["reward"]["spells"].isNull())
			return true;
	return false;
}


std::unique_ptr<IObjectInfo> CBankInstanceConstructor::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return std::unique_ptr<IObjectInfo>(new CBankInfo(levels));
}

VCMI_LIB_NAMESPACE_END

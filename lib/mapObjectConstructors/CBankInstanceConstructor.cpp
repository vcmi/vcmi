/*
* CBankInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "CBankInstanceConstructor.h"

#include "../json/JsonRandom.h"
#include "../CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../CRandomGenerator.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CBankInstanceConstructor::hasNameTextID() const
{
	return true;
}

void CBankInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (input.Struct().count("name") == 0)
		logMod->warn("Bank %s missing name!", getJsonKey());

	VLC->generaltexth->registerString(input.getModScope(), getNameTextID(), input["name"].String());

	levels = input["levels"].Vector();
	bankResetDuration = static_cast<si32>(input["resetDuration"].Float());
	blockVisit = input["blockedVisitable"].Bool();
	coastVisitable = input["coastVisitable"].Bool();
}

BankConfig CBankInstanceConstructor::generateConfig(IGameCallback * cb, const JsonNode & level, CRandomGenerator & rng) const
{
	BankConfig bc;
	JsonRandom randomizer(cb);
	JsonRandom::Variables emptyVariables;

	bc.chance = static_cast<ui32>(level["chance"].Float());
	bc.guards = randomizer.loadCreatures(level["guards"], rng, emptyVariables);

	bc.resources = ResourceSet(level["reward"]["resources"]);
	bc.creatures = randomizer.loadCreatures(level["reward"]["creatures"], rng, emptyVariables);
	bc.artifacts = randomizer.loadArtifacts(level["reward"]["artifacts"], rng, emptyVariables);
	bc.spells = randomizer.loadSpells(level["reward"]["spells"], rng, emptyVariables);

	return bc;
}

void CBankInstanceConstructor::randomizeObject(CBank * bank, CRandomGenerator & rng) const
{
	bank->resetDuration = bankResetDuration;
	bank->blockVisit = blockVisit;
	bank->coastVisitable = coastVisitable;

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
			bank->setConfig(generateConfig(bank->cb, node, rng));
			break;
		}
	}
}

CBankInfo::CBankInfo(const JsonVector & Config) :
	config(Config)
{
	assert(!Config.empty());
}

TPossibleGuards CBankInfo::getPossibleGuards(IGameCallback * cb) const
{
	JsonRandom::Variables emptyVariables;
	JsonRandom randomizer(cb);
	TPossibleGuards out;

	for(const JsonNode & configEntry : config)
	{
		const JsonNode & guardsInfo = configEntry["guards"];
		auto stacks = randomizer.evaluateCreatures(guardsInfo, emptyVariables);
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

std::vector<PossibleReward<CStackBasicDescriptor>> CBankInfo::getPossibleCreaturesReward(IGameCallback * cb) const
{
	JsonRandom::Variables emptyVariables;
	JsonRandom randomizer(cb);
	std::vector<PossibleReward<CStackBasicDescriptor>> aproximateReward;

	for(const JsonNode & configEntry : config)
	{
		const JsonNode & guardsInfo = configEntry["reward"]["creatures"];
		auto stacks = randomizer.evaluateCreatures(guardsInfo, emptyVariables);

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

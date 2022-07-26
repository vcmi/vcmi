/*
 * CRewardableConstructor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CRewardableConstructor.h"

#include "../CRandomGenerator.h"
#include "../StringConstants.h"
#include "../CCreatureHandler.h"
#include "JsonRandom.h"
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace {
	MetaString loadMessage(const JsonNode & value)
	{
		MetaString ret;
		if (value.isNumber())
			ret.addTxt(MetaString::ADVOB_TXT, static_cast<ui32>(value.Float()));
		else
			ret << value.String();
		return ret;
	}

	bool testForKey(const JsonNode & value, const std::string & key)
	{
		for( auto & reward : value["rewards"].Vector() )
		{
			if (!reward[key].isNull())
				return true;
		}
		return false;
	}
}

void CRandomRewardObjectInfo::init(const JsonNode & objectConfig)
{
	parameters = objectConfig;
}

void CRandomRewardObjectInfo::configureObject(CRewardableObject * object, CRandomGenerator & rng) const
{
	std::map<si32, si32> thrownDice;

	for (const JsonNode & reward : parameters["rewards"].Vector())
	{
		if (!reward["appearChance"].isNull())
		{
			JsonNode chance = reward["appearChance"];
			si32 diceID = static_cast<si32>(chance["dice"].Float());

			if (thrownDice.count(diceID) == 0)
				thrownDice[diceID] = rng.getIntRange(1, 100)();

			if (!chance["min"].isNull())
			{
				int min = static_cast<int>(chance["min"].Float());
				if (min > thrownDice[diceID])
					continue;
			}
			if (!chance["max"].isNull())
			{
				int max = static_cast<int>(chance["max"].Float());
				if (max < thrownDice[diceID])
					continue;
			}
		}

		const JsonNode & limiter = reward["limiter"];
		CVisitInfo info;
		// load limiter
		info.limiter.numOfGrants = JsonRandom::loadValue(limiter["numOfGrants"], rng);
		info.limiter.dayOfWeek = JsonRandom::loadValue(limiter["dayOfWeek"], rng);
		info.limiter.minLevel = JsonRandom::loadValue(limiter["minLevel"], rng);
		info.limiter.resources = JsonRandom::loadResources(limiter["resources"], rng);

		info.limiter.primary = JsonRandom::loadPrimary(limiter["primary"], rng);
		info.limiter.secondary = JsonRandom::loadSecondary(limiter["secondary"], rng);
		info.limiter.artifacts = JsonRandom::loadArtifacts(limiter["artifacts"], rng);
		info.limiter.creatures = JsonRandom::loadCreatures(limiter["creatures"], rng);

		info.reward.resources = JsonRandom::loadResources(reward["resources"], rng);

		info.reward.gainedExp = JsonRandom::loadValue(reward["gainedExp"], rng);
		info.reward.gainedLevels = JsonRandom::loadValue(reward["gainedLevels"], rng);

		info.reward.manaDiff = JsonRandom::loadValue(reward["manaPoints"], rng);
		info.reward.manaPercentage = JsonRandom::loadValue(reward["manaPercentage"], rng, -1);

		info.reward.movePoints = JsonRandom::loadValue(reward["movePoints"], rng);
		info.reward.movePercentage = JsonRandom::loadValue(reward["movePercentage"], rng, -1);
		
		info.reward.removeObject = reward["removeObject"].Bool();

		//FIXME: compile this line on Visual
		//info.reward.bonuses = JsonRandom::loadBonuses(reward["bonuses"]);

		info.reward.primary = JsonRandom::loadPrimary(reward["primary"], rng);
		info.reward.secondary = JsonRandom::loadSecondary(reward["secondary"], rng);

		std::vector<SpellID> spells;
		for (size_t i=0; i<6; i++)
			IObjectInterface::cb->getAllowedSpells(spells, static_cast<ui16>(i));

		info.reward.artifacts = JsonRandom::loadArtifacts(reward["artifacts"], rng);
		info.reward.spells = JsonRandom::loadSpells(reward["spells"], rng, spells);
		info.reward.creatures = JsonRandom::loadCreatures(reward["creatures"], rng);

		info.message = loadMessage(reward["message"]);
		info.selectChance = JsonRandom::loadValue(reward["selectChance"], rng);
		
		object->info.push_back(info);
	}

	object->onSelect  = loadMessage(parameters["onSelectMessage"]);
	object->onVisited = loadMessage(parameters["onVisitedMessage"]);
	object->onEmpty   = loadMessage(parameters["onEmptyMessage"]);
	object->resetDuration = static_cast<ui16>(parameters["resetDuration"].Float());
	object->canRefuse = parameters["canRefuse"].Bool();
	
	auto visitMode = parameters["visitMode"].String();
	for(int i = 0; Rewardable::VisitModeString.size(); ++i)
	{
		if(Rewardable::VisitModeString[i] == visitMode)
		{
			object->visitMode = i;
			break;
		}
	}
	
	auto selectMode = parameters["selectMode"].String();
	for(int i = 0; Rewardable::SelectModeString.size(); ++i)
	{
		if(Rewardable::SelectModeString[i] == selectMode)
		{
			object->selectMode = i;
			break;
		}
	}	
}

bool CRandomRewardObjectInfo::givesResources() const
{
	return testForKey(parameters, "resources");
}

bool CRandomRewardObjectInfo::givesExperience() const
{
	return testForKey(parameters, "gainedExp") || testForKey(parameters, "gainedLevels");
}

bool CRandomRewardObjectInfo::givesMana() const
{
	return testForKey(parameters, "manaPoints") || testForKey(parameters, "manaPercentage");
}

bool CRandomRewardObjectInfo::givesMovement() const
{
	return testForKey(parameters, "movePoints") || testForKey(parameters, "movePercentage");
}

bool CRandomRewardObjectInfo::givesPrimarySkills() const
{
	return testForKey(parameters, "primary");
}

bool CRandomRewardObjectInfo::givesSecondarySkills() const
{
	return testForKey(parameters, "secondary");
}

bool CRandomRewardObjectInfo::givesArtifacts() const
{
	return testForKey(parameters, "artifacts");
}

bool CRandomRewardObjectInfo::givesCreatures() const
{
	return testForKey(parameters, "spells");
}

bool CRandomRewardObjectInfo::givesSpells() const
{
	return testForKey(parameters, "creatures");
}

bool CRandomRewardObjectInfo::givesBonuses() const
{
	return testForKey(parameters, "bonuses");
}

CRewardableConstructor::CRewardableConstructor()
{
}

void CRewardableConstructor::initTypeData(const JsonNode & config)
{
	objectInfo.init(config);
}

CGObjectInstance * CRewardableConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	auto ret = new CRewardableObject();
	preInitObject(ret);
	ret->appearance = tmpl;
	return ret;
}

void CRewardableConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	objectInfo.configureObject(dynamic_cast<CRewardableObject*>(object), rng);
}

std::unique_ptr<IObjectInfo> CRewardableConstructor::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return std::unique_ptr<IObjectInfo>(new CRandomRewardObjectInfo(objectInfo));
}

VCMI_LIB_NAMESPACE_END

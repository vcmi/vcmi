#include "StdInc.h"
#include "CRewardableConstructor.h"

#include "../CRandomGenerator.h"
#include "../StringConstants.h"
#include "../CCreatureHandler.h"

/*
 * CRewardableConstructor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace {
	si32 loadValue(const JsonNode & value, CRandomGenerator & rng, si32 defaultValue = 0)
	{
		if (value.isNull())
			return defaultValue;
		if (value.getType() == JsonNode::DATA_FLOAT)
			return value.Float();
		si32 min = value["min"].Float();
		si32 max = value["max"].Float();
		return rng.getIntRange(min, max)();
	}

	TResources loadResources(const JsonNode & value, CRandomGenerator & rng)
	{
		TResources ret;
		for (size_t i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		{
			ret[i] = loadValue(value[GameConstants::RESOURCE_NAMES[i]], rng);
		}
		return ret;
	}

	std::vector<si32> loadPrimary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<si32> ret;
		for (auto & name : PrimarySkill::names)
		{
			ret.push_back(loadValue(value[name], rng));
		}
		return ret;
	}

	std::map<SecondarySkill, si32> loadSecondary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::map<SecondarySkill, si32> ret;
		for (auto & pair : value.Struct())
		{
			SecondarySkill id(VLC->modh->identifiers.getIdentifier(pair.second.meta, "skill", pair.first).get());
			ret[id] = loadValue(pair.second, rng);
		}
		return ret;
	}

	std::vector<ArtifactID> loadArtifacts(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<ArtifactID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			ArtifactID art(VLC->modh->identifiers.getIdentifier("artifact", entry).get());
			ret.push_back(art);
		}
		return ret;
	}

	std::vector<SpellID> loadSpells(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<SpellID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			SpellID spell(VLC->modh->identifiers.getIdentifier("spell", entry).get());
			ret.push_back(spell);
		}
		return ret;
	}

	std::vector<CStackBasicDescriptor> loadCreatures(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<CStackBasicDescriptor> ret;
		for (auto & pair : value.Struct())
		{
			CStackBasicDescriptor stack;
			stack.type = VLC->creh->creatures[VLC->modh->identifiers.getIdentifier(pair.second.meta, "creature", pair.first).get()];
			stack.count = loadValue(pair.second, rng);
			ret.push_back(stack);
		}
		return ret;
	}

	std::vector<Bonus> loadBonuses(const JsonNode & value)
	{
		std::vector<Bonus> ret;
		for (const JsonNode & entry : value.Vector())
		{
			Bonus * bonus = JsonUtils::parseBonus(entry);
			ret.push_back(*bonus);
			delete bonus;
		}
		return ret;
	}

	std::vector<Component> loadComponents(const JsonNode & value)
	{
		//TODO
	}

	MetaString loadMessage(const JsonNode & value)
	{
		MetaString ret;
		if (value.getType() == JsonNode::DATA_FLOAT)
			ret.addTxt(MetaString::ADVOB_TXT, value.Float());
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
			si32 diceID = chance["dice"].Float();

			if (thrownDice.count(diceID) == 0)
				thrownDice[diceID] = rng.getIntRange(1, 100)();

			if (!chance["min"].isNull())
			{
				int min = chance["min"].Float();
				if (min > thrownDice[diceID])
					continue;
			}
			if (!chance["max"].isNull())
			{
				int max = chance["max"].Float();
				if (max < thrownDice[diceID])
					continue;
			}
		}

		const JsonNode & limiter = reward["limiter"];
		CVisitInfo info;
		// load limiter
		info.limiter.numOfGrants = loadValue(limiter["numOfGrants"], rng);
		info.limiter.dayOfWeek = loadValue(limiter["dayOfWeek"], rng);
		info.limiter.minLevel = loadValue(limiter["minLevel"], rng);
		info.limiter.resources = loadResources(limiter["resources"], rng);

		info.limiter.primary = loadPrimary(limiter["primary"], rng);
		info.limiter.secondary = loadSecondary(limiter["secondary"], rng);
		info.limiter.artifacts = loadArtifacts(limiter["artifacts"], rng);
		info.limiter.creatures = loadCreatures(limiter["creatures"], rng);

		info.reward.resources = loadResources(reward["resources"], rng);

		info.reward.gainedExp = loadValue(reward["gainedExp"], rng);
		info.reward.gainedLevels = loadValue(reward["gainedLevels"], rng);

		info.reward.manaDiff = loadValue(reward["manaPoints"], rng);
		info.reward.manaPercentage = loadValue(reward["manaPercentage"], rng, -1);

		info.reward.movePoints = loadValue(reward["movePoints"], rng);
		info.reward.movePercentage = loadValue(reward["movePercentage"], rng, -1);

		info.reward.bonuses = loadBonuses(reward["bonuses"]);

		info.reward.primary = loadPrimary(reward["primary"], rng);
		info.reward.secondary = loadSecondary(reward["secondary"], rng);

		info.reward.artifacts = loadArtifacts(reward["artifacts"], rng);
		info.reward.spells = loadSpells(reward["spells"], rng);
		info.reward.creatures = loadCreatures(reward["creatures"], rng);

		info.message = loadMessage(reward["message"]);
		info.selectChance = loadValue(reward["selectChance"], rng);
	}

	object->onSelect  = loadMessage(parameters["onSelectMessage"]);
	object->onVisited = loadMessage(parameters["onVisitedMessage"]);
	object->onEmpty   = loadMessage(parameters["onEmptyMessage"]);

	//TODO: visitMode and selectMode

	object->soundID = parameters["soundID"].Float();
	object->resetDuration = parameters["resetDuration"].Float();
	object->canRefuse =parameters["canRefuse"].Bool();
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
	AObjectTypeHandler::init(config);
	objectInfo.init(config);
}

CGObjectInstance * CRewardableConstructor::create(ObjectTemplate tmpl) const
{
	auto ret = new CRewardableObject();
	ret->appearance = tmpl;
	return ret;
}

void CRewardableConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	objectInfo.configureObject(dynamic_cast<CRewardableObject*>(object), rng);
}

const IObjectInfo * CRewardableConstructor::getObjectInfo(ObjectTemplate tmpl) const
{
	return &objectInfo;
}

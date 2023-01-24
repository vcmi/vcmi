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
#include "../CModHandler.h"
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

TRewardLimitersList CRandomRewardObjectInfo::configureSublimiters(CRewardableObject * object, CRandomGenerator & rng, const JsonNode & source) const
{
	TRewardLimitersList result;
	for (const auto & input : source.Vector())
	{
		auto newLimiter = std::make_shared<CRewardLimiter>();

		configureLimiter(object, rng, *newLimiter, input);

		result.push_back(newLimiter);
	}

	return result;
}

void CRandomRewardObjectInfo::configureLimiter(CRewardableObject * object, CRandomGenerator & rng, CRewardLimiter & limiter, const JsonNode & source) const
{
	limiter.dayOfWeek = JsonRandom::loadValue(source["dayOfWeek"], rng);
	limiter.daysPassed = JsonRandom::loadValue(source["daysPassed"], rng);
	limiter.minLevel = JsonRandom::loadValue(source["minLevel"], rng);
	limiter.manaPercentage = JsonRandom::loadValue(source["manaPercentage"], rng);
	limiter.manaPoints = JsonRandom::loadValue(source["manaPoints"], rng);

	limiter.resources = JsonRandom::loadResources(source["resources"], rng);

	limiter.primary = JsonRandom::loadPrimary(source["primary"], rng);
	limiter.secondary = JsonRandom::loadSecondary(source["secondary"], rng);
	limiter.artifacts = JsonRandom::loadArtifacts(source["artifacts"], rng);
	limiter.creatures = JsonRandom::loadCreatures(source["creatures"], rng);

	limiter.allOf  = configureSublimiters(object, rng, source["allOf"] );
	limiter.anyOf  = configureSublimiters(object, rng, source["anyOf"] );
	limiter.noneOf = configureSublimiters(object, rng, source["noneOf"] );
}

void CRandomRewardObjectInfo::configureReward(CRewardableObject * object, CRandomGenerator & rng, CRewardInfo & reward, const JsonNode & source) const
{
	reward.resources = JsonRandom::loadResources(source["resources"], rng);

	reward.gainedExp = JsonRandom::loadValue(source["gainedExp"], rng);
	reward.gainedLevels = JsonRandom::loadValue(source["gainedLevels"], rng);

	reward.manaDiff = JsonRandom::loadValue(source["manaPoints"], rng);
	reward.manaOverflowFactor = JsonRandom::loadValue(source["manaOverflowFactor"], rng);
	reward.manaPercentage = JsonRandom::loadValue(source["manaPercentage"], rng, -1);

	reward.movePoints = JsonRandom::loadValue(source["movePoints"], rng);
	reward.movePercentage = JsonRandom::loadValue(source["movePercentage"], rng, -1);

	reward.removeObject = source["removeObject"].Bool();
	reward.bonuses = JsonRandom::loadBonuses(source["bonuses"]);

	for (auto & bonus : reward.bonuses)
	{
		bonus.source = Bonus::OBJECT;
		bonus.sid = object->ID;
		//TODO: bonus.description = object->getObjectName();
		if (bonus.type == Bonus::MORALE)
			reward.extraComponents.push_back(Component(Component::MORALE, 0, bonus.val, 0));
		if (bonus.type == Bonus::LUCK)
			reward.extraComponents.push_back(Component(Component::LUCK, 0, bonus.val, 0));
	}

	reward.primary = JsonRandom::loadPrimary(source["primary"], rng);
	reward.secondary = JsonRandom::loadSecondary(source["secondary"], rng);

	std::vector<SpellID> spells;
	for (size_t i=0; i<6; i++)
		IObjectInterface::cb->getAllowedSpells(spells, static_cast<ui16>(i));

	reward.artifacts = JsonRandom::loadArtifacts(source["artifacts"], rng);
	reward.spells = JsonRandom::loadSpells(source["spells"], rng, spells);
	reward.creatures = JsonRandom::loadCreatures(source["creatures"], rng);

	for ( auto node : source["changeCreatures"].Struct() )
	{
		CreatureID from (VLC->modh->identifiers.getIdentifier (node.second.meta, "creature", node.first) .get());
		CreatureID dest (VLC->modh->identifiers.getIdentifier (node.second.meta, "creature", node.second.String()).get());

		reward.extraComponents.push_back(Component(Component::CREATURE, dest.getNum(), 0, 0));

		reward.creaturesChange[from] = dest;
	}
}

void CRandomRewardObjectInfo::configureResetInfo(CRewardableObject * object, CRandomGenerator & rng, CRewardResetInfo & resetParameters, const JsonNode & source) const
{
	resetParameters.period   = static_cast<ui32>(source["period"].Float());
	resetParameters.visitors = source["visitors"].Bool();
	resetParameters.rewards  = source["rewards"].Bool();
}

void CRandomRewardObjectInfo::configureObject(CRewardableObject * object, CRandomGenerator & rng) const
{
	object->info.clear();

	std::map<si32, si32> thrownDice;

	for (const JsonNode & reward : parameters["rewards"].Vector())
	{
		if (!reward["appearChance"].isNull())
		{
			JsonNode chance = reward["appearChance"];
			si32 diceID = static_cast<si32>(chance["dice"].Float());

			if (thrownDice.count(diceID) == 0)
				thrownDice[diceID] = rng.getIntRange(0, 99)();

			if (!chance["min"].isNull())
			{
				int min = static_cast<int>(chance["min"].Float());
				if (min > thrownDice[diceID])
					continue;
			}
			if (!chance["max"].isNull())
			{
				int max = static_cast<int>(chance["max"].Float());
				if (max <= thrownDice[diceID])
					continue;
			}
		}

		CRewardVisitInfo info;
		configureLimiter(object, rng, info.limiter, reward["limiter"]);
		configureReward(object, rng, info.reward, reward);

		info.message = loadMessage(reward["message"]);

		for (const auto & artifact : info.reward.artifacts )
			info.message.addReplacement(MetaString::ART_NAMES, artifact.getNum());
		
		for (const auto & artifact : info.reward.spells )
			info.message.addReplacement(MetaString::SPELL_NAME, artifact.getNum());

		object->info.push_back(info);
	}

	object->blockVisit= parameters["blockedVisitable"].Bool();
	object->onSelect  = loadMessage(parameters["onSelectMessage"]);
	object->onVisited = loadMessage(parameters["onVisitedMessage"]);
	object->onEmpty   = loadMessage(parameters["onEmptyMessage"]);

	configureResetInfo(object, rng, object->resetParameters, parameters["resetParameters"]);

	object->canRefuse = parameters["canRefuse"].Bool();
	
	auto visitMode = parameters["visitMode"].String();
	for(int i = 0; i < Rewardable::VisitModeString.size(); ++i)
	{
		if(Rewardable::VisitModeString[i] == visitMode)
		{
			object->visitMode = i;
			break;
		}
	}
	
	auto selectMode = parameters["selectMode"].String();
	for(int i = 0; i < Rewardable::SelectModeString.size(); ++i)
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

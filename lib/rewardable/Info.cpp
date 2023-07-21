/*
 * Info.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Info.h"

#include "Configuration.h"
#include "Limiter.h"
#include "Reward.h"

#include "../CGeneralTextHandler.h"
#include "../CModHandler.h"
#include "../IGameCallback.h"
#include "../JsonRandom.h"
#include "../mapObjects/IObjectInterface.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace {
	MetaString loadMessage(const JsonNode & value, const TextIdentifier & textIdentifier )
	{
		MetaString ret;
		if (value.isNumber())
		{
			ret.appendLocalString(EMetaText::ADVOB_TXT, static_cast<ui32>(value.Float()));
			return ret;
		}

		if (value.String().empty())
			return ret;

		ret.appendTextID(textIdentifier.get());
		return ret;
	}

	bool testForKey(const JsonNode & value, const std::string & key)
	{
		for(const auto & reward : value["rewards"].Vector())
		{
			if (!reward[key].isNull())
				return true;
		}
		return false;
	}
}

void Rewardable::Info::init(const JsonNode & objectConfig, const std::string & objectName)
{
	objectTextID = objectName;

	auto loadString = [&](const JsonNode & entry, const TextIdentifier & textID){
		if (entry.isString() && !entry.String().empty())
			VLC->generaltexth->registerString(entry.meta, textID, entry.String());
	};

	parameters = objectConfig;

	for(size_t i = 0; i < parameters["rewards"].Vector().size(); ++i)
	{
		const JsonNode message = parameters["rewards"][i]["message"];
		loadString(message, TextIdentifier(objectName, "rewards", i));
	}

	for(size_t i = 0; i < parameters["onVisited"].Vector().size(); ++i)
	{
		const JsonNode message = parameters["onVisited"][i]["message"];
		loadString(message, TextIdentifier(objectName, "onVisited", i));
	}

	for(size_t i = 0; i < parameters["onEmpty"].Vector().size(); ++i)
	{
		const JsonNode message = parameters["onEmpty"][i]["message"];
		loadString(message, TextIdentifier(objectName, "onEmpty", i));
	}

	loadString(parameters["onSelectMessage"], TextIdentifier(objectName, "onSelect"));
	loadString(parameters["onVisitedMessage"], TextIdentifier(objectName, "onVisited"));
	loadString(parameters["onEmptyMessage"], TextIdentifier(objectName, "onEmpty"));
}

Rewardable::LimitersList Rewardable::Info::configureSublimiters(Rewardable::Configuration & object, CRandomGenerator & rng, const JsonNode & source) const
{
	Rewardable::LimitersList result;
	for (const auto & input : source.Vector())
	{
		auto newLimiter = std::make_shared<Rewardable::Limiter>();

		configureLimiter(object, rng, *newLimiter, input);

		result.push_back(newLimiter);
	}

	return result;
}

void Rewardable::Info::configureLimiter(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::Limiter & limiter, const JsonNode & source) const
{
	std::vector<SpellID> spells;
	IObjectInterface::cb->getAllowedSpells(spells);


	limiter.dayOfWeek = JsonRandom::loadValue(source["dayOfWeek"], rng);
	limiter.daysPassed = JsonRandom::loadValue(source["daysPassed"], rng);
	limiter.heroExperience = JsonRandom::loadValue(source["heroExperience"], rng);
	limiter.heroLevel = JsonRandom::loadValue(source["heroLevel"], rng)
					 + JsonRandom::loadValue(source["minLevel"], rng); // VCMI 1.1 compatibilty

	limiter.manaPercentage = JsonRandom::loadValue(source["manaPercentage"], rng);
	limiter.manaPoints = JsonRandom::loadValue(source["manaPoints"], rng);

	limiter.resources = JsonRandom::loadResources(source["resources"], rng);

	limiter.primary = JsonRandom::loadPrimary(source["primary"], rng);
	limiter.secondary = JsonRandom::loadSecondary(source["secondary"], rng);
	limiter.artifacts = JsonRandom::loadArtifacts(source["artifacts"], rng);
	limiter.spells  = JsonRandom::loadSpells(source["spells"], rng, spells);
	limiter.creatures = JsonRandom::loadCreatures(source["creatures"], rng);

	limiter.allOf  = configureSublimiters(object, rng, source["allOf"] );
	limiter.anyOf  = configureSublimiters(object, rng, source["anyOf"] );
	limiter.noneOf = configureSublimiters(object, rng, source["noneOf"] );
}

void Rewardable::Info::configureReward(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::Reward & reward, const JsonNode & source) const
{
	reward.resources = JsonRandom::loadResources(source["resources"], rng);

	reward.heroExperience = JsonRandom::loadValue(source["heroExperience"], rng)
						  + JsonRandom::loadValue(source["gainedExp"], rng); // VCMI 1.1 compatibilty

	reward.heroLevel = JsonRandom::loadValue(source["heroLevel"], rng)
						+ JsonRandom::loadValue(source["gainedLevels"], rng); // VCMI 1.1 compatibilty

	reward.manaDiff = JsonRandom::loadValue(source["manaPoints"], rng);
	reward.manaOverflowFactor = JsonRandom::loadValue(source["manaOverflowFactor"], rng);
	reward.manaPercentage = JsonRandom::loadValue(source["manaPercentage"], rng, -1);

	reward.movePoints = JsonRandom::loadValue(source["movePoints"], rng);
	reward.movePercentage = JsonRandom::loadValue(source["movePercentage"], rng, -1);

	reward.removeObject = source["removeObject"].Bool();
	reward.bonuses = JsonRandom::loadBonuses(source["bonuses"]);

	reward.primary = JsonRandom::loadPrimary(source["primary"], rng);
	reward.secondary = JsonRandom::loadSecondary(source["secondary"], rng);

	std::vector<SpellID> spells;
	IObjectInterface::cb->getAllowedSpells(spells);

	reward.artifacts = JsonRandom::loadArtifacts(source["artifacts"], rng);
	reward.spells = JsonRandom::loadSpells(source["spells"], rng, spells);
	reward.creatures = JsonRandom::loadCreatures(source["creatures"], rng);
	if(!source["spellCast"].isNull() && source["spellCast"].isStruct())
	{
		reward.spellCast.first = JsonRandom::loadSpell(source["spellCast"]["spell"], rng);
		reward.spellCast.second = source["spellCast"]["schoolLevel"].Integer();
	}

	for ( auto node : source["changeCreatures"].Struct() )
	{
		CreatureID from(VLC->modh->identifiers.getIdentifier(node.second.meta, "creature", node.first).value());
		CreatureID dest(VLC->modh->identifiers.getIdentifier(node.second.meta, "creature", node.second.String()).value());

		reward.extraComponents.emplace_back(Component::EComponentType::CREATURE, dest.getNum(), 0, 0);

		reward.creaturesChange[from] = dest;
	}
}

void Rewardable::Info::configureResetInfo(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::ResetInfo & resetParameters, const JsonNode & source) const
{
	resetParameters.period   = static_cast<ui32>(source["period"].Float());
	resetParameters.visitors = source["visitors"].Bool();
	resetParameters.rewards  = source["rewards"].Bool();
}

void Rewardable::Info::configureRewards(
		Rewardable::Configuration & object,
		CRandomGenerator & rng, const
		JsonNode & source,
		std::map<si32, si32> & thrownDice,
		Rewardable::EEventType event,
		const std::string & modeName) const
{
	for(size_t i = 0; i < source.Vector().size(); ++i)
	{
		const JsonNode reward = source.Vector()[i];

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

		Rewardable::VisitInfo info;
		configureLimiter(object, rng, info.limiter, reward["limiter"]);
		configureReward(object, rng, info.reward, reward);

		info.visitType = event;
		info.message = loadMessage(reward["message"], TextIdentifier(objectTextID, modeName, i));

		for (const auto & artifact : info.reward.artifacts )
			info.message.replaceLocalString(EMetaText::ART_NAMES, artifact.getNum());

		for (const auto & artifact : info.reward.spells )
			info.message.replaceLocalString(EMetaText::SPELL_NAME, artifact.getNum());

		object.info.push_back(info);
	}
}

void Rewardable::Info::configureObject(Rewardable::Configuration & object, CRandomGenerator & rng) const
{
	object.info.clear();

	std::map<si32, si32> thrownDice;

	configureRewards(object, rng, parameters["rewards"], thrownDice, Rewardable::EEventType::EVENT_FIRST_VISIT, "rewards");
	configureRewards(object, rng, parameters["onVisited"], thrownDice, Rewardable::EEventType::EVENT_ALREADY_VISITED, "onVisited");
	configureRewards(object, rng, parameters["onEmpty"], thrownDice, Rewardable::EEventType::EVENT_NOT_AVAILABLE, "onEmpty");

	object.onSelect   = loadMessage(parameters["onSelectMessage"], TextIdentifier(objectTextID, "onSelect"));

	if (!parameters["onVisitedMessage"].isNull())
	{
		Rewardable::VisitInfo onVisited;
		onVisited.visitType = Rewardable::EEventType::EVENT_ALREADY_VISITED;
		onVisited.message = loadMessage(parameters["onVisitedMessage"], TextIdentifier(objectTextID, "onVisited"));
		object.info.push_back(onVisited);
	}

	if (!parameters["onEmptyMessage"].isNull())
	{
		Rewardable::VisitInfo onEmpty;
		onEmpty.visitType = Rewardable::EEventType::EVENT_NOT_AVAILABLE;
		onEmpty.message = loadMessage(parameters["onEmptyMessage"], TextIdentifier(objectTextID, "onEmpty"));
		object.info.push_back(onEmpty);
	}

	configureResetInfo(object, rng, object.resetParameters, parameters["resetParameters"]);

	object.canRefuse = parameters["canRefuse"].Bool();

	if(parameters["showInInfobox"].isNull())
		object.infoWindowType = EInfoWindowMode::AUTO;
	else
		object.infoWindowType = parameters["showInInfobox"].Bool() ? EInfoWindowMode::INFO : EInfoWindowMode::MODAL;
	
	auto visitMode = parameters["visitMode"].String();
	for(int i = 0; i < Rewardable::VisitModeString.size(); ++i)
	{
		if(Rewardable::VisitModeString[i] == visitMode)
		{
			object.visitMode = i;
			break;
		}
	}
	
	auto selectMode = parameters["selectMode"].String();
	for(int i = 0; i < Rewardable::SelectModeString.size(); ++i)
	{
		if(Rewardable::SelectModeString[i] == selectMode)
		{
			object.selectMode = i;
			break;
		}
	}
}

bool Rewardable::Info::givesResources() const
{
	return testForKey(parameters, "resources");
}

bool Rewardable::Info::givesExperience() const
{
	return testForKey(parameters, "gainedExp") || testForKey(parameters, "gainedLevels");
}

bool Rewardable::Info::givesMana() const
{
	return testForKey(parameters, "manaPoints") || testForKey(parameters, "manaPercentage");
}

bool Rewardable::Info::givesMovement() const
{
	return testForKey(parameters, "movePoints") || testForKey(parameters, "movePercentage");
}

bool Rewardable::Info::givesPrimarySkills() const
{
	return testForKey(parameters, "primary");
}

bool Rewardable::Info::givesSecondarySkills() const
{
	return testForKey(parameters, "secondary");
}

bool Rewardable::Info::givesArtifacts() const
{
	return testForKey(parameters, "artifacts");
}

bool Rewardable::Info::givesCreatures() const
{
	return testForKey(parameters, "spells");
}

bool Rewardable::Info::givesSpells() const
{
	return testForKey(parameters, "creatures");
}

bool Rewardable::Info::givesBonuses() const
{
	return testForKey(parameters, "bonuses");
}

const JsonNode & Rewardable::Info::getParameters() const
{
	return parameters;
}

VCMI_LIB_NAMESPACE_END

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
#include "../IGameCallback.h"
#include "../JsonRandom.h"
#include "../mapObjects/IObjectInterface.h"
#include "../modding/IdentifierStorage.h"

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
	auto const & variables = object.variables.values;

	limiter.dayOfWeek = JsonRandom::loadValue(source["dayOfWeek"], rng, variables);
	limiter.daysPassed = JsonRandom::loadValue(source["daysPassed"], rng, variables);
	limiter.heroExperience = JsonRandom::loadValue(source["heroExperience"], rng, variables);
	limiter.heroLevel = JsonRandom::loadValue(source["heroLevel"], rng, variables);
	limiter.canLearnSkills = source["canLearnSkills"].Bool();

	limiter.manaPercentage = JsonRandom::loadValue(source["manaPercentage"], rng, variables);
	limiter.manaPoints = JsonRandom::loadValue(source["manaPoints"], rng, variables);

	limiter.resources = JsonRandom::loadResources(source["resources"], rng, variables);

	limiter.primary = JsonRandom::loadPrimary(source["primary"], rng, variables);
	limiter.secondary = JsonRandom::loadSecondaries(source["secondary"], rng, variables);
	limiter.artifacts = JsonRandom::loadArtifacts(source["artifacts"], rng, variables);
	limiter.spells  = JsonRandom::loadSpells(source["spells"], rng, variables);
	limiter.creatures = JsonRandom::loadCreatures(source["creatures"], rng, variables);
	
	limiter.players = JsonRandom::loadColors(source["colors"], rng);
	limiter.heroes = JsonRandom::loadHeroes(source["heroes"], rng);
	limiter.heroClasses = JsonRandom::loadHeroClasses(source["heroClasses"], rng);

	limiter.allOf  = configureSublimiters(object, rng, source["allOf"] );
	limiter.anyOf  = configureSublimiters(object, rng, source["anyOf"] );
	limiter.noneOf = configureSublimiters(object, rng, source["noneOf"] );
}

void Rewardable::Info::configureReward(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::Reward & reward, const JsonNode & source) const
{
	auto const & variables = object.variables.values;

	reward.resources = JsonRandom::loadResources(source["resources"], rng, variables);

	reward.heroExperience = JsonRandom::loadValue(source["heroExperience"], rng, variables);
	reward.heroLevel = JsonRandom::loadValue(source["heroLevel"], rng, variables);

	reward.manaDiff = JsonRandom::loadValue(source["manaPoints"], rng, variables);
	reward.manaOverflowFactor = JsonRandom::loadValue(source["manaOverflowFactor"], rng, variables);
	reward.manaPercentage = JsonRandom::loadValue(source["manaPercentage"], rng, variables, -1);

	reward.movePoints = JsonRandom::loadValue(source["movePoints"], rng, variables);
	reward.movePercentage = JsonRandom::loadValue(source["movePercentage"], rng, variables, -1);

	reward.removeObject = source["removeObject"].Bool();
	reward.bonuses = JsonRandom::loadBonuses(source["bonuses"]);

	reward.primary = JsonRandom::loadPrimary(source["primary"], rng, variables);
	reward.secondary = JsonRandom::loadSecondaries(source["secondary"], rng, variables);

	reward.artifacts = JsonRandom::loadArtifacts(source["artifacts"], rng, variables);
	reward.spells = JsonRandom::loadSpells(source["spells"], rng, variables);
	reward.creatures = JsonRandom::loadCreatures(source["creatures"], rng, variables);
	if(!source["spellCast"].isNull() && source["spellCast"].isStruct())
	{
		reward.spellCast.first = JsonRandom::loadSpell(source["spellCast"]["spell"], rng, variables);
		reward.spellCast.second = source["spellCast"]["schoolLevel"].Integer();
	}

	if (!source["revealTiles"].isNull())
	{
		auto const & entry = source["revealTiles"];

		reward.revealTiles = RewardRevealTiles();
		reward.revealTiles->radius = JsonRandom::loadValue(entry["radius"], rng, variables);
		reward.revealTiles->hide = entry["hide"].Bool();

		reward.revealTiles->scoreSurface = JsonRandom::loadValue(entry["surface"], rng, variables);
		reward.revealTiles->scoreSubterra = JsonRandom::loadValue(entry["subterra"], rng, variables);
		reward.revealTiles->scoreWater = JsonRandom::loadValue(entry["water"], rng, variables);
		reward.revealTiles->scoreRock = JsonRandom::loadValue(entry["rock"], rng, variables);
	}

	for ( auto node : source["changeCreatures"].Struct() )
	{
		CreatureID from(VLC->identifiers()->getIdentifier(node.second.meta, "creature", node.first).value());
		CreatureID dest(VLC->identifiers()->getIdentifier(node.second.meta, "creature", node.second.String()).value());

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

void Rewardable::Info::configureVariables(Rewardable::Configuration & object, CRandomGenerator & rng, const JsonNode & source) const
{
	for(const auto & category : source.Struct())
	{
		for(const auto & entry : category.second.Struct())
		{
			JsonNode preset = object.getPresetVariable(category.first, entry.first);
			const JsonNode & input = preset.isNull() ? entry.second : preset;
			int32_t value = -1;

			if (category.first == "number")
				value = JsonRandom::loadValue(input, rng, object.variables.values);

			if (category.first == "artifact")
				value = JsonRandom::loadArtifact(input, rng, object.variables.values).getNum();

			if (category.first == "spell")
				value = JsonRandom::loadSpell(input, rng, object.variables.values).getNum();

			// TODO
			//	if (category.first == "creature")
			//		value = JsonRandom::loadCreature(input, rng, object.variables.values).type->getId();

			// TODO
			//	if (category.first == "resource")
			//		value = JsonRandom::loadResource(input, rng, object.variables.values).getNum();

			// TODO
			//	if (category.first == "primarySkill")
			//		value = JsonRandom::loadCreature(input, rng, object.variables.values).getNum();

			if (category.first == "secondarySkill")
				value = JsonRandom::loadSecondary(input, rng, object.variables.values).getNum();

			object.initVariable(category.first, entry.first, value);
		}
	}
}

void Rewardable::Info::configureRewards(
		Rewardable::Configuration & object,
		CRandomGenerator & rng, const
		JsonNode & source,
		Rewardable::EEventType event,
		const std::string & modeName) const
{
	for(size_t i = 0; i < source.Vector().size(); ++i)
	{
		const JsonNode reward = source.Vector()[i];

		if (!reward["appearChance"].isNull())
		{
			JsonNode chance = reward["appearChance"];
			std::string diceID = std::to_string(chance["dice"].Integer());

			auto diceValue = object.getVariable("dice", diceID);

			if (!diceValue.has_value())
			{
				object.initVariable("dice", diceID, rng.getIntRange(0, 99)());
				diceValue = object.getVariable("dice", diceID);
			}
			assert(diceValue.has_value());

			if (!chance["min"].isNull())
			{
				int min = static_cast<int>(chance["min"].Float());
				if (min > *diceValue)
					continue;
			}
			if (!chance["max"].isNull())
			{
				int max = static_cast<int>(chance["max"].Float());
				if (max <= *diceValue)
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

	configureVariables(object, rng, parameters["variables"]);

	configureRewards(object, rng, parameters["rewards"], Rewardable::EEventType::EVENT_FIRST_VISIT, "rewards");
	configureRewards(object, rng, parameters["onVisited"], Rewardable::EEventType::EVENT_ALREADY_VISITED, "onVisited");
	configureRewards(object, rng, parameters["onEmpty"], Rewardable::EEventType::EVENT_NOT_AVAILABLE, "onEmpty");

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

	configureLimiter(object, rng, object.visitLimiter, parameters["visitLimiter"]);

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

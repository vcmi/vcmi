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

#include "../CCreatureHandler.h"
#include "../GameLibrary.h"
#include "../callback/IGameRandomizer.h"
#include "../json/JsonRandom.h"
#include "../mapObjects/IObjectInterface.h"
#include "../modding/IdentifierStorage.h"
#include "../texts/CGeneralTextHandler.h"
#include "../entities/ResourceTypeHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace {
	MetaString loadMessage(const JsonNode & value, const TextIdentifier & textIdentifier, EMetaText textSource = EMetaText::ADVOB_TXT )
	{
		MetaString ret;

		if (value.isVector())
		{
			for(const auto & entry : value.Vector())
			{
				if (entry.isNumber())
					ret.appendLocalString(textSource, static_cast<ui32>(entry.Float()));
				if (entry.isString())
					ret.appendRawString(entry.String());
			}
			return ret;
		}

		if (value.isNumber())
		{
			ret.appendLocalString(textSource, static_cast<ui32>(value.Float()));
			return ret;
		}

		if (value.String().empty())
			return ret;

		if (value.String()[0] == '@')
			ret.appendTextID(value.String().substr(1));
		else
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
		if (entry.isString() && !entry.String().empty() && entry.String()[0] != '@')
			LIBRARY->generaltexth->registerString(entry.getModScope(), textID, entry);
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
	loadString(parameters["description"], TextIdentifier(objectName, "description"));
	loadString(parameters["notVisitedTooltip"], TextIdentifier(objectName, "notVisitedText"));
	loadString(parameters["visitedTooltip"], TextIdentifier(objectName, "visitedTooltip"));
	loadString(parameters["onVisitedMessage"], TextIdentifier(objectName, "onVisited"));
	loadString(parameters["onEmptyMessage"], TextIdentifier(objectName, "onEmpty"));
	loadString(parameters["onGuardedMessage"], TextIdentifier(objectName, "onGuarded"));
}

Rewardable::LimitersList Rewardable::Info::configureSublimiters(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source) const
{
	Rewardable::LimitersList result;
	for (const auto & input : source.Vector())
	{
		auto newLimiter = std::make_shared<Rewardable::Limiter>();

		configureLimiter(object, gameRandomizer, cb, *newLimiter, input);

		result.push_back(newLimiter);
	}

	return result;
}

void Rewardable::Info::configureLimiter(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Limiter & limiter, const JsonNode & source) const
{
	auto const & variables = object.variables.values;
	JsonRandom randomizer(cb, gameRandomizer);

	limiter.dayOfWeek = randomizer.loadValue(source["dayOfWeek"], variables);
	limiter.daysPassed = randomizer.loadValue(source["daysPassed"], variables);
	limiter.heroExperience = randomizer.loadValue(source["heroExperience"], variables);
	limiter.heroLevel = randomizer.loadValue(source["heroLevel"], variables);
	limiter.canLearnSkills = source["canLearnSkills"].Bool();
	limiter.commanderAlive = source["commanderAlive"].Bool();
	limiter.hasExtraCreatures = source["hasExtraCreatures"].Bool();

	limiter.manaPercentage = randomizer.loadValue(source["manaPercentage"], variables);
	limiter.manaPoints = randomizer.loadValue(source["manaPoints"], variables);

	limiter.resources = randomizer.loadResources(source["resources"], variables);

	limiter.primary = randomizer.loadPrimaries(source["primary"], variables);
	limiter.secondary = randomizer.loadSecondaries(source["secondary"], variables, true);
	limiter.artifacts = randomizer.loadArtifacts(source["artifacts"], variables);
	limiter.availableSlots = randomizer.loadArtifactSlots(source["availableSlots"], variables);
	limiter.spells  = randomizer.loadSpells(source["spells"], variables);
	limiter.scrolls  = randomizer.loadSpells(source["scrolls"], variables);
	limiter.canLearnSpells  = randomizer.loadSpells(source["canLearnSpells"], variables);
	limiter.creatures = randomizer.loadCreatures(source["creatures"], variables);
	limiter.canReceiveCreatures = randomizer.loadCreatures(source["canReceiveCreatures"], variables);
	
	limiter.players = randomizer.loadColors(source["colors"], variables);
	limiter.heroes = randomizer.loadHeroes(source["heroes"]);
	limiter.heroClasses = randomizer.loadHeroClasses(source["heroClasses"]);

	limiter.allOf  = configureSublimiters(object, gameRandomizer, cb, source["allOf"]);
	limiter.anyOf  = configureSublimiters(object, gameRandomizer, cb, source["anyOf"]);
	limiter.noneOf = configureSublimiters(object, gameRandomizer, cb, source["noneOf"]);
}

void Rewardable::Info::configureReward(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Reward & reward, const JsonNode & source) const
{
	auto const & variables = object.variables.values;
	JsonRandom randomizer(cb, gameRandomizer);

	reward.resources = randomizer.loadResources(source["resources"], variables);

	reward.heroExperience = randomizer.loadValue(source["heroExperience"], variables);
	reward.heroLevel = randomizer.loadValue(source["heroLevel"], variables);

	reward.manaDiff = randomizer.loadValue(source["manaPoints"], variables);
	reward.manaOverflowFactor = randomizer.loadValue(source["manaOverflowFactor"], variables);
	reward.manaPercentage = randomizer.loadValue(source["manaPercentage"], variables, -1);

	reward.movePoints = randomizer.loadValue(source["movePoints"], variables);
	reward.movePercentage = randomizer.loadValue(source["movePercentage"], variables, -1);

	reward.removeObject = source["removeObject"].Bool();
	reward.heroBonuses = randomizer.loadBonuses(source["bonuses"]);
	reward.commanderBonuses = randomizer.loadBonuses(source["commanderBonuses"]);
	reward.playerBonuses = randomizer.loadBonuses(source["playerBonuses"]);

	reward.guards = randomizer.loadCreatures(source["guards"], variables);

	reward.primary = randomizer.loadPrimaries(source["primary"], variables);
	reward.secondary = randomizer.loadSecondaries(source["secondary"], variables);

	reward.grantedArtifacts = randomizer.loadArtifacts(source["artifacts"], variables);
	reward.takenArtifacts = randomizer.loadArtifacts(source["takenArtifacts"], variables);
	reward.takenArtifactSlots = randomizer.loadArtifactSlots(source["takenArtifactSlots"], variables);
	reward.grantedScrolls = randomizer.loadSpells(source["scrolls"], variables);
	reward.takenScrolls = randomizer.loadSpells(source["takenScrolls"], variables);
	reward.spells = randomizer.loadSpells(source["spells"], variables);
	reward.creatures = randomizer.loadCreatures(source["creatures"], variables);
	reward.takenCreatures = randomizer.loadCreatures(source["takenCreatures"], variables);
	if(!source["spellCast"].isNull() && source["spellCast"].isStruct())
	{
		reward.spellCast.first = randomizer.loadSpell(source["spellCast"]["spell"], variables);
		reward.spellCast.second = source["spellCast"]["schoolLevel"].Integer();
	}

	if (!source["revealTiles"].isNull())
	{
		auto const & entry = source["revealTiles"];

		reward.revealTiles = RewardRevealTiles();
		reward.revealTiles->radius = randomizer.loadValue(entry["radius"], variables);
		reward.revealTiles->hide = entry["hide"].Bool();

		reward.revealTiles->scoreSurface = randomizer.loadValue(entry["surface"], variables);
		reward.revealTiles->scoreSubterra = randomizer.loadValue(entry["subterra"], variables);
		reward.revealTiles->scoreWater = randomizer.loadValue(entry["water"], variables);
		reward.revealTiles->scoreRock = randomizer.loadValue(entry["rock"], variables);
	}

	for ( auto node : source["changeCreatures"].Struct() )
	{
		CreatureID from(LIBRARY->identifiers()->getIdentifier(node.second.getModScope(), "creature", node.first).value());
		CreatureID dest(LIBRARY->identifiers()->getIdentifier(node.second.getModScope(), "creature", node.second.String()).value());

		reward.extraComponents.emplace_back(ComponentType::CREATURE, dest);

		reward.creaturesChange[from] = dest;
	}
}

void Rewardable::Info::configureResetInfo(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, Rewardable::ResetInfo & resetParameters, const JsonNode & source) const
{
	resetParameters.period   = static_cast<ui32>(source["period"].Float());
	resetParameters.visitors = source["visitors"].Bool();
	resetParameters.rewards  = source["rewards"].Bool();
}

void Rewardable::Info::configureVariables(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source) const
{
	JsonRandom randomizer(cb, gameRandomizer);

	for(const auto & category : source.Struct())
	{
		for(const auto & entry : category.second.Struct())
		{
			JsonNode preset = object.getPresetVariable(category.first, entry.first);
			const JsonNode & input = preset.isNull() ? entry.second : preset;
			int32_t value = -1;

			if (category.first == "number")
				value = randomizer.loadValue(input, object.variables.values);

			if (category.first == "artifact")
				value = randomizer.loadArtifact(input, object.variables.values).getNum();

			if (category.first == "spell")
				value = randomizer.loadSpell(input, object.variables.values).getNum();

			if (category.first == "primarySkill")
				value = randomizer.loadPrimary(input, object.variables.values).getNum();

			if (category.first == "secondarySkill")
				value = randomizer.loadSecondary(input, object.variables.values).getNum();

			object.initVariable(category.first, entry.first, value);
		}
	}
}

void Rewardable::Info::replaceTextPlaceholders(MetaString & target, const Variables & variables) const
{
	for (const auto & variable : variables.values )
	{
		if( boost::algorithm::starts_with(variable.first, "spell"))
			target.replaceName(SpellID(variable.second));

		if( boost::algorithm::starts_with(variable.first, "secondarySkill"))
			target.replaceName(SecondarySkill(variable.second));
	}
}

void Rewardable::Info::replaceTextPlaceholders(MetaString & target, const Variables & variables, const VisitInfo & info) const
{
	if (!info.reward.guards.empty())
	{
		replaceTextPlaceholders(target, variables);

		CreatureID strongest = info.reward.guards.at(0).getId();

		for (const auto & guard : info.reward.guards )
		{
			if (strongest.toEntity(LIBRARY)->getFightValue() < guard.getId().toEntity(LIBRARY)->getFightValue())
				strongest = guard.getId();
		}
		target.replaceNamePlural(strongest); // FIXME: use singular if only 1 such unit is in guards

		MetaString loot;

		for (GameResID it : LIBRARY->resourceTypeHandler->getAllObjects())
		{
			if (info.reward.resources[it] != 0)
			{
				loot.appendRawString("%d %s");
				loot.replaceNumber(info.reward.resources[it]);
				loot.replaceName(it);
			}
		}

		for (const auto & artifact : info.reward.grantedArtifacts )
		{
			loot.appendRawString("%s");
			loot.replaceName(artifact);
		}

		for (const auto & scroll : info.reward.grantedScrolls )
		{
			loot.appendRawString("%s");
			loot.replaceName(scroll);
		}

		for (const auto & spell : info.reward.spells )
		{
			loot.appendRawString("%s");
			loot.replaceName(spell);
		}

		for (const auto & secondary : info.reward.secondary )
		{
			loot.appendRawString("%s");
			loot.replaceName(secondary.first);
		}

		target.replaceRawString(loot.buildList());
	}
	else
	{
		for (const auto & artifact : info.reward.grantedArtifacts )
			target.replaceName(artifact);

		for (const auto & scroll : info.reward.grantedScrolls )
			target.replaceName(scroll);

		for (const auto & spell : info.reward.spells )
			target.replaceName(spell);

		for (const auto & secondary : info.reward.secondary )
			target.replaceName(secondary.first);

		replaceTextPlaceholders(target, variables);
	}
}

void Rewardable::Info::configureRewards(
		Rewardable::Configuration & object,
		IGameRandomizer & gameRandomizer,
		IGameInfoCallback * cb,
		const JsonNode & source,
		Rewardable::EEventType event,
		const std::string & modeName) const
{
	for(size_t i = 0; i < source.Vector().size(); ++i)
	{
		const JsonNode & reward = source.Vector().at(i);

		auto diceValue = object.getPresetVariable("dice", "map");

		if (!diceValue.isNull())
		{
			if (!reward["mapDice"].isNull() && reward["mapDice"] != diceValue)
				continue;
		}
		else
		{
			if (!reward["appearChance"].isNull())
			{
				const JsonNode & chance = reward["appearChance"];
				std::string diceID = std::to_string(chance["dice"].Integer());

				auto diceValue = object.getVariable("dice", diceID);
				if (!diceValue.has_value())
				{
					object.initVariable("dice", diceID, gameRandomizer.getDefault().nextInt(0, 99));
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
		}

		Rewardable::VisitInfo info;
		configureLimiter(object, gameRandomizer, cb, info.limiter, reward["limiter"]);
		configureReward(object, gameRandomizer, cb, info.reward, reward);

		info.visitType = event;
		info.message = loadMessage(reward["message"], TextIdentifier(objectTextID, modeName, i));
		info.description = loadMessage(reward["description"], TextIdentifier(objectTextID, "description", modeName, i), EMetaText::GENERAL_TXT);

		replaceTextPlaceholders(info.message, object.variables, info);
		replaceTextPlaceholders(info.description, object.variables, info);

		object.info.push_back(info);
	}
}

void Rewardable::Info::configureObject(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb) const
{
	object.info.clear();
	object.variables.values.clear();

	configureVariables(object, gameRandomizer, cb, parameters["variables"]);

	configureRewards(object, gameRandomizer, cb, parameters["rewards"], Rewardable::EEventType::EVENT_FIRST_VISIT, "rewards");
	configureRewards(object, gameRandomizer, cb, parameters["onVisited"], Rewardable::EEventType::EVENT_ALREADY_VISITED, "onVisited");
	configureRewards(object, gameRandomizer, cb, parameters["onEmpty"], Rewardable::EEventType::EVENT_NOT_AVAILABLE, "onEmpty");

	object.onSelect = loadMessage(parameters["onSelectMessage"], TextIdentifier(objectTextID, "onSelect"));
	object.description = loadMessage(parameters["description"], TextIdentifier(objectTextID, "description"));
	object.notVisitedTooltip = loadMessage(parameters["notVisitedTooltip"], TextIdentifier(objectTextID, "notVisitedTooltip"), EMetaText::GENERAL_TXT);
	object.visitedTooltip = loadMessage(parameters["visitedTooltip"], TextIdentifier(objectTextID, "visitedTooltip"), EMetaText::GENERAL_TXT);

	if (object.notVisitedTooltip.empty())
		object.notVisitedTooltip.appendTextID("core.genrltxt.353");

	if (object.visitedTooltip.empty())
		object.visitedTooltip.appendTextID("core.genrltxt.352");

	if (!parameters["onVisitedMessage"].isNull())
	{
		Rewardable::VisitInfo onVisited;
		onVisited.visitType = Rewardable::EEventType::EVENT_ALREADY_VISITED;
		onVisited.message = loadMessage(parameters["onVisitedMessage"], TextIdentifier(objectTextID, "onVisited"));
		replaceTextPlaceholders(onVisited.message, object.variables);

		object.info.push_back(onVisited);
	}

	if (!parameters["onEmptyMessage"].isNull())
	{
		Rewardable::VisitInfo onEmpty;
		onEmpty.visitType = Rewardable::EEventType::EVENT_NOT_AVAILABLE;
		onEmpty.message = loadMessage(parameters["onEmptyMessage"], TextIdentifier(objectTextID, "onEmpty"));
		replaceTextPlaceholders(onEmpty.message, object.variables);

		object.info.push_back(onEmpty);
	}

	if (!parameters["onGuardedMessage"].isNull())
	{
		Rewardable::VisitInfo onGuarded;
		onGuarded.visitType = Rewardable::EEventType::EVENT_GUARDED;
		onGuarded.message = loadMessage(parameters["onGuardedMessage"], TextIdentifier(objectTextID, "onGuarded"));
		replaceTextPlaceholders(onGuarded.message, object.variables);

		object.info.push_back(onGuarded);
	}

	configureResetInfo(object, gameRandomizer, object.resetParameters, parameters["resetParameters"]);

	object.canRefuse = parameters["canRefuse"].Bool();
	object.showScoutedPreview = parameters["showScoutedPreview"].Bool();
	object.forceCombat = parameters["forceCombat"].Bool();
	object.guardsLayout = parameters["guardsLayout"].String();
	object.coastVisitable = parameters["coastVisitable"].Bool();

	if(parameters["showInInfobox"].isNull())
		object.infoWindowType = EInfoWindowMode::AUTO;
	else
		object.infoWindowType = parameters["showInInfobox"].Bool() ? EInfoWindowMode::INFO : EInfoWindowMode::MODAL;
	
	auto visitMode = parameters["visitMode"].String();
	for(int i = 0; i < Rewardable::VisitModeString.size(); ++i)
	{
		if(Rewardable::VisitModeString[i] == visitMode)
		{
			object.visitMode = static_cast<EVisitMode>(i);
			break;
		}
	}
	
	auto selectMode = parameters["selectMode"].String();
	for(int i = 0; i < Rewardable::SelectModeString.size(); ++i)
	{
		if(Rewardable::SelectModeString[i] == selectMode)
		{
			object.selectMode = static_cast<ESelectMode>(i);
			break;
		}
	}

	if (object.visitMode == Rewardable::VISIT_LIMITER)
		configureLimiter(object, gameRandomizer, cb, object.visitLimiter, parameters["visitLimiter"]);

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

bool Rewardable::Info::hasGuards() const
{
	return testForKey(parameters, "guards");
}

const JsonNode & Rewardable::Info::getParameters() const
{
	return parameters;
}

VCMI_LIB_NAMESPACE_END

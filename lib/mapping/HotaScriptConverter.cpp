/*
 * HotaScriptConverter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HotaScriptConverter.h"

#include "MapReaderH3M.h"

#include "../constants/EntityIdentifiers.h"
#include "../texts/TextIdentifier.h"

namespace
{
enum class HotaScriptActions : int32_t
{
	CONDITIONAL_CHAIN = 1,
	SET_VARIABLE_CONDITIONAL = 2,
	MODIFY_VARIABLE = 3,
	RESOURCES = 4,
	REMOVE_CURRENT_OBJECT_OR_FINISH_QUEST = 5,
	SHOW_REWARDS_MESSAGE = 6,
	QUEST_ACTION = 7,
	CREATURES = 8,
	ARTIFACT = 9,
	CONSTRUCT_BUILDING = 10,
	SET_QUEST_HINT = 11,
	SHOW_QUESTION = 12,
	CONDITIONAL = 13,
	CREATURES_TO_HIRE = 14,
	SPELL = 15,
	EXPERIENCE = 16,
	SPELL_POINTS = 17,
	MOVEMENT_POINTS = 18,
	PRIMARY_SKILL = 19,
	SECONDARY_SKILL = 20,
	LUCK = 21,
	MORALE = 22,
	START_COMBAT = 23,
	EXECUTE_EVENT = 24,
	WAR_MACHINE = 25,
	SPELLBOOK = 26,
	DISABLE_EVENT = 27,
	LOOP_FOR = 28,
	SHOW_MESSAGE = 29
};

enum class HotaScriptCondition : int32_t
{
	CONSTANT = 0,
	ALL_OF = 1,
	ANY_OF = 2,
	LESSER = 3,
	GREATER = 4,
	EQUAL = 5,
	NOT = 6,
	HAS_ARTIFACT = 7,
	GREATER_OR_EQUAL = 8,
	LESSER_OR_EQUAL = 9,
	NOT_EQUAL = 10,
	CURRENT_PLAYER = 11,
	HERO_OWNER = 12,
	PLAYER_DEFEATED_MONSTER = 14,
	PLAYER_DEFEATED_HERO = 15,
	HERO_SECONDARY_SKILL = 16,
	PLAYER_DEFEATED = 17,
	PLAYER_OWNS_TOWN = 18,
	PLAYER_IS_HUMAN = 19,
	PLAYER_STARTING_FACTION = 20,
	TOWN_IS_NEUTRAL = 21
};

enum class HotaScriptExpression : int32_t
{
	INTEGER_VALUE = 0,
	VARIABLE_VALUE = 1,
	NEGATE = 2,
	ADD = 3,
	SUBSTRACT = 4,
	RESOURCE = 5,
	MULTIPLY = 6,
	DIVIDE = 7,
	REMAINDER = 8,
	CREATURE_COUNT_IN_ARMY = 9,
	CURRENT_DIFFICULTY = 10,
	COMPARE_DIFFICULTY = 11,
	CURRENT_DATE = 12,
	HERO_EXPERIENCE = 13,
	HERO_LEVEL = 14,
	HERO_PRIMARY_SKILL = 15,
	RANDOM_NUMBER = 16,
	HERO_OWNED_ARTIFACTS = 17
};

std::string num(int64_t value)
{
	return std::to_string(value);
}

std::string boolStr(bool value)
{
	return value ? "true" : "false";
}

/// EXECUTE_EVENT stores the target bucket as an integer in map order.
std::string bucketName(int eventType)
{
	switch(eventType)
	{
		case 0: return "heroEvents";
		case 1: return "playerEvents";
		case 2: return "townEvents";
		case 3: return "questEvents";
		default: return "heroEvents";
	}
}
}

HotaScriptConverter::HotaScriptConverter(MapReaderH3M & reader, std::string mapName, LocalizeCallback localizeString)
	: reader(reader)
	, mapName(std::move(mapName))
	, localizeString(std::move(localizeString))
{
}

std::string HotaScriptConverter::convert()
{
	bool eventsSystemActive = reader.readBool();
	if(!eventsSystemActive)
		return {};

	std::string result;
	result += "-- generated from " + mapName + ".h3m HotA event system\n";
	result += "local Map = {\n";
	result += "\theroEvents = {},\n";
	result += "\tplayerEvents = {},\n";
	result += "\ttownEvents = {},\n";
	result += "\tquestEvents = {},\n";
	result += "}\n\n";

	result += loadEventList("heroEvents");
	result += loadEventList("playerEvents");
	result += loadEventList("townEvents");
	result += loadEventList("questEvents");

	reader.readInt32(); // next variable ID
	reader.readInt32(); // next hero event ID
	reader.readInt32(); // next player event ID
	reader.readInt32(); // next town event ID
	reader.readInt32(); // next quest event ID

	result += loadVariables();

	loadEventMap(); // hero events
	loadEventMap(); // player events
	loadEventMap(); // town events
	loadEventMap(); // quest events
	loadEventMap(); // variables

	result += "\nreturn Map\n";
	return result;
}

std::string HotaScriptConverter::loadEventList(const std::string & bucket)
{
	std::string result;
	int eventsCount = reader.readInt32();
	for(int i = 0; i < eventsCount; ++i)
	{
		int eventID = reader.readInt32();
		currentBucket = bucket;
		currentEventID = eventID;
		stringCounter = 0;

		std::string body = loadActions(1);
		std::string eventName = reader.readBaseString(); // internal name, not shown to players

		result += "-- \"" + eventName + "\" (" + bucket + " id " + std::to_string(eventID) + ")\n";
		result += "Map." + bucket + "[" + std::to_string(eventID) + "] = function(ctx)\n";
		result += body;
		result += "end\n\n";
	}
	return result;
}

std::string HotaScriptConverter::loadVariables()
{
	int variablesCount = reader.readInt32();
	std::string result;
	if(variablesCount > 0)
		result += "\n-- variables (managed by engine, listed for reference)\n";
	for(int i = 0; i < variablesCount; ++i)
	{
		int uniqueID = reader.readInt32();
		std::string variableID = reader.readBaseString();
		bool persistInCampaign = reader.readBool();
		bool importFromPrevMap = reader.readBool();
		int initialValue = reader.readInt32();
		result += "--   [" + std::to_string(uniqueID) + "] " + variableID + " = " + std::to_string(initialValue)
			+ (persistInCampaign ? " (persist)" : "")
			+ (importFromPrevMap ? " (import)" : "") + "\n";
	}
	return result;
}

void HotaScriptConverter::loadEventMap()
{
	int mappingSize = reader.readInt32();
	for(int i = 0; i < mappingSize; ++i)
		reader.readInt32(); // UID of map object bound to an event
}

std::string HotaScriptConverter::localizedText(const std::string & role)
{
	TextIdentifier identifier(currentBucket, static_cast<size_t>(currentEventID), role, static_cast<size_t>(stringCounter++));
	return '"' + localizeString(identifier) + '"';
}

std::string HotaScriptConverter::loadImageList(int count)
{
	std::string result;
	for(int i = 0; i < count; ++i)
	{
		int imageType = reader.readInt32();
		int imageSubtype = reader.readInt32();
		std::string amount = loadExpression();
		if(i != 0)
			result += ", ";
		result += "{" + num(imageType) + ", " + num(imageSubtype) + ", " + amount + "}";
	}
	return result;
}

std::string HotaScriptConverter::loadActions(int indent)
{
	reader.readInt32(); // block marker, always 1
	reader.readInt8(); // always 0

	std::string pad(indent, '\t');
	std::string inner(indent + 1, '\t');
	std::string result;

	int actionsCount = reader.readInt32();
	for(int j = 0; j < actionsCount; ++j)
	{
		auto actionType = static_cast<HotaScriptActions>(reader.readInt32());
		switch(actionType)
		{
			case HotaScriptActions::SHOW_MESSAGE:
			{
				std::string text = localizedText("message");
				int numberOfImages = reader.readInt32();
				std::string images = loadImageList(numberOfImages);
				result += pad + "ctx:showMessage(" + text + ", {" + images + "})\n";
				break;
			}
			case HotaScriptActions::SHOW_REWARDS_MESSAGE:
			{
				std::string text = localizedText("rewardMessage");
				std::string body = loadActions(indent + 1);
				result += pad + "ctx:showRewardsMessage(" + text + ", function()\n" + body + pad + "end)\n";
				break;
			}
			case HotaScriptActions::REMOVE_CURRENT_OBJECT_OR_FINISH_QUEST:
			{
				result += pad + "ctx:finishQuestOrRemoveObject()\n";
				break;
			}
			case HotaScriptActions::DISABLE_EVENT:
			{
				result += pad + "ctx:disableEvent()\n";
				break;
			}
			case HotaScriptActions::QUEST_ACTION:
			{
				std::string condition = loadCondition();
				std::string proposal = localizedText("questProposal");
				std::string progression = localizedText("questProgression");
				std::string completion = localizedText("questCompletion");
				std::string hint = localizedText("questHint");
				std::string reward = loadActions(indent + 2);
				reader.readBool(); // always 1

				result += pad + "ctx:registerQuest{\n";
				result += inner + "proposal = " + proposal + ",\n";
				result += inner + "progression = " + progression + ",\n";
				result += inner + "completion = " + completion + ",\n";
				result += inner + "hint = " + hint + ",\n";
				result += inner + "condition = function() return " + condition + " end,\n";
				result += inner + "reward = function()\n" + reward + inner + "end,\n";
				result += pad + "}\n";
				break;
			}
			case HotaScriptActions::CONDITIONAL:
			{
				std::string condition = loadCondition();
				std::string thenBody = loadActions(indent + 1);
				std::string elseBody = loadActions(indent + 1);
				result += pad + "if " + condition + " then\n" + thenBody + pad + "else\n" + elseBody + pad + "end\n";
				break;
			}
			case HotaScriptActions::CONDITIONAL_CHAIN:
			{
				bool first = true;
				for(;;)
				{
					std::string condition = loadCondition();
					std::string body = loadActions(indent + 1);
					result += pad + (first ? "if " : "elseif ") + condition + " then\n" + body;
					first = false;

					reader.readBool(); // always 1
					int hasMoreBranches = reader.readInt32();
					if(hasMoreBranches == 0)
						break;
				}
				reader.readInt32(); // unknown trailing value
				result += pad + "end\n";
				break;
			}
			case HotaScriptActions::LOOP_FOR:
			{
				std::string body = loadActions(indent + 1); // body precedes bounds on disk
				std::string from = loadExpression();
				std::string to = loadExpression();
				int variableID = reader.readInt32();
				result += pad + "for __i = " + from + ", " + to + " do\n";
				result += inner + "ctx:setVariable(" + num(variableID) + ", __i)\n";
				result += body;
				result += pad + "end\n";
				break;
			}
			case HotaScriptActions::SET_QUEST_HINT:
			{
				std::string text = localizedText("questHint");
				int numberOfImages = reader.readInt32();
				std::string images = loadImageList(numberOfImages);
				bool showInLog = reader.readBool();
				result += pad + "ctx:setQuestHint(" + text + ", {" + images + "}, " + boolStr(showInLog) + ")\n";
				break;
			}
			case HotaScriptActions::SHOW_QUESTION:
			{
				// 0 = no images, 1 = no exit, 2 = can exit, 3 = specify images
				int mode = reader.readInt8();
				std::string text = localizedText("question");
				std::string onYes = loadActions(indent + 2);
				std::string onNo = loadActions(indent + 2);
				std::string onCancel;
				bool hasCancel = mode == 2;
				if(hasCancel)
					onCancel = loadActions(indent + 2);

				int numberOfImages = 2;
				if(mode == 0 || mode == 3)
					numberOfImages = reader.readInt32();
				std::string images = loadImageList(numberOfImages);

				if(mode == 1 || mode == 2)
				{
					reader.readBool(); // show OR between images
					reader.readInt32(); // unknown
				}

				result += pad + "ctx:showQuestion{\n";
				result += inner + "text = " + text + ",\n";
				result += inner + "mode = " + num(mode) + ",\n";
				result += inner + "images = {" + images + "},\n";
				result += inner + "onYes = function()\n" + onYes + inner + "end,\n";
				result += inner + "onNo = function()\n" + onNo + inner + "end,\n";
				if(hasCancel)
					result += inner + "onCancel = function()\n" + onCancel + inner + "end,\n";
				result += pad + "}\n";
				break;
			}
			case HotaScriptActions::ARTIFACT:
			{
				bool take = reader.readBool();
				ArtifactID artifact = reader.readArtifact32();
				SpellID scrollSpell = reader.readSpell32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantArtifact(" + boolStr(take) + ", " + num(artifact.getNum()) + ", " + num(scrollSpell.getNum()) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::WAR_MACHINE:
			{
				bool take = reader.readBool();
				ArtifactID machine = reader.readArtifact32();
				reader.skipUnused(4); // garbage padding
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantWarMachine(" + boolStr(take) + ", " + num(machine.getNum()) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::SPELL:
			{
				SpellID spell = reader.readSpell32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantSpell(" + num(spell.getNum()) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::SPELLBOOK:
			{
				bool take = reader.readBool();
				reader.skipUnused(8); // garbage padding
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantSpellbook(" + boolStr(take) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::CREATURES:
			{
				bool take = reader.readBool();
				CreatureID creature = reader.readCreature32();
				std::string count = loadExpression();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantCreatures(" + boolStr(take) + ", " + num(creature.getNum()) + ", " + count + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::START_COMBAT:
			{
				std::string slots;
				for(int i = 0; i < 7; ++i)
				{
					std::string count = loadExpression();
					CreatureID creature = reader.readCreature32();
					if(i != 0)
						slots += ", ";
					slots += "{" + count + ", " + num(creature.getNum()) + "}";
				}
				result += pad + "ctx:startCombat({" + slots + "})\n";
				break;
			}
			case HotaScriptActions::SECONDARY_SKILL:
			{
				int mastery = reader.readInt32();
				SecondarySkill skill = reader.readSkill32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantSecondarySkill(" + num(skill.getNum()) + ", " + num(mastery) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::MORALE:
			{
				int amount = reader.readInt32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantMorale(" + num(amount) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::LUCK:
			{
				int amount = reader.readInt32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantLuck(" + num(amount) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::EXPERIENCE:
			{
				std::string amount = loadExpression();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantExperience(" + amount + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::SPELL_POINTS:
			{
				std::string amount = loadExpression();
				int mode = reader.readInt32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantSpellPoints(" + amount + ", " + num(mode) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::MOVEMENT_POINTS:
			{
				std::string amount = loadExpression();
				int mode = reader.readInt32();
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantMovementPoints(" + amount + ", " + num(mode) + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::CREATURES_TO_HIRE:
			{
				int dwelling = reader.readInt32();
				std::string amount = loadExpression();
				int unknown = reader.readInt32(); // TBD: possibly factory 8th dwelling marker
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantCreaturesToHire(" + num(dwelling) + ", " + amount + ", " + boolStr(showMessage) + ") -- unknown=" + num(unknown) + "\n";
				break;
			}
			case HotaScriptActions::CONSTRUCT_BUILDING:
			{
				BuildingID building = reader.readBuilding32(std::nullopt);
				int unknownA = reader.readInt16(); // faction ID?
				int unknownB = reader.readInt16(); // faction building ID?
				bool showMessage = reader.readBool();
				result += pad + "ctx:constructBuilding(" + num(building.getNum()) + ", " + boolStr(showMessage) + ") -- faction fields " + num(unknownA) + "/" + num(unknownB) + "\n";
				break;
			}
			case HotaScriptActions::EXECUTE_EVENT:
			{
				int eventType = reader.readInt32();
				int eventID = reader.readInt32();
				result += pad + "Map." + bucketName(eventType) + "[" + num(eventID) + "](ctx)\n";
				break;
			}
			case HotaScriptActions::RESOURCES:
			{
				int mode = reader.readInt8();
				std::string amounts;
				for(int i = 0; i < 7; ++i)
				{
					if(i != 0)
						amounts += ", ";
					amounts += loadExpression();
				}
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantResources(" + num(mode) + ", {" + amounts + "}, " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::PRIMARY_SKILL:
			{
				std::string amount = loadExpression();
				PrimarySkill skill(reader.readInt32());
				bool showMessage = reader.readBool();
				result += pad + "ctx:grantPrimarySkill(" + num(skill.getNum()) + ", " + amount + ", " + boolStr(showMessage) + ")\n";
				break;
			}
			case HotaScriptActions::MODIFY_VARIABLE:
			{
				int variableID = reader.readInt32();
				int mode = reader.readInt8(); // 0 = add, 1 = subtract, 2 = set
				std::string value = loadExpressionInternal();
				std::string id = num(variableID);
				if(mode == 0)
					result += pad + "ctx:setVariable(" + id + ", (ctx:getVariable(" + id + ") + " + value + "))\n";
				else if(mode == 1)
					result += pad + "ctx:setVariable(" + id + ", (ctx:getVariable(" + id + ") - " + value + "))\n";
				else
					result += pad + "ctx:setVariable(" + id + ", " + value + ")\n";
				break;
			}
			case HotaScriptActions::SET_VARIABLE_CONDITIONAL:
			{
				int variableID = reader.readInt32();
				std::string condition = loadCondition();
				std::string valueTrue = loadExpression();
				std::string valueFalse = loadExpression();
				std::string id = num(variableID);
				result += pad + "if " + condition + " then\n";
				result += inner + "ctx:setVariable(" + id + ", " + valueTrue + ")\n";
				result += pad + "else\n";
				result += inner + "ctx:setVariable(" + id + ", " + valueFalse + ")\n";
				result += pad + "end\n";
				break;
			}
			default:
				throw std::runtime_error("Unknown event action code:" + std::to_string(static_cast<int>(actionType)));
		}
	}
	return result;
}

std::string HotaScriptConverter::loadCondition()
{
	reader.readBool(); // always true
	return loadConditionInternal();
}

std::string HotaScriptConverter::loadConditionInternal()
{
	auto conditionCode = static_cast<HotaScriptCondition>(reader.readInt32());
	switch(conditionCode)
	{
		case HotaScriptCondition::CONSTANT:
			return boolStr(reader.readBool());
		case HotaScriptCondition::ANY_OF:
		case HotaScriptCondition::ALL_OF:
		{
			const std::string separator = conditionCode == HotaScriptCondition::ALL_OF ? " and " : " or ";
			int argumentsCount = reader.readInt32();
			if(argumentsCount == 0)
				return conditionCode == HotaScriptCondition::ALL_OF ? "true" : "false";
			std::string result = "(";
			for(int i = 0; i < argumentsCount; ++i)
			{
				if(i != 0)
					result += separator;
				result += loadConditionInternal();
			}
			return result + ")";
		}
		case HotaScriptCondition::LESSER:
		case HotaScriptCondition::GREATER:
		case HotaScriptCondition::EQUAL:
		case HotaScriptCondition::GREATER_OR_EQUAL:
		case HotaScriptCondition::LESSER_OR_EQUAL:
		case HotaScriptCondition::NOT_EQUAL:
		{
			std::string left = loadExpression();
			std::string right = loadExpression();
			std::string op;
			switch(conditionCode)
			{
				case HotaScriptCondition::LESSER: op = " < "; break;
				case HotaScriptCondition::GREATER: op = " > "; break;
				case HotaScriptCondition::EQUAL: op = " == "; break;
				case HotaScriptCondition::GREATER_OR_EQUAL: op = " >= "; break;
				case HotaScriptCondition::LESSER_OR_EQUAL: op = " <= "; break;
				default: op = " ~= "; break;
			}
			return "(" + left + op + right + ")";
		}
		case HotaScriptCondition::NOT:
			return "(not " + loadCondition() + ")";
		case HotaScriptCondition::HAS_ARTIFACT:
		{
			ArtifactID artifact = reader.readArtifact32();
			SpellID scrollSpell = reader.readSpell32();
			return "ctx:hasArtifact(" + num(artifact.getNum()) + ", " + num(scrollSpell.getNum()) + ")";
		}
		case HotaScriptCondition::CURRENT_PLAYER:
		{
			PlayerColor player = reader.readPlayer32();
			return "(ctx:currentPlayer() == " + num(player.getNum()) + ")";
		}
		case HotaScriptCondition::HERO_OWNER:
		{
			HeroTypeID hero = reader.readHero32();
			PlayerColor player = reader.readPlayer32(); // -2 = current hero, -1 = current player
			return "ctx:heroOwner(" + num(hero.getNum()) + ", " + num(player.getNum()) + ")";
		}
		case HotaScriptCondition::HERO_SECONDARY_SKILL:
		{
			SecondarySkill skill = reader.readSkill32();
			int mastery = reader.readInt32();
			return "ctx:heroSecondarySkill(" + num(skill.getNum()) + ", " + num(mastery) + ")";
		}
		case HotaScriptCondition::TOWN_IS_NEUTRAL:
			return "ctx:townIsNeutral()";
		case HotaScriptCondition::PLAYER_DEFEATED:
		{
			PlayerColor player = reader.readPlayer32();
			return "ctx:playerDefeated(" + num(player.getNum()) + ")";
		}
		case HotaScriptCondition::PLAYER_IS_HUMAN:
		{
			PlayerColor player = reader.readPlayer32();
			return "ctx:playerIsHuman(" + num(player.getNum()) + ")";
		}
		case HotaScriptCondition::PLAYER_STARTING_FACTION:
		{
			PlayerColor player = reader.readPlayer32();
			FactionID faction = reader.readFaction32();
			return "ctx:playerStartingFaction(" + num(player.getNum()) + ", " + num(faction.getNum()) + ")";
		}
		case HotaScriptCondition::PLAYER_DEFEATED_MONSTER:
		{
			PlayerColor player = reader.readPlayer32();
			int targetObjectID = reader.readInt32();
			return "ctx:playerDefeatedMonster(" + num(player.getNum()) + ", " + num(targetObjectID) + ")";
		}
		case HotaScriptCondition::PLAYER_DEFEATED_HERO:
		{
			PlayerColor player = reader.readPlayer32();
			int targetObjectID = reader.readInt32();
			return "ctx:playerDefeatedHero(" + num(player.getNum()) + ", " + num(targetObjectID) + ")";
		}
		case HotaScriptCondition::PLAYER_OWNS_TOWN:
		{
			PlayerColor player = reader.readPlayer32();
			int targetObjectID = reader.readInt32();
			return "ctx:playerOwnsTown(" + num(player.getNum()) + ", " + num(targetObjectID) + ")";
		}
		default:
			throw std::runtime_error("Unknown event condition code:" + std::to_string(static_cast<int>(conditionCode)));
	}
}

std::string HotaScriptConverter::loadExpression()
{
	bool isExpression = reader.readBool();
	if(!isExpression)
		return num(reader.readInt32());
	return loadExpressionInternal();
}

std::string HotaScriptConverter::loadExpressionInternal()
{
	reader.readBool(); // always true
	auto expressionCode = static_cast<HotaScriptExpression>(reader.readInt32());
	switch(expressionCode)
	{
		case HotaScriptExpression::INTEGER_VALUE:
			return num(reader.readInt32());
		case HotaScriptExpression::VARIABLE_VALUE:
			return "ctx:getVariable(" + num(reader.readInt32()) + ")";
		case HotaScriptExpression::NEGATE:
		{
			reader.readInt32(); // always 1
			return "(-(" + loadExpression() + "))";
		}
		case HotaScriptExpression::ADD:
		case HotaScriptExpression::SUBSTRACT:
		case HotaScriptExpression::MULTIPLY:
		case HotaScriptExpression::DIVIDE:
		case HotaScriptExpression::REMAINDER:
		{
			std::string left = loadExpressionInternal();
			std::string right = loadExpressionInternal();
			switch(expressionCode)
			{
				case HotaScriptExpression::ADD: return "(" + left + " + " + right + ")";
				case HotaScriptExpression::SUBSTRACT: return "(" + left + " - " + right + ")";
				case HotaScriptExpression::MULTIPLY: return "(" + left + " * " + right + ")";
				case HotaScriptExpression::DIVIDE: return "math.floor(" + left + " / " + right + ")"; // HotA expressions are integer
				default: return "(" + left + " % " + right + ")";
			}
		}
		case HotaScriptExpression::RESOURCE:
		{
			PlayerColor player = reader.readPlayer(); // special value for current player
			GameResID resource = reader.readGameResID32();
			return "ctx:resource(" + num(player.getNum()) + ", " + num(resource.getNum()) + ")";
		}
		case HotaScriptExpression::CREATURE_COUNT_IN_ARMY:
		{
			CreatureID creature = reader.readCreature32();
			return "ctx:creatureCountInArmy(" + num(creature.getNum()) + ")";
		}
		case HotaScriptExpression::CURRENT_DIFFICULTY:
			return "ctx:currentDifficulty()";
		case HotaScriptExpression::COMPARE_DIFFICULTY:
			return "ctx:compareDifficulty(" + num(reader.readInt32()) + ")";
		case HotaScriptExpression::CURRENT_DATE:
			return "ctx:currentDate()";
		case HotaScriptExpression::HERO_EXPERIENCE:
			return "ctx:heroExperience()";
		case HotaScriptExpression::HERO_LEVEL:
			return "ctx:heroLevel()";
		case HotaScriptExpression::HERO_PRIMARY_SKILL:
		{
			PrimarySkill skill(reader.readInt32());
			return "ctx:heroPrimarySkill(" + num(skill.getNum()) + ")";
		}
		case HotaScriptExpression::RANDOM_NUMBER:
		{
			std::string low = loadExpression();
			std::string high = loadExpression();
			return "ctx:random(" + low + ", " + high + ")";
		}
		case HotaScriptExpression::HERO_OWNED_ARTIFACTS:
		{
			ArtifactID artifact = reader.readArtifact32();
			SpellID scrollSpell = reader.readSpell32();
			return "ctx:heroOwnedArtifacts(" + num(artifact.getNum()) + ", " + num(scrollSpell.getNum()) + ")";
		}
		default:
			throw std::runtime_error("Unknown event expression code:" + std::to_string(static_cast<int>(expressionCode)));
	}
}

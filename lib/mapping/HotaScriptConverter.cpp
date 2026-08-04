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

#include "../GameConstants.h"
#include "../constants/EntityIdentifiers.h"
#include "../texts/TextIdentifier.h"
#include "mapping/CMap.h"

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

static std::string num(int64_t value)
{
	return std::to_string(value);
}

static std::string boolStr(bool value)
{
	return value ? "true" : "false";
}

/// Renders a string as a quoted Lua literal. Only used for editor-supplied variable names, which
/// are expected to be plain identifiers - anything needing escaping means a malformed map.
static std::string luaString(const std::string & value)
{
	if(value.find_first_of("\\\"\n\r") != std::string::npos)
		throw std::runtime_error("Script variable name contains unsupported characters: " + value);

	return '"' + value + '"';
}

/// Reference to a script variable by its HotA unique id, resolved to a name through the
/// generated `Vars` table so scripts read as named access.
static std::string varRef(int uniqueID)
{
	return "Vars[" + std::to_string(uniqueID) + "]";
}

/// Renders a content identifier as a quoted Lua string holding its JSON key
template<typename IdentifierType>
static std::string entityKey(IdentifierType identifier)
{
	return '"' + IdentifierType::encode(identifier.getNum()) + '"';
}

/// Player colours are exposed as an enum group; HotA stores them as raw indices.
static std::string playerRef(PlayerColor player)
{
	// HotA's negative "current player" sentinels have no colour name of their own
	if(!player.isValidPlayer())
		return num(player.getNum());

	return "ENUM.PlayerColor." + PlayerColor::encode(player.getNum());
}

/// Primary skills are exposed as an enum group rather than a content catalogue.
static std::string primarySkillRef(PrimarySkill skill)
{
	return "ENUM.PrimarySkill." + PrimarySkill::encode(skill.getNum());
}

/// Resolves a content identifier through the Services catalogue, which is what the bindings expect.
/// `lookup` is the Services method name, e.g. `getCreatureByName`.
template<typename IdentifierType>
static std::string entityRef(const std::string & lookup, IdentifierType identifier)
{
	return "LIBRARY:" + lookup + "(" + entityKey(identifier) + ")";
}

/// EXECUTE_EVENT stores the target bucket as an integer in map order.
static std::string bucketName(int eventType)
{
	switch(eventType)
	{
		case 0: return "heroEvents";
		case 1: return "playerEvents";
		case 2: return "townEvents";
		case 3: return "questEvents";
		default: throw std::runtime_error("Unknown event bucket code:" + std::to_string(eventType));
	}
}

std::string HotaScriptConverter::eventHandlerName(const std::string & bucket, int eventID)
{
	// Used verbatim as a Lua method name (Map:<name>) and as the engine-side handler key,
	// so it must be a valid Lua identifier - hence '_' rather than '.' as the separator.
	return bucket + "_" + std::to_string(eventID);
}

/// Shape of a generated event handler, decided by the bucket it belongs to.
struct BucketTraits
{
	/// Parameter list without parentheses; the engine dispatcher supplies these, and EXECUTE_EVENT
	/// forwards them verbatim - two buckets can only call each other when their lists match
	std::string args;
	/// Statement establishing `player` for player sentinels and player-scoped calls,
	/// empty when the handler already receives it as a parameter
	std::string playerLocal;
	/// Name of the removable object in scope, used by the object-removing actions
	std::string subject;
};

static const BucketTraits & bucketTraits(const std::string & bucket)
{
	static const BucketTraits town{"game, server, town", "\tlocal player = town:getOwner()\n", "town"};
	// player events carry no object, so `subject` has nothing to name for them
	static const BucketTraits player{"game, server, player", "", "object"};
	static const BucketTraits object{"game, server, object, hero", "\tlocal player = hero:getOwner()\n", "object"};

	if(bucket == "townEvents")
		return town;
	if(bucket == "playerEvents")
		return player;
	return object; // heroEvents, questEvents
}

/// Free helpers shared by every generated handler: player-sentinel resolution, HotA image
/// triples to engine Component tables, the resource-set loop and the object-identity predicates.
static std::string helpersPrelude()
{
	// HotA addresses resources by index while the bindings take a resource handle, so the generated
	// script carries the engine's resource order to translate between the two.
	std::string resourceOrder;
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
		resourceOrder += (i == 0 ? "" : ", ") + entityKey(GameResID(i));

	return "-- HotA event helpers\n\nlocal RESOURCES = {" + resourceOrder + "}\n" + R"lua(
-- HotA player sentinels: negative values mean "the event's own player".
local function resolvePlayer(player, eventPlayer)
	if player < 0 then return eventPlayer end
	return player
end

-- Converts HotA image triples {kind, subtype, amount} to engine Component tables.
local function toComponents(images)
	local result = {}
	for _, image in ipairs(images) do
		result[#result + 1] = {type = image[1], subType = image[2], value = image[3]}
	end
	return result
end

-- Adds a whole resource set to a player; the amounts follow the engine resource order.
local function grantResources(server, player, amounts)
	for index, key in ipairs(RESOURCES) do
		local amount = amounts[index]
		if amount and amount ~= 0 then server:giveResource(player, LIBRARY:getResourceByName(key), amount) end
	end
end

-- HotA object-identity predicates. The engine only offers the lookups; the comparison lives here.
local function heroOwner(game, heroType, player)
	local hero = game:getHeroByType(heroType)
	return hero ~= nil and hero:getOwner() == player
end

local function playerOwnsTown(game, player, objectName)
	local town = game:getObjectByName(objectName)
	return town ~= nil and town:getOwner() == player
end

local function playerDefeated(game, player)
	return game:getPlayerStatus(player) == ENUM.PlayerStatus.loser
end

local function playerStartingFaction(game, player, faction)
	return game:getPlayerFaction(player) == faction
end

local function playerDestroyedObject(game, player, objectName)
	local target = game:getObjectByName(objectName)
	return target ~= nil and game:playerDestroyedObject(player, target)
end

-- Negative if the current difficulty is easier than the reference, zero if equal, positive if harder.
local function compareDifficulty(game, reference)
	return game:getDifficulty() - reference
end

-- HotA scripted quest (a seer hut / quest guard whose condition and reward are script-driven
-- instead of the engine's built-in mission types). Evaluated fresh on every visit: shows the
-- completion message and runs the reward once the condition holds, otherwise the proposal (first
-- time) or progression (repeat visits) message, tracked per player via markQuestProposed.
local function registerQuest(game, server, player, object, opts)
	if opts.condition() then
		server:showMessage(player, opts.completion, {})
		opts.reward()
	elseif game:wasQuestProposed(object, player) then
		server:showMessage(player, opts.progression, {})
	else
		server:markQuestProposed(object, player)
		server:setQuestHintText(object, opts.hint)
		server:addToQuestLog(object, player)
		server:showMessage(player, opts.proposal, {})
	end
end

-- Updates a scripted quest's quest-log / hover hint text, optionally adding it to the player's
-- quest log (HotA's SET_QUEST_HINT action).
-- TODO: `images` is ignored - a quest-log hint has no engine-side component list yet.
local function setQuestHint(server, player, object, text, images, showInLog)
	server:setQuestHintText(object, text)
	if showInLog then server:addToQuestLog(object, player) end
end

)lua";
}

std::string HotaScriptConverter::questObjectRef(uint32_t identifier)
{
	referencedObjects.insert(identifier);

	char hex[9];
	std::snprintf(hex, sizeof(hex), "%08x", identifier);
	return std::string("questObjects[\"") + hex + "\"]";
}

HotaScriptConverter::HotaScriptConverter(MapReaderH3M & reader, std::string mapName, LocalizeCallback localizeString)
	: reader(reader)
	, mapName(std::move(mapName))
	, localizeString(std::move(localizeString))
{
}

void HotaScriptConverter::readScript()
{
	// Event bodies reference variables by id and are read before the declarations, so
	// buffer them and prepend the `Vars` id->name table once the declarations are known.
	events += loadEventList("heroEvents");
	events += loadEventList("playerEvents");
	events += loadEventList("townEvents");
	events += loadEventList("questEvents");

	reader.readInt32(); // next variable ID
	reader.readInt32(); // next hero event ID
	reader.readInt32(); // next player event ID
	reader.readInt32(); // next town event ID
	reader.readInt32(); // next quest event ID

	variablesTable = loadVariables();

	loadEventMap(); // hero events
	loadEventMap(); // player events
	loadEventMap(); // town events
	loadEventMap(); // quest events
	loadEventMap(); // variables
}

void HotaScriptConverter::convert(CMap * map, const std::map<si32, ObjectInstanceID> & questIdentifierToId)
{
	std::string source;
	source += "-- generated from " + mapName + ".h3m HotA event system\n";
	source += "local Map = {}\n\n";
	source += helpersPrelude();
	source += loadQuestReferences(map, questIdentifierToId);
	source += variablesTable;
	source += events;
	source += "\nreturn Map\n";

	map->scriptSource = std::move(source);
	map->scriptVariableDefinitions = std::move(variables);
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
		result += "function Map:" + eventHandlerName(bucket, eventID) + "(" + bucketTraits(bucket).args + ")\n";
		result += bucketTraits(bucket).playerLocal;
		result += body;
		result += "end\n\n";
	}
	return result;
}

std::string HotaScriptConverter::loadVariables()
{
	int variablesCount = reader.readInt32();
	if(variablesCount == 0)
		return {};

	std::string result = "local Vars = {\n";
	for(int i = 0; i < variablesCount; ++i)
	{
		int uniqueID = reader.readInt32();
		std::string variableID = reader.readBaseString();

		ScriptVariableDefinition & declaration = variables.emplace_back();
		// storage is keyed by name; synthesize a stable name when the editor left it blank
		declaration.name = variableID.empty() ? "var" + std::to_string(uniqueID) : variableID;
		declaration.persistInCampaign = reader.readBool();
		declaration.importFromPreviousScenario = reader.readBool();
		declaration.initialValue.Integer() = reader.readInt32();

		result += "\t[" + std::to_string(uniqueID) + "] = " + luaString(declaration.name) + ",\n";
	}
	result += "}\n\n";
	return result;
}

void HotaScriptConverter::loadEventMap()
{
	int mappingSize = reader.readInt32();
	for(int i = 0; i < mappingSize; ++i)
		reader.readInt32(); // UID of map object bound to an event
}

std::runtime_error HotaScriptConverter::unsupported(const std::string & message) const
{
	return std::runtime_error("Map '" + mapName + "', " + currentBucket + " id " + std::to_string(currentEventID) + ": " + message);
}

std::string HotaScriptConverter::localizedText(const std::string & role)
{
	TextIdentifier identifier(currentBucket, static_cast<size_t>(currentEventID), role, static_cast<size_t>(stringCounter++));
	return "{append = {\"" + localizeString(identifier) + "\"}}";
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
				result += pad + "server:showMessage(player, " + text + ", toComponents({" + images + "}))\n";
				break;
			}
			case HotaScriptActions::SHOW_REWARDS_MESSAGE:
			{
				std::string text = localizedText("rewardMessage");
				std::string body = loadActions(indent + 1);
				result += pad + "server:showRewardsMessage(player, " + text + ", function()\n" + body + pad + "end)\n";
				break;
			}
			case HotaScriptActions::REMOVE_CURRENT_OBJECT_OR_FINISH_QUEST:
			{
				// only a quest bucket runs on a quest source; elsewhere the action just removes the object
				if(currentBucket == "questEvents")
					result += pad + "server:finishQuestOrRemoveObject(object)\n";
				else
					result += pad + "server:removeObject(" + bucketTraits(currentBucket).subject + ")\n";
				break;
			}
			case HotaScriptActions::DISABLE_EVENT:
			{
				// In HotA an event lives on its map object, so disabling it means removing that object
				result += pad + "server:removeObject(" + bucketTraits(currentBucket).subject + ")\n";
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

				result += pad + "registerQuest(game, server, player, " + bucketTraits(currentBucket).subject + ", {\n";
				result += inner + "proposal = " + proposal + ",\n";
				result += inner + "progression = " + progression + ",\n";
				result += inner + "completion = " + completion + ",\n";
				result += inner + "hint = " + hint + ",\n";
				result += inner + "condition = function() return " + condition + " end,\n";
				result += inner + "reward = function()\n" + reward + inner + "end,\n";
				result += pad + "})\n";
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
				result += inner + "server:setMapVariable(" + varRef(variableID) + ", __i)\n";
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
				result += pad + "setQuestHint(server, player, " + bucketTraits(currentBucket).subject + ", " + text + ", toComponents({" + images + "}), " + boolStr(showInLog) + ")\n";
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

				result += pad + "server:showQuestion{\n";
				result += inner + "text = " + text + ",\n";
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
				reader.readBool(); // showMessage flag - always shown by the granting call
				if(take)
					result += pad + "server:takeArtifact(hero, " + entityRef("getArtifactByName", artifact) + ")\n";
				else if(scrollSpell.getNum() >= 0)
					result += pad + "server:grantScroll(hero, " + entityRef("getSpellByName", scrollSpell) + ")\n";
				else
					result += pad + "server:grantArtifact(hero, " + entityRef("getArtifactByName", artifact) + ")\n";
				break;
			}
			case HotaScriptActions::WAR_MACHINE:
			{
				bool take = reader.readBool();
				ArtifactID machine = reader.readArtifact32();
				reader.skipUnused(4); // garbage padding
				reader.readBool(); // showMessage flag
				result += pad + "server:" + (take ? "takeWarMachine" : "grantWarMachine") + "(hero, " + entityRef("getArtifactByName", machine) + ")\n";
				break;
			}
			case HotaScriptActions::SPELL:
			{
				SpellID spell = reader.readSpell32();
				reader.readBool(); // showMessage flag
				result += pad + "server:grantSpell(hero, " + entityRef("getSpellByName", spell) + ")\n";
				break;
			}
			case HotaScriptActions::SPELLBOOK:
			{
				bool take = reader.readBool();
				reader.skipUnused(8); // garbage padding
				reader.readBool(); // showMessage flag
				result += pad + "server:" + (take ? "takeSpellbook" : "grantSpellbook") + "(hero)\n";
				break;
			}
			case HotaScriptActions::CREATURES:
			{
				bool take = reader.readBool();
				CreatureID creature = reader.readCreature32();
				std::string count = loadExpression();
				reader.readBool(); // showMessage flag
				result += pad + "server:" + (take ? "takeCreatures" : "grantCreatures") + "(hero, " + entityRef("getCreatureByName", creature) + ", " + count + ")\n";
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
					slots += "{" + count + ", " + entityKey(creature) + "}";
				}
				result += pad + "server:startCombat(hero, {" + slots + "})\n";
				break;
			}
			case HotaScriptActions::SECONDARY_SKILL:
			{
				int mastery = reader.readInt32();
				SecondarySkill skill = reader.readSkill32();
				reader.readBool(); // showMessage flag
				result += pad + "server:grantSecondarySkill(hero, " + entityRef("getSecondarySkillByName", skill) + ", " + num(mastery) + ")\n";
				break;
			}
			case HotaScriptActions::MORALE:
			{
				int amount = reader.readInt32();
				reader.readBool(); // showMessage flag
				result += pad + "server:grantMorale(hero, " + num(amount) + ")\n";
				break;
			}
			case HotaScriptActions::LUCK:
			{
				int amount = reader.readInt32();
				reader.readBool(); // showMessage flag
				result += pad + "server:grantLuck(hero, " + num(amount) + ")\n";
				break;
			}
			case HotaScriptActions::EXPERIENCE:
			{
				std::string amount = loadExpression();
				reader.readBool(); // showMessage flag
				result += pad + "server:giveExperience(hero, " + amount + ")\n";
				break;
			}
			case HotaScriptActions::SPELL_POINTS:
			{
				std::string amount = loadExpression();
				int mode = reader.readInt32();
				reader.readBool(); // showMessage flag
				result += pad + "server:grantSpellPoints(hero, " + amount + ", " + num(mode) + ")\n";
				break;
			}
			case HotaScriptActions::MOVEMENT_POINTS:
			{
				std::string amount = loadExpression();
				int mode = reader.readInt32();
				reader.readBool(); // showMessage flag
				result += pad + "server:grantMovementPoints(hero, " + amount + ", " + num(mode) + ")\n";
				break;
			}
			case HotaScriptActions::CREATURES_TO_HIRE:
			{
				uint32_t level = reader.readUInt32(); // creature tier 0-7 within the town this script runs on
				std::string amount = loadExpression();
				int unknown = reader.readInt32(); // TODO: meaning unknown, possibly factory 8th dwelling marker
				reader.readBool(); // showMessage flag
				if(unknown != -1)
					throw unsupported("CREATURES_TO_HIRE with unknown field set to " + num(unknown));

				result += pad + "server:grantCreaturesToHire(town, " + num(static_cast<int>(level)) + ", " + amount + ")\n";
				break;
			}
			case HotaScriptActions::CONSTRUCT_BUILDING:
			{
				BuildingID building = reader.readBuilding32(std::nullopt);
				int unknownA = reader.readInt16(); // faction ID?
				int unknownB = reader.readInt16(); // faction building ID?
				reader.readBool(); // showMessage flag
				// the building was read without faction context, so a set faction field would mean the emitted
				// building id is the wrong one - refuse rather than erect something else
				if(unknownA != -1 || unknownB != -1)
					throw unsupported("CONSTRUCT_BUILDING with faction fields set to " + num(unknownA) + "/" + num(unknownB));

				result += pad + "server:constructBuilding(town, " + num(building.getNum()) + ")\n";
				break;
			}
			case HotaScriptActions::EXECUTE_EVENT:
			{
				int eventType = reader.readInt32();
				int eventID = reader.readInt32();
				std::string targetBucket = bucketName(eventType);

				// The call forwards the caller's locals, so the target handler must take the same
				// parameters - a town handler invoked from a hero event would bind `object` to `town`
				const std::string & args = bucketTraits(currentBucket).args;
				if(bucketTraits(targetBucket).args != args)
					throw unsupported("EXECUTE_EVENT targeting " + targetBucket + ", which takes different parameters");

				result += pad + "Map:" + eventHandlerName(targetBucket, eventID) + "(" + args + ")\n";
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
				reader.readBool(); // showMessage flag
				// mode may scale or redirect the grant; handing out the raw amounts would be wrong
				if(mode != 0)
					throw unsupported("RESOURCES with mode " + num(mode));

				result += pad + "grantResources(server, player, {" + amounts + "})\n";
				break;
			}
			case HotaScriptActions::PRIMARY_SKILL:
			{
				std::string amount = loadExpression();
				PrimarySkill skill(reader.readInt32());
				reader.readBool(); // showMessage flag
				result += pad + "server:grantPrimarySkill(hero, " + primarySkillRef(skill) + ", " + amount + ")\n";
				break;
			}
			case HotaScriptActions::MODIFY_VARIABLE:
			{
				int variableID = reader.readInt32();
				int mode = reader.readInt8(); // 0 = add, 1 = subtract, 2 = set
				std::string value = loadExpressionInternal();
				std::string id = varRef(variableID);
				if(mode == 0)
					result += pad + "server:setMapVariable(" + id + ", (game:getMapVariable(" + id + ") + " + value + "))\n";
				else if(mode == 1)
					result += pad + "server:setMapVariable(" + id + ", (game:getMapVariable(" + id + ") - " + value + "))\n";
				else
					result += pad + "server:setMapVariable(" + id + ", " + value + ")\n";
				break;
			}
			case HotaScriptActions::SET_VARIABLE_CONDITIONAL:
			{
				int variableID = reader.readInt32();
				std::string condition = loadCondition();
				std::string valueTrue = loadExpression();
				std::string valueFalse = loadExpression();
				std::string id = varRef(variableID);
				result += pad + "if " + condition + " then\n";
				result += inner + "server:setMapVariable(" + id + ", " + valueTrue + ")\n";
				result += pad + "else\n";
				result += inner + "server:setMapVariable(" + id + ", " + valueFalse + ")\n";
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
			reader.readSpell32(); // scroll spell - carried in a separate slot, not needed for the check
			return "hero:hasArtifact(" + entityRef("getArtifactByName", artifact) + ")";
		}
		case HotaScriptCondition::CURRENT_PLAYER:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			return "(player == " + playerRef(conditionPlayer) + ")";
		}
		case HotaScriptCondition::HERO_OWNER:
		{
			HeroTypeID hero = reader.readHero32();
			// TODO: the -2 ("current hero") sentinel is treated as the event's own player by
			// resolvePlayer; verify whether HotA means the hero's owner or something else here.
			PlayerColor conditionPlayer = reader.readPlayer32(); // -2 = current hero, -1 = current player
			return "heroOwner(game, " + entityRef("getHeroTypeByName", hero) + ", resolvePlayer(" + num(conditionPlayer.getNum()) + ", player))";
		}
		case HotaScriptCondition::HERO_SECONDARY_SKILL:
		{
			SecondarySkill skill = reader.readSkill32();
			int mastery = reader.readInt32();
			return "(hero:getSecondarySkill(" + entityRef("getSecondarySkillByName", skill) + ") >= " + num(mastery) + ")";
		}
		case HotaScriptCondition::TOWN_IS_NEUTRAL:
			return "(town:getOwner() == ENUM.PlayerColor.neutral)";
		case HotaScriptCondition::PLAYER_DEFEATED:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			return "playerDefeated(game, resolvePlayer(" + num(conditionPlayer.getNum()) + ", player))";
		}
		case HotaScriptCondition::PLAYER_IS_HUMAN:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			return "game:playerIsHuman(resolvePlayer(" + num(conditionPlayer.getNum()) + ", player))";
		}
		case HotaScriptCondition::PLAYER_STARTING_FACTION:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			FactionID faction = reader.readFaction32();
			return "playerStartingFaction(game, resolvePlayer(" + num(conditionPlayer.getNum()) + ", player), " + entityRef("getFactionByName", faction) + ")";
		}
		case HotaScriptCondition::PLAYER_DEFEATED_MONSTER:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			uint32_t targetObjectID = reader.readUInt32();
			return "playerDestroyedObject(game, resolvePlayer(" + num(conditionPlayer.getNum()) + ", player), " + questObjectRef(targetObjectID) + ")";
		}
		case HotaScriptCondition::PLAYER_DEFEATED_HERO:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			uint32_t targetObjectID = reader.readUInt32();
			return "playerDestroyedObject(game, resolvePlayer(" + num(conditionPlayer.getNum()) + ", player), " + questObjectRef(targetObjectID) + ")";
		}
		case HotaScriptCondition::PLAYER_OWNS_TOWN:
		{
			PlayerColor conditionPlayer = reader.readPlayer32();
			uint32_t targetObjectID = reader.readUInt32();
			return "playerOwnsTown(game, resolvePlayer(" + num(conditionPlayer.getNum()) + ", player), " + questObjectRef(targetObjectID) + ")";
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
			return "game:getMapVariable(" + varRef(reader.readInt32()) + ")";
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
			PlayerColor resourcePlayer = reader.readPlayer(); // special value for current player
			GameResID resource = reader.readGameResID32();
			return "game:getResource(resolvePlayer(" + num(resourcePlayer.getNum()) + ", player), " + entityRef("getResourceByName", resource) + ")";
		}
		case HotaScriptExpression::CREATURE_COUNT_IN_ARMY:
		{
			CreatureID creature = reader.readCreature32();
			return "hero:creatureCountInArmy(" + entityRef("getCreatureByName", creature) + ")";
		}
		case HotaScriptExpression::CURRENT_DIFFICULTY:
			return "game:getDifficulty()";
		case HotaScriptExpression::COMPARE_DIFFICULTY:
			return "compareDifficulty(game, " + num(reader.readInt32()) + ")";
		case HotaScriptExpression::CURRENT_DATE:
			return "game:getCalendar():getCurrentDay()";
		case HotaScriptExpression::HERO_EXPERIENCE:
			return "hero:getExperience()";
		case HotaScriptExpression::HERO_LEVEL:
			return "hero:getLevel()";
		case HotaScriptExpression::HERO_PRIMARY_SKILL:
		{
			PrimarySkill skill(reader.readInt32());
			return "hero:getPrimarySkill(" + primarySkillRef(skill) + ")";
		}
		case HotaScriptExpression::RANDOM_NUMBER:
		{
			std::string low = loadExpression();
			std::string high = loadExpression();
			return "server:random(" + low + ", " + high + ")";
		}
		case HotaScriptExpression::HERO_OWNED_ARTIFACTS:
		{
			ArtifactID artifact = reader.readArtifact32();
			reader.readSpell32(); // scroll spell slot, unused for the count
			return "hero:ownedArtifacts(" + entityRef("getArtifactByName", artifact) + ")";
		}
		default:
			throw std::runtime_error("Unknown event expression code:" + std::to_string(static_cast<int>(expressionCode)));
	}
}

std::string HotaScriptConverter::loadQuestReferences(CMap * map, const std::map<si32, ObjectInstanceID> & questIdentifierToId)
{
	if(referencedObjects.empty())
		return {};

	std::string table = "local questObjects = {\n";
	for(uint32_t identifier : referencedObjects)
	{
		// an identifier with no object behind it means a malformed map - the script would reference
		// something that was never placed, so fail the load rather than emit a broken lookup table
		ObjectInstanceID objectID = questIdentifierToId.at(identifier);
		std::string name = map->objects.at(objectID.getNum())->instanceName;

		char hex[9];
		std::snprintf(hex, sizeof(hex), "%08x", identifier);
		table += std::string("\t[\"") + hex + "\"] = \"" + name + "\",\n";
	}
	table += "}\n\n";

	return table;
}


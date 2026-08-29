/*
 * DecisionPolicy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "DecisionPolicy.h"
#include "PriorityEvaluator.h"

#if __has_include(<lua.hpp>)
#	include <lua.hpp>
#else
extern "C"
{
#	include <lua.h>
#	include <lauxlib.h>
#	include <lualib.h>
}
#endif

#include "../../../luascript/LuaContext.h"
#include "../../../luascript/LuaModule.h"
#include "../../../luascript/LuaScriptInstance.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/ResourceTypeHandler.h"
#include "../../../lib/json/JsonNode.h"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

namespace NK2AI
{

namespace
{

int roundFloat32(lua_State * state)
{
	const auto value = static_cast<float>(luaL_checknumber(state, 1));
	lua_pushnumber(state, value);
	return 1;
}

std::string readTextFile(const boost::filesystem::path & path)
{
	boost::filesystem::ifstream input(path, std::ios::binary);
	if(!input)
		throw std::runtime_error("unable to open " + path.string());

	return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string getEnvironmentValue(const std::string & name)
{
	const char * value = std::getenv(name.c_str());
	return value ? value : std::string();
}

std::string uppercaseAscii(std::string value)
{
	for(char & character : value)
	{
		if(character >= 'a' && character <= 'z')
			character = static_cast<char>(character - 'a' + 'A');
	}
	return value;
}

std::string getPlayerEnvironmentValue(const std::string & name, const PlayerColor player)
{
	const std::string playerValue = getEnvironmentValue(name + "_" + uppercaseAscii(player.toString()));
	return playerValue.empty() ? getEnvironmentValue(name) : playerValue;
}

std::string makeVersion(const std::string & value)
{
	uint64_t hash = 14695981039346656037ULL;
	for(const unsigned char character : value)
	{
		hash ^= character;
		hash *= 1099511628211ULL;
	}

	std::ostringstream stream;
	stream << std::hex << std::setfill('0') << std::setw(16) << hash;
	return stream.str();
}

bool readScores(const JsonNode & value, std::vector<DecisionPolicyScore> & scores, std::string & error)
{
	if(!value.isStruct())
	{
		error = "rank must return a table containing a scores array";
		return false;
	}

	const JsonNode & scoreValues = value["scores"];
	if(!scoreValues.isVector())
	{
		error = "rank result has no scores array";
		return false;
	}

	scores.reserve(scoreValues.Vector().size());
	for(const JsonNode & scoreValue : scoreValues.Vector())
	{
		if(!scoreValue.isStruct())
		{
			error = "scores contains a non-table entry";
			return false;
		}

		const JsonNode & idValue = scoreValue["id"];
		const JsonNode & scoreNumberValue = scoreValue["score"];
		if(!idValue.isNumber() || !scoreNumberValue.isNumber())
		{
			error = "score entries require numeric id and score fields";
			return false;
		}

		const double idNumber = idValue.Float();
		if(!std::isfinite(idNumber)
			|| idNumber < 0
			|| !vstd::isAlmostEqual(std::floor(idNumber), idNumber)
			|| idNumber >= static_cast<double>(std::numeric_limits<size_t>::max()))
		{
			error = "scores contains a negative or non-integral candidate ID";
			return false;
		}

		const auto score = static_cast<float>(scoreNumberValue.Float());
		if(!std::isfinite(score))
		{
			error = "scores contains a non-finite value";
			return false;
		}
		scores.push_back({static_cast<size_t>(idNumber), score});
	}
	return true;
}

bool validateScores(const JsonNode & request, const std::vector<DecisionPolicyScore> & scores, std::string & error)
{
	std::set<size_t> expected;
	for(const JsonNode & candidate : request["candidates"].Vector())
		expected.insert(static_cast<size_t>(candidate["id"].Integer()));

	std::set<size_t> proposed;
	for(const DecisionPolicyScore & score : scores)
		proposed.insert(score.id);

	if(proposed.size() != scores.size())
	{
		error = "scores contains duplicate candidate IDs";
		return false;
	}
	if(proposed != expected)
	{
		error = "scores must contain every current candidate exactly once";
		return false;
	}
	return true;
}

class LuaDecisionPolicy final : public IDecisionPolicy
{
	std::string name;
	std::string version;
	JsonNode parameters;
	bool shadow;
	std::unique_ptr<scripting::LuaScriptInstance> script;
	std::shared_ptr<scripting::LuaContext> context;

public:
	LuaDecisionPolicy(std::string name, const std::string & source, const JsonNode & parameters, const Environment * environment, bool shadow)
		: name(std::move(name))
		, version(makeVersion(source + parameters.toCompactString()))
		, parameters(parameters)
		, shadow(shadow)
	{
		if(!environment)
			throw std::runtime_error("Lua policy requires a game environment");

		const auto * luaModule = dynamic_cast<const scripting::LuaModule *>(LIBRARY->scripts());
		if(!luaModule)
			throw std::runtime_error("Lua scripting host is unavailable");

		script = std::make_unique<scripting::LuaScriptInstance>(*luaModule, "nk2-policy-" + this->name, source);
		context = script->createContext(environment);
		context->setGlobalFunction<roundFloat32>("float32");
		context->initialize();
		if(!context->hasFunction("rank"))
			throw std::runtime_error("script must return a policy table with a rank function");
	}

	DecisionPolicyResult rank(const JsonNode & request) override
	{
		DecisionPolicyResult result;
		JsonNode scriptRequest = request;
		scriptRequest["parameters"] = parameters;
		const JsonNode answer = context->callFunction<JsonNode>("rank", scriptRequest);
		if(!readScores(answer, result.scores, result.error))
			return result;
		if(!validateScores(request, result.scores, result.error))
			return result;

		result.applied = true;
		return result;
	}

	std::string getName() const override
	{
		return name;
	}

	std::string getVersion() const override
	{
		return version;
	}

	bool isShadow() const override
	{
		return shadow;
	}
};

JsonNode parseProfile(const boost::filesystem::path & path)
{
	const std::string source = readTextFile(path);
	JsonParsingSettings settings;
	settings.mode = JsonParsingSettings::JsonFormatMode::JSON5;
	settings.strict = true;
	return JsonNode(source.data(), source.size(), settings, path.string());
}

}

JsonNode priorityEvaluationInputToJson(const PriorityEvaluationInput & input)
{
	const auto & settings = input.settings;
	const auto & goal = input.goal;
	const auto & rewards = input.rewards;
	const auto & building = input.building;
	const auto & resourceState = input.resourceState;
	const auto & escapeState = input.escape;
	std::vector<std::string> resourceNames;
	for(const GameResID & resource : LIBRARY->resourceTypeHandler->getAllObjects())
		resourceNames.push_back(resource.toResource()->getJsonKey());

	auto resourcesToJson = [&resourceNames](const std::vector<int32_t> & resources)
	{
		JsonNode result;
		for(size_t index = 0; index < resourceNames.size(); ++index)
		{
			const int32_t value = index < resources.size() ? resources[index] : 0;
			result[resourceNames[index]].Integer() = value;
		}
		return result;
	};

	auto priorityTierName = [](const int priorityTier)
	{
		switch(priorityTier)
		{
		case PriorityEvaluator::BUILDINGS: return "buildings";
		case PriorityEvaluator::INSTAKILL: return "instakill";
		case PriorityEvaluator::INSTADEFEND: return "instaDefend";
		case PriorityEvaluator::KILL: return "kill";
		case PriorityEvaluator::ESCAPE: return "escape";
		case PriorityEvaluator::EXPLORE_AND_GATHER: return "exploreAndGather";
		case PriorityEvaluator::DEFEND: return "defend";
		default: return "unknown";
		}
	};

	JsonNode result;
	result["priorityTier"].String() = priorityTierName(settings.priorityTier);
	result["priorityTierId"].Integer() = settings.priorityTier;

	JsonNode & state = result["state"];
	state["daysWithoutCastle"].Bool() = settings.daysWithoutCastle;
	state["maximumArmyLossTarget"].Float() = settings.maximumArmyLossTarget;
	state["dayOfWeek"].Integer() = settings.dayOfWeek;
	state["daysInWeek"].Integer() = settings.daysInWeek;
	for(const std::string & resourceName : resourceNames)
		state["resourceNames"].Vector().emplace_back(resourceName);
	state["resources"] = resourcesToJson(resourceState.available);
	state["dailyIncome"] = resourcesToJson(resourceState.dailyIncome);
	state["lockedResourceMarketValue"].Integer() = resourceState.lockedResourceMarketValue;
	state["goldPressureOverMax"].Bool() = resourceState.goldPressureOverMax;
	state["hasTownWithoutMarketplace"].Bool() = resourceState.hasTownWithoutMarketplace;

	JsonNode & evaluation = result["evaluation"];
	evaluation["movementCost"].Float() = goal.movementCost;
	evaluation["danger"].Integer() = static_cast<int64_t>(
		std::min<uint64_t>(goal.danger, std::numeric_limits<int64_t>::max()));
	evaluation["closestWayRatio"].Float() = goal.closestWayRatio;
	evaluation["armyLossRatio"].Float() = goal.armyLossRatio;
	evaluation["armyReward"].Float() = rewards.armyReward;
	evaluation["armyGrowth"].Integer() = static_cast<int64_t>(
		std::min<uint64_t>(rewards.armyGrowth, std::numeric_limits<int64_t>::max()));
	evaluation["goldReward"].Integer() = rewards.goldReward;
	evaluation["goldCost"].Integer() = rewards.goldCost;
	evaluation["skillReward"].Float() = rewards.skillReward;
	evaluation["strategicalValue"].Float() = rewards.strategicalValue;
	evaluation["conquestValue"].Float() = rewards.conquestValue;
	evaluation["heroRole"].String() = goal.heroRole == HeroRole::MAIN ? "main" : "scout";
	evaluation["turn"].Integer() = goal.turn;
	evaluation["enemyHeroDangerRatio"].Float() = goal.enemyHeroDangerRatio;
	evaluation["threat"].Float() = goal.threat;
	evaluation["armyInvolvement"].Float() = goal.armyInvolvement;
	evaluation["isDefend"].Bool() = goal.isDefend;
	evaluation["threatTurns"].Integer() = goal.threatTurns;
	evaluation["buildingCost"]["resources"] = resourcesToJson(building.cost);
	evaluation["buildingCost"]["marketValue"].Integer() = building.costMarketValue;
	evaluation["isTradeBuilding"].Bool() = building.isTradeBuilding;
	evaluation["isExchange"].Bool() = goal.isExchange;
	evaluation["isArmyUpgrade"].Bool() = goal.isArmyUpgrade;
	evaluation["isHero"].Bool() = goal.isHero;
	evaluation["isEnemy"].Bool() = goal.isEnemy;
	evaluation["explorePriority"].Integer() = goal.explorePriority;
	evaluation["powerRatio"].Float() = goal.powerRatio;

	JsonNode & escape = result["escape"];
	escape["hasHero"].Bool() = escapeState.hasHero;
	escape["currentDangerTurn"].Integer() = escapeState.currentDangerTurn;
	escape["currentDanger"].Integer() = static_cast<int64_t>(
		std::min<uint64_t>(escapeState.currentDanger, std::numeric_limits<int64_t>::max()));
	escape["currentThreat"].Float() = escapeState.currentThreat;
	escape["destinationThreat"].Float() = escapeState.destinationThreat;
	escape["heroTotalStrength"].Integer() = static_cast<int64_t>(
		std::min<uint64_t>(escapeState.heroTotalStrength, std::numeric_limits<int64_t>::max()));
	return result;
}

PriorityEvaluationInput priorityEvaluationInputFromJson(const JsonNode & value)
{
	std::vector<std::string> resourceNames;
	if(value["state"]["resourceNames"].isVector())
	{
		for(const JsonNode & resourceName : value["state"]["resourceNames"].Vector())
			resourceNames.push_back(resourceName.String());
	}
	else
	{
		for(const std::string & resourceName : GameConstants::RESOURCE_NAMES)
			resourceNames.push_back(resourceName);
	}

	auto resourcesFromJson = [&resourceNames](const JsonNode & resources)
	{
		std::vector<int32_t> result(resourceNames.size());
		for(size_t index = 0; index < result.size(); ++index)
			result[index] = resources[resourceNames[index]].Integer();
		return result;
	};

	PriorityEvaluationInput input;
	auto & settings = input.settings;
	auto & goal = input.goal;
	auto & rewards = input.rewards;
	auto & building = input.building;
	auto & resourceState = input.resourceState;

	settings.priorityTier = value["priorityTierId"].Integer();
	const JsonNode & state = value["state"];
	settings.daysWithoutCastle = state["daysWithoutCastle"].Bool();
	settings.maximumArmyLossTarget = state["maximumArmyLossTarget"].Float();
	settings.dayOfWeek = state["dayOfWeek"].Integer();
	settings.daysInWeek = state["daysInWeek"].Integer();
	resourceState.available = resourcesFromJson(state["resources"]);
	resourceState.dailyIncome = resourcesFromJson(state["dailyIncome"]);
	resourceState.lockedResourceMarketValue = state["lockedResourceMarketValue"].Integer();
	resourceState.goldPressureOverMax = state["goldPressureOverMax"].Bool();
	resourceState.hasTownWithoutMarketplace = state["hasTownWithoutMarketplace"].Bool();

	const JsonNode & evaluation = value["evaluation"];
	goal.movementCost = evaluation["movementCost"].Float();
	goal.danger = evaluation["danger"].Integer();
	goal.closestWayRatio = evaluation["closestWayRatio"].Float();
	goal.armyLossRatio = evaluation["armyLossRatio"].Float();
	goal.heroRole = evaluation["heroRole"].String() == "main" ? HeroRole::MAIN : HeroRole::SCOUT;
	goal.turn = evaluation["turn"].Integer();
	goal.enemyHeroDangerRatio = evaluation["enemyHeroDangerRatio"].Float();
	goal.threat = evaluation["threat"].Float();
	goal.armyInvolvement = evaluation["armyInvolvement"].Float();
	goal.isDefend = evaluation["isDefend"].Bool();
	goal.threatTurns = evaluation["threatTurns"].Integer();
	goal.isExchange = evaluation["isExchange"].Bool();
	goal.isArmyUpgrade = evaluation["isArmyUpgrade"].Bool();
	goal.isHero = evaluation["isHero"].Bool();
	goal.isEnemy = evaluation["isEnemy"].Bool();
	goal.explorePriority = evaluation["explorePriority"].Integer();
	goal.powerRatio = evaluation["powerRatio"].Float();

	rewards.armyReward = evaluation["armyReward"].Float();
	rewards.armyGrowth = evaluation["armyGrowth"].Integer();
	rewards.goldReward = evaluation["goldReward"].Integer();
	rewards.goldCost = evaluation["goldCost"].Integer();
	rewards.skillReward = evaluation["skillReward"].Float();
	rewards.strategicalValue = evaluation["strategicalValue"].Float();
	rewards.conquestValue = evaluation["conquestValue"].Float();

	building.cost = resourcesFromJson(evaluation["buildingCost"]["resources"]);
	building.costMarketValue = evaluation["buildingCost"]["marketValue"].Integer();
	building.isTradeBuilding = evaluation["isTradeBuilding"].Bool();

	const JsonNode & escape = value["escape"];
	input.escape.hasHero = escape["hasHero"].Bool();
	input.escape.currentDangerTurn = escape["currentDangerTurn"].Integer();
	input.escape.currentDanger = escape["currentDanger"].Integer();
	input.escape.currentThreat = escape["currentThreat"].Float();
	input.escape.destinationThreat = escape["destinationThreat"].Float();
	input.escape.heroTotalStrength = escape["heroTotalStrength"].Integer();
	return input;
}

std::unique_ptr<IDecisionPolicy> createLuaDecisionPolicy(
	const std::string & name,
	const std::string & source,
	const JsonNode & parameters,
	const Environment * environment,
	bool shadow)
{
	return std::make_unique<LuaDecisionPolicy>(name, source, parameters, environment, shadow);
}

std::unique_ptr<IDecisionPolicy> createDecisionPolicy(const PlayerColor player, const Environment * environment)
{
	const std::string selectedPolicy = getPlayerEnvironmentValue("VCMI_NK2_POLICY", player);
	if(selectedPolicy.empty() || selectedPolicy == "builtin")
		return nullptr;

	try
	{
		const boost::filesystem::path selectedPath(selectedPolicy);
		const JsonNode profile = parseProfile(selectedPath);
		if(profile["schemaVersion"].getType() != JsonNode::JsonType::DATA_INTEGER || profile["schemaVersion"].Integer() != 1)
			throw std::runtime_error("unsupported or missing schemaVersion");
		if(!profile["type"].isString() || profile["type"].String() != "lua")
			throw std::runtime_error("policy type must be 'lua'");
		if(!profile["script"].isString())
			throw std::runtime_error("Lua policy has no script path");

		boost::filesystem::path scriptPath(profile["script"].String());
		if(scriptPath.is_relative())
			scriptPath = selectedPath.parent_path() / scriptPath;

		JsonNode parameters;
		parameters.Struct();
		if(profile["parameters"].isStruct())
			parameters = profile["parameters"];
		else if(!profile["parameters"].isNull())
			throw std::runtime_error("policy parameters must be an object");
		if(!profile["shadow"].isNull() && !profile["shadow"].isBool())
			throw std::runtime_error("policy shadow flag must be a boolean");

		const std::string name = profile["name"].isString() ? profile["name"].String() : selectedPath.stem().string();
		const bool shadow = profile["shadow"].isBool() && profile["shadow"].Bool();
		return createLuaDecisionPolicy(name, readTextFile(scriptPath), parameters, environment, shadow);
	}
	catch(const std::runtime_error & exception)
	{
		logAi->error("Unable to load NK2 decision policy '%s': %s. Using compiled policy.", selectedPolicy, exception.what());
		return nullptr;
	}
}

}

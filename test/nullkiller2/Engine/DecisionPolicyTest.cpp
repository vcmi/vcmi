/*
 * DecisionPolicyTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/DecisionPolicy.h"
#include "AI/Nullkiller2/Engine/PriorityEvaluator.h"
#include "lib/constants/EntityIdentifiers.h"
#include "lib/gameState/CGameState.h"
#include "lib/json/JsonNode.h"

namespace
{

class ScopedEnvironmentVariable
{
public:
	ScopedEnvironmentVariable(std::string name, const std::string & value)
		: name(std::move(name))
	{
		if(const char * previous = std::getenv(this->name.c_str()))
			previousValue = previous;
		set(value);
	}
	ScopedEnvironmentVariable(const ScopedEnvironmentVariable &) = delete;
	ScopedEnvironmentVariable(ScopedEnvironmentVariable &&) = delete;
	ScopedEnvironmentVariable & operator=(const ScopedEnvironmentVariable &) = delete;
	ScopedEnvironmentVariable & operator=(ScopedEnvironmentVariable &&) = delete;

	~ScopedEnvironmentVariable()
	{
		if(previousValue)
			set(*previousValue);
		else
			unset();
	}

private:
	void set(const std::string & value) const
	{
#ifdef VCMI_WINDOWS
		_putenv_s(name.c_str(), value.c_str());
#else
		setenv(name.c_str(), value.c_str(), 1);
#endif
	}

	void unset() const
	{
#ifdef VCMI_WINDOWS
		_putenv_s(name.c_str(), "");
#else
		unsetenv(name.c_str());
#endif
	}

	std::string name;
	std::optional<std::string> previousValue;
};

JsonNode makeRequest()
{
	JsonNode request;
	JsonNode first;
	first["id"].Integer() = 3;
	first["baselinePriority"].Float() = 10;
	request["candidates"].Vector().push_back(first);

	JsonNode second;
	second["id"].Integer() = 7;
	second["baselinePriority"].Float() = 20;
	request["candidates"].Vector().push_back(second);
	return request;
}

JsonNode parameters(float divisor = 1)
{
	JsonNode result;
	result["divisor"].Float() = divisor;
	return result;
}

const Environment * policyEnvironment()
{
	static const CGameState gameState;
	return &gameState.getScriptingEnvironment();
}

std::string readTextFile(const std::string & path)
{
	std::ifstream input(path, std::ios::binary);
	if(!input)
		throw std::runtime_error("Unable to read " + path);
	return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::unique_ptr<NK2AI::IDecisionPolicy> createReferencePolicy()
{
	return NK2AI::createLuaDecisionPolicy(
		"reference",
		readTextFile("test/testdata/nk2/reference.lua"),
		parameters(),
		policyEnvironment());
}

JsonNode makeReferenceRequest(const std::string & tier)
{
	const std::map<std::string, int> tierIds = {
		{"buildings", NK2AI::PriorityEvaluator::BUILDINGS},
		{"instakill", NK2AI::PriorityEvaluator::INSTAKILL},
		{"instaDefend", NK2AI::PriorityEvaluator::INSTADEFEND},
		{"kill", NK2AI::PriorityEvaluator::KILL},
		{"escape", NK2AI::PriorityEvaluator::ESCAPE},
		{"exploreAndGather", NK2AI::PriorityEvaluator::EXPLORE_AND_GATHER},
		{"defend", NK2AI::PriorityEvaluator::DEFEND}
	};
	NK2AI::PriorityEvaluationInput input;
	input.settings.priorityTier = tierIds.at(tier);
	input.settings.maximumArmyLossTarget = 0.2;
	input.settings.dayOfWeek = 1;
	input.settings.daysInWeek = 7;
	input.goal.powerRatio = 1;
	input.goal.closestWayRatio = 0.5;

	JsonNode request;
	request["priorityTier"].String() = tier;
	request["priorityTierId"].Integer() = input.settings.priorityTier;

	JsonNode candidate;
	candidate["id"].Integer() = 0;
	candidate["initialPriority"].Float() = 0;
	candidate["rankingInput"] = NK2AI::priorityEvaluationInputToJson(input);
	request["candidates"].Vector().push_back(std::move(candidate));
	return request;
}

void addRankingInputs(JsonNode & request)
{
	for(JsonNode & candidate : request["candidates"].Vector())
	{
		JsonNode input;
		input["priorityTier"] = request["priorityTier"];
		input["priorityTierId"] = request["priorityTierId"];
		input["state"] = request["state"];
		input["evaluation"] = candidate["evaluation"];

		JsonNode & escape = input["escape"];
		escape["hasHero"].Bool() = request["priorityTierId"].Integer() == NK2AI::PriorityEvaluator::ESCAPE
			&& !candidate["heroId"].isNull();
		escape["currentDangerTurn"].Integer() = 0;
		escape["currentDanger"].Integer() = 0;
		escape["currentThreat"].Float() = 0;
		escape["destinationThreat"].Float() = 0;
		escape["heroTotalStrength"].Integer() = 0;
		if(candidate["reference"]["escape"].isStruct())
		{
			const JsonNode & fixtureEscape = candidate["reference"]["escape"];
			escape["currentDangerTurn"] = fixtureEscape["currentDangerTurn"];
			escape["currentDanger"] = fixtureEscape["currentDanger"];
			escape["currentThreat"] = fixtureEscape["currentThreat"];
			escape["destinationThreat"] = fixtureEscape["destinationThreat"];
			escape["heroTotalStrength"] = fixtureEscape["heroTotalStrength"];
		}
		const auto fixtureInput = NK2AI::priorityEvaluationInputFromJson(input);
		JsonNode canonicalInput = NK2AI::priorityEvaluationInputToJson(fixtureInput);
		const auto roundTrippedInput = NK2AI::priorityEvaluationInputFromJson(canonicalInput);
		const float fixtureScore = NK2AI::evaluatePriority(fixtureInput);
		const float roundTrippedScore = NK2AI::evaluatePriority(roundTrippedInput);
		const float tolerance = std::max(1.0f, std::abs(fixtureScore)) * 0.0001f;
		EXPECT_NEAR(roundTrippedScore, fixtureScore, tolerance);
		candidate["rankingInput"] = std::move(canonicalInput);
	}
}

void expectReferenceMatchesCompiled(NK2AI::IDecisionPolicy & policy, JsonNode request)
{
	const auto result = policy.rank(request);
	ASSERT_TRUE(result.applied) << request["priorityTier"].String() << ": " << result.error;

	std::map<size_t, float> actual;
	for(const NK2AI::DecisionPolicyScore & score : result.scores)
		actual[score.id] = score.score;

	for(const JsonNode & candidate : request["candidates"].Vector())
	{
		const size_t id = candidate["id"].Integer();
		ASSERT_TRUE(actual.contains(id));
		const auto input = NK2AI::priorityEvaluationInputFromJson(candidate["rankingInput"]);
		const float compiledScore = input.settings.priorityTier == NK2AI::PriorityEvaluator::BUILDINGS
			&& candidate["initialPriority"].Float() > 0
			? candidate["initialPriority"].Float()
			: NK2AI::evaluatePriority(input);
		const float tolerance = std::max(1.0f, std::abs(compiledScore)) * 0.0001f;
		EXPECT_NEAR(actual.at(id), compiledScore, tolerance)
			<< request["priorityTier"].String() << " candidate " << id;
	}
}

}

TEST(Nullkiller2_Engine_DecisionPolicy, colorSpecificProfileUsesUppercaseColorName)
{
	ScopedEnvironmentVariable defaultPolicy("VCMI_NK2_POLICY", "builtin");
	ScopedEnvironmentVariable redPolicy("VCMI_NK2_POLICY_RED", "test/testdata/nk2/reference.json");

	auto policy = NK2AI::createDecisionPolicy(PlayerColor(0), policyEnvironment());

	ASSERT_NE(policy, nullptr);
	EXPECT_EQ(policy->getName(), "reference");
}

TEST(Nullkiller2_Engine_DecisionPolicy, directLuaPathFallsBackToCompiledPolicy)
{
	ScopedEnvironmentVariable defaultPolicy("VCMI_NK2_POLICY", "builtin");
	ScopedEnvironmentVariable redPolicy("VCMI_NK2_POLICY_RED", "test/testdata/nk2/reference.lua");

	auto policy = NK2AI::createDecisionPolicy(PlayerColor(0), policyEnvironment());

	EXPECT_EQ(policy, nullptr);
}

TEST(Nullkiller2_Engine_DecisionPolicy, luaCanScoreEveryCandidateUsingParameters)
{
	const std::string source = R"lua(
		return { rank = function(request)
			local scores = {}
			for _, candidate in ipairs(request.candidates) do
				table.insert(scores, {
					id = candidate.id,
					score = float32(candidate.baselinePriority / request.parameters.divisor)
				})
			end
			return { scores = scores }
		end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("scores", source, parameters(3), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	ASSERT_TRUE(result.applied) << result.error;
	ASSERT_EQ(result.scores.size(), 2);
	EXPECT_EQ(result.scores[0].id, 3);
	EXPECT_FLOAT_EQ(result.scores[0].score, static_cast<float>(10.0 / 3.0));
	EXPECT_EQ(result.scores[1].id, 7);
	EXPECT_FLOAT_EQ(result.scores[1].score, static_cast<float>(20.0 / 3.0));
}

TEST(Nullkiller2_Engine_DecisionPolicy, incompleteScoresFallBackToCompiledPolicy)
{
	const std::string source = R"lua(
		return { rank = function(request)
			return { scores = {{ id = request.candidates[1].id, score = 1 }} }
		end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("incomplete", source, parameters(), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	EXPECT_FALSE(result.applied);
	EXPECT_FALSE(result.error.empty());
}

TEST(Nullkiller2_Engine_DecisionPolicy, duplicateScoresFallBackToCompiledPolicy)
{
	const std::string source = R"lua(
		return { rank = function(request)
			local id = request.candidates[1].id
			return { scores = {{ id = id, score = 1 }, { id = id, score = 2 }} }
		end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("duplicate", source, parameters(), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	EXPECT_FALSE(result.applied);
	EXPECT_FALSE(result.error.empty());
}

TEST(Nullkiller2_Engine_DecisionPolicy, scoresOutsideFloatRangeFallBackToCompiledPolicy)
{
	const std::string source = R"lua(
		return { rank = function(request)
			local scores = {}
			for _, candidate in ipairs(request.candidates) do
				table.insert(scores, { id = candidate.id, score = 1e300 })
			end
			return { scores = scores }
		end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("overflow", source, parameters(), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	EXPECT_FALSE(result.applied);
	EXPECT_FALSE(result.error.empty());
}

TEST(Nullkiller2_Engine_DecisionPolicy, nonTableResultFallsBackWithoutEscapingLua)
{
	const std::string source = R"lua(
		return { rank = function(request) return 42 end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("invalid-result", source, parameters(), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	EXPECT_FALSE(result.applied);
	EXPECT_FALSE(result.error.empty());
}

TEST(Nullkiller2_Engine_DecisionPolicy, standardLuaBindingsAreAvailable)
{
	const std::string source = R"lua(
		return { rank = function(request)
			assert(GAME ~= nil and LIBRARY ~= nil and ENUM ~= nil)
			assert(type(require) == "function" and type(print) == "function")
			assert(LIBRARY:getResourceByName("gold") ~= nil)
			assert(io == nil and os == nil and package == nil)
			assert(debug == nil and dofile == nil and loadfile == nil and load == nil)
			assert(math.random == nil and math.randomseed == nil)
			local scores = {}
			for _, candidate in ipairs(request.candidates) do
				table.insert(scores, { id = candidate.id, score = candidate.baselinePriority })
			end
			return { scores = scores }
		end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("bindings", source, parameters(), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	EXPECT_TRUE(result.applied) << result.error;
}

TEST(Nullkiller2_Engine_DecisionPolicy, luaErrorsFallBackToCompiledPolicy)
{
	const std::string source = R"lua(
		return { rank = function(request) error("expected policy failure") end }
	)lua";
	auto policy = NK2AI::createLuaDecisionPolicy("failure", source, parameters(), policyEnvironment());
	const auto result = policy->rank(makeRequest());

	EXPECT_FALSE(result.applied);
	EXPECT_FALSE(result.error.empty());
}

TEST(Nullkiller2_Engine_DecisionPolicy, referencePolicyMatchesCompiledRankingFixtures)
{
	const std::string source = readTextFile("test/testdata/nk2/decision-policy-reference.json");
	const JsonNode fixtures(
		source.data(),
		source.size(),
		"decision-policy-reference.json");
	auto policy = createReferencePolicy();

	for(const JsonNode & fixture : fixtures.Vector())
	{
		JsonNode request = fixture["request"];
		addRankingInputs(request);
		expectReferenceMatchesCompiled(*policy, std::move(request));
	}
}

TEST(Nullkiller2_Engine_DecisionPolicy, referencePolicyCoversExploreAndDefendTiers)
{
	auto policy = createReferencePolicy();

	JsonNode explore = makeReferenceRequest("exploreAndGather");
	auto & exploreEvaluation = explore["candidates"].Vector()[0]["rankingInput"]["evaluation"];
	exploreEvaluation["danger"].Integer() = 0;
	exploreEvaluation["strategicalValue"].Float() = 0.5;
	exploreEvaluation["explorePriority"].Integer() = 0;
	exploreEvaluation["goldReward"].Integer() = 0;
	exploreEvaluation["heroRole"].String() = "main";
	exploreEvaluation["skillReward"].Float() = 0;
	exploreEvaluation["armyInvolvement"].Float() = 0;
	exploreEvaluation["armyReward"].Float() = 100;
	exploreEvaluation["armyGrowth"].Integer() = 50;
	exploreEvaluation["goldCost"].Integer() = 0;
	expectReferenceMatchesCompiled(*policy, explore);

	JsonNode defend = makeReferenceRequest("defend");
	auto & defendEvaluation = defend["candidates"].Vector()[0]["rankingInput"]["evaluation"];
	defendEvaluation["isDefend"].Bool() = true;
	defendEvaluation["isArmyUpgrade"].Bool() = false;
	defendEvaluation["armyInvolvement"].Float() = 400;
	expectReferenceMatchesCompiled(*policy, defend);
}

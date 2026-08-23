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
#include "lib/gameState/CGameState.h"
#include "lib/json/JsonNode.h"

namespace
{

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

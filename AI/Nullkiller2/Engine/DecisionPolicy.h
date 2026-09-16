/*
 * DecisionPolicy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

class JsonNode;
class Environment;
class PlayerColor;

namespace NK2AI
{

struct PriorityEvaluationInput;

struct DecisionPolicyScore
{
	size_t id;
	float score;
};

struct DecisionPolicyResult
{
	bool applied = false;
	std::vector<DecisionPolicyScore> scores;
	std::string error;
};

/// Boundary between NK2 planning and an optional configurable policy.
class IDecisionPolicy
{
public:
	virtual ~IDecisionPolicy() = default;

	virtual DecisionPolicyResult rank(const JsonNode & request) = 0;
	virtual std::string getName() const = 0;
	virtual std::string getVersion() const = 0;
	virtual bool isShadow() const { return false; }
};

/// Loads the JSON profile selected by VCMI_NK2_POLICY[_<PLAYER COLOR>]. An
/// unset variable or the value "builtin" preserves the compiled NK2 evaluator.
std::unique_ptr<IDecisionPolicy> createDecisionPolicy(PlayerColor player, const Environment * environment);

/// Constructs a Lua policy from source. Public for tests and policy tooling, so
/// they exercise the same bindings and validation as live NK2.
std::unique_ptr<IDecisionPolicy> createLuaDecisionPolicy(
	const std::string & name,
	const std::string & source,
	const JsonNode & parameters,
	const Environment * environment,
	bool shadow = false);

/// Stable JSON representation passed to scripts and stored as differential
/// test input. Both scorers consume the values represented here.
JsonNode priorityEvaluationInputToJson(const PriorityEvaluationInput & input);
PriorityEvaluationInput priorityEvaluationInputFromJson(const JsonNode & value);

}

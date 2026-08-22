/*
 * Info.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <atomic>

#include "../json/JsonNode.h"
#include "../mapObjectConstructors/IObjectInfo.h"

#include <vstd/ProfilerMacros.h>

class MetaString;
class IGameInfoCallback;
class IGameRandomizer;

namespace Rewardable
{
struct Limiter;
using LimitersList = std::vector<std::shared_ptr<Rewardable::Limiter>>;
struct Reward;
struct Configuration;
struct Variables;
struct VisitInfo;
struct ResetInfo;
enum class EEventType : uint8_t;

class DLL_LINKAGE Info : public IObjectInfo
{
	JsonNode parameters;
	std::string objectTextID;

	void replaceTextPlaceholders(MetaString & target, const Variables & variables) const;
	void replaceTextPlaceholders(MetaString & target, const Variables & variables, const VisitInfo & info) const;

	void configureVariables(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source) const;
	void configureRewards(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source, Rewardable::EEventType mode, const std::string & textPrefix) const;

	void configureLimiter(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Limiter & limiter, const JsonNode & source) const;
	Rewardable::LimitersList configureSublimiters(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source) const;

	void configureReward(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Reward & info, const JsonNode & source) const;
	void configureResetInfo(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, Rewardable::ResetInfo & info, const JsonNode & source) const;
public:
	const JsonNode & getParameters() const;

	bool givesResources() const override;

	bool givesExperience() const override;
	bool givesMana() const override;
	bool givesMovement() const override;

	bool givesPrimarySkills() const override;
	bool givesSecondarySkills() const override;

	bool givesArtifacts() const override;
	bool givesCreatures() const override;
	bool givesSpells() const override;

	bool givesBonuses() const override;

	bool hasGuards() const override;

	void configureObject(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb) const;

	void init(const JsonNode & objectConfig, const std::string & objectTextID);

	PERF_ONLY(
	/// Logs (via logProfiling) a phase-by-phase breakdown of time spent in configureObject() calls
	/// accumulated since the last call, then resets the accumulators. Diagnostic only.
	static void logAndResetInitProfilingStats();
	)

	template <typename Handler> void serialize(Handler &h)
	{
		h & parameters;
	}
};

PERF_ONLY(
/// configureObject() may run concurrently for multiple objects on worker threads (see
/// CGameState::initMapObjects()), so these accumulators must support concurrent increments -
/// hence std::atomic rather than plain int64_t. Reset (in logAndResetInitProfilingStats) always
/// happens back on the main thread, after the parallel work has joined.
struct InfoConfigureProfilingStats
{
	std::atomic<int64_t> calls = 0;
	std::atomic<int64_t> variablesUs = 0;
	std::atomic<int64_t> rewardsUs = 0;
	std::atomic<int64_t> messagesUs = 0;
	std::atomic<int64_t> resetInfoUs = 0;
	std::atomic<int64_t> visitLimiterUs = 0;

	void reset()
	{
		calls = 0;
		variablesUs = 0;
		rewardsUs = 0;
		messagesUs = 0;
		resetInfoUs = 0;
		visitLimiterUs = 0;
	}
};
)

}

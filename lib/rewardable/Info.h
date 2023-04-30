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

#include "../JsonNode.h"
#include "../mapObjects/CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;

namespace Rewardable
{

struct Limiter;
using LimitersList = std::vector<std::shared_ptr<Rewardable::Limiter>>;
struct Reward;
struct Configuration;
struct ResetInfo;
enum class EEventType;

class DLL_LINKAGE Info : public IObjectInfo
{
	JsonNode parameters;

	void configureRewards(Rewardable::Configuration & object, CRandomGenerator & rng, const JsonNode & source, std::map<si32, si32> & thrownDice, Rewardable::EEventType mode) const;

	void configureLimiter(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::Limiter & limiter, const JsonNode & source) const;
	Rewardable::LimitersList configureSublimiters(Rewardable::Configuration & object, CRandomGenerator & rng, const JsonNode & source) const;

	void configureReward(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::Reward & info, const JsonNode & source) const;
	void configureResetInfo(Rewardable::Configuration & object, CRandomGenerator & rng, Rewardable::ResetInfo & info, const JsonNode & source) const;
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

	void configureObject(Rewardable::Configuration & object, CRandomGenerator & rng) const;

	void init(const JsonNode & objectConfig);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & parameters;
	}
};

}

VCMI_LIB_NAMESPACE_END

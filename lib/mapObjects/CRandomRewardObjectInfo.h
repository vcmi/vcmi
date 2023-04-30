/*
 * CRandomRewardObjectInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../JsonNode.h"
#include "CObjectClassesHandler.h"
#include "Rewardable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;

class DLL_LINKAGE CRandomRewardObjectInfo : public IObjectInfo
{
	JsonNode parameters;

	void configureRewards(Rewardable::Configuration & object, CRandomGenerator & rng, const JsonNode & source, std::map<si32, si32> & thrownDice, CRewardVisitInfo::ERewardEventType mode) const;

	void configureLimiter(Rewardable::Configuration & object, CRandomGenerator & rng, CRewardLimiter & limiter, const JsonNode & source) const;
	TRewardLimitersList configureSublimiters(Rewardable::Configuration & object, CRandomGenerator & rng, const JsonNode & source) const;

	void configureReward(Rewardable::Configuration & object, CRandomGenerator & rng, CRewardInfo & info, const JsonNode & source) const;
	void configureResetInfo(Rewardable::Configuration & object, CRandomGenerator & rng, CRewardResetInfo & info, const JsonNode & source) const;
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

VCMI_LIB_NAMESPACE_END

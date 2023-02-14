/*
 * CRewardableConstructor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CRewardableObject.h"
#include "CObjectClassesHandler.h"

#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CRandomRewardObjectInfo : public IObjectInfo
{
	JsonNode parameters;

	void configureRewards(CRewardableObject * object, CRandomGenerator & rng, const JsonNode & source, std::map<si32, si32> & thrownDice, CRewardVisitInfo::ERewardEventType mode) const;

	void configureLimiter(CRewardableObject * object, CRandomGenerator & rng, CRewardLimiter & limiter, const JsonNode & source) const;
	TRewardLimitersList configureSublimiters(CRewardableObject * object, CRandomGenerator & rng, const JsonNode & source) const;

	void configureReward(CRewardableObject * object, CRandomGenerator & rng, CRewardInfo & info, const JsonNode & source) const;
	void configureResetInfo(CRewardableObject * object, CRandomGenerator & rng, CRewardResetInfo & info, const JsonNode & source) const;
public:
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

	void configureObject(CRewardableObject * object, CRandomGenerator & rng) const;

	void init(const JsonNode & objectConfig);
};

class DLL_LINKAGE CRewardableConstructor : public AObjectTypeHandler
{
	CRandomRewardObjectInfo objectInfo;

	void initTypeData(const JsonNode & config) override;

public:
	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;

	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override;
};

VCMI_LIB_NAMESPACE_END

#pragma once

#include "CRewardableObject.h"
#include "CObjectClassesHandler.h"

#include "../JsonNode.h"

/*
 * CObjectConstructor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CRandomRewardObjectInfo : public IObjectInfo
{
	JsonNode parameters;
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

	CRandomRewardObjectInfo()
	{}

	void init(const JsonNode & objectConfig);
};

class CRewardableConstructor : public AObjectTypeHandler
{
	CRandomRewardObjectInfo objectInfo;

public:
	CRewardableConstructor();
	void init(const JsonNode & config) override;

	CGObjectInstance * create(ObjectTemplate tmpl) const override;

	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const override;
};

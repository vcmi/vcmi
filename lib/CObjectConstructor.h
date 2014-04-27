#pragma once

#include "CObjectWithReward.h"
#include "CDefObjInfoHandler.h"
#include "JsonNode.h"

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
	JsonNode objectConfig;

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

	void configureObject(CObjectWithReward * object) const;

	CRandomRewardObjectInfo()
	{}

	void init(JsonNode objectConfig);
};

class CObjectWithRewardConstructor : public IObjectTypeHandler
{
	std::map<ui32, std::map<ui32, CRewardObjectInfo> > objectConfigs;

public:
	CGObjectInstance * create(ui32 id, ui32 subID) const override;

	bool handlesID(ui32 id) const override;

	void configureObject(CGObjectInstance * object) const override;

	IObjectInfo * getObjectInfo(ui32 id, ui32 subID) const override;
};

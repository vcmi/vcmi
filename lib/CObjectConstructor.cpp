#include "StdInc.h"
#include "CObjectConstructor.h"

/*
 * CObjectConstructor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CRandomRewardObjectInfo::init(const JsonNode & objectConfig)
{
	parameters = objectConfig;
}

void CRandomRewardObjectInfo::configureObject(CObjectWithReward * object) const
{

}

bool CRandomRewardObjectInfo::givesResources() const
{
}

bool CRandomRewardObjectInfo::givesExperience() const
{
}

bool CRandomRewardObjectInfo::givesMana() const
{
}

bool CRandomRewardObjectInfo::givesMovement() const
{
}

bool CRandomRewardObjectInfo::givesPrimarySkills() const
{
}

bool CRandomRewardObjectInfo::givesSecondarySkills() const
{
}

bool CRandomRewardObjectInfo::givesArtifacts() const
{
}

bool CRandomRewardObjectInfo::givesCreatures() const
{
}

bool CRandomRewardObjectInfo::givesSpells() const
{
}

bool CRandomRewardObjectInfo::givesBonuses() const
{
}

CObjectWithRewardConstructor::CObjectWithRewardConstructor()
{
}

void CObjectWithRewardConstructor::init(const JsonNode & config)
{
	objectInfo.init(config);
}

CGObjectInstance * CObjectWithRewardConstructor::create(ObjectTemplate tmpl) const
{
	auto ret = new CObjectWithReward();
	ret->appearance = tmpl;
	return ret;
}

void CObjectWithRewardConstructor::configureObject(CGObjectInstance * object) const
{
	objectInfo.configureObject(dynamic_cast<CObjectWithReward*>(object));
}

const IObjectInfo * CObjectWithRewardConstructor::getObjectInfo(ObjectTemplate tmpl) const
{
	return &objectInfo;
}

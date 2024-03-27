/*
 * CRewardableConstructor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CRewardableConstructor.h"

#include "../mapObjects/CRewardableObject.h"
#include "../CGeneralTextHandler.h"
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

void CRewardableConstructor::initTypeData(const JsonNode & config)
{
	objectInfo.init(config, getBaseTextID());
	blockVisit = config["blockedVisitable"].Bool();

	if (!config["name"].isNull())
		VLC->generaltexth->registerString( config.getModScope(), getNameTextID(), config["name"].String());
	
}

bool CRewardableConstructor::hasNameTextID() const
{
	return !objectInfo.getParameters()["name"].isNull();
}

CGObjectInstance * CRewardableConstructor::create(IGameCallback * cb, std::shared_ptr<const ObjectTemplate> tmpl) const
{
	auto * ret = new CRewardableObject(cb);
	preInitObject(ret);
	ret->appearance = tmpl;
	ret->blockVisit = blockVisit;
	return ret;
}

void CRewardableConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	if(auto * rewardableObject = dynamic_cast<CRewardableObject*>(object))
	{
		objectInfo.configureObject(rewardableObject->configuration, rng, object->cb);
		for(auto & rewardInfo : rewardableObject->configuration.info)
		{
			for (auto & bonus : rewardInfo.reward.bonuses)
			{
				bonus.source = BonusSource::OBJECT_TYPE;
				bonus.sid = BonusSourceID(rewardableObject->ID);
			}
		}
		assert(!rewardableObject->configuration.info.empty());
	}
}

std::unique_ptr<IObjectInfo> CRewardableConstructor::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return std::unique_ptr<IObjectInfo>(new Rewardable::Info(objectInfo));
}

VCMI_LIB_NAMESPACE_END

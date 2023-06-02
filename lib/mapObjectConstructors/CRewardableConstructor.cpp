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
	objectInfo.init(config);
	blockVisit = config["blockedVisitable"].Bool();

	if (!config["name"].isNull())
		VLC->generaltexth->registerString( config.meta, getNameTextID(), config["name"].String());
	
}

bool CRewardableConstructor::hasNameTextID() const
{
	return !objectInfo.getParameters()["name"].isNull();
}

CGObjectInstance * CRewardableConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	auto * ret = new CRewardableObject();
	preInitObject(ret);
	ret->appearance = tmpl;
	ret->blockVisit = blockVisit;
	return ret;
}

void CRewardableConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	if(auto * rewardableObject = dynamic_cast<CRewardableObject*>(object))
	{
		objectInfo.configureObject(rewardableObject->configuration, rng);
		for(auto & rewardInfo : rewardableObject->configuration.info)
		{
			for (auto & bonus : rewardInfo.reward.bonuses)
			{
				bonus.source = BonusSource::OBJECT;
				bonus.sid = rewardableObject->ID;
				//TODO: bonus.description = object->getObjectName();
				if (bonus.type == BonusType::MORALE)
					rewardInfo.reward.extraComponents.emplace_back(Component::EComponentType::MORALE, 0, bonus.val, 0);
				if (bonus.type == BonusType::LUCK)
					rewardInfo.reward.extraComponents.emplace_back(Component::EComponentType::LUCK, 0, bonus.val, 0);
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

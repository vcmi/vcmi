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

#include "../json/JsonUtils.h"
#include "../json/JsonRandom.h"
#include "../mapObjects/CRewardableObject.h"
#include "../texts/CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../CConfigHandler.h"
#include "../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

void CRewardableConstructor::initTypeData(const JsonNode & config)
{
	objectInfo.init(config, getBaseTextID());
	blockVisit = config["blockedVisitable"].Bool();

	if (!config["name"].isNull())
		LIBRARY->generaltexth->registerString( config.getModScope(), getNameTextID(), config["name"]);

	if (settings["mods"]["validation"].String() != "off")
		JsonUtils::validate(config, "vcmi:rewardable", getJsonKey());
	
}

bool CRewardableConstructor::hasNameTextID() const
{
	return !objectInfo.getParameters()["name"].isNull();
}

std::shared_ptr<CGObjectInstance> CRewardableConstructor::create(IGameCallback * cb, std::shared_ptr<const ObjectTemplate> tmpl) const
{
	auto ret = std::make_shared<CRewardableObject>(cb);
	preInitObject(ret.get());
	ret->appearance = tmpl;
	ret->blockVisit = blockVisit;
	return ret;
}

void CRewardableConstructor::assignBonuses(std::vector<Bonus> & bonuses, MapObjectID objectID) const
{
	for (auto & bonus : bonuses)
	{
		bonus.source = BonusSource::OBJECT_TYPE;
		bonus.sid = BonusSourceID(objectID);
	}
}

Rewardable::Configuration CRewardableConstructor::generateConfiguration(IGameCallback * cb, vstd::RNG & rand, MapObjectID objectID, const std::map<std::string, JsonNode> & presetVariables) const
{
	Rewardable::Configuration result;
	result.variables.preset = presetVariables;

	try {
		objectInfo.configureObject(result, rand, cb);
	}
	catch (const JsonRandomizationException & e)
	{
		throw std::runtime_error("Failed to generate configuration for object '" + getJsonKey() + "'! Reason: " + e.what());
	}

	for(auto & rewardInfo : result.info)
	{
		assignBonuses(rewardInfo.reward.heroBonuses, objectID);
		assignBonuses(rewardInfo.reward.commanderBonuses, objectID);
		assignBonuses(rewardInfo.reward.playerBonuses, objectID);
	}

	return result;
}

void CRewardableConstructor::configureObject(CGObjectInstance * object, vstd::RNG & rng) const
{
	auto * rewardableObject = dynamic_cast<CRewardableObject*>(object);

	if (!rewardableObject)
		throw std::runtime_error("Object " + std::to_string(object->getObjGroupIndex()) + ", " + std::to_string(object->getObjTypeIndex()) + " is not a rewardable object!" );

	rewardableObject->configuration = generateConfiguration(object->cb, rng, object->ID, rewardableObject->configuration.variables.preset);
	rewardableObject->initializeGuards();

	if (rewardableObject->configuration.info.empty())
	{
		if (objectInfo.getParameters()["rewards"].isNull())
			logMod->error("Object %s has invalid configuration! No defined rewards found!", getJsonKey());
		else
			logMod->error("Object %s has invalid configuration! Make sure that defined appear chances are continuous!", getJsonKey());
	}
}

std::unique_ptr<IObjectInfo> CRewardableConstructor::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return std::unique_ptr<IObjectInfo>(new Rewardable::Info(objectInfo));
}

VCMI_LIB_NAMESPACE_END

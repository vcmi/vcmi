/*
 * TownBuildingInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TownBuildingInstance.h"

#include "CGTownInstance.h"
#include "../IGameCallback.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../entities/building/CBuilding.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

TownBuildingInstance::TownBuildingInstance(IGameCallback * cb)
	: IObjectInterface(cb)
	, town(nullptr)
{}

TownBuildingInstance::TownBuildingInstance(CGTownInstance * town, const BuildingID & index)
	: IObjectInterface(town->cb)
	, town(town)
	, bID(index)
{}

PlayerColor TownBuildingInstance::getOwner() const
{
	return town->getOwner();
}

MapObjectID TownBuildingInstance::getObjGroupIndex() const
{
	return -1;
}

MapObjectSubID TownBuildingInstance::getObjTypeIndex() const
{
	return 0;
}

const IOwnableObject * TownBuildingInstance::asOwnable() const
{
	return nullptr;
}

int3 TownBuildingInstance::visitablePos() const
{
	return town->visitablePos();
}

int3 TownBuildingInstance::anchorPos() const
{
	return town->anchorPos();
}

TownRewardableBuildingInstance::TownRewardableBuildingInstance(IGameCallback *cb)
	: TownBuildingInstance(cb)
{}

TownRewardableBuildingInstance::TownRewardableBuildingInstance(CGTownInstance * town, const BuildingID & index, vstd::RNG & rand)
	: TownBuildingInstance(town, index)
{
	initObj(rand);
}

void TownRewardableBuildingInstance::initObj(vstd::RNG & rand)
{
	assert(town && town->getTown());
	configuration = generateConfiguration(rand);
}

Rewardable::Configuration TownRewardableBuildingInstance::generateConfiguration(vstd::RNG & rand) const
{
	Rewardable::Configuration result;
	auto building = town->getTown()->buildings.at(getBuildingType());

	building->rewardableObjectInfo.configureObject(result, rand, cb);
	for(auto & rewardInfo : result.info)
	{
		for (auto & bonus : rewardInfo.reward.bonuses)
		{
			bonus.source = BonusSource::TOWN_STRUCTURE;
			bonus.sid = BonusSourceID(building->getUniqueTypeID());
		}
	}
	return result;
}

void TownRewardableBuildingInstance::newTurn(vstd::RNG & rand) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		auto newConfiguration = generateConfiguration(rand);
		cb->setRewardableObjectConfiguration(town->id, getBuildingType(), newConfiguration);

		if(configuration.resetParameters.visitors)
		{
			cb->setObjPropertyValue(town->id, ObjProperty::STRUCTURE_CLEAR_VISITORS, getBuildingType());
		}
	}
}

void TownRewardableBuildingInstance::setProperty(ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::VISITORS:
			visitors.insert(identifier.as<ObjectInstanceID>());
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			visitors.clear();
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = identifier.getNum();
			break;
	}
}

void TownRewardableBuildingInstance::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(configuration.info.at(selectedReward), town, hero);
}

void TownRewardableBuildingInstance::blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const
{
	onBlockingDialogAnswered(hero, answer);
}

void TownRewardableBuildingInstance::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	grantRewardBeforeLevelup(configuration.info.at(rewardID), hero);
	
	// hero is not blocked by levelup dialog - grant remainder immediately
	if(!cb->isVisitCoveredByAnotherQuery(town, hero))
	{
		grantRewardAfterLevelup(configuration.info.at(rewardID), town, hero);
	}
}

bool TownRewardableBuildingInstance::wasVisited(const CGHeroInstance * contextHero) const
{
	return wasVisitedBefore(contextHero);
}

bool TownRewardableBuildingInstance::wasVisitedBefore(const CGHeroInstance * contextHero) const
{
	switch (configuration.visitMode)
	{
		case Rewardable::VISIT_UNLIMITED:
			return false;
		case Rewardable::VISIT_ONCE:
			return !visitors.empty();
		case Rewardable::VISIT_PLAYER:
			return false; //not supported
		case Rewardable::VISIT_BONUS:
		{
			const auto building = town->getTown()->buildings.at(getBuildingType());
			return contextHero->hasBonusFrom(BonusSource::TOWN_STRUCTURE, BonusSourceID(building->getUniqueTypeID()));
		}
		case Rewardable::VISIT_HERO:
			return visitors.find(contextHero->id) != visitors.end();
		case Rewardable::VISIT_LIMITER:
			return configuration.visitLimiter.heroAllowed(contextHero);
		default:
			return false;
	}
}

void TownRewardableBuildingInstance::onHeroVisit(const CGHeroInstance *h) const
{
	assert(town->hasBuilt(getBuildingType()));

	if(town->hasBuilt(getBuildingType()))
		doHeroVisit(h);
}

const IObjectInterface * TownRewardableBuildingInstance::getObject() const
{
	return this;
}

bool TownRewardableBuildingInstance::wasVisited(PlayerColor player) const
{
	switch (configuration.visitMode)
	{
		case Rewardable::VISIT_UNLIMITED:
		case Rewardable::VISIT_BONUS:
		case Rewardable::VISIT_HERO:
		case Rewardable::VISIT_LIMITER:
			return false;
		case Rewardable::VISIT_ONCE:
		case Rewardable::VISIT_PLAYER:
			return !visitors.empty();
		default:
			return false;
	}
}

void TownRewardableBuildingInstance::markAsVisited(const CGHeroInstance * hero) const
{
	town->addHeroToStructureVisitors(hero, getBuildingType());
}

void TownRewardableBuildingInstance::markAsScouted(const CGHeroInstance * hero) const
{
	// no-op - town building is always 'scouted' by owner
}


VCMI_LIB_NAMESPACE_END

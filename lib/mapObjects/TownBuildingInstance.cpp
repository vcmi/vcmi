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
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../entities/building/CBuilding.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

TownBuildingInstance::TownBuildingInstance(IGameInfoCallback * cb)
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

TownRewardableBuildingInstance::TownRewardableBuildingInstance(IGameInfoCallback *cb)
	: TownBuildingInstance(cb)
{}

TownRewardableBuildingInstance::TownRewardableBuildingInstance(CGTownInstance * town, const BuildingID & index, IGameRandomizer & gameRandomizer)
	: TownBuildingInstance(town, index)
{
	assert(town && town->getTown());
	configuration = generateConfiguration(gameRandomizer);
}

void TownRewardableBuildingInstance::assignBonuses(std::vector<std::shared_ptr<Bonus>> & bonuses) const
{
	const auto & building = town->getTown()->buildings.at(getBuildingType());

	for (auto & bonus : bonuses)
	{
		if (building->mapObjectLikeBonuses.hasValue())
		{
			bonus->source = BonusSource::OBJECT_TYPE;
			bonus->sid = BonusSourceID(building->mapObjectLikeBonuses);
		}
		else
		{
			bonus->source = BonusSource::TOWN_STRUCTURE;
			bonus->sid = BonusSourceID(building->getUniqueTypeID());
		}
	}
}

Rewardable::Configuration TownRewardableBuildingInstance::generateConfiguration(IGameRandomizer & gameRandomizer) const
{
	Rewardable::Configuration result;
	const auto & building = town->getTown()->buildings.at(getBuildingType());

	// force modal info window instead of displaying in inactive info box on adventure map
	result.infoWindowType = EInfoWindowMode::MODAL;
	building->rewardableObjectInfo.configureObject(result, gameRandomizer, cb);
	for(auto & rewardInfo : result.info)
	{
		assignBonuses(rewardInfo.reward.heroBonuses);
		assignBonuses(rewardInfo.reward.commanderBonuses);
		assignBonuses(rewardInfo.reward.playerBonuses);
	}
	return result;
}

void TownRewardableBuildingInstance::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		auto newConfiguration = generateConfiguration(gameRandomizer);
		gameEvents.setRewardableObjectConfiguration(town->id, getBuildingType(), newConfiguration);

		if(configuration.resetParameters.visitors)
		{
			gameEvents.setObjPropertyValue(town->id, ObjProperty::STRUCTURE_CLEAR_VISITORS, getBuildingType().getNum());
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

void TownRewardableBuildingInstance::heroLevelUpDone(IGameEventCallback & gameEvents, const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(gameEvents, configuration.info.at(selectedReward), town, hero);
}

void TownRewardableBuildingInstance::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	onBlockingDialogAnswered(gameEvents, hero, answer);
}

void TownRewardableBuildingInstance::grantReward(IGameEventCallback & gameEvents, ui32 rewardID, const CGHeroInstance * hero) const
{
	grantRewardBeforeLevelup(gameEvents, configuration.info.at(rewardID), hero);
	
	// hero is not blocked by levelup dialog - grant remainder immediately
	if(!gameEvents.isVisitCoveredByAnotherQuery(town, hero))
	{
		grantRewardAfterLevelup(gameEvents, configuration.info.at(rewardID), town, hero);
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
		case Rewardable::VISIT_PLAYER_GLOBAL:
			return false; //not supported
		case Rewardable::VISIT_BONUS:
		{
			const auto & building = town->getTown()->buildings.at(getBuildingType());
			if (building->mapObjectLikeBonuses.hasValue())
				return contextHero->hasBonusFrom(BonusSource::OBJECT_TYPE, BonusSourceID(building->mapObjectLikeBonuses));
			else
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

void TownRewardableBuildingInstance::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	assert(town->hasBuilt(getBuildingType()));

	if(town->hasBuilt(getBuildingType()))
		doHeroVisit(gameEvents, h);
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
		case Rewardable::VISIT_PLAYER_GLOBAL:
			return false;
		case Rewardable::VISIT_ONCE:
		case Rewardable::VISIT_PLAYER:
			return !visitors.empty();
		default:
			return false;
	}
}

void TownRewardableBuildingInstance::markAsVisited(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const
{
	town->addHeroToStructureVisitors(gameEvents, hero, getBuildingType().getNum());
}

void TownRewardableBuildingInstance::markAsScouted(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const
{
	// no-op - town building is always 'scouted' by owner
}


VCMI_LIB_NAMESPACE_END

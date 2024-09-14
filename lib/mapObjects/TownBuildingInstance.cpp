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
#include "../texts/CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../gameState/CGameState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"
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

int3 TownBuildingInstance::getPosition() const
{
	return town->getPosition();
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
	assert(town && town->town);
	configuration = generateConfiguration(rand);
}

Rewardable::Configuration TownRewardableBuildingInstance::generateConfiguration(vstd::RNG & rand) const
{
	Rewardable::Configuration result;
	auto building = town->town->buildings.at(getBuildingType());

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
	grantRewardAfterLevelup(cb, configuration.info.at(selectedReward), town, hero);
}

void TownRewardableBuildingInstance::blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const
{
	if(answer == 0)
		return; // player refused
	
	if(visitors.find(hero->id) != visitors.end())
		return; // query not for this building

	if(answer > 0 && answer-1 < configuration.info.size())
	{
		auto list = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
		grantReward(list[answer - 1], hero);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

void TownRewardableBuildingInstance::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	town->addHeroToStructureVisitors(hero, getBuildingType());
	
	grantRewardBeforeLevelup(cb, configuration.info.at(rewardID), hero);
	
	// hero is not blocked by levelup dialog - grant remainder immediately
	if(!cb->isVisitCoveredByAnotherQuery(town, hero))
	{
		grantRewardAfterLevelup(cb, configuration.info.at(rewardID), town, hero);
	}
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
	auto grantRewardWithMessage = [&](int index) -> void
	{
		auto vi = configuration.info.at(index);
		logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
		
		town->addHeroToStructureVisitors(h, getBuildingType()); //adding to visitors

		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text = vi.message;
		vi.reward.loadComponents(iw.components, h);
		iw.type = EInfoWindowMode::MODAL;
		if(!iw.components.empty() || !iw.text.toString().empty())
			cb->showInfoDialog(&iw);
		
		grantReward(index, h);
	};
	auto selectRewardsMessage = [&](const std::vector<ui32> & rewards, const MetaString & dialog) -> void
	{
		BlockingDialog sd(configuration.canRefuse, rewards.size() > 1);
		sd.player = h->tempOwner;
		sd.text = dialog;

		if (rewards.size() > 1)
			for (auto index : rewards)
				sd.components.push_back(configuration.info.at(index).reward.getDisplayedComponent(h));

		if (rewards.size() == 1)
			configuration.info.at(rewards.front()).reward.loadComponents(sd.components, h);

		cb->showBlockingDialog(this, &sd);
	};
	
	if(!town->hasBuilt(getBuildingType()))
		return;

	if(!wasVisitedBefore(h))
	{
		auto rewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT);

		logGlobal->debug("Visiting object with %d possible rewards", rewards.size());
		switch (rewards.size())
		{
			case 0: // no available rewards, e.g. visiting School of War without gold
			{
				auto emptyRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_NOT_AVAILABLE);
				if (!emptyRewards.empty())
					grantRewardWithMessage(emptyRewards[0]);
				else
					logMod->warn("No applicable message for visiting empty object!");
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (configuration.canRefuse)
					selectRewardsMessage(rewards, configuration.info.at(rewards.front()).message);
				else
					grantRewardWithMessage(rewards.front());
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (configuration.selectMode) {
					case Rewardable::SELECT_PLAYER: // player must select
						selectRewardsMessage(rewards, configuration.onSelect);
						break;
					case Rewardable::SELECT_FIRST: // give first available
						grantRewardWithMessage(rewards.front());
						break;
					case Rewardable::SELECT_RANDOM: // give random
						grantRewardWithMessage(*RandomGeneratorUtil::nextItem(rewards, cb->gameState()->getRandomGenerator()));
						break;
				}
				break;
			}
		}
	}
	else
	{
		logGlobal->debug("Revisiting already visited object");

		auto visitedRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_ALREADY_VISITED);
		if (!visitedRewards.empty())
			grantRewardWithMessage(visitedRewards[0]);
		else
			logMod->debug("No applicable message for visiting already visited object!");
	}
}


VCMI_LIB_NAMESPACE_END

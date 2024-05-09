/*
 * CRewardableObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRewardableObject.h"
#include "../gameState/CGameState.h"
#include "../CGeneralTextHandler.h"
#include "../CPlayerState.h"
#include "../IGameCallback.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

void CRewardableObject::grantRewardWithMessage(const CGHeroInstance * contextHero, int index, bool markAsVisit) const
{
	auto vi = configuration.info.at(index);
	logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
	// show message only if it is not empty or in infobox
	if (configuration.infoWindowType != EInfoWindowMode::MODAL || !vi.message.toString().empty())
	{
		InfoWindow iw;
		iw.player = contextHero->tempOwner;
		iw.text = vi.message;
		vi.reward.loadComponents(iw.components, contextHero);
		iw.type = configuration.infoWindowType;
		if(!iw.components.empty() || !iw.text.toString().empty())
			cb->showInfoDialog(&iw);
	}
	// grant reward afterwards. Note that it may remove object
	if(markAsVisit)
		markAsVisited(contextHero);
	grantReward(index, contextHero);
}

void CRewardableObject::selectRewardWthMessage(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, const MetaString & dialog) const
{
	BlockingDialog sd(configuration.canRefuse, rewardIndices.size() > 1);
	sd.player = contextHero->tempOwner;
	sd.text = dialog;
	sd.components = loadComponents(contextHero, rewardIndices);
	cb->showBlockingDialog(&sd);

}

void CRewardableObject::grantAllRewardsWthMessage(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, bool markAsVisit) const
{
	if (rewardIndices.empty())
		return;
		
	for (auto index : rewardIndices)
	{
		// TODO: Merge all rewards of same type, with single message?
		grantRewardWithMessage(contextHero, index, false);
	}
	// Mark visited only after all rewards were processed
	if(markAsVisit)
		markAsVisited(contextHero);
}

std::vector<Component> CRewardableObject::loadComponents(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const
{
	std::vector<Component> result;

	if (rewardIndices.empty())
		return result;

	if (configuration.selectMode != Rewardable::SELECT_FIRST && rewardIndices.size() > 1)
	{
		for (auto index : rewardIndices)
			result.push_back(configuration.info.at(index).reward.getDisplayedComponent(contextHero));
	}
	else
	{
		configuration.info.at(rewardIndices.front()).reward.loadComponents(result, contextHero);
	}

	return result;
}

void CRewardableObject::onHeroVisit(const CGHeroInstance *h) const
{
	if(!wasVisitedBefore(h))
	{
		auto rewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT);
		bool objectRemovalPossible = false;
		for(auto index : rewards)
		{
			if(configuration.info.at(index).reward.removeObject)
				objectRemovalPossible = true;
		}

		logGlobal->debug("Visiting object with %d possible rewards", rewards.size());
		switch (rewards.size())
		{
			case 0: // no available rewards, e.g. visiting School of War without gold
			{
				auto emptyRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_NOT_AVAILABLE);
				if (!emptyRewards.empty())
					grantRewardWithMessage(h, emptyRewards[0], false);
				else
					logMod->warn("No applicable message for visiting empty object!");
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (configuration.canRefuse)
					selectRewardWthMessage(h, rewards, configuration.info.at(rewards.front()).message);
				else
					grantRewardWithMessage(h, rewards.front(), true);
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (configuration.selectMode) {
					case Rewardable::SELECT_PLAYER: // player must select
						selectRewardWthMessage(h, rewards, configuration.onSelect);
						break;
					case Rewardable::SELECT_FIRST: // give first available
						if (configuration.canRefuse)
							selectRewardWthMessage(h, { rewards.front() }, configuration.info.at(rewards.front()).message);
						else
							grantRewardWithMessage(h, rewards.front(), true);
						break;
					case Rewardable::SELECT_RANDOM: // give random
					{
						ui32 rewardIndex = *RandomGeneratorUtil::nextItem(rewards, cb->gameState()->getRandomGenerator());
						if (configuration.canRefuse)
							selectRewardWthMessage(h, { rewardIndex }, configuration.info.at(rewardIndex).message);
						else
							grantRewardWithMessage(h, rewardIndex, true);
						break;
					}
					case Rewardable::SELECT_ALL: // grant all possible
						grantAllRewardsWthMessage(h, rewards, true);
						break;
				}
				break;
			}
		}

		if(!objectRemovalPossible && getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT).empty())
		{
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_TEAM, id, h->id);
			cb->sendAndApply(&cov);
		}
	}
	else
	{
		logGlobal->debug("Revisiting already visited object");

		if (!wasVisited(h->getOwner()))
		{
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_TEAM, id, h->id);
			cb->sendAndApply(&cov);
		}

		auto visitedRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_ALREADY_VISITED);
		if (!visitedRewards.empty())
			grantRewardWithMessage(h, visitedRewards[0], false);
		else
			logMod->warn("No applicable message for visiting already visited object!");
	}
}

void CRewardableObject::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(cb, configuration.info.at(selectedReward), this, hero);
}

void CRewardableObject::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer == 0)
		return; // player refused

	if(answer > 0 && answer-1 < configuration.info.size())
	{
		auto list = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
		markAsVisited(hero);
		grantReward(list[answer - 1], hero);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

void CRewardableObject::markAsVisited(const CGHeroInstance * hero) const
{
	cb->setObjPropertyValue(id, ObjProperty::REWARD_CLEARED, true);

	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD, id, hero->id);
	cb->sendAndApply(&cov);
}

void CRewardableObject::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	cb->setObjPropertyValue(id, ObjProperty::REWARD_SELECT, rewardID);
	grantRewardBeforeLevelup(cb, configuration.info.at(rewardID), hero);
	
	// hero is not blocked by levelup dialog - grant remainer immediately
	if(!cb->isVisitCoveredByAnotherQuery(this, hero))
	{
		grantRewardAfterLevelup(cb, configuration.info.at(rewardID), this, hero);
	}
}

bool CRewardableObject::wasVisitedBefore(const CGHeroInstance * contextHero) const
{
	switch (configuration.visitMode)
	{
		case Rewardable::VISIT_UNLIMITED:
			return false;
		case Rewardable::VISIT_ONCE:
			return onceVisitableObjectCleared;
		case Rewardable::VISIT_PLAYER:
			return vstd::contains(cb->getPlayerState(contextHero->getOwner())->visitedObjects, ObjectInstanceID(id));
		case Rewardable::VISIT_BONUS:
			return contextHero->hasBonusFrom(BonusSource::OBJECT_TYPE, BonusSourceID(ID));
		case Rewardable::VISIT_HERO:
			return contextHero->visitedObjects.count(ObjectInstanceID(id));
		case Rewardable::VISIT_LIMITER:
			return configuration.visitLimiter.heroAllowed(contextHero);
		default:
			return false;
	}
}

bool CRewardableObject::wasVisited(PlayerColor player) const
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
			return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
		default:
			return false;
	}
}

bool CRewardableObject::wasScouted(PlayerColor player) const
{
	return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
}

bool CRewardableObject::wasVisited(const CGHeroInstance * h) const
{
	switch (configuration.visitMode)
	{
		case Rewardable::VISIT_BONUS:
			return h->hasBonusFrom(BonusSource::OBJECT_TYPE, BonusSourceID(ID));
		case Rewardable::VISIT_HERO:
			return h->visitedObjects.count(ObjectInstanceID(id));
		case Rewardable::VISIT_LIMITER:
			return wasScouted(h->getOwner()) && configuration.visitLimiter.heroAllowed(h);
		default:
			return wasVisited(h->getOwner());
	}
}

std::string CRewardableObject::getDisplayTextImpl(PlayerColor player, const CGHeroInstance * hero, bool includeDescription) const
{
	std::string result = getObjectName();

	if (includeDescription && !getDescriptionMessage(player, hero).empty())
		result += "\n" + getDescriptionMessage(player, hero);

	if (hero)
	{
		if(configuration.visitMode != Rewardable::VISIT_UNLIMITED)
		{
			if (wasVisited(hero))
				result += "\n" + configuration.visitedTooltip.toString();
			else
				result += "\n " + configuration.notVisitedTooltip.toString();
		}
	}
	else
	{
		if(configuration.visitMode == Rewardable::VISIT_PLAYER || configuration.visitMode == Rewardable::VISIT_ONCE)
		{
			if (wasVisited(player))
				result += "\n" + configuration.visitedTooltip.toString();
			else
				result += "\n" + configuration.notVisitedTooltip.toString();
		}
	}
	return result;
}

std::string CRewardableObject::getHoverText(PlayerColor player) const
{
	return getDisplayTextImpl(player, nullptr, false);
}

std::string CRewardableObject::getHoverText(const CGHeroInstance * hero) const
{
	return getDisplayTextImpl(hero->getOwner(), hero, false);
}

std::string CRewardableObject::getPopupText(PlayerColor player) const
{
	return getDisplayTextImpl(player, nullptr, true);
}

std::string CRewardableObject::getPopupText(const CGHeroInstance * hero) const
{
	return getDisplayTextImpl(hero->getOwner(), hero, true);
}

std::string CRewardableObject::getDescriptionMessage(PlayerColor player, const CGHeroInstance * hero) const
{
	if (!wasScouted(player) || configuration.info.empty())
		return configuration.description.toString();

	auto rewardIndices = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
	if (rewardIndices.empty() || !configuration.info[0].description.empty())
		return configuration.info[0].description.toString();

	if (!configuration.info[rewardIndices.front()].description.empty())
		return configuration.info[rewardIndices.front()].description.toString();

	return configuration.description.toString();
}

std::vector<Component> CRewardableObject::getPopupComponentsImpl(PlayerColor player, const CGHeroInstance * hero) const
{
	if (!wasScouted(player))
		return {};

	if (!configuration.showScoutedPreview)
		return {};

	auto rewardIndices = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
	if (rewardIndices.empty() && !configuration.info.empty())
		rewardIndices.push_back(0);

	if (rewardIndices.empty())
		return {};

	return loadComponents(hero, rewardIndices);
}

std::vector<Component> CRewardableObject::getPopupComponents(PlayerColor player) const
{
	return getPopupComponentsImpl(player, nullptr);
}

std::vector<Component> CRewardableObject::getPopupComponents(const CGHeroInstance * hero) const
{
	return getPopupComponentsImpl(hero->getOwner(), hero);
}

void CRewardableObject::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::REWARD_RANDOMIZE:
			initObj(cb->gameState()->getRandomGenerator());
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = identifier.getNum();
			break;
		case ObjProperty::REWARD_CLEARED:
			onceVisitableObjectCleared = identifier.getNum();
			break;
	}
}

void CRewardableObject::newTurn(CRandomGenerator & rand) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		if (configuration.resetParameters.rewards)
		{
			cb->setObjPropertyValue(id, ObjProperty::REWARD_RANDOMIZE, 0);
		}
		if (configuration.resetParameters.visitors)
		{
			cb->setObjPropertyValue(id, ObjProperty::REWARD_CLEARED, false);
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_CLEAR, id);
			cb->sendAndApply(&cov);
		}
	}
}

void CRewardableObject::initObj(CRandomGenerator & rand)
{
	getObjectHandler()->configureObject(this, rand);
}

CRewardableObject::CRewardableObject(IGameCallback *cb)
	:CArmedInstance(cb)
{}

void CRewardableObject::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CArmedInstance::serializeJsonOptions(handler);
	handler.serializeStruct("rewardable", static_cast<Rewardable::Interface&>(*this));
}

VCMI_LIB_NAMESPACE_END

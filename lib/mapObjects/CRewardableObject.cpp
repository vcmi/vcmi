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
#include "../CGameState.h"
#include "../CGeneralTextHandler.h"
#include "../CPlayerState.h"
#include "../IGameCallback.h"
#include "../NetPacks.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

// FIXME: copy-pasted from CObjectHandler
static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

void CRewardableObject::onHeroVisit(const CGHeroInstance *h) const
{
	auto grantRewardWithMessage = [&](int index, bool markAsVisit) -> void
	{
		auto vi = configuration.info.at(index);
		logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
 		// show message only if it is not empty or in infobox
		if (configuration.infoWindowType != EInfoWindowMode::MODAL || !vi.message.toString().empty())
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text = vi.message;
			vi.reward.loadComponents(iw.components, h);
			iw.type = configuration.infoWindowType;
			if(!iw.components.empty() || !iw.text.toString().empty())
				cb->showInfoDialog(&iw);
		}
		// grant reward afterwards. Note that it may remove object
		if(markAsVisit)
			markAsVisited(h);
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

		cb->showBlockingDialog(&sd);
	};

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
					grantRewardWithMessage(emptyRewards[0], false);
				else
					logMod->warn("No applicable message for visiting empty object!");
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (configuration.canRefuse)
					selectRewardsMessage(rewards, configuration.info.at(rewards.front()).message);
				else
					grantRewardWithMessage(rewards.front(), true);
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (configuration.selectMode) {
					case Rewardable::SELECT_PLAYER: // player must select
						selectRewardsMessage(rewards, configuration.onSelect);
						break;
					case Rewardable::SELECT_FIRST: // give first available
						grantRewardWithMessage(rewards.front(), true);
						break;
					case Rewardable::SELECT_RANDOM: // give random
						grantRewardWithMessage(*RandomGeneratorUtil::nextItem(rewards, cb->gameState()->getRandomGenerator()), true);
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

		auto visitedRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_ALREADY_VISITED);
		if (!visitedRewards.empty())
			grantRewardWithMessage(visitedRewards[0], false);
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
	cb->setObjProperty(id, ObjProperty::REWARD_CLEARED, true);

	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD, id, hero->id);
	cb->sendAndApply(&cov);
}

void CRewardableObject::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	cb->setObjProperty(id, ObjProperty::REWARD_SELECT, rewardID);
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
			return contextHero->hasBonusFrom(BonusSource::OBJECT, ID);
		case Rewardable::VISIT_HERO:
			return contextHero->visitedObjects.count(ObjectInstanceID(id));
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
			return false;
		case Rewardable::VISIT_ONCE:
		case Rewardable::VISIT_PLAYER:
			return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
		default:
			return false;
	}
}

bool CRewardableObject::wasVisited(const CGHeroInstance * h) const
{
	switch (configuration.visitMode)
	{
		case Rewardable::VISIT_BONUS:
			return h->hasBonusFrom(BonusSource::OBJECT, ID);
		case Rewardable::VISIT_HERO:
			return h->visitedObjects.count(ObjectInstanceID(id));
		default:
			return wasVisited(h->tempOwner);
	}
}

std::string CRewardableObject::getHoverText(PlayerColor player) const
{
	if(configuration.visitMode == Rewardable::VISIT_PLAYER || configuration.visitMode == Rewardable::VISIT_ONCE)
		return getObjectName() + " " + visitedTxt(wasVisited(player));
	return getObjectName();
}

std::string CRewardableObject::getHoverText(const CGHeroInstance * hero) const
{
	if(configuration.visitMode != Rewardable::VISIT_UNLIMITED)
		return getObjectName() + " " + visitedTxt(wasVisited(hero));
	return getObjectName();
}

void CRewardableObject::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::REWARD_RANDOMIZE:
			initObj(cb->gameState()->getRandomGenerator());
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = val;
			break;
		case ObjProperty::REWARD_CLEARED:
			onceVisitableObjectCleared = val;
			break;
	}
}

void CRewardableObject::newTurn(CRandomGenerator & rand) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		if (configuration.resetParameters.rewards)
		{
			cb->setObjProperty(id, ObjProperty::REWARD_RANDOMIZE, 0);
		}
		if (configuration.resetParameters.visitors)
		{
			cb->setObjProperty(id, ObjProperty::REWARD_CLEARED, false);
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_CLEAR, id);
			cb->sendAndApply(&cov);
		}
	}
}

void CRewardableObject::initObj(CRandomGenerator & rand)
{
	VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, rand);
	assert(!configuration.info.empty());
}

CRewardableObject::CRewardableObject()
{}

VCMI_LIB_NAMESPACE_END

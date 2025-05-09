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

#include "../CPlayerState.h"
#include "../IGameCallback.h"
#include "../IGameSettings.h"
#include "../battle/BattleLayout.h"
#include "../gameState/CGameState.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CRewardableConstructor.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../networkPacks/StackLocation.h"
#include "../serializer/JsonSerializeFormat.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

const IObjectInterface * CRewardableObject::getObject() const
{
	return this;
}

void CRewardableObject::markAsScouted(const CGHeroInstance * hero) const
{
	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_PLAYER, id, hero->id);
	cb->sendAndApply(cov);
}

bool CRewardableObject::isGuarded() const
{
	return stacksCount() > 0;
}

void CRewardableObject::onHeroVisit(const CGHeroInstance *hero) const
{
	if(!wasScouted(hero->getOwner()))
	{
		ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_SCOUTED, id, hero->id);
		cb->sendAndApply(cov);
	}

	if (!isGuarded())
	{
		doHeroVisit(hero);
	}
	else if (configuration.forceCombat)
	{
		doStartBattle(hero);
	}
	else
	{
		auto guardedIndexes = getAvailableRewards(hero, Rewardable::EEventType::EVENT_GUARDED);
		auto guardedReward = configuration.info.at(guardedIndexes.at(0));

		// ask player to confirm attack
		BlockingDialog bd(true, false);
		bd.player = hero->getOwner();
		bd.text = guardedReward.message;
		bd.components = getPopupComponents(hero->getOwner());

		cb->showBlockingDialog(this, &bd);
	}
}

void CRewardableObject::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(configuration.info.at(selectedReward), this, hero);
}

void CRewardableObject::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == BattleSide::ATTACKER)
	{
		doHeroVisit(hero);
	}
}

void CRewardableObject::garrisonDialogClosed(const CGHeroInstance *hero) const
{
	// if visitor received creatures as rewards, but does not have free slots, he will leave some units
	// inside rewardable object, which might get treated as guards later
	while(!stacks.empty())
		cb->eraseStack(StackLocation(id, stacks.begin()->first));
}

void CRewardableObject::doStartBattle(const CGHeroInstance * hero) const
{
	auto layout = BattleLayout::createLayout(cb, configuration.guardsLayout, hero, this);
	cb->startBattle(hero, this, visitablePos(), hero, nullptr, layout, nullptr);
}

void CRewardableObject::blockingDialogAnswered(const CGHeroInstance * hero, int32_t answer) const
{
	if(isGuarded())
	{
		if (answer)
			doStartBattle(hero);
	}
	else
	{
		onBlockingDialogAnswered(hero, answer);
	}
}

void CRewardableObject::markAsVisited(const CGHeroInstance * hero) const
{
	cb->setObjPropertyValue(id, ObjProperty::REWARD_CLEARED, true);

	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_HERO, id, hero->id);
	cb->sendAndApply(cov);
}

void CRewardableObject::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	cb->setObjPropertyValue(id, ObjProperty::REWARD_SELECT, rewardID);
	grantRewardBeforeLevelup(configuration.info.at(rewardID), hero);
	
	// hero is not blocked by levelup dialog - grant remainder immediately
	if(!cb->isVisitCoveredByAnotherQuery(this, hero))
	{
		grantRewardAfterLevelup(configuration.info.at(rewardID), this, hero);
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
			return cb->getPlayerState(contextHero->getOwner())->visitedObjects.count(ObjectInstanceID(id)) != 0;
		case Rewardable::VISIT_PLAYER_GLOBAL:
			return cb->getPlayerState(contextHero->getOwner())->visitedObjectsGlobal.count({ID, subID}) != 0;
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
			return cb->getPlayerState(player)->visitedObjects.count(ObjectInstanceID(id)) != 0;
		case Rewardable::VISIT_PLAYER_GLOBAL:
			return cb->getPlayerState(player)->visitedObjectsGlobal.count({ID, subID}) != 0;
		default:
			return false;
	}
}

bool CRewardableObject::wasScouted(PlayerColor player) const
{
	return vstd::contains(cb->getPlayerTeam(player)->scoutedObjects, ObjectInstanceID(id));
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
		if(configuration.visitMode == Rewardable::VISIT_PLAYER || configuration.visitMode == Rewardable::VISIT_ONCE || configuration.visitMode == Rewardable::VISIT_PLAYER_GLOBAL)
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

	if (isGuarded())
	{
		if (!cb->getSettings().getBoolean(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION))
			return {};

		std::map<CreatureID, int> guardsAmounts;
		std::vector<Component> result;

		for (auto const & slot : Slots())
			if (slot.second)
				guardsAmounts[slot.second->getCreatureID()] += slot.second->getCount();

		for (auto const & guard : guardsAmounts)
		{
			Component comp(ComponentType::CREATURE, guard.first, guard.second);
			result.push_back(comp);
		}
		return result;
	}
	else
	{
		if (!configuration.showScoutedPreview)
			return {};

		auto rewardIndices = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
		if (rewardIndices.empty() && !configuration.info.empty())
		{
			// Object has valid config, but current hero has no rewards that he can receive.
			// Usually this happens if hero has already visited this object -> show reward using context without any hero
			// since reward may be context-sensitive - e.g. Witch Hut that gives 1 skill, but always at basic level
			return loadComponents(nullptr, {0});
		}

		if (rewardIndices.empty())
			return {};

		return loadComponents(hero, rewardIndices);
	}
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
		case ObjProperty::REWARD_SELECT:
			selectedReward = identifier.getNum();
			break;
		case ObjProperty::REWARD_CLEARED:
			onceVisitableObjectCleared = identifier.getNum();
			break;
	}
}

void CRewardableObject::newTurn(vstd::RNG & rand) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		if (configuration.resetParameters.rewards)
		{
			auto handler = std::dynamic_pointer_cast<const CRewardableConstructor>(getObjectHandler());
			auto newConfiguration = handler->generateConfiguration(cb, rand, ID, configuration.variables.preset);
			cb->setRewardableObjectConfiguration(id, newConfiguration);
		}
		if (configuration.resetParameters.visitors)
		{
			cb->setObjPropertyValue(id, ObjProperty::REWARD_CLEARED, false);
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_CLEAR, id);
			cb->sendAndApply(cov);
		}
	}
}

void CRewardableObject::initObj(vstd::RNG & rand)
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

void CRewardableObject::initializeGuards()
{
	clearSlots();

	// Workaround for default creature banks strings that has placeholder for object name
	// TODO: find better location for this code
	for (auto & visitInfo : configuration.info)
		visitInfo.message.replaceRawString(getObjectName());

	for (auto const & visitInfo : configuration.info)
	{
		for (auto const & guard : visitInfo.reward.guards)
		{
			auto slotID = getFreeSlot();
			if (!slotID.validSlot())
				return;

			putStack(slotID, std::make_unique<CStackInstance>(cb, guard.getId(), guard.getCount()));
		}
	}
}

bool CRewardableObject::isCoastVisitable() const
{
	return configuration.coastVisitable;
}

VCMI_LIB_NAMESPACE_END

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
#include "../CCreatureHandler.h"
#include "../IGameSettings.h"
#include "../battle/BattleLayout.h"
#include "../battle/CombatValue.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../json/JsonNode.h"
#include "../gameState/CGameState.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CRewardableConstructor.h"
#include "CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../networkPacks/StackLocation.h"
#include "../serializer/JsonSerializeFormat.h"

#include <vstd/RNG.h>

const IObjectInterface * CRewardableObject::getObject() const
{
	return this;
}

void CRewardableObject::markAsScouted(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const
{
	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_SCOUTED, id, hero->id);
	gameEvents.sendAndApply(cov);
}

bool CRewardableObject::isGuarded() const
{
	return stacksCount() > 0;
}

void CRewardableObject::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance *hero) const
{
	if(!wasScouted(hero->getOwner()))
	{
		ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_SCOUTED, id, hero->id);
		gameEvents.sendAndApply(cov);
	}

	if (!isGuarded())
	{
		doHeroVisit(gameEvents, hero);
	}
	else if (configuration.forceCombat)
	{
		doStartBattle(gameEvents, hero);
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

		gameEvents.showBlockingDialog(&bd);
	}
}

void CRewardableObject::experienceApplied(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const JsonNode & visitState) const
{
	resumeAfterExperience(gameEvents, hero, visitState);
}

void CRewardableObject::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == BattleSide::ATTACKER)
	{
		while(!stacks.empty())
		{
			logMod->warn("Attacker won, but defender still has remaining units in configurable map objects! Make sure that guards configuration is valid!");
			gameEvents.eraseStack(StackLocation(id, stacks.begin()->first));
		}

		doHeroVisit(gameEvents, hero);
	}
}

void CRewardableObject::garrisonDialogClosed(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const JsonNode & visitState) const
{
	closeGarrisonDialog(gameEvents, hero, visitState);
}

bool CRewardableObject::closeGarrisonDialog(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const JsonNode & visitState) const
{
	// if visitor received creatures as rewards, but does not have free slots, he will leave some units
	// inside rewardable object, which might get treated as guards later
	while(!stacks.empty())
		gameEvents.eraseStack(StackLocation(id, stacks.begin()->first));

	return resumeAfterGarrison(gameEvents, hero, visitState);
}

void CRewardableObject::doStartBattle(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const
{
	auto layout = BattleLayout::createLayout(*cb, configuration.guardsLayout, hero, this);
	gameEvents.startBattle(hero, this, visitablePos(), hero, nullptr, layout, nullptr);
}

void CRewardableObject::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer, const JsonNode & visitState) const
{
	answerBlockingDialog(gameEvents, hero, answer, visitState);
}

bool CRewardableObject::answerBlockingDialog(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer, const JsonNode & visitState) const
{
	// A reward choice carries what it offered, the question whether to attack the guards nothing
	if(!visitState.isNull())
		return onBlockingDialogAnswered(gameEvents, hero, answer, visitState);

	if(!isGuarded())
		throw std::runtime_error("Object at " + visitablePos().toString() + " got an answer to a question it did not ask");

	if(!answer)
		return false;

	doStartBattle(gameEvents, hero);
	return true;
}

void CRewardableObject::markAsVisited(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const
{
	gameEvents.setObjPropertyValue(id, ObjProperty::REWARD_CLEARED, true);

	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_HERO, id, hero->id);
	gameEvents.sendAndApply(cov);
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
			if(!onceVisitableObjectCleared) return false;
			for(PlayerColor teamPlayer : cb->getPlayerTeam(player)->players)
				if(cb->getPlayerState(teamPlayer)->visitedObjects.count(ObjectInstanceID(id)) != 0) return true;
			return false;
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
	if(wasObjectTypeVisitedByTeam(player)) return true;
	return vstd::contains(cb->getPlayerTeam(player)->scoutedObjects, ObjectInstanceID(id));
}

bool CRewardableObject::wasObjectTypeVisitedByTeam(PlayerColor player) const
{
	if(configuration.getVisitMode() != Rewardable::VISIT_PLAYER_GLOBAL) return false;
	return std::ranges::any_of(
		cb->getPlayerTeam(player)->players,
		[this](PlayerColor teamMember)
		{
			return wasVisited(teamMember);
		});
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

MetaString CRewardableObject::getDisplayTextImpl(PlayerColor player, const CGHeroInstance * hero, bool includeDescription) const
{
	MetaString result = getObjectName();

	auto appendOnNewLine = [&result](const MetaString & text)
	{
		result.appendEOL();
		result.append(text);
	};

	if (includeDescription)
	{
		if (!getDescriptionMessage(player, hero).empty())
			appendOnNewLine(getDescriptionMessage(player, hero));
	}
	else
	{
		// scouted version is included unconditionally into hover text (e.g. Shrines - "learn X spell")
		if (!getScoutedDescriptionMessage(hero).empty())
			appendOnNewLine(getDescriptionMessage(player, hero));
	}

	if (hero)
	{
		if(configuration.visitMode != Rewardable::VISIT_UNLIMITED)
			appendOnNewLine(wasVisited(hero) ? configuration.visitedTooltip : configuration.notVisitedTooltip);
	}
	else
	{
		if(configuration.visitMode == Rewardable::VISIT_PLAYER || configuration.visitMode == Rewardable::VISIT_ONCE || configuration.visitMode == Rewardable::VISIT_PLAYER_GLOBAL)
			appendOnNewLine(wasVisited(player) ? configuration.visitedTooltip : configuration.notVisitedTooltip);
	}
	return result;
}

MetaString CRewardableObject::getHoverText(PlayerColor player) const
{
	return getDisplayTextImpl(player, nullptr, false);
}

MetaString CRewardableObject::getHoverText(const CGHeroInstance * hero) const
{
	return getDisplayTextImpl(hero->getOwner(), hero, false);
}

MetaString CRewardableObject::getPopupText(PlayerColor player) const
{
	return getDisplayTextImpl(player, nullptr, true);
}

MetaString CRewardableObject::getPopupText(const CGHeroInstance * hero) const
{
	return getDisplayTextImpl(hero->getOwner(), hero, true);
}

MetaString CRewardableObject::getScoutedDescriptionMessage(const CGHeroInstance * hero) const
{
	if (configuration.info.empty())
		return {};

	auto rewardIndices = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
	if (rewardIndices.empty() || !configuration.info[0].description.empty())
		return configuration.info[0].description;

	if (!configuration.info[rewardIndices.front()].description.empty())
		return configuration.info[rewardIndices.front()].description;

	return {};
}

MetaString CRewardableObject::getGenericDescriptionMessage() const
{
	return configuration.description;
}

MetaString CRewardableObject::getDescriptionMessage(PlayerColor player, const CGHeroInstance * hero) const
{
	if (!wasScouted(player) || configuration.info.empty())
		return getGenericDescriptionMessage();

	auto scoutedMessage = getScoutedDescriptionMessage(hero);
	if (!scoutedMessage.empty())
		return scoutedMessage;

	return getGenericDescriptionMessage();
}

std::vector<Component> CRewardableObject::getPopupComponentsImpl(PlayerColor player, const CGHeroInstance * hero) const
{
	auto guardsState = getGuardState(player);
	if(!guardsState)
		return {};

	if(guardsState->guarded)
	{
		std::vector<Component> result;

		for(auto const & guard : guardsState->composition)
		{
			Component comp(ComponentType::CREATURE, guard.getId(), guard.getCount());
			result.push_back(comp);
		}
		return result;
	}
	else
	{
		if(!configuration.showScoutedPreview)
			return {};

		auto rewardIndices = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
		if(rewardIndices.empty() && !configuration.info.empty())
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
		case ObjProperty::REWARD_CLEARED:
			onceVisitableObjectCleared = identifier.getNum();
			break;
	}
}

void CRewardableObject::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{
	auto calendar = cb->getCalendar();
	int currentDay = calendar.getCurrentDay();
	ui32 resetDuration = configuration.getResetDuration(calendar);
	if (resetDuration != 0 && currentDay > 1 && ((currentDay-1) % resetDuration) == 0)
	{
		if (configuration.resetParameters.rewards)
		{
			auto handler = std::dynamic_pointer_cast<const CRewardableConstructor>(getObjectHandler());
			auto newConfiguration = handler->generateConfiguration(cb, gameRandomizer, ID, configuration.variables.preset);
			gameEvents.setRewardableObjectConfiguration(id, newConfiguration);
		}
		if (configuration.resetParameters.visitors)
		{
			gameEvents.setObjPropertyValue(id, ObjProperty::REWARD_CLEARED, false);
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_CLEAR, id);
			gameEvents.sendAndApply(cov);
		}
	}
}

void CRewardableObject::initObj(IGameRandomizer & gameRandomizer)
{
	getObjectHandler()->configureObject(this, gameRandomizer);
}

CRewardableObject::CRewardableObject(IGameInfoCallback *cb)
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
		visitInfo.message.replaceTextID(getObjectNameTextID());

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

std::optional<GuardState> CRewardableObject::getGuardState(PlayerColor player) const
{
	if(!wasScouted(player))
		return std::nullopt;

	GuardState result;

	const bool guardedReward = std::any_of(configuration.info.begin(), configuration.info.end(), [](const auto & info)
	{
		return info.visitType == Rewardable::EEventType::EVENT_FIRST_VISIT && !info.reward.guards.empty();
	});

	result.guarded = isGuarded() || (guardedReward && !wasVisited(player));

	if(result.guarded)
	{
		if(!cb->getSettings().getBoolean(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION))
			return result;

		std::map<CreatureID, TQuantity> guardAmounts;
		if(isGuarded())
		{
			for(const auto & slot : Slots())
				if(slot.second)
					guardAmounts[slot.second->getCreatureID()] += slot.second->getCount();
		}
		else
		{
			for(const auto & info : configuration.info)
				if(info.visitType == Rewardable::EEventType::EVENT_FIRST_VISIT)
					for(const auto & guard : info.reward.guards)
						guardAmounts[guard.getId()] += guard.getCount();
		}

		for(const auto & [creature, amount] : guardAmounts)
			result.composition.emplace_back(creature, amount);
	}

	return result;
}

RewardableObjectInfo CRewardableObject::getVisibleInfo(PlayerColor player, const CGHeroInstance * hero) const
{
	RewardableObjectInfo result;
	const bool isVisitOnce = configuration.getVisitMode() == Rewardable::VISIT_ONCE;
	const bool visited = hero && !isVisitOnce ? wasVisited(hero) : wasVisited(player);
	result.scouted = wasScouted(player);

	if(!result.scouted)
		return result;

	const auto guardsState = getGuardState(player);
	if(!guardsState)
		return result;

	result.guarded = guardsState->guarded;
	if(result.guarded && guardsState->compositionKnown())
	{
		const auto & combatValue = LIBRARY->creh->getCombatValue();
		for(const auto & guard : guardsState->composition)
			result.guardStrength += guard.getCount() * combatValue.getAIValue(guard.getId().toCreature());
	}
	if(result.guarded)
		return result;

	std::vector<ui32> rewardIndices;
	if(!visited)
		rewardIndices = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);

	result.rewardAvailable = !rewardIndices.empty();
	result.rewardValueKnown = configuration.showScoutedPreview;
	if(result.rewardValueKnown)
		for(auto index : rewardIndices)
			result.rewards.push_back(configuration.info[index].reward);

	return result;
}

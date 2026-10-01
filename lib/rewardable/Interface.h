/*
 * Interface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../spells/ExternalCaster.h"
#include "Configuration.h"

class IObjectInterface;
class IGameEventCallback;
class CArmedInstance;
class JsonNode;
struct InfoWindow;

namespace Rewardable
{

class Interface
{
private:
	
	/// caster to cast adveture spells, no serialize
	mutable spells::ExternalCaster caster;
	
protected:
	
	/// Grants the part of the reward that must wait until any level-up it caused is resolved.
	/// Returns true if it opened a garrison window for creatures that did not fit.
	bool grantRewardAfterLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & reward, const CGHeroInstance * hero, const std::vector<ui32> & pending) const;

	/// Grants the part of the reward that must be applied before any level-up. Returns true
	/// if it granted experience, which always ends in experienceApplied() applying the rest.
	bool grantRewardBeforeLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & reward, const CGHeroInstance * hero) const;
	
	/// Returns true if the visit is suspended until an activity the reward started finishes.
	bool grantRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const;
	virtual void showRewardMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int rewardIndex) const;
	virtual void configureInfoWindow(InfoWindow & infoWindow, const CGHeroInstance * contextHero, int rewardIndex) const;
	void selectRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, const MetaString & dialog) const;
	/// Grants rewards in order. Returns true if one suspended the visit, which then keeps the rest for later.
	bool grantRewardsWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const;
	std::vector<Component> loadComponents(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const;

	void doHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance *h) const;

	virtual const IObjectInterface * getObject() const = 0;
	virtual bool wasVisitedBefore(const CGHeroInstance * hero) const = 0;
	virtual bool wasVisited(PlayerColor player) const = 0;
	virtual void markAsVisited(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const = 0;
	virtual void markAsScouted(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const = 0;

	/// Grants a reward, then those pending after it. Returns true if the visit is suspended - by experience,
	/// which continues from resumeAfterExperience(), or by a garrison window, which continues from
	/// resumeAfterGarrison().
	bool grantReward(IGameEventCallback & gameEvents, ui32 rewardID, const CGHeroInstance * hero, const std::vector<ui32> & pending = {}) const;

	/// Finishes the reward that granted experience, once its level-ups are resolved. Returns true if the
	/// visit is suspended again.
	bool resumeAfterExperience(IGameEventCallback & gameEvents, const CGHeroInstance * hero, const JsonNode & visitState) const;

	/// Grants the rewards still pending when a garrison window closes. Returns true if the visit is suspended again.
	bool resumeAfterGarrison(IGameEventCallback & gameEvents, const CGHeroInstance * hero, const JsonNode & visitState) const;

	bool isRewardIndex(const JsonNode & node) const;
	bool isRewardList(const JsonNode & node) const;
	static JsonNode toJson(const std::vector<ui32> & rewardIndices);

	/// Grants the reward picked from those a reward choice offered. Returns true if the visit is suspended.
	bool onBlockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer, const JsonNode & offeredRewards) const;
public:

	/// filters list of visit info and returns rewards that can be granted to current hero
	std::vector<ui32> getAvailableRewards(const CGHeroInstance * hero, Rewardable::EEventType event) const;
	
	Rewardable::Configuration configuration;
	
	void serializeJson(JsonSerializeFormat & handler);
	
	template <typename Handler> void serialize(Handler &h)
	{
		h & configuration;
	}
};

}

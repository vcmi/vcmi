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

VCMI_LIB_NAMESPACE_BEGIN

class IObjectInterface;

namespace Rewardable
{

class DLL_LINKAGE Interface
{
private:
	
	/// caster to cast adveture spells, no serialize
	mutable spells::ExternalCaster caster;
	
protected:
	
	/// function that must be called if hero got level-up during grantReward call
	void grantRewardAfterLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & reward, const CArmedInstance * army, const CGHeroInstance * hero) const;

	/// grants reward to hero
	void grantRewardBeforeLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & reward, const CGHeroInstance * hero) const;
	
	virtual void grantRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const;
	void selectRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, const MetaString & dialog) const;
	void grantAllRewardsWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32>& rewardIndices, bool markAsVisit) const;
	std::vector<Component> loadComponents(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const;

	void doHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance *h) const;

	virtual const IObjectInterface * getObject() const = 0;
	virtual bool wasVisitedBefore(const CGHeroInstance * hero) const = 0;
	virtual bool wasVisited(PlayerColor player) const = 0;
	virtual void markAsVisited(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const = 0;
	virtual void markAsScouted(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const = 0;
	virtual void grantReward(IGameEventCallback & gameEvents, ui32 rewardID, const CGHeroInstance * hero) const = 0;

	void onBlockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer) const;
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

VCMI_LIB_NAMESPACE_END

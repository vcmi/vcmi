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

#include "../CCreatureSet.h"
#include "../ResourceSet.h"
#include "../spells/ExternalCaster.h"
#include "Configuration.h"

VCMI_LIB_NAMESPACE_BEGIN

class IGameCallback;

namespace Rewardable
{

class DLL_LINKAGE Interface
{
private:
	
	/// caster to cast adveture spells, no serialize
	mutable spells::ExternalCaster caster;
	
protected:
	
	/// filters list of visit info and returns rewards that can be granted to current hero
	std::vector<ui32> getAvailableRewards(const CGHeroInstance * hero, Rewardable::EEventType event) const;
	
	/// function that must be called if hero got level-up during grantReward call
	virtual void grantRewardAfterLevelup(IGameCallback * cb, const Rewardable::VisitInfo & reward, const CArmedInstance * army, const CGHeroInstance * hero) const;

	/// grants reward to hero
	virtual void grantRewardBeforeLevelup(IGameCallback * cb, const Rewardable::VisitInfo & reward, const CGHeroInstance * hero) const;
	
public:
	
	Rewardable::Configuration configuration;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & configuration;
	}
};

}

VCMI_LIB_NAMESPACE_END

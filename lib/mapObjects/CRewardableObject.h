/*
 * CRewardableObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArmedInstance.h"
#include "../rewardable/Interface.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Base class that can handle granting rewards to visiting heroes.
/// Inherits from CArmedInstance for proper transfer of armies
class DLL_LINKAGE CRewardableObject : public CArmedInstance, public Rewardable::Interface
{
protected:

	bool onceVisitableObjectCleared = false;
	
	/// reward selected by player, no serialize
	ui16 selectedReward = 0;
	
	void grantReward(ui32 rewardID, const CGHeroInstance * hero) const;
	void markAsVisited(const CGHeroInstance * hero) const;
	
	/// return true if this object was "cleared" before and no longer has rewards applicable to selected hero
	/// unlike wasVisited, this method uses information not available to player owner, for example, if object was cleared by another player before
	bool wasVisitedBefore(const CGHeroInstance * contextHero) const;
	
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
	
	virtual void grantRewardWithMessage(const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const;
	virtual void selectRewardWithMessage(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, const MetaString & dialog) const;

	virtual void grantAllRewardsWithMessage(const CGHeroInstance * contextHero, const std::vector<ui32>& rewardIndices, bool markAsVisit) const;

	std::vector<Component> loadComponents(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const;

	std::string getDisplayTextImpl(PlayerColor player, const CGHeroInstance * hero, bool includeDescription) const;
	std::string getDescriptionMessage(PlayerColor player, const CGHeroInstance * hero) const;
	std::vector<Component> getPopupComponentsImpl(PlayerColor player, const CGHeroInstance * hero) const;

public:
	/// Visitability checks. Note that hero check includes check for hero owner (returns true if object was visited by player)
	bool wasVisited(PlayerColor player) const override;
	bool wasVisited(const CGHeroInstance * h) const override;

	/// Returns true if object was scouted by player and he is aware of its internal state
	bool wasScouted(PlayerColor player) const;
	
	/// gives reward to player or ask for choice in case of multiple rewards
	void onHeroVisit(const CGHeroInstance *h) const override;

	///possibly resets object state
	void newTurn(vstd::RNG & rand) const override;

	/// gives second part of reward after hero level-ups for proper granting of spells/mana
	void heroLevelUpDone(const CGHeroInstance *hero) const override;

	/// applies player selection of reward
	void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const override;

	void initObj(vstd::RNG & rand) override;
	
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;

	CRewardableObject(IGameCallback *cb);
	
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;

	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;

	std::vector<Component> getPopupComponents(PlayerColor player) const override;
	std::vector<Component> getPopupComponents(const CGHeroInstance * hero) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<Rewardable::Interface&>(*this);
		h & onceVisitableObjectCleared;
	}
};

//TODO:

// MAX
// class DLL_LINKAGE CBank : public CArmedInstance
// class DLL_LINKAGE CGPyramid : public CBank

// EXTRA
// class DLL_LINKAGE COPWBonus : public CGTownBuilding
// class DLL_LINKAGE CTownBonus : public CGTownBuilding
// class DLL_LINKAGE CGKeys : public CGObjectInstance //Base class for Keymaster and guards
// class DLL_LINKAGE CGKeymasterTent : public CGKeys
// class DLL_LINKAGE CGBorderGuard : public CGKeys, public IQuestObject

// POSSIBLE
// class DLL_LINKAGE CGSignBottle : public CGObjectInstance //signs and ocean bottles

VCMI_LIB_NAMESPACE_END

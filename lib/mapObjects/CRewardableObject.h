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

#include "../mapObjects/CArmedInstance.h"
#include "../rewardable/Interface.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Base class that can handle granting rewards to visiting heroes.
/// Inherits from CArmedInstance for proper trasfer of armies
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

public:
	/// Visitability checks. Note that hero check includes check for hero owner (returns true if object was visited by player)
	bool wasVisited(PlayerColor player) const override;
	bool wasVisited(const CGHeroInstance * h) const override;
	
	/// gives reward to player or ask for choice in case of multiple rewards
	void onHeroVisit(const CGHeroInstance *h) const override;

	///possibly resets object state
	void newTurn(CRandomGenerator & rand) const override;

	/// gives second part of reward after hero level-ups for proper granting of spells/mana
	void heroLevelUpDone(const CGHeroInstance *hero) const override;

	/// applies player selection of reward
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void initObj(CRandomGenerator & rand) override;
	
	void setPropertyDer(ui8 what, ui32 val) override;

	CRewardableObject();
	
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<Rewardable::Interface&>(*this);
		h & onceVisitableObjectCleared;
	}
};

//TODO:

// MAX
// class DLL_LINKAGE CGPandoraBox : public CArmedInstance
// class DLL_LINKAGE CGEvent : public CGPandoraBox  //event objects
// class DLL_LINKAGE CGSeerHut : public CArmedInstance, public IQuestObject //army is used when giving reward
// class DLL_LINKAGE CGQuestGuard : public CGSeerHut
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
// class DLL_LINKAGE CGWitchHut : public CPlayersVisited
// class DLL_LINKAGE CGScholar : public CGObjectInstance

VCMI_LIB_NAMESPACE_END

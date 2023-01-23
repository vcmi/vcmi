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

#include "CObjectHandler.h"
#include "CArmedInstance.h"

#include "../NetPacksBase.h"
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomRewardObjectInfo;

class CRewardLimiter;
using TRewardLimitersList = std::vector<std::shared_ptr<CRewardLimiter>>;

/// Limiters of rewards. Rewards will be granted to hero only if he satisfies requirements
/// Note: for this is only a test - it won't remove anything from hero (e.g. artifacts or creatures)
/// NOTE: in future should (partially) replace seer hut/quest guard quests checks
class DLL_LINKAGE CRewardLimiter
{
public:
	/// day of week, unused if 0, 1-7 will test for current day of week
	si32 dayOfWeek;

	/// level that hero needs to have
	si32 minLevel;

	/// mana points that hero needs to have
	si32 manaPoints;

	/// percentage of mana points that hero needs to have
	si32 manaPercentage;

	/// resources player needs to have in order to trigger reward
	TResources resources;

	/// skills hero needs to have
	std::vector<si32> primary;
	std::map<SecondarySkill, si32> secondary;

	/// artifacts that hero needs to have (equipped or in backpack) to trigger this
	/// Note: does not checks for multiple copies of the same arts
	std::vector<ArtifactID> artifacts;

	/// creatures that hero needs to have
	std::vector<CStackBasicDescriptor> creatures;

	/// sub-limiters, all must pass for this limiter to pass
	TRewardLimitersList allOf;

	/// sub-limiters, at least one should pass for this limiter to pass
	TRewardLimitersList anyOf;

	/// sub-limiters, none should pass for this limiter to pass
	TRewardLimitersList noneOf;

	CRewardLimiter():
		dayOfWeek(0),
		minLevel(0),
		primary(4, 0)
	{}

	bool heroAllowed(const CGHeroInstance * hero) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dayOfWeek;
		h & minLevel;
		h & manaPoints;
		h & manaPercentage;
		h & resources;
		h & primary;
		h & secondary;
		h & artifacts;
		h & creatures;
		h & allOf;
		h & anyOf;
		h & noneOf;
	}
};

/// Reward that can be granted to a hero
/// NOTE: eventually should replace seer hut rewards and events/pandoras
class DLL_LINKAGE CRewardInfo
{
public:
	/// resources that will be given to player
	TResources resources;

	/// received experience
	ui32 gainedExp;
	/// received levels (converted into XP during grant)
	ui32 gainedLevels;

	/// mana given to/taken from hero, fixed value
	si32 manaDiff;

	/// if giving mana points puts hero above mana pool, any overflow will be multiplied by specified percentage
	si32 manaOverflowFactor;

	/// fixed value, in form of percentage from max
	si32 manaPercentage;

	/// movement points, only for current day. Bonuses should be used to grant MP on any other day
	si32 movePoints;
	/// fixed value, in form of percentage from max
	si32 movePercentage;

	/// list of bonuses, e.g. morale/luck
	std::vector<Bonus> bonuses;

	/// skills that hero may receive or lose
	std::vector<si32> primary;
	std::map<SecondarySkill, si32> secondary;

	/// objects that hero may receive
	std::vector<ArtifactID> artifacts;
	std::vector<SpellID> spells;
	std::vector<CStackBasicDescriptor> creatures;

	/// list of components that will be added to reward description. First entry in list will override displayed component
	std::vector<Component> extraComponents;

	/// if set to true, object will be removed after granting reward
	bool removeObject;

	/// Generates list of components that describes reward for a specific hero
	virtual void loadComponents(std::vector<Component> & comps,
	                            const CGHeroInstance * h) const;
	Component getDisplayedComponent(const CGHeroInstance * h) const;

	CRewardInfo() :
		gainedExp(0),
		gainedLevels(0),
		manaDiff(0),
		manaPercentage(-1),
		movePoints(0),
		movePercentage(-1),
		primary(4, 0),
		removeObject(false)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & resources;
		h & extraComponents;
		h & removeObject;
		h & manaPercentage;
		h & movePercentage;
		h & gainedExp;
		h & gainedLevels;
		h & manaDiff;
		h & manaOverflowFactor;
		h & movePoints;
		h & primary;
		h & secondary;
		h & bonuses;
		h & artifacts;
		h & spells;
		h & creatures;
	}
};

class DLL_LINKAGE CVisitInfo
{
public:
	CRewardLimiter limiter;
	CRewardInfo reward;

	/// Message that will be displayed on granting of this reward, if not empty
	MetaString message;

	/// Chance for this reward to be selected in case of random choice
	si32 selectChance;

	/// How many times this reward can be granted between two resets
	si32 numOfGrantsAllowed;

	/// How many times this reward has been granted since last reset
	si32 numOfGrantsPerformed;

	CVisitInfo():
		selectChance(0),
		numOfGrantsAllowed(0),
		numOfGrantsPerformed(0)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & limiter;
		h & reward;
		h & message;
		h & selectChance;
		h & numOfGrantsAllowed;
		h & numOfGrantsPerformed;
	}
};

namespace Rewardable
{
	const std::array<std::string, 3> SelectModeString{"selectFirst", "selectPlayer", "selectRandom"};
	const std::array<std::string, 5> VisitModeString{"unlimited", "once", "hero", "bonus", "player"};
}

/// Base class that can handle granting rewards to visiting heroes.
/// Inherits from CArmedInstance for proper trasfer of armies
class DLL_LINKAGE CRewardableObject : public CArmedInstance
{
	/// function that must be called if hero got level-up during grantReward call
	void grantRewardAfterLevelup(const CVisitInfo & reward, const CGHeroInstance * hero) const;

	/// grants reward to hero
	void grantRewardBeforeLevelup(const CVisitInfo & reward, const CGHeroInstance * hero) const;

public:
	enum EVisitMode
	{
		VISIT_UNLIMITED, // any number of times. Side effect - object hover text won't contain visited/not visited text
		VISIT_ONCE,      // only once, first to visit get all the rewards
		VISIT_HERO,      // every hero can visit object once
		VISIT_BONUS,     // can be visited by any hero that don't have bonus from this object
		VISIT_PLAYER     // every player can visit object once
	};

protected:
	/// controls selection of reward granted to player
	enum ESelectMode
	{
		SELECT_FIRST,  // first reward that matches limiters
		SELECT_PLAYER, // player can select from all allowed rewards
		SELECT_RANDOM  // reward will be selected from allowed randomly
	};

	/// filters list of visit info and returns rewards that can be granted to current hero
	virtual std::vector<ui32> getAvailableRewards(const CGHeroInstance * hero) const;

	virtual void grantReward(ui32 rewardID, const CGHeroInstance * hero) const;

	virtual CVisitInfo getVisitInfo(int index, const CGHeroInstance *h) const;

	virtual void triggerRewardReset() const;

	/// Rewards that can be granted by an object
	std::vector<CVisitInfo> info;

	/// MetaString's that contain text for messages for specific situations
	MetaString onSelect;
	MetaString onVisited;
	MetaString onEmpty;

	/// how reward will be selected, uses ESelectMode enum
	ui8 selectMode;
	/// contols who can visit an object, uses EVisitMode enum
	ui8 visitMode;
	/// reward selected by player
	ui16 selectedReward;

	/// object visitability info will be reset each resetDuration days
	ui16 resetDuration;

	/// if true - player can refuse visiting an object (e.g. Tomb)
	bool canRefuse;

public:
	EVisitMode getVisitMode() const;
	ui16 getResetDuration() const;

	void setPropertyDer(ui8 what, ui32 val) override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;

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

	/// function that will be called once reward is fully granted to hero
	virtual void onRewardGiven(const CGHeroInstance * hero) const;
	
	void initObj(CRandomGenerator & rand) override;

	CRewardableObject();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & info;
		h & canRefuse;
		h & resetDuration;
		h & onSelect;
		h & onVisited;
		h & onEmpty;
		h & visitMode;
		h & selectMode;
		h & selectedReward;
	}

	// for configuration/object setup
	friend class CRandomRewardObjectInfo;
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

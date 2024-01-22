/*
 * CGTownBuilding.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "IObjectInterface.h"
#include "../rewardable/Interface.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;
class CBuilding;

class DLL_LINKAGE CGTownBuilding : public IObjectInterface
{
///basic class for town structures handled as map objects
public:
	CGTownBuilding(CGTownInstance * town);
	CGTownBuilding(IGameCallback *cb);

	si32 indexOnTV = 0; //identifies its index on towns vector
	
	CGTownInstance * town;

	STRONG_INLINE
	BuildingSubID::EBuildingSubID getBuildingSubtype() const
	{
		return bType;
	}

	STRONG_INLINE
	const BuildingID & getBuildingType() const
	{
		return bID;
	}

	STRONG_INLINE
	void setBuildingSubtype(BuildingSubID::EBuildingSubID subId)
	{
		bType = subId;
	}

	PlayerColor getOwner() const override;
	MapObjectID getObjGroupIndex() const override;
	MapObjectSubID getObjTypeIndex() const override;

	int3 visitablePos() const override;
	int3 getPosition() const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & bID;
		h & indexOnTV;
		h & bType;
	}

protected:
	BuildingID bID; //from buildig list
	BuildingSubID::EBuildingSubID bType = BuildingSubID::NONE;

	std::string getVisitingBonusGreeting() const;
	std::string getCustomBonusGreeting(const Bonus & bonus) const;
};

class DLL_LINKAGE COPWBonus : public CGTownBuilding
{///used for OPW bonusing structures
public:
	std::set<ObjectInstanceID> visitors;
	void setProperty(ObjProperty what, ObjPropertyID identifier) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	COPWBonus(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * TOWN);
	COPWBonus(IGameCallback *cb);

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};

class DLL_LINKAGE CTownBonus : public CGTownBuilding
{
///used for one-time bonusing structures
///feel free to merge inheritance tree
public:
	std::set<ObjectInstanceID> visitors;
	void setProperty(ObjProperty what, ObjPropertyID identifier) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	CTownBonus(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * TOWN);
	CTownBonus(IGameCallback *cb);

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}

private:
	void applyBonuses(CGHeroInstance * h, const BonusList & bonuses) const;
};

class DLL_LINKAGE CTownRewardableBuilding : public CGTownBuilding, public Rewardable::Interface
{
	/// reward selected by player, no serialize
	ui16 selectedReward = 0;
		
	std::set<ObjectInstanceID> visitors;
	
	bool wasVisitedBefore(const CGHeroInstance * contextHero) const;
	
	void grantReward(ui32 rewardID, const CGHeroInstance * hero) const;
	
public:
	void setProperty(ObjProperty what, ObjPropertyID identifier) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	
	void newTurn(CRandomGenerator & rand) const override;
	
	/// gives second part of reward after hero level-ups for proper granting of spells/mana
	void heroLevelUpDone(const CGHeroInstance *hero) const override;
	
	void initObj(CRandomGenerator & rand) override;
	
	/// applies player selection of reward
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;
	
	CTownRewardableBuilding(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * town, CRandomGenerator & rand);
	CTownRewardableBuilding(IGameCallback *cb);
	
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & static_cast<Rewardable::Interface&>(*this);
		h & visitors;
	}
};

VCMI_LIB_NAMESPACE_END

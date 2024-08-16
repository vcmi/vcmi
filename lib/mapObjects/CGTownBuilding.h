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
	CGTownBuilding(CGTownInstance * town, const BuildingID & index);
	CGTownBuilding(IGameCallback *cb);

	
	CGTownInstance * town;

	STRONG_INLINE
	const BuildingID & getBuildingType() const
	{
		return bID;
	}

	PlayerColor getOwner() const override;
	MapObjectID getObjGroupIndex() const override;
	MapObjectSubID getObjTypeIndex() const override;

	int3 visitablePos() const override;
	int3 getPosition() const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & bID;
		if (h.version >= Handler::Version::NEW_TOWN_BUILDINGS)
		{
			// no-op
		}
		else
		{
			si32 indexOnTV = 0; //identifies its index on towns vector
			BuildingSubID::EBuildingSubID bType = BuildingSubID::NONE;

			h & indexOnTV;
			h & bType;

		}
	}

private:
	BuildingID bID; //from building list
};

class DLL_LINKAGE CTownRewardableBuilding : public CGTownBuilding, public Rewardable::Interface
{
	/// reward selected by player, no serialize
	ui16 selectedReward = 0;
		
	std::set<ObjectInstanceID> visitors;
	
	bool wasVisitedBefore(const CGHeroInstance * contextHero) const;
	
	void grantReward(ui32 rewardID, const CGHeroInstance * hero) const;

	Rewardable::Configuration generateConfiguration(vstd::RNG & rand) const;
	
public:
	void setProperty(ObjProperty what, ObjPropertyID identifier) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	
	void newTurn(vstd::RNG & rand) const override;
	
	/// gives second part of reward after hero level-ups for proper granting of spells/mana
	void heroLevelUpDone(const CGHeroInstance *hero) const override;
	
	void initObj(vstd::RNG & rand) override;
	
	/// applies player selection of reward
	void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const override;
	
	CTownRewardableBuilding(CGTownInstance * town, const BuildingID & index, vstd::RNG & rand);
	CTownRewardableBuilding(IGameCallback *cb);
	
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & static_cast<Rewardable::Interface&>(*this);
		h & visitors;
	}
};

/// Compatibility for old code
class DLL_LINKAGE CTownCompatBuilding1 : public CTownRewardableBuilding
{
public:
	using CTownRewardableBuilding::CTownRewardableBuilding;

	template <typename Handler> void serialize(Handler &h)
	{
		if (h.version >= Handler::Version::NEW_TOWN_BUILDINGS)
		{
			h & static_cast<CTownRewardableBuilding&>(*this);
		}
		else
		{
			h & static_cast<CGTownBuilding&>(*this);
			std::set<ObjectInstanceID> visitors;
			h & visitors;
		}
	}
};

/// Compatibility for old code
class DLL_LINKAGE CTownCompatBuilding2 : public CTownRewardableBuilding
{
public:
	using CTownRewardableBuilding::CTownRewardableBuilding;

	template <typename Handler> void serialize(Handler &h)
	{
		if (h.version >= Handler::Version::NEW_TOWN_BUILDINGS)
		{
			h & static_cast<CTownRewardableBuilding&>(*this);
		}
		else
		{
			h & static_cast<CGTownBuilding&>(*this);
			std::set<ObjectInstanceID> visitors;
			h & visitors;
		}
	}
};

VCMI_LIB_NAMESPACE_END

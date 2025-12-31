/*
 * TownBuildingInstance.h, part of VCMI engine
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

class DLL_LINKAGE TownBuildingInstance : public IObjectInterface
{
///basic class for town structures handled as map objects
public:
	TownBuildingInstance(CGTownInstance * town, const BuildingID & index);
	TownBuildingInstance(IGameInfoCallback *cb);

	CGTownInstance * town;

	const BuildingID & getBuildingType() const
	{
		return bID;
	}

	PlayerColor getOwner() const override;
	MapObjectID getObjGroupIndex() const override;
	MapObjectSubID getObjTypeIndex() const override;
	const IOwnableObject * asOwnable() const override;

	int3 visitablePos() const override;
	int3 anchorPos() const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & bID;
	}

private:
	BuildingID bID; //from building list
};

class DLL_LINKAGE TownRewardableBuildingInstance : public TownBuildingInstance, public Rewardable::Interface
{
	/// reward selected by player, no serialize
	ui16 selectedReward = 0;
	std::set<ObjectInstanceID> visitors;

	bool wasVisitedBefore(const CGHeroInstance * contextHero) const override;
	void grantReward(IGameEventCallback & gameEvents, ui32 rewardID, const CGHeroInstance * hero) const override;
	Rewardable::Configuration generateConfiguration(IGameRandomizer & gameRandomizer) const;
	void assignBonuses(std::vector<std::shared_ptr<Bonus>> & bonuses) const;

	const IObjectInterface * getObject() const override;
	bool wasVisited(PlayerColor player) const override;
	void markAsVisited(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const override;
	void markAsScouted(IGameEventCallback & gameEvents, const CGHeroInstance * hero) const override;
public:
	void setProperty(ObjProperty what, ObjPropertyID identifier) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	bool wasVisited(const CGHeroInstance * contextHero) const override;
	
	void newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const override;
	
	/// gives second part of reward after hero level-ups for proper granting of spells/mana
	void heroLevelUpDone(IGameEventCallback & gameEvents, const CGHeroInstance *hero) const override;
	
	/// applies player selection of reward
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;
	
	TownRewardableBuildingInstance(CGTownInstance * town, const BuildingID & index, IGameRandomizer & gameRandomizer);
	TownRewardableBuildingInstance(IGameInfoCallback *cb);
	
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<TownBuildingInstance&>(*this);
		h & static_cast<Rewardable::Interface&>(*this);
		h & visitors;
	}
};

VCMI_LIB_NAMESPACE_END

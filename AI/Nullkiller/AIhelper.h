/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

/*
	!!!	Note: Include THIS file at the end of include list to avoid "undefined base class" error
*/

#include "ArmyManager.h"
#include "HeroManager.h"
#include "Pathfinding/PathfindingManager.h"

class ResourceManager;
class BuildingManager;


//TODO: remove class, put managers to engine
class DLL_EXPORT AIhelper : public IPathfindingManager, public IArmyManager, public IHeroManager
{
	//std::shared_ptr<ResourceManager> resourceManager;
	//std::shared_ptr<BuildingManager> buildingManager;
	std::shared_ptr<PathfindingManager> pathfindingManager;
	std::shared_ptr<ArmyManager> armyManager;
	std::shared_ptr<HeroManager> heroManager;
	//TODO: vector<IAbstractManager>
public:
	AIhelper();
	~AIhelper();

	std::vector<AIPath> getPathsToTile(const HeroPtr & hero, const int3 & tile) const override;
	std::vector<AIPath> getPathsToTile(const int3 & tile) const override;
	void updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain = false) override;

	STRONG_INLINE
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const
	{
		return pathfindingManager->isTileAccessible(hero, tile);
	}

	bool canGetArmy(const CArmedInstance * target, const CArmedInstance * source) const override;
	ui64 howManyReinforcementsCanBuy(const CCreatureSet * target, const CGDwelling * source) const override;
	ui64 howManyReinforcementsCanGet(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<SlotInfo> getBestArmy(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<SlotInfo>::iterator getWeakestCreature(std::vector<SlotInfo> & army) const override;
	std::vector<SlotInfo> getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<creInfo> getArmyAvailableToBuy(const CCreatureSet * hero, const CGDwelling * dwelling) const override;
	uint64_t evaluateStackPower(const CCreature * creature, int count) const override;
	SlotInfo getTotalCreaturesAvailable(CreatureID creatureID) const override;
	ArmyUpgradeInfo calculateCreateresUpgrade(
		const CCreatureSet * army,
		const CGObjectInstance * upgrader,
		const TResources & availableResources) const override;

	const std::map<HeroPtr, HeroRole> & getHeroRoles() const override;
	HeroRole getHeroRole(const HeroPtr & hero) const override;
	int selectBestSkill(const HeroPtr & hero, const std::vector<SecondarySkill> & skills) const override;
	float evaluateSecSkill(SecondarySkill skill, const CGHeroInstance * hero) const override;
	float evaluateHero(const CGHeroInstance * hero) const override;

	void update() override;

	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;
};


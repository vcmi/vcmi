/*
 * CBattleInfoEssentials.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "IBattleInfoCallback.h"
#include "BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;
class CGHeroInstance;
class CStack;
class IBonusBearer;
struct InfoAboutHero;
struct TownFortifications;
class CArmedInstance;

using TStacks = std::vector<const CStack *>;
using TStackFilter = std::function<bool (const CStack *)>;

class DLL_LINKAGE CBattleInfoEssentials : public IBattleInfoCallback
{
protected:
	bool battleDoWeKnowAbout(BattleSide side) const;

public:
	enum EStackOwnership
	{
		ONLY_MINE, ONLY_ENEMY, MINE_AND_ENEMY
	};

	bool duringBattle() const;
	BattleSide battleGetMySide() const;
	const IBonusBearer * getBonusBearer() const override;

	TerrainId battleTerrainType() const override;
	BattleField battleGetBattlefieldType() const override;
	int32_t battleGetEnchanterCounter(BattleSide side) const;

	std::vector<std::shared_ptr<const CObstacleInstance>> battleGetAllObstacles(std::optional<BattleSide> perspective = std::nullopt) const; //returns all obstacles on the battlefield

	std::shared_ptr<const CObstacleInstance> battleGetObstacleByID(uint32_t ID) const;

	/** @brief Main method for getting battle stacks
	 * returns also turrets and removed stacks
	 * @param predicate Functor that shall return true for desired stack
	 * @return filtered stacks
	 *
	 */
	TStacks battleGetStacksIf(const TStackFilter & predicate) const; //deprecated
	battle::Units battleGetUnitsIf(const battle::UnitFilter & predicate) const override;

	const battle::Unit * battleGetUnitByID(uint32_t ID) const override;
	const battle::Unit * battleActiveUnit() const override;

	uint32_t battleNextUnitId() const override;

	bool battleHasNativeStack(BattleSide side) const;
	const CGTownInstance * battleGetDefendedTown() const; //returns defended town if current battle is a siege, nullptr instead

	si8 battleTacticDist() const override; //returns tactic distance in current tactics phase; 0 if not in tactics phase
	BattleSide battleGetTacticsSide() const override; //returns which side is in tactics phase, undefined if none (?)

	bool battleCanFlee(const PlayerColor & player) const;
	bool battleCanSurrender(const PlayerColor & player) const;

	static BattleSide otherSide(BattleSide side);
	PlayerColor otherPlayer(const PlayerColor & player) const;

	BattleSide playerToSide(const PlayerColor & player) const;
	PlayerColor sideToPlayer(BattleSide side) const;
	bool playerHasAccessToHeroInfo(const PlayerColor & player, const CGHeroInstance * h) const;
	TownFortifications battleGetFortifications() const;
	bool battleHasHero(BattleSide side) const;
	int32_t battleCastSpells(BattleSide side) const; //how many spells has given side cast
	const CGHeroInstance * battleGetFightingHero(BattleSide side) const; //deprecated for players callback, easy to get wrong
	const CArmedInstance * battleGetArmyObject(BattleSide side) const;
	InfoAboutHero battleGetHeroInfo(BattleSide side) const;

	// for determining state of a part of the wall; format: parameter [0] - keep, [1] - bottom tower, [2] - bottom wall,
	// [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate; returned value: 1 - intact, 2 - damaged, 3 - destroyed; 0 - no battle
	EWallState battleGetWallState(EWallPart partOfWall) const;
	EGateState battleGetGateState() const;
	bool battleIsGatePassable() const;

	//helpers
	///returns all stacks, alive or dead or undead or mechanical :)
	TStacks battleGetAllStacks(bool includeTurrets = false) const;

	const CStack * battleGetStackByID(int ID, bool onlyAlive = true) const; //returns stack info by given ID
	bool battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattleSide side) const;

	///returns player that controls given stack; mind control included
	PlayerColor battleGetOwner(const battle::Unit * unit) const;

	///returns hero that controls given stack; nullptr if none; mind control included
	const CGHeroInstance * battleGetOwnerHero(const battle::Unit * unit) const;

	///check that stacks are controlled by same|other player(s) depending on positiveness
	///mind control included
	bool battleMatchOwner(const battle::Unit * attacker, const battle::Unit * defender, const boost::logic::tribool positivness = false) const;
	bool battleMatchOwner(const PlayerColor & attacker, const battle::Unit * defender, const boost::logic::tribool positivness = false) const;
};

VCMI_LIB_NAMESPACE_END

/*
 * BattleInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../int3.h"
#include "../HeroBonus.h"
#include "CBattleInfoCallback.h"
#include "IBattleState.h"
#include "SiegeInfo.h"
#include "SideInBattle.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class CStackInstance;
class CStackBasicDescriptor;
class BattleField;

class DLL_LINKAGE BattleInfo : public CBonusSystemNode, public CBattleInfoCallback, public IBattleState
{
public:
	enum BattleSide
	{
		ATTACKER = 0,
		DEFENDER
	};
	std::array<SideInBattle, 2> sides; //sides[0] - attacker, sides[1] - defender
	si32 round, activeStack;
	const CGTownInstance * town; //used during town siege, nullptr if this is not a siege (note that fortless town IS also a siege)
	int3 tile; //for background and bonuses
	std::vector<CStack*> stacks;
	std::vector<std::shared_ptr<CObstacleInstance> > obstacles;
	SiegeInfo si;

	BattleField battlefieldType; //like !!BA:B
	TerrainId terrainType; //used for some stack nativity checks (not the bonus limiters though that have their own copy)

	ui8 tacticsSide; //which side is requested to play tactics phase
	ui8 tacticDistance; //how many hexes we can go forward (1 = only hexes adjacent to margin line)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sides;
		h & round;
		h & activeStack;
		h & town;
		h & tile;
		h & stacks;
		h & obstacles;
		h & si;
		h & battlefieldType;
		h & terrainType;
		h & tacticsSide;
		h & tacticDistance;
		h & static_cast<CBonusSystemNode&>(*this);
	}

	//////////////////////////////////////////////////////////////////////////
	BattleInfo();
	virtual ~BattleInfo();

	//////////////////////////////////////////////////////////////////////////
	// IBattleInfo

	int32_t getActiveStackID() const override;

	TStacks getStacksIf(TStackFilter predicate) const override;

	battle::Units getUnitsIf(battle::UnitFilter predicate) const override;

	BattleField getBattlefieldType() const override;
	TerrainId getTerrainType() const override;

	ObstacleCList getAllObstacles() const override;

	PlayerColor getSidePlayer(ui8 side) const override;
	const CArmedInstance * getSideArmy(ui8 side) const override;
	const CGHeroInstance * getSideHero(ui8 side) const override;

	ui8 getTacticDist() const override;
	ui8 getTacticsSide() const override;

	const CGTownInstance * getDefendedTown() const override;
	EWallState getWallState(EWallPart partOfWall) const override;
	EGateState getGateState() const override;

	uint32_t getCastSpells(ui8 side) const override;
	int32_t getEnchanterCounter(ui8 side) const override;

	const IBonusBearer * asBearer() const override;

	uint32_t nextUnitId() const override;

	int64_t getActualDamage(const DamageRange & damage, int32_t attackerCount, vstd::RNG & rng) const override;

	//////////////////////////////////////////////////////////////////////////
	// IBattleState

	void nextRound(int32_t roundNr) override;
	void nextTurn(uint32_t unitId) override;

	void addUnit(uint32_t id, const JsonNode & data) override;
	void moveUnit(uint32_t id, BattleHex destination) override;
	void setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta) override;
	void removeUnit(uint32_t id) override;
	void updateUnit(uint32_t id, const JsonNode & data) override;

	void addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;

	void setWallState(EWallPart partOfWall, EWallState state) override;

	void addObstacle(const ObstacleChanges & changes) override;
	void updateObstacle(const ObstacleChanges& changes) override;
	void removeObstacle(uint32_t id) override;

	static void addOrUpdateUnitBonus(CStack * sta, const Bonus & value, bool forceAdd);

	//////////////////////////////////////////////////////////////////////////
	CStack * getStack(int stackID, bool onlyAlive = true);
	using CBattleInfoEssentials::battleGetArmyObject;
	CArmedInstance * battleGetArmyObject(ui8 side) const;
	using CBattleInfoEssentials::battleGetFightingHero;
	CGHeroInstance * battleGetFightingHero(ui8 side) const;

	std::pair< std::vector<BattleHex>, int > getPath(BattleHex start, BattleHex dest, const CStack * stack); //returned value: pair<path, length>; length may be different than number of elements in path since flying vreatures jump between distant hexes

	void calculateCasualties(std::map<ui32,si32> * casualties) const; //casualties are array of maps size 2 (attacker, defeneder), maps are (crid => amount)

	CStack * generateNewStack(uint32_t id, const CStackInstance & base, ui8 side, const SlotID & slot, BattleHex position);
	CStack * generateNewStack(uint32_t id, const CStackBasicDescriptor & base, ui8 side, const SlotID & slot, BattleHex position);

	const CGHeroInstance * getHero(const PlayerColor & player) const; //returns fighting hero that belongs to given player

	void localInit();
	static BattleInfo * setupBattle(const int3 & tile, TerrainId, const BattleField & battlefieldType, const CArmedInstance * armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance * town);

	ui8 whatSide(const PlayerColor & player) const;

protected:
#if SCRIPTING_ENABLED
	scripting::Pool * getContextPool() const override;
#endif
};


class DLL_LINKAGE CMP_stack
{
	int phase; //rules of which phase will be used
	int turn;
	uint8_t side;
public:
	bool operator()(const battle::Unit * a, const battle::Unit * b) const;
	CMP_stack(int Phase = 1, int Turn = 0, uint8_t Side = BattleSide::ATTACKER);
};

VCMI_LIB_NAMESPACE_END

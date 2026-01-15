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

#include "CBattleInfoCallback.h"
#include "IBattleState.h"
#include "SideInBattle.h"
#include "SiegeInfo.h"

#include "../callback/GameCallbackHolder.h"
#include "../bonuses/Bonus.h"
#include "../bonuses/CBonusSystemNode.h"
#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class CStackInstance;
class CStackBasicDescriptor;
class BattleField;
struct BattleLayout;

class DLL_LINKAGE BattleInfo : public CBonusSystemNode, public CBattleInfoCallback, public IBattleState, public GameCallbackHolder
{
	BattleSideArray<SideInBattle> sides; //sides[0] - attacker, sides[1] - defender
	std::unique_ptr<BattleLayout> layout;
	si32 round;

	void postDeserialize();
public:
	BattleID battleID = BattleID(0);

	si32 activeStack;
	ObjectInstanceID townID; //used during town siege, nullptr if this is not a siege (note that fortless town IS also a siege)
	int3 tile; //for background and bonuses
	bool replayAllowed;
	std::vector<std::unique_ptr<CStack>> stacks;
	std::vector<std::shared_ptr<CObstacleInstance> > obstacles;
	SiegeInfo si;

	BattleField battlefieldType; //like !!BA:B
	TerrainId terrainType; //used for some stack nativity checks (not the bonus limiters though that have their own copy)

	BattleSide tacticsSide; //which side is requested to play tactics phase
	ui8 tacticDistance; //how many hexes we can go forward (1 = only hexes adjacent to margin line)

	template <typename Handler> void serialize(Handler &h)
	{
		h & battleID;
		h & sides;
		h & round;
		h & activeStack;
		h & townID;
		h & tile;
		h & stacks;
		h & obstacles;
		h & si;
		h & battlefieldType;
		h & terrainType;
		h & tacticsSide;
		h & tacticDistance;
		h & static_cast<CBonusSystemNode&>(*this);
		h & replayAllowed;

		if(!h.saving)
			postDeserialize();
	}

	//////////////////////////////////////////////////////////////////////////
	BattleInfo(IGameInfoCallback *cb, const BattleLayout & layout);
	BattleInfo(IGameInfoCallback *cb);
	virtual ~BattleInfo();

	const IBattleInfo * getBattle() const override;
	std::optional<PlayerColor> getPlayerID() const override;

	//////////////////////////////////////////////////////////////////////////
	// IBattleInfo

	BattleID getBattleID() const override;

	int32_t getActiveStackID() const override;

	TStacks getStacksIf(const TStackFilter & predicate) const override;

	battle::Units getUnitsIf(const battle::UnitFilter & predicate) const override;

	BattleField getBattlefieldType() const override;
	TerrainId getTerrainType() const override;

	ObstacleCList getAllObstacles() const override;

	PlayerColor getSidePlayer(BattleSide side) const override;
	const CArmedInstance * getSideArmy(BattleSide side) const override;
	const CGHeroInstance * getSideHero(BattleSide side) const override;

	ui8 getTacticDist() const override;
	BattleSide getTacticsSide() const override;
	int32_t getRound() const override;

	const CGTownInstance * getDefendedTown() const override;
	EWallState getWallState(EWallPart partOfWall) const override;
	EGateState getGateState() const override;

	int32_t getCastSpells(BattleSide side) const override;
	int32_t getEnchanterCounter(BattleSide side) const override;

	const IBonusBearer * getBonusBearer() const override;

	uint32_t nextUnitId() const override;

	int64_t getActualDamage(const DamageRange & damage, int32_t attackerCount, vstd::RNG & rng) const override;

	int3 getLocation() const override;
	BattleLayout getLayout() const override;

	std::vector<SpellID> getUsedSpells(BattleSide side) const override;

	//////////////////////////////////////////////////////////////////////////
	// IBattleState

	void nextRound() override;
	void nextTurn(uint32_t unitId, BattleUnitTurnReason reason) override;

	void addUnit(uint32_t id, const JsonNode & data) override;
	void moveUnit(uint32_t id, const BattleHex & destination) override;
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
	CArmedInstance * battleGetArmyObject(BattleSide side) const;
	using CBattleInfoEssentials::battleGetFightingHero;
	CGHeroInstance * battleGetFightingHero(BattleSide side) const;

	void generateNewStack(uint32_t id, const CStackInstance & base, BattleSide side, const SlotID & slot, const BattleHex & position);
	void generateNewStack(uint32_t id, const CStackBasicDescriptor & base, BattleSide side, const SlotID & slot, const BattleHex & position);

	const SideInBattle & getSide(BattleSide side) const;
	SideInBattle & getSide(BattleSide side);

	const CGHeroInstance * getHero(const PlayerColor & player) const; //returns fighting hero that belongs to given player

	void localInit();
	static std::unique_ptr<BattleInfo> setupBattle(IGameInfoCallback *cb, const int3 & tile, TerrainId, const BattleField & battlefieldType, BattleSideArray<const CArmedInstance *> armies, BattleSideArray<const CGHeroInstance *> heroes, const BattleLayout & layout, const CGTownInstance * town);

	BattleSide whatSide(const PlayerColor & player) const;

protected:
#if SCRIPTING_ENABLED
	scripting::Pool * getContextPool() const override;
#endif
};


class DLL_LINKAGE CMP_stack
{
	int phase; //rules of which phase will be used
	int turn;
	BattleSide side;
public:
	bool operator()(const battle::Unit * a, const battle::Unit * b) const;
	CMP_stack(int Phase = 1, int Turn = 0, BattleSide Side = BattleSide::ATTACKER);
};

VCMI_LIB_NAMESPACE_END

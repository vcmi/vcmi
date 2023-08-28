/*
 * CBattleInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/spells/Magic.h>

#include "CCallbackBase.h"
#include "ReachabilityInfo.h"
#include "BattleAttackInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CStack;
class ISpellCaster;
class SpellCastEnvironment;
class CSpell;
struct CObstacleInstance;
class IBonusBearer;
class CRandomGenerator;
class PossiblePlayerBattleAction;

namespace spells
{
	class Caster;
	class Spell;
}

struct DLL_LINKAGE AttackableTiles
{
	std::set<BattleHex> hostileCreaturePositions;
	std::set<BattleHex> friendlyCreaturePositions; //for Dragon Breath
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hostileCreaturePositions;
		h & friendlyCreaturePositions;
	}
};

struct DLL_LINKAGE BattleClientInterfaceData
{
	std::vector<SpellID> creatureSpellsToCast;
	ui8 tacticsMode;
};

class DLL_LINKAGE CBattleInfoCallback : public virtual CBattleInfoEssentials
{
public:
	enum ERandomSpell
	{
		RANDOM_GENIE, RANDOM_AIMED
	};

	std::optional<int> battleIsFinished() const override; //return none if battle is ongoing; otherwise the victorious side (0/1) or 2 if it is a draw

	std::vector<std::shared_ptr<const CObstacleInstance>> battleGetAllObstaclesOnPos(BattleHex tile, bool onlyBlocking = true) const override;
	std::vector<std::shared_ptr<const CObstacleInstance>> getAllAffectedObstaclesByStack(const battle::Unit * unit, const std::set<BattleHex> & passed) const override;
	//Handle obstacle damage here, requires SpellCastEnvironment
	bool handleObstacleTriggersForUnit(SpellCastEnvironment & spellEnv, const battle::Unit & unit, const std::set<BattleHex> & passed = {}) const;

	const CStack * battleGetStackByPos(BattleHex pos, bool onlyAlive = true) const;

	const battle::Unit * battleGetUnitByPos(BattleHex pos, bool onlyAlive = true) const override;

	///returns all alive units excluding turrets
	battle::Units battleAliveUnits() const;
	///returns all alive units from particular side excluding turrets
	battle::Units battleAliveUnits(ui8 side) const;

	void battleGetTurnOrder(std::vector<battle::Units> & out, const size_t maxUnits, const int maxTurns, const int turn = 0, int8_t lastMoved = -1) const;

	///returns reachable hexes (valid movement destinations), DOES contain stack current position
	std::vector<BattleHex> battleGetAvailableHexes(const battle::Unit * unit, bool obtainMovementRange, bool addOccupiable, std::vector<BattleHex> * attackable) const;

	///returns reachable hexes (valid movement destinations), DOES contain stack current position (lite version)
	std::vector<BattleHex> battleGetAvailableHexes(const battle::Unit * unit, bool obtainMovementRange) const;

	std::vector<BattleHex> battleGetAvailableHexes(const ReachabilityInfo & cache, const battle::Unit * unit, bool obtainMovementRange) const;

	int battleGetSurrenderCost(const PlayerColor & Player) const; //returns cost of surrendering battle, -1 if surrendering is not possible
	ReachabilityInfo::TDistances battleGetDistances(const battle::Unit * unit, BattleHex assumedPosition) const;
	std::set<BattleHex> battleGetAttackedHexes(const battle::Unit * attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID) const;
	bool isEnemyUnitWithinSpecifiedRange(BattleHex attackerPosition, const battle::Unit * defenderUnit, unsigned int range) const;

	std::pair< std::vector<BattleHex>, int > getPath(BattleHex start, BattleHex dest, const battle::Unit * stack) const;

	bool battleCanAttack(const battle::Unit * stack, const battle::Unit * target, BattleHex dest) const; //determines if stack with given ID can attack target at the selected destination
	bool battleCanShoot(const battle::Unit * attacker, BattleHex dest) const; //determines if stack with given ID shoot at the selected destination
	bool battleCanShoot(const battle::Unit * attacker) const; //determines if stack with given ID shoot in principle
	bool battleIsUnitBlocked(const battle::Unit * unit) const; //returns true if there is neighboring enemy stack
	std::set<const battle::Unit *> battleAdjacentUnits(const battle::Unit * unit) const;

	DamageEstimation calculateDmgRange(const BattleAttackInfo & info) const;

	/// estimates damage dealt by attacker to defender;
	/// only non-random bonuses are considered in estimation
	/// returns pair <min dmg, max dmg>
	DamageEstimation battleEstimateDamage(const BattleAttackInfo & bai, DamageEstimation * retaliationDmg = nullptr) const;
	DamageEstimation battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPosition, DamageEstimation * retaliationDmg = nullptr) const;
	DamageEstimation battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, int movementDistance, DamageEstimation * retaliationDmg = nullptr) const;

	bool battleHasPenaltyOnLine(BattleHex from, BattleHex dest, bool checkWall, bool checkMoat) const;
	bool battleHasDistancePenalty(const IBonusBearer * shooter, BattleHex shooterPosition, BattleHex destHex) const;
	bool battleHasWallPenalty(const IBonusBearer * shooter, BattleHex shooterPosition, BattleHex destHex) const;
	bool battleHasShootingPenalty(const battle::Unit * shooter, BattleHex destHex) const;

	BattleHex wallPartToBattleHex(EWallPart part) const;
	EWallPart battleHexToWallPart(BattleHex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	bool isWallPartPotentiallyAttackable(EWallPart wallPart) const; // returns true if the wall part is potentially attackable (independent of wall state), false if not
	bool isWallPartAttackable(EWallPart wallPart) const; // returns true if the wall part is actually attackable, false if not
	std::vector<BattleHex> getAttackableBattleHexes() const;

	si8 battleMinSpellLevel(ui8 side) const; //calculates maximum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	si8 battleMaxSpellLevel(ui8 side) const; //calculates minimum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	int32_t battleGetSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	ESpellCastProblem battleCanCastSpell(const spells::Caster * caster, spells::Mode mode) const; //returns true if there are no general issues preventing from casting a spell

	SpellID battleGetRandomStackSpell(CRandomGenerator & rand, const CStack * stack, ERandomSpell mode) const;
	SpellID getRandomBeneficialSpell(CRandomGenerator & rand, const CStack * subject) const;
	SpellID getRandomCastedSpell(CRandomGenerator & rand, const CStack * caster) const; //called at the beginning of turn for Faerie Dragon

	std::vector<PossiblePlayerBattleAction> getClientActionsForStack(const CStack * stack, const BattleClientInterfaceData & data);
	PossiblePlayerBattleAction getCasterAction(const CSpell * spell, const spells::Caster * caster, spells::Mode mode) const;

	//convenience methods using the ones above
	bool isInTacticRange(BattleHex dest) const;
	si8 battleGetTacticDist() const; //returns tactic distance for calling player or 0 if this player is not in tactic phase (for ALL_KNOWING actual distance for tactic side)

	AttackableTiles getPotentiallyAttackableHexes(const  battle::Unit* attacker, BattleHex destinationTile, BattleHex attackerPos) const; //TODO: apply rotation to two-hex attacker
	AttackableTiles getPotentiallyShootableHexes(const  battle::Unit* attacker, BattleHex destinationTile, BattleHex attackerPos) const;
	std::vector<const battle::Unit *> getAttackedBattleUnits(const battle::Unit* attacker, BattleHex destinationTile, bool rangedAttack, BattleHex attackerPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	std::set<const CStack*> getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, bool rangedAttack, BattleHex attackerPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	bool isToReverse(const battle::Unit * attacker, const battle::Unit * defender) const; //determines if attacker standing at attackerHex should reverse in order to attack defender

	ReachabilityInfo getReachability(const battle::Unit * unit) const;
	ReachabilityInfo getReachability(const ReachabilityInfo::Parameters & params) const;
	AccessibilityInfo getAccesibility() const;
	AccessibilityInfo getAccesibility(const battle::Unit * stack) const; //Hexes ocupied by stack will be marked as accessible.
	AccessibilityInfo getAccesibility(const std::vector<BattleHex> & accessibleHexes) const; //given hexes will be marked as accessible
	std::pair<const battle::Unit *, BattleHex> getNearestStack(const battle::Unit * closest) const;

	BattleHex getAvaliableHex(const CreatureID & creID, ui8 side, int initialPos = -1) const; //find place for adding new stack
protected:
	ReachabilityInfo getFlyingReachability(const ReachabilityInfo::Parameters & params) const;
	ReachabilityInfo makeBFS(const AccessibilityInfo & accessibility, const ReachabilityInfo::Parameters & params) const;
	bool isInObstacle(BattleHex hex, const std::set<BattleHex> & obstacles, const ReachabilityInfo::Parameters & params) const;
	std::set<BattleHex> getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const; //get hexes with stopping obstacles (quicksands)
};

VCMI_LIB_NAMESPACE_END

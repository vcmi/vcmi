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
class PossiblePlayerBattleAction;

namespace vstd
{
class RNG;
}

namespace spells
{
	class Caster;
	class Spell;
}

struct DLL_LINKAGE AttackableTiles
{
	/// Hexes on which only hostile units will be targeted
	BattleHexArray hostileCreaturePositions;
	/// for Dragon Breath, hexes on which both friendly and hostile creatures will be targeted
	BattleHexArray friendlyCreaturePositions;
	/// for animation purposes, if any of targets are on specified positions, unit should play alternative animation
	BattleHexArray overrideAnimationPositions;
};

struct DLL_LINKAGE BattleClientInterfaceData
{
	std::vector<SpellID> creatureSpellsToCast;
	ui8 tacticsMode;
};

struct ForcedAction {
	EActionType type;
	BattleHex position;
	const battle::Unit * target;
};

class DLL_LINKAGE CBattleInfoCallback : public virtual CBattleInfoEssentials
{
public:
	std::optional<BattleSide> battleIsFinished() const override; //return none if battle is ongoing; otherwise the victorious side (0/1) or 2 if it is a draw

	std::vector<std::shared_ptr<const CObstacleInstance>> battleGetAllObstaclesOnPos(const BattleHex & tile, bool onlyBlocking = true) const override;
	std::vector<std::shared_ptr<const CObstacleInstance>> getAllAffectedObstaclesByStack(const battle::Unit * unit, const BattleHexArray & passed) const override;
	//Handle obstacle damage here, requires SpellCastEnvironment
	bool handleObstacleTriggersForUnit(SpellCastEnvironment & spellEnv, const battle::Unit & unit, const BattleHexArray & passed = {}) const;

	const CStack * battleGetStackByPos(const BattleHex & pos, bool onlyAlive = true) const;

	const battle::Unit * battleGetUnitByPos(const BattleHex & pos, bool onlyAlive = true) const override;

	///returns all alive units excluding turrets
	battle::Units battleAliveUnits() const;
	///returns all alive units from particular side excluding turrets
	battle::Units battleAliveUnits(BattleSide side) const;

	void battleGetTurnOrder(std::vector<battle::Units> & out, const size_t maxUnits, const int maxTurns, const int turn = 0, BattleSide lastMoved = BattleSide::NONE) const;

	///returns reachable hexes (valid movement destinations), DOES contain stack current position
	BattleHexArray battleGetAvailableHexes(const battle::Unit * unit, bool obtainMovementRange, bool addOccupiable, BattleHexArray * attackable) const;

	///returns reachable hexes (valid movement destinations), DOES contain stack current position (lite version)
	BattleHexArray battleGetAvailableHexes(const battle::Unit * unit, bool obtainMovementRange) const;

	BattleHexArray battleGetAvailableHexes(const ReachabilityInfo & cache, const battle::Unit * unit, bool obtainMovementRange) const;

	int battleGetSurrenderCost(const PlayerColor & Player) const; //returns cost of surrendering battle, -1 if surrendering is not possible
	ReachabilityInfo::TDistances battleGetDistances(const battle::Unit * unit, const BattleHex & assumedPosition) const;
	BattleHexArray battleGetAttackedHexes(const battle::Unit * attacker, const BattleHex & destinationTile, const BattleHex & attackerPos = BattleHex::INVALID) const;
	bool isEnemyUnitWithinSpecifiedRange(const BattleHex & attackerPosition, const battle::Unit * defenderUnit, unsigned int range) const;
	bool isHexWithinSpecifiedRange(const BattleHex & attackerPosition, const BattleHex & targetPosition, unsigned int range) const;

	std::pair< BattleHexArray, int > getPath(const BattleHex & start, const BattleHex & dest, const battle::Unit * stack) const;

	bool battleCanTargetEmptyHex(const battle::Unit * attacker) const; //determines of stack with given ID can target empty hex to attack - currently used only for SPELL_LIKE_ATTACK shooting
	bool battleCanAttack(const battle::Unit * stack, const battle::Unit * target, const BattleHex & dest) const; //determines if stack with given ID can attack target at the selected destination
	bool battleCanShoot(const battle::Unit * attacker, const BattleHex & dest) const; //determines if stack with given ID shoot at the selected destination
	bool battleCanShoot(const battle::Unit * attacker) const; //determines if stack with given ID shoot in principle
	bool battleIsUnitBlocked(const battle::Unit * unit) const; //returns true if there is neighboring enemy stack
	battle::Units battleAdjacentUnits(const battle::Unit * unit) const;

	DamageEstimation calculateDmgRange(const BattleAttackInfo & info) const;

	/// estimates damage dealt by attacker to defender;
	/// only non-random bonuses are considered in estimation
	/// returns pair <min dmg, max dmg>
	DamageEstimation battleEstimateDamage(const BattleAttackInfo & bai, DamageEstimation * retaliationDmg = nullptr) const;
	DamageEstimation battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, const BattleHex & attackerPosition, DamageEstimation * retaliationDmg = nullptr) const;
	DamageEstimation battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, int getMovementRange, DamageEstimation * retaliationDmg = nullptr) const;

	bool battleIsInsideWalls(const BattleHex & from) const;
	bool battleHasPenaltyOnLine(const BattleHex & from, const BattleHex & dest, bool checkWall, bool checkMoat) const;
	bool battleHasDistancePenalty(const IBonusBearer * shooter, const BattleHex & shooterPosition, const BattleHex & destHex) const;
	bool battleHasWallPenalty(const IBonusBearer * shooter, const BattleHex & shooterPosition, const BattleHex & destHex) const;
	bool battleHasShootingPenalty(const battle::Unit * shooter, const BattleHex & destHex) const;

	BattleHex wallPartToBattleHex(EWallPart part) const;
	EWallPart battleHexToWallPart(const BattleHex & hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	bool isWallPartPotentiallyAttackable(EWallPart wallPart) const; // returns true if the wall part is potentially attackable (independent of wall state), false if not
	bool isWallPartAttackable(EWallPart wallPart) const; // returns true if the wall part is actually attackable, false if not
	BattleHexArray getAttackableWallParts() const;

	si8 battleMinSpellLevel(BattleSide side) const; //calculates maximum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	si8 battleMaxSpellLevel(BattleSide side) const; //calculates minimum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	int32_t battleGetSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	ESpellCastProblem battleCanCastSpell(const spells::Caster * caster, spells::Mode mode) const; //returns true if there are no general issues preventing from casting a spell

	SpellID getRandomBeneficialSpell(vstd::RNG & rand, const battle::Unit * caster, const battle::Unit * target) const;
	SpellID getRandomCastedSpell(vstd::RNG & rand, const CStack * caster) const; //called at the beginning of turn for Faerie Dragon

	std::vector<PossiblePlayerBattleAction> getClientActionsForStack(const CStack * stack, const BattleClientInterfaceData & data);
	PossiblePlayerBattleAction getCasterAction(const CSpell * spell, const spells::Caster * caster, spells::Mode mode) const;

	//convenience methods using the ones above
	bool isInTacticRange(const BattleHex & dest) const;
	si8 battleGetTacticDist() const; //returns tactic distance for calling player or 0 if this player is not in tactic phase (for ALL_KNOWING actual distance for tactic side)

	AttackableTiles getPotentiallyAttackableHexes(
		const  battle::Unit* attacker,
		const  battle::Unit* defender,
		BattleHex destinationTile,
		BattleHex attackerPos,
		BattleHex defenderPos) const; //TODO: apply rotation to two-hex attacker

	AttackableTiles getPotentiallyAttackableHexes(
		const  battle::Unit * attacker,
		BattleHex destinationTile,
		BattleHex attackerPos) const;

	AttackableTiles getPotentiallyShootableHexes(const  battle::Unit* attacker, const BattleHex & destinationTile, const BattleHex & attackerPos) const;

	battle::Units getAttackedBattleUnits(
		const battle::Unit* attacker,
		const  battle::Unit * defender,
		BattleHex destinationTile,
		bool rangedAttack,
		BattleHex attackerPos = BattleHex::INVALID,
		BattleHex defenderPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	
	std::pair<std::set<const CStack*>, bool> getAttackedCreatures(const CStack* attacker, const BattleHex & destinationTile, bool rangedAttack, BattleHex attackerPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	bool isToReverse(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerHex = BattleHex::INVALID, BattleHex defenderHex = BattleHex::INVALID) const; //determines if attacker standing at attackerHex should reverse in order to attack defender

	ReachabilityInfo getReachability(const battle::Unit * unit) const;
	ReachabilityInfo getReachability(const ReachabilityInfo::Parameters & params) const;
	AccessibilityInfo getAccessibility() const;
	AccessibilityInfo getAccessibility(const battle::Unit * stack) const; //Hexes occupied by stack will be marked as accessible.
	AccessibilityInfo getAccessibility(const BattleHexArray & accessibleHexes) const; //given hexes will be marked as accessible
	ForcedAction getBerserkForcedAction(const battle::Unit * berserker) const;

	BattleHex getAvailableHex(const CreatureID & creID, BattleSide side, int initialPos = -1) const; //find place for adding new stack
protected:
	ReachabilityInfo getFlyingReachability(const ReachabilityInfo::Parameters & params) const;
	ReachabilityInfo makeBFS(const AccessibilityInfo & accessibility, const ReachabilityInfo::Parameters & params) const;
	bool isInObstacle(const BattleHex & hex, const BattleHexArray & obstacles, const ReachabilityInfo::Parameters & params) const;
	BattleHexArray getStoppers(BattleSide whichSidePerspective) const; //get hexes with stopping obstacles (quicksands)
};

VCMI_LIB_NAMESPACE_END

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
#include "CCallbackBase.h"
#include "ReachabilityInfo.h"
#include "BattleAttackInfo.h"
#include "../spells/Magic.h"

class CGHeroInstance;
class CStack;
class ISpellCaster;
class CSpell;
struct CObstacleInstance;
class IBonusBearer;
class CRandomGenerator;

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

enum class PossiblePlayerBattleAction // actions performed at l-click
{
	INVALID = -1, CREATURE_INFO,
	MOVE_TACTICS, CHOOSE_TACTICS_STACK,
	MOVE_STACK, ATTACK, WALK_AND_ATTACK, ATTACK_AND_RETURN, SHOOT, //OPEN_GATE, //we can open castle gate during siege
	NO_LOCATION, ANY_LOCATION, OBSTACLE, TELEPORT, SACRIFICE, RANDOM_GENIE_SPELL,
	FREE_LOCATION, //used with Force Field and Fire Wall - all tiles affected by spell must be free
	CATAPULT, HEAL, RISE_DEMONS,
	AIMED_SPELL_CREATURE
};

struct DLL_LINKAGE BattleClientInterfaceData
{
	si32 creatureSpellToCast;
	ui8 tacticsMode;
};

class DLL_LINKAGE CBattleInfoCallback : public virtual CBattleInfoEssentials
{
public:
	enum ERandomSpell
	{
		RANDOM_GENIE, RANDOM_AIMED
	};

	boost::optional<int> battleIsFinished() const; //return none if battle is ongoing; otherwise the victorious side (0/1) or 2 if it is a draw

	std::vector<std::shared_ptr<const CObstacleInstance>> battleGetAllObstaclesOnPos(BattleHex tile, bool onlyBlocking = true) const; //blocking obstacles makes tile inaccessible, others cause special effects (like Land Mines, Moat, Quicksands)
	std::vector<std::shared_ptr<const CObstacleInstance>> getAllAffectedObstaclesByStack(const CStack * stack) const;

	const CStack * battleGetStackByPos(BattleHex pos, bool onlyAlive = true) const;

	const battle::Unit * battleGetUnitByPos(BattleHex pos, bool onlyAlive = true) const;

	///returns all alive units excluding turrets
	battle::Units battleAliveUnits() const;
	///returns all alive units from particular side excluding turrets
	battle::Units battleAliveUnits(ui8 side) const;

	void battleGetTurnOrder(std::vector<battle::Units> & out, const size_t maxUnits, const int maxTurns, const int turn = 0, int8_t lastMoved = -1) const;

	void battleGetStackCountOutsideHexes(bool *ac) const; // returns hexes which when in front of a stack cause us to move the amount box back

	///returns reachable hexes (valid movement destinations), DOES contain stack current position
	std::vector<BattleHex> battleGetAvailableHexes(const battle::Unit * unit, bool addOccupiable, std::vector<BattleHex> * attackable) const;

	///returns reachable hexes (valid movement destinations), DOES contain stack current position (lite version)
	std::vector<BattleHex> battleGetAvailableHexes(const battle::Unit * unit) const;

	std::vector<BattleHex> battleGetAvailableHexes(const ReachabilityInfo & cache, const battle::Unit * unit) const;

	int battleGetSurrenderCost(PlayerColor Player) const; //returns cost of surrendering battle, -1 if surrendering is not possible
	ReachabilityInfo::TDistances battleGetDistances(const battle::Unit * unit, BattleHex assumedPosition) const;
	std::set<BattleHex> battleGetAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID) const;

	bool battleCanAttack(const CStack * stack, const CStack * target, BattleHex dest) const; //determines if stack with given ID can attack target at the selected destination
	bool battleCanShoot(const battle::Unit * attacker, BattleHex dest) const; //determines if stack with given ID shoot at the selected destination
	bool battleIsUnitBlocked(const battle::Unit * unit) const; //returns true if there is neighboring enemy stack
	std::set<const battle::Unit *> battleAdjacentUnits(const battle::Unit * unit) const;

	TDmgRange calculateDmgRange(const BattleAttackInfo & info) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>

	TDmgRange battleEstimateDamage(const BattleAttackInfo & bai, TDmgRange * retaliationDmg = nullptr) const; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	TDmgRange battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg = nullptr) const; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>

	bool battleHasDistancePenalty(const IBonusBearer * shooter, BattleHex shooterPosition, BattleHex destHex) const;
	bool battleHasWallPenalty(const IBonusBearer * shooter, BattleHex shooterPosition, BattleHex destHex) const;
	bool battleHasShootingPenalty(const battle::Unit * shooter, BattleHex destHex) const;

	BattleHex wallPartToBattleHex(EWallPart::EWallPart part) const;
	EWallPart::EWallPart battleHexToWallPart(BattleHex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	bool isWallPartPotentiallyAttackable(EWallPart::EWallPart wallPart) const; // returns true if the wall part is potentially attackable (independent of wall state), false if not
	std::vector<BattleHex> getAttackableBattleHexes() const;

	si8 battleMinSpellLevel(ui8 side) const; //calculates maximum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	si8 battleMaxSpellLevel(ui8 side) const; //calculates minimum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	ui32 battleGetSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	ESpellCastProblem::ESpellCastProblem battleCanCastSpell(const spells::Caster * caster, spells::Mode mode) const; //returns true if there are no general issues preventing from casting a spell

	SpellID battleGetRandomStackSpell(CRandomGenerator & rand, const CStack * stack, ERandomSpell mode) const;
	SpellID getRandomBeneficialSpell(CRandomGenerator & rand, const CStack * subject) const;
	SpellID getRandomCastedSpell(CRandomGenerator & rand, const CStack * caster) const; //called at the beginning of turn for Faerie Dragon

	si8 battleCanTeleportTo(const battle::Unit * stack, BattleHex destHex, int telportLevel) const; //checks if teleportation of given stack to given position can take place
	std::vector<PossiblePlayerBattleAction> getClientActionsForStack(const CStack * stack, const BattleClientInterfaceData & data);
	PossiblePlayerBattleAction getCasterAction(const CSpell * spell, const spells::Caster * caster, spells::Mode mode) const;

	//convenience methods using the ones above
	bool isInTacticRange(BattleHex dest) const;
	si8 battleGetTacticDist() const; //returns tactic distance for calling player or 0 if this player is not in tactic phase (for ALL_KNOWING actual distance for tactic side)

	AttackableTiles getPotentiallyAttackableHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const; //TODO: apply rotation to two-hex attacker
	AttackableTiles getPotentiallyShootableHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const;
	std::set<const CStack*> getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, bool rangedAttack, BattleHex attackerPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	bool isToReverse(BattleHex hexFrom, BattleHex hexTo, bool curDir /*if true, creature is in attacker's direction*/, bool toDoubleWide, bool toDir) const; //determines if creature should be reversed (it stands on hexFrom and should 'see' hexTo)
	bool isToReverseHlp(BattleHex hexFrom, BattleHex hexTo, bool curDir) const; //helper for isToReverse

	ReachabilityInfo getReachability(const battle::Unit * unit) const;
	ReachabilityInfo getReachability(const ReachabilityInfo::Parameters & params) const;
	AccessibilityInfo getAccesibility() const;
	AccessibilityInfo getAccesibility(const battle::Unit * stack) const; //Hexes ocupied by stack will be marked as accessible.
	AccessibilityInfo getAccesibility(const std::vector<BattleHex> & accessibleHexes) const; //given hexes will be marked as accessible
	std::pair<const battle::Unit *, BattleHex> getNearestStack(const battle::Unit * closest) const;

	BattleHex getAvaliableHex(CreatureID creID, ui8 side, int initialPos = -1) const; //find place for adding new stack
protected:
	ReachabilityInfo getFlyingReachability(const ReachabilityInfo::Parameters & params) const;
	ReachabilityInfo makeBFS(const AccessibilityInfo & accessibility, const ReachabilityInfo::Parameters & params) const;
	std::set<BattleHex> getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const; //get hexes with stopping obstacles (quicksands)
};

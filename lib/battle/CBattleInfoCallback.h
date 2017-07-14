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
		h & hostileCreaturePositions & friendlyCreaturePositions;
	}
};

class DLL_LINKAGE CBattleInfoCallback : public virtual CBattleInfoEssentials
{
public:
	enum ERandomSpell
	{
		RANDOM_GENIE, RANDOM_AIMED
	};
	//battle
	boost::optional<int> battleIsFinished() const; //return none if battle is ongoing; otherwise the victorious side (0/1) or 2 if it is a draw

	std::vector<std::shared_ptr<const CObstacleInstance>> battleGetAllObstaclesOnPos(BattleHex tile, bool onlyBlocking = true) const; //blocking obstacles makes tile inaccessible, others cause special effects (like Land Mines, Moat, Quicksands)
	std::vector<std::shared_ptr<const CObstacleInstance>> getAllAffectedObstaclesByStack(const CStack * stack) const;

	const CStack * battleGetStackByPos(BattleHex pos, bool onlyAlive = true) const; //returns stack info by given pos
	void battleGetStackQueue(std::vector<const CStack *> &out, const int howMany, const int turn = 0, int lastMoved = -1) const;
	void battleGetStackCountOutsideHexes(bool *ac) const; // returns hexes which when in front of a stack cause us to move the amount box back

	std::vector<BattleHex> battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<BattleHex> * attackable = nullptr) const; //returns hexes reachable by creature with id ID (valid movement destinations), DOES contain stack current position

	int battleGetSurrenderCost(PlayerColor Player) const; //returns cost of surrendering battle, -1 if surrendering is not possible
	ReachabilityInfo::TDistances battleGetDistances(const CStack * stack, BattleHex hex = BattleHex::INVALID, BattleHex * predecessors = nullptr) const; //returns vector of distances to [dest hex number]
	std::set<BattleHex> battleGetAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID) const;

	bool battleCanAttack(const CStack * stack, const CStack * target, BattleHex dest) const; //determines if stack with given ID can attack target at the selected destination
	bool battleCanShoot(const CStack * stack, BattleHex dest) const; //determines if stack with given ID shoot at the selected destination
	bool battleIsStackBlocked(const CStack * stack) const; //returns true if there is neighboring enemy stack
	std::set<const CStack*> batteAdjacentCreatures (const CStack * stack) const;

	TDmgRange calculateDmgRange(const BattleAttackInfo & info) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>

	//hextowallpart //int battleGetWallUnderHex(BattleHex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	std::pair<ui32, ui32> battleEstimateDamage(CRandomGenerator & rand, const BattleAttackInfo & bai, std::pair<ui32, ui32> * retaliationDmg = nullptr) const; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	std::pair<ui32, ui32> battleEstimateDamage(CRandomGenerator & rand, const CStack * attacker, const CStack * defender, std::pair<ui32, ui32> * retaliationDmg = nullptr) const; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	si8 battleHasDistancePenalty(const CStack * stack, BattleHex destHex) const;
	si8 battleHasDistancePenalty(const IBonusBearer * bonusBearer, BattleHex shooterPosition, BattleHex destHex) const;
	si8 battleHasWallPenalty(const CStack * stack, BattleHex destHex) const; //checks if given stack has wall penalty
	si8 battleHasWallPenalty(const IBonusBearer * bonusBearer, BattleHex shooterPosition, BattleHex destHex) const; //checks if given stack has wall penalty

	BattleHex wallPartToBattleHex(EWallPart::EWallPart part) const;
	EWallPart::EWallPart battleHexToWallPart(BattleHex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	bool isWallPartPotentiallyAttackable(EWallPart::EWallPart wallPart) const; // returns true if the wall part is potentially attackable (independent of wall state), false if not
	std::vector<BattleHex> getAttackableBattleHexes() const;

	//*** MAGIC
	si8 battleMaxSpellLevel(ui8 side) const; //calculates minimum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	ui32 battleGetSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	ESpellCastProblem::ESpellCastProblem battleCanCastSpell(const ISpellCaster * caster, ECastingMode::ECastingMode mode) const; //returns true if there are no general issues preventing from casting a spell

	SpellID battleGetRandomStackSpell(CRandomGenerator & rand, const CStack * stack, ERandomSpell mode) const;
	SpellID getRandomBeneficialSpell(CRandomGenerator & rand, const CStack * subject) const;
	SpellID getRandomCastedSpell(CRandomGenerator & rand, const CStack * caster) const; //called at the beginning of turn for Faerie Dragon

	const CStack * getStackIf(std::function<bool(const CStack*)> pred) const;

	si8 battleHasShootingPenalty(const CStack * stack, BattleHex destHex);
	si8 battleCanTeleportTo(const CStack * stack, BattleHex destHex, int telportLevel) const; //checks if teleportation of given stack to given position can take place

	//convenience methods using the ones above
	bool isInTacticRange(BattleHex dest) const;
	si8 battleGetTacticDist() const; //returns tactic distance for calling player or 0 if this player is not in tactic phase (for ALL_KNOWING actual distance for tactic side)

	AttackableTiles getPotentiallyAttackableHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const; //TODO: apply rotation to two-hex attacker
	std::set<const CStack*> getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	bool isToReverse(BattleHex hexFrom, BattleHex hexTo, bool curDir /*if true, creature is in attacker's direction*/, bool toDoubleWide, bool toDir) const; //determines if creature should be reversed (it stands on hexFrom and should 'see' hexTo)
	bool isToReverseHlp(BattleHex hexFrom, BattleHex hexTo, bool curDir) const; //helper for isToReverse

	ReachabilityInfo getReachability(const CStack *stack) const;
	ReachabilityInfo getReachability(const ReachabilityInfo::Parameters & params) const;
	AccessibilityInfo getAccesibility() const;
	AccessibilityInfo getAccesibility(const CStack *stack) const; //Hexes ocupied by stack will be marked as accessible.
	AccessibilityInfo getAccesibility(const std::vector<BattleHex> & accessibleHexes) const; //given hexes will be marked as accessible
	std::pair<const CStack *, BattleHex> getNearestStack(const CStack * closest, BattleSideOpt side) const;
protected:
	ReachabilityInfo getFlyingReachability(const ReachabilityInfo::Parameters & params) const;
	ReachabilityInfo makeBFS(const AccessibilityInfo & accessibility, const ReachabilityInfo::Parameters & params) const;
	ReachabilityInfo makeBFS(const CStack * stack) const; //uses default parameters -> stack position and owner's perspective
	std::set<BattleHex> getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const; //get hexes with stopping obstacles (quicksands)
};

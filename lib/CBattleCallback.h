#pragma once
#include "BattleHex.h"

/*
 * CBattleCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGameState;
class CGTownInstance;
class CGHeroInstance;
class CStack;
class ISpellCaster;
class CSpell;
struct BattleInfo;
struct CObstacleInstance;
class IBonusBearer;
struct InfoAboutHero;
class CArmedInstance;

namespace boost
{class shared_mutex;}

namespace BattleSide
{
	enum {ATTACKER = 0, DEFENDER = 1};
}

typedef std::vector<const CStack*> TStacks;
typedef std::function<bool(const CStack *)> TStackFilter;

class CBattleInfoEssentials;

//Basic class for various callbacks (interfaces called by players to get info about game and so forth)
class DLL_LINKAGE CCallbackBase
{
	const BattleInfo *battle; //battle to which the player is engaged, nullptr if none or not applicable

	const BattleInfo * getBattle() const
	{
		return battle;
	}

protected:
	CGameState *gs;
	boost::optional<PlayerColor> player; // not set gives access to all information, otherwise callback provides only information "visible" for player

	CCallbackBase(CGameState *GS, boost::optional<PlayerColor> Player)
		: battle(nullptr), gs(GS), player(Player)
	{}
	CCallbackBase()
		: battle(nullptr), gs(nullptr)
	{}

	void setBattle(const BattleInfo *B);
	bool duringBattle() const;

public:
	boost::shared_mutex &getGsMutex(); //just return a reference to mutex, does not lock nor anything
	boost::optional<PlayerColor> getPlayerID() const;

	friend class CBattleInfoEssentials;
};


struct DLL_LINKAGE AttackableTiles
{
	std::set<BattleHex> hostileCreaturePositions;
	std::set<BattleHex> friendlyCreaturePositions; //for Dragon Breath
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hostileCreaturePositions & friendlyCreaturePositions;
	}
};

//Accessibility is property of hex in battle. It doesn't depend on stack, side's perspective and so on.
namespace EAccessibility
{
	enum EAccessibility
	{
		ACCESSIBLE,
		ALIVE_STACK,
		OBSTACLE,
		DESTRUCTIBLE_WALL,
		GATE, //sieges -> gate opens only for defender stacks
		UNAVAILABLE, //indestructible wall parts, special battlefields (like boat-to-boat)
		SIDE_COLUMN //used for first and last columns of hexes that are unavailable but wat machines can stand there
	};
}

typedef std::array<EAccessibility::EAccessibility, GameConstants::BFIELD_SIZE> TAccessibilityArray;

struct DLL_LINKAGE AccessibilityInfo : TAccessibilityArray
{
	bool occupiable(const CStack *stack, BattleHex tile) const;
	bool accessible(BattleHex tile, const CStack *stack) const; //checks for both tiles if stack is double wide
	bool accessible(BattleHex tile, bool doubleWide, bool attackerOwned) const; //checks for both tiles if stack is double wide
};

namespace BattlePerspective
{
	enum BattlePerspective
	{
		INVALID = -2,
		ALL_KNOWING = -1,
		LEFT_SIDE,
		RIGHT_SIDE
	};
}

// Reachability info is result of BFS calculation. It's dependent on stack (it's owner, whether it's flying),
// startPosition and perpective.
struct DLL_LINKAGE ReachabilityInfo
{
	typedef std::array<int, GameConstants::BFIELD_SIZE> TDistances;
	typedef std::array<BattleHex, GameConstants::BFIELD_SIZE> TPredecessors;

	enum { 	INFINITE_DIST = 1000000 };

	struct DLL_LINKAGE Parameters
	{
		const CStack *stack; //stack for which calculation is mage => not required (kept for debugging mostly), following variables are enough

		bool attackerOwned;
		bool doubleWide;
		bool flying;
		std::vector<BattleHex> knownAccessible; //hexes that will be treated as accessible, even if they're occupied by stack (by default - tiles occupied by stack we do reachability for, so it doesn't block itself)

		BattleHex startPosition; //assumed position of stack
		BattlePerspective::BattlePerspective perspective; //some obstacles (eg. quicksands) may be invisible for some side

		Parameters();
		Parameters(const CStack *Stack);
	};

	Parameters params;
	AccessibilityInfo accessibility;
	TDistances distances;
	TPredecessors predecessors;

	ReachabilityInfo()
	{
		distances.fill(INFINITE_DIST);
		predecessors.fill(BattleHex::INVALID);
	}

	bool isReachable(BattleHex hex) const
	{
		return distances[hex] < INFINITE_DIST;
	}
};

class DLL_LINKAGE CBattleInfoEssentials : public virtual CCallbackBase
{
protected:
	bool battleDoWeKnowAbout(ui8 side) const;
	const IBonusBearer * getBattleNode() const;
public:
	enum EStackOwnership
	{
		ONLY_MINE, ONLY_ENEMY, MINE_AND_ENEMY
	};

	BattlePerspective::BattlePerspective battleGetMySide() const;

	ETerrainType battleTerrainType() const;
	BFieldType battleGetBattlefieldType() const;
	std::vector<std::shared_ptr<const CObstacleInstance> > battleGetAllObstacles(boost::optional<BattlePerspective::BattlePerspective> perspective = boost::none) const; //returns all obstacles on the battlefield

    /** @brief Main method for getting battle stacks
     *
     * @param predicate Functor that shall return true for desired stack
     * @return filtered stacks
     *
     */
	TStacks battleGetStacksIf(TStackFilter predicate) const;

	bool battleHasNativeStack(ui8 side) const;
	int battleGetMoatDmg() const; //what dmg unit will suffer if ending turn in the moat
	const CGTownInstance * battleGetDefendedTown() const; //returns defended town if current battle is a siege, nullptr instead
	const CStack *battleActiveStack() const;
	si8 battleTacticDist() const; //returns tactic distance in current tactics phase; 0 if not in tactics phase
	si8 battleGetTacticsSide() const; //returns which side is in tactics phase, undefined if none (?)
	bool battleCanFlee(PlayerColor player) const;
	bool battleCanSurrender(PlayerColor player) const;
	ui8 playerToSide(PlayerColor player) const;
	ui8 battleGetSiegeLevel() const; //returns 0 when there is no siege, 1 if fort, 2 is citadel, 3 is castle
	bool battleHasHero(ui8 side) const;
	int battleCastSpells(ui8 side) const; //how many spells has given side cast
	const CGHeroInstance * battleGetFightingHero(ui8 side) const; //depracated for players callback, easy to get wrong
	const CArmedInstance * battleGetArmyObject(ui8 side) const;
	InfoAboutHero battleGetHeroInfo(ui8 side) const;

	// for determining state of a part of the wall; format: parameter [0] - keep, [1] - bottom tower, [2] - bottom wall,
	// [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate; returned value: 1 - intact, 2 - damaged, 3 - destroyed; 0 - no battle
	si8 battleGetWallState(int partOfWall) const;
	EGateState battleGetGateState() const;

	//helpers
	///returns all stacks, alive or dead or undead or mechanical :)
	TStacks battleGetAllStacks(bool includeTurrets = false) const;

	///returns all alive stacks excluding turrets
	TStacks battleAliveStacks() const;
	///returns all alive stacks from particular side excluding turrets
	TStacks battleAliveStacks(ui8 side) const;
	const CStack * battleGetStackByID(int ID, bool onlyAlive = true) const; //returns stack info by given ID
	bool battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattlePerspective::BattlePerspective side) const;
	//ESpellCastProblem::ESpellCastProblem battleCanCastSpell(int player, ECastingMode::ECastingMode mode) const; //Checks if player is able to cast spells (at all) at the moment
};

struct DLL_LINKAGE BattleAttackInfo
{
	const IBonusBearer *attackerBonuses, *defenderBonuses;
	const CStack *attacker, *defender;
	BattleHex attackerPosition, defenderPosition;

	int attackerCount, defenderCount;
	bool shooting;
	int chargedFields;

	bool luckyHit;
	bool unluckyHit;
	bool deathBlow;
	bool ballistaDoubleDamage;

	BattleAttackInfo(const CStack *Attacker, const CStack *Defender, bool Shooting = false);
	BattleAttackInfo reverse() const;
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

	std::shared_ptr<const CObstacleInstance> battleGetObstacleOnPos(BattleHex tile, bool onlyBlocking = true) const; //blocking obstacles makes tile inaccessible, others cause special effects (like Land Mines, Moat, Quicksands)
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
	std::set<const CStack*>  batteAdjacentCreatures (const CStack * stack) const;

	TDmgRange calculateDmgRange(const BattleAttackInfo &info) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>
	TDmgRange calculateDmgRange(const CStack* attacker, const CStack* defender, TQuantity attackerCount, bool shooting, ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>
	TDmgRange calculateDmgRange(const CStack* attacker, const CStack* defender, bool shooting, ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>

	//hextowallpart  //int battleGetWallUnderHex(BattleHex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	std::pair<ui32, ui32> battleEstimateDamage(const BattleAttackInfo &bai, std::pair<ui32, ui32> * retaliationDmg = nullptr) const; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	std::pair<ui32, ui32> battleEstimateDamage(const CStack * attacker, const CStack * defender, std::pair<ui32, ui32> * retaliationDmg = nullptr) const; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	si8 battleHasDistancePenalty( const CStack * stack, BattleHex destHex ) const;
	si8 battleHasDistancePenalty(const IBonusBearer *bonusBearer, BattleHex shooterPosition, BattleHex destHex ) const;
	si8 battleHasWallPenalty(const CStack * stack, BattleHex destHex) const; //checks if given stack has wall penalty
	si8 battleHasWallPenalty(const IBonusBearer *bonusBearer, BattleHex shooterPosition, BattleHex destHex) const; //checks if given stack has wall penalty

	BattleHex wallPartToBattleHex(EWallPart::EWallPart part) const;
	EWallPart::EWallPart battleHexToWallPart(BattleHex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	bool isWallPartPotentiallyAttackable(EWallPart::EWallPart wallPart) const; // returns true if the wall part is potentially attackable (independent of wall state), false if not
	std::vector<BattleHex> getAttackableBattleHexes() const;

	//*** MAGIC
	si8 battleMaxSpellLevel(ui8 side) const; //calculates minimum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	ui32 battleGetSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	ESpellCastProblem::ESpellCastProblem battleCanCastSpell(PlayerColor player, ECastingMode::ECastingMode mode) const; //returns true if there are no general issues preventing from casting a spell
	ESpellCastProblem::ESpellCastProblem battleCanCastThisSpell(const ISpellCaster * caster, const CSpell * spell, ECastingMode::ECastingMode mode) const; //checks if given player can cast given spell
	ESpellCastProblem::ESpellCastProblem battleCanCastThisSpellHere(const ISpellCaster * caster, const CSpell * spell, ECastingMode::ECastingMode mode, BattleHex dest) const; //checks if given player can cast given spell at given tile in given mode

	SpellID battleGetRandomStackSpell(const CStack * stack, ERandomSpell mode) const;
	SpellID getRandomBeneficialSpell(const CStack * subject) const;
	SpellID getRandomCastedSpell(const CStack * caster) const; //called at the beginning of turn for Faerie Dragon

	const CStack * getStackIf(std::function<bool(const CStack*)> pred) const;

	si8 battleHasShootingPenalty(const CStack * stack, BattleHex destHex)
	{
		return battleHasDistancePenalty(stack, destHex) || battleHasWallPenalty(stack, destHex);
	}
	si8 battleCanTeleportTo(const CStack * stack, BattleHex destHex, int telportLevel) const; //checks if teleportation of given stack to given position can take place


	//convenience methods using the ones above
	bool isInTacticRange( BattleHex dest ) const;
	si8 battleGetTacticDist() const; //returns tactic distance for calling player or 0 if this player is not in tactic phase (for ALL_KNOWING actual distance for tactic side)

	AttackableTiles getPotentiallyAttackableHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const; //TODO: apply rotation to two-hex attacker
	std::set<const CStack*> getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID) const; //calculates range of multi-hex attacks
	bool isToReverse(BattleHex hexFrom, BattleHex hexTo, bool curDir /*if true, creature is in attacker's direction*/, bool toDoubleWide, bool toDir) const; //determines if creature should be reversed (it stands on hexFrom and should 'see' hexTo)
	bool isToReverseHlp(BattleHex hexFrom, BattleHex hexTo, bool curDir) const; //helper for isToReverse

	ReachabilityInfo getReachability(const CStack *stack) const;
	ReachabilityInfo getReachability(const ReachabilityInfo::Parameters &params) const;
	AccessibilityInfo getAccesibility() const;
	AccessibilityInfo getAccesibility(const CStack *stack) const; //Hexes ocupied by stack will be marked as accessible.
	AccessibilityInfo getAccesibility(const std::vector<BattleHex> &accessibleHexes) const; //given hexes will be marked as accessible
	std::pair<const CStack *, BattleHex> getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const;
protected:
	ReachabilityInfo getFlyingReachability(const ReachabilityInfo::Parameters &params) const;
	ReachabilityInfo makeBFS(const AccessibilityInfo &accessibility, const ReachabilityInfo::Parameters &params) const;
	ReachabilityInfo makeBFS(const CStack *stack) const; //uses default parameters -> stack position and owner's perspective
	std::set<BattleHex> getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const; //get hexes with stopping obstacles (quicksands)
};

class DLL_LINKAGE CPlayerBattleCallback : public CBattleInfoCallback
{
public:
	bool battleCanFlee() const; //returns true if caller can flee from the battle
	TStacks battleGetStacks(EStackOwnership whose = MINE_AND_ENEMY, bool onlyAlive = true) const; //returns stacks on battlefield
	ESpellCastProblem::ESpellCastProblem battleCanCastThisSpell(const CSpell * spell) const; //determines if given spell can be cast (and returns problem description)

	int battleGetSurrenderCost() const; //returns cost of surrendering battle, -1 if surrendering is not possible

	bool battleCanCastSpell(ESpellCastProblem::ESpellCastProblem *outProblem = nullptr) const; //returns true, if caller can cast a spell. If not, if pointer is given via arg, the reason will be written.
	const CGHeroInstance * battleGetMyHero() const;
	InfoAboutHero battleGetEnemyHero() const;
};

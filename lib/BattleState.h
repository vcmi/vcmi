/*
 * BattleState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BattleHex.h"
#include "mapObjects/CArmedInstance.h" // for army serialization
#include "mapObjects/CGHeroInstance.h" // for commander serialization
#include "CCreatureHandler.h"
#include "ConstTransitivePtr.h"
#include "GameConstants.h"
#include "CBattleCallback.h"
#include "int3.h"
#include "spells/Magic.h"

class CGHeroInstance;
class CStack;
class CArmedInstance;
class CGTownInstance;
class CStackInstance;
struct BattleStackAttacked;
class CRandomGenerator;


//only for use in BattleInfo
struct DLL_LINKAGE SiegeInfo
{
	std::array<si8, EWallPart::PARTS_COUNT> wallState;
	EGateState gateState;

	// return EWallState decreased by value of damage points
	static EWallState::EWallState applyDamage(EWallState::EWallState state, unsigned int value)
	{
		if (value == 0)
			return state;

		switch (applyDamage(state, value - 1))
		{
		case EWallState::INTACT    : return EWallState::DAMAGED;
		case EWallState::DAMAGED   : return EWallState::DESTROYED;
		case EWallState::DESTROYED : return EWallState::DESTROYED;
		default: return EWallState::NONE;
		}
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & wallState & gateState;
	}
};

struct DLL_LINKAGE SideInBattle
{
	PlayerColor color;
	const CGHeroInstance *hero; //may be NULL if army is not commanded by hero
	const CArmedInstance *armyObject; //adv. map object with army that participates in battle; may be same as hero

	ui8 castSpellsCount; //how many spells each side has cast this turn
	std::vector<const CSpell *> usedSpellsHistory; //every time hero casts spell, it's inserted here -> eagle eye skill
	si16 enchanterCounter; //tends to pass through 0, so sign is needed

	SideInBattle();
	void init(const CGHeroInstance *Hero, const CArmedInstance *Army);


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & color & hero & armyObject;
		h & castSpellsCount & usedSpellsHistory & enchanterCounter;
	}
};

struct DLL_LINKAGE BattleInfo : public CBonusSystemNode, public CBattleInfoCallback
{
	std::array<SideInBattle, 2> sides; //sides[0] - attacker, sides[1] - defender
	si32 round, activeStack, selectedStack;
	const CGTownInstance * town; //used during town siege, nullptr if this is not a siege (note that fortless town IS also a siege)
	int3 tile; //for background and bonuses
	std::vector<CStack*> stacks;
	std::vector<std::shared_ptr<CObstacleInstance> > obstacles;
	SiegeInfo si;

	BFieldType battlefieldType; //like !!BA:B
	ETerrainType terrainType; //used for some stack nativity checks (not the bonus limiters though that have their own copy)

	ui8 tacticsSide; //which side is requested to play tactics phase
	ui8 tacticDistance; //how many hexes we can go forward (1 = only hexes adjacent to margin line)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sides;
		h & round & activeStack & selectedStack & town & tile & stacks & obstacles
			& si & battlefieldType & terrainType;
		h & tacticsSide & tacticDistance;
		h & static_cast<CBonusSystemNode&>(*this);
	}

	//////////////////////////////////////////////////////////////////////////
	BattleInfo();
	~BattleInfo(){};

	//////////////////////////////////////////////////////////////////////////
	CStack * getStack(int stackID, bool onlyAlive = true);
	using CBattleInfoEssentials::battleGetArmyObject;
	CArmedInstance * battleGetArmyObject(ui8 side) const;
	using CBattleInfoEssentials::battleGetFightingHero;
	CGHeroInstance * battleGetFightingHero(ui8 side) const;

	const CStack * getNextStack() const; //which stack will have turn after current one
	//void getStackQueue(std::vector<const CStack *> &out, int howMany, int turn = 0, int lastMoved = -1) const; //returns stack in order of their movement action

	//void getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<BattleHex> & occupyable, bool flying, const CStack* stackToOmmit = nullptr) const; //send pointer to at least 187 allocated bytes
	//static bool isAccessible(BattleHex hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos); //helper for makeBFS
	int getAvaliableHex(CreatureID creID, bool attackerOwned, int initialPos = -1) const; //find place for summon / clone effects
	//void makeBFS(BattleHex start, bool*accessibility, BattleHex *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying, bool fillPredecessors) const; //*accessibility must be prepared bool[187] array; last two pointers must point to the at least 187-elements int arrays - there is written result
	std::pair< std::vector<BattleHex>, int > getPath(BattleHex start, BattleHex dest, const CStack *stack); //returned value: pair<path, length>; length may be different than number of elements in path since flying vreatures jump between distant hexes
	//std::vector<BattleHex> getAccessibility(const CStack * stack, bool addOccupiable, std::vector<BattleHex> * attackable = nullptr, bool forPassingBy = false) const; //returns vector of accessible tiles (taking into account the creature range)

	//bool isObstacleVisibleForSide(const CObstacleInstance &obstacle, ui8 side) const;
	std::shared_ptr<CObstacleInstance> getObstacleOnTile(BattleHex tile) const;
	std::set<BattleHex> getStoppers(bool whichSidePerspective) const;

	ui32 calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg, CRandomGenerator & rand); //charge - number of hexes travelled before attack (for champion's jousting)
	void calculateCasualties(std::map<ui32,si32> *casualties) const; //casualties are array of maps size 2 (attacker, defeneder), maps are (crid => amount)

	//void getPotentiallyAttackableHexes(AttackableTiles &at, const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos); //hexes around target that could be attacked in melee
	//std::set<CStack*> getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID); //calculates range of multi-hex attacks
	//std::set<BattleHex> getAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID); //calculates range of multi-hex attacks

	CStack * generateNewStack(const CStackInstance &base, bool attackerOwned, SlotID slot, BattleHex position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	CStack * generateNewStack(const CStackBasicDescriptor &base, bool attackerOwned, SlotID slot, BattleHex position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	int getIdForNewStack() const; //suggest a currently unused ID that'd suitable for generating a new stack
	//std::pair<const CStack *, BattleHex> getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const; //if attackerOwned is indetermnate, returened stack is of any owner; hex is the number of hex we should be looking from; returns (nerarest creature, predecessorHex)

	const CGHeroInstance * getHero(PlayerColor player) const; //returns fighting hero that belongs to given player

	const CGHeroInstance * battleGetOwner(const CStack * stack) const; //returns hero that owns given stack; nullptr if none
	void localInit();

	void localInitStack(CStack * s);
	static BattleInfo * setupBattle( int3 tile, ETerrainType terrain, BFieldType battlefieldType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town );
	//bool hasNativeStack(ui8 side) const;

	PlayerColor theOtherPlayer(PlayerColor player) const;
	ui8 whatSide(PlayerColor player) const;

	static BattlefieldBI::BattlefieldBI battlefieldTypeToBI(BFieldType bfieldType); //converts above to ERM BI format
	static int battlefieldTypeToTerrain(int bfieldType); //converts above to ERM BI format
};

class DLL_LINKAGE CStack : public CBonusSystemNode, public CStackBasicDescriptor, public ISpellCaster
{
public:
	const CStackInstance *base; //garrison slot from which stack originates (nullptr for war machines, summoned cres, etc)

	ui32 ID; //unique ID of stack
	ui32 baseAmount;
	ui32 firstHPleft; //HP of first creature in stack
	PlayerColor owner; //owner - player colour (255 for neutrals)
	SlotID slot;  //slot - position in garrison (may be 255 for neutrals/called creatures)
	bool attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	BattleHex position; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower
	///how many times this stack has been counterattacked this round
	ui8 counterAttacksPerformed;
	///cached total count of counterattacks; should be cleared each round;do not serialize
	mutable ui8 counterAttacksTotalCache;
	si16 shots; //how many shots left
	ui8 casts; //how many casts left
	TQuantity resurrected; // these units will be taken back after battle is over
	///id of alive clone of this stack clone if any
	si32 cloneID;
	std::set<EBattleStackState::EBattleStackState> state;
	//overrides
	const CCreature* getCreature() const {return type;}

	CStack(const CStackInstance *base, PlayerColor O, int I, bool AO, SlotID S); //c-tor
	CStack(const CStackBasicDescriptor *stack, PlayerColor O, int I, bool AO, SlotID S = SlotID(255)); //c-tor
	CStack(); //c-tor
	~CStack();
	std::string nodeName() const override;

	void init(); //set initial (invalid) values
	void postInit(); //used to finish initialization when inheriting creature parameters is working
	std::string getName() const; //plural or singular
	bool willMove(int turn = 0) const; //if stack has remaining move this turn
	bool ableToRetaliate() const; //if stack can retaliate after attacked
	///how many times this stack can counterattack in one round
	ui8 counterAttacksTotal() const;
	///how many times this stack can counterattack in one round more
	si8 counterAttacksRemaining() const;
	bool moved(int turn = 0) const; //if stack was already moved this turn
	bool waited(int turn = 0) const;
	bool canMove(int turn = 0) const; //if stack can move
	bool canBeHealed() const; //for first aid tent - only harmed stacks that are not war machines
	///returns actual heal value based on internal state
	ui32 calculateHealedHealthPoints(ui32 toHeal, const bool resurrect) const;
	ui32 level() const;
	si32 magicResistance() const override; //include aura of resistance
	static void stackEffectToFeature(std::vector<Bonus> & sf, const Bonus & sse);
	std::vector<si32> activeSpells() const; //returns vector of active spell IDs sorted by time of cast
	const CGHeroInstance *getMyHero() const; //if stack belongs to hero (directly or was by him summoned) returns hero, nullptr otherwise
	ui32 totalHelth() const; // total health for all creatures in stack;

	static inline Bonus featureGenerator(Bonus::BonusType type, si16 subtype, si32 value, ui16 turnsRemain, si32 additionalInfo = 0, Bonus::LimitEffect limit = Bonus::NO_LIMIT)
	{
		Bonus hb = makeFeatureVal(type, Bonus::N_TURNS, subtype, value, Bonus::SPELL_EFFECT, turnsRemain, additionalInfo);
		hb.effectRange = limit;
		return hb;
	}

	static bool isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos = BattleHex::INVALID, BattleHex defenderPos = BattleHex::INVALID);

	bool doubleWide() const;
	BattleHex occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1
	BattleHex occupiedHex(BattleHex assumedPos) const; //returns number of occupied hex (not the position) if stack is double wide and would stand on assumedPos; otherwise -1
	std::vector<BattleHex> getHexes() const; //up to two occupied hexes, starting from front
	std::vector<BattleHex> getHexes(BattleHex assumedPos) const; //up to two occupied hexes, starting from front
	static std::vector<BattleHex> getHexes(BattleHex assumedPos, bool twoHex, bool AttackerOwned); //up to two occupied hexes, starting from front
	bool coversPos(BattleHex position) const; //checks also if unit is double-wide
	std::vector<BattleHex> getSurroundingHexes(BattleHex attackerPos = BattleHex::INVALID) const; // get six or 8 surrounding hexes depending on creature size

	std::pair<int,int> countKilledByAttack(int damageReceived) const; //returns pair<killed count, new left HP>
	void prepareAttacked(BattleStackAttacked &bsa, CRandomGenerator & rand, boost::optional<int> customCount = boost::none) const; //requires bsa.damageAmout filled

	///ISpellCaster
	ui8 getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool = nullptr) const override;
	ui32 getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const override;

	///default spell school level for effect calculation
	int getEffectLevel(const CSpell * spell) const override;

	///default spell-power for damage/heal calculation
	int getEffectPower(const CSpell * spell) const override;

	///default spell-power for timed effects duration
	int getEnchantPower(const CSpell * spell) const override;

	///damage/heal override(ignores spell configuration, effect level and effect power)
	int getEffectValue(const CSpell * spell) const override;

	const PlayerColor getOwner() const override;

	///stack will be ghost in next battle state update
	void makeGhost();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		assert(isIndependentNode());
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CStackBasicDescriptor&>(*this);
		h & ID & baseAmount & firstHPleft & owner & slot & attackerOwned & position & state & counterAttacksPerformed
			& shots & casts & count & resurrected;

		const CArmedInstance *army = (base ? base->armyObj : nullptr);
		SlotID slot = (base ? base->armyObj->findStack(base) : SlotID());

		if(h.saving)
		{
			h & army & slot;
		}
		else
		{
			h & army & slot;
			if (slot == SlotID::COMMANDER_SLOT_PLACEHOLDER) //TODO
			{
				auto hero = dynamic_cast<const CGHeroInstance *>(army);
				assert (hero);
				base = hero->commander;
			}
			else if(!army || slot == SlotID() || !army->hasStackAtSlot(slot))
			{
				base = nullptr;
				logGlobal->warnStream() << type->nameSing << " doesn't have a base stack!";
			}
			else
			{
				base = &army->getStack(slot);
			}
		}

	}
	bool alive() const //determines if stack is alive
	{
		return vstd::contains(state,EBattleStackState::ALIVE);
	}

	bool isDead() const;
	bool isGhost() const; //determines if stack was removed
	bool isValidTarget(bool allowDead = false) const; //non-turret non-ghost stacks (can be attacked or be object of magic effect)
	bool isTurret() const;
};

class DLL_LINKAGE CMP_stack
{
	int phase; //rules of which phase will be used
	int turn;
public:

	bool operator ()(const CStack* a, const CStack* b);
	CMP_stack(int Phase = 1, int Turn = 0);
};

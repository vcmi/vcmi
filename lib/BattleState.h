#pragma once

#include "../global.h"
#include "HeroBonus.h"
#include "CCreatureSet.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"

#include "ConstTransitivePtr.h"

/*
 * BattleState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGHeroInstance;
class CStack;
class CArmedInstance;
class CGTownInstance;
class CStackInstance;
struct BattleStackAttacked;

struct DLL_EXPORT CObstacleInstance
{
	int uniqueID;
	int ID; //ID of obstacle (defines type of it)
	int pos; //position on battlefield
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & pos & uniqueID;
	}
};

//only for use in BattleInfo
struct DLL_EXPORT SiegeInfo
{
	ui8 wallState[8]; //[0] - keep, [1] - bottom tower, [2] - bottom wall, [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate; 1 - intact, 2 - damaged, 3 - destroyed

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & wallState;
	}
};

struct DLL_EXPORT BattleInfo : public CBonusSystemNode
{
	ui8 sides[2]; //sides[0] - attacker, sides[1] - defender
	si32 round, activeStack;
	ui8 siege; //    = 0 ordinary battle    = 1 a siege with a Fort    = 2 a siege with a Citadel    = 3 a siege with a Castle
	const CGTownInstance * town; //used during town siege - id of attacked town; -1 if not town defence
	int3 tile; //for background and bonuses
	CGHeroInstance *heroes[2];
	CArmedInstance *belligerents[2]; //may be same as heroes
	std::vector<CStack*> stacks;
	std::vector<CObstacleInstance> obstacles;
	ui8 castSpells[2]; //how many spells each side has cast this turn [0] - attacker, [1] - defender
	std::vector<const CSpell *> usedSpellsHistory[2]; //each time hero casts spell, it's inserted here -> eagle eye skill 
	SiegeInfo si;
	si32 battlefieldType;

	ui8 tacticsSide; //which side is requested to play tactics phase
	ui8 tacticDistance; //how many hexes we can go forward (1 = only hexes adjacent to margin line)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sides & round & activeStack & siege & town & tile & stacks & belligerents & obstacles
			& castSpells & si & battlefieldType;
		h & heroes;
		h & usedSpellsHistory;
		h & tacticsSide & tacticDistance;
		h & static_cast<CBonusSystemNode&>(*this);
	}

	//////////////////////////////////////////////////////////////////////////
	//void getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root = NULL) const;
	//////////////////////////////////////////////////////////////////////////

	const CStack * getNextStack() const; //which stack will have turn after current one
	void getStackQueue(std::vector<const CStack *> &out, int howMany, int turn = 0, int lastMoved = -1) const; //returns stack in order of their movement action
	CStack * getStack(int stackID, bool onlyAlive = true);
	const CStack * getStack(int stackID, bool onlyAlive = true) const;
	CStack * getStackT(THex tileID, bool onlyAlive = true);
	const CStack * getStackT(THex tileID, bool onlyAlive = true) const;
	void getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<THex> & occupyable, bool flying, const CStack* stackToOmmit = NULL) const; //send pointer to at least 187 allocated bytes
	static bool isAccessible(THex hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos); //helper for makeBFS
	void makeBFS(THex start, bool*accessibility, THex *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying, bool fillPredecessors) const; //*accessibility must be prepared bool[187] array; last two pointers must point to the at least 187-elements int arrays - there is written result
	std::pair< std::vector<THex>, int > getPath(THex start, THex dest, bool*accessibility, bool flyingCreature, bool twoHex, bool attackerOwned); //returned value: pair<path, length>; length may be different than number of elements in path since flying vreatures jump between distant hexes
	std::vector<THex> getAccessibility(const CStack * stack, bool addOccupiable) const; //returns vector of accessible tiles (taking into account the creature range)

	bool isStackBlocked(const CStack * stack) const; //returns true if there is neighboring enemy stack

	ui32 calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky); //charge - number of hexes travelled before attack (for champion's jousting)
	TDmgRange calculateDmgRange( const CStack* attacker, const CStack* defender, TQuantity attackerCount, TQuantity defenderCount, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>
	TDmgRange calculateDmgRange(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky) const; //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>
	void calculateCasualties(std::map<ui32,si32> *casualties) const; //casualties are array of maps size 2 (attacker, defeneder), maps are (crid => amount)
	std::set<CStack*> getAttackedCreatures(const CSpell * s, int skillLevel, ui8 attackerOwner, THex destinationTile); //calculates stack affected by given spell
	static int calculateSpellDuration(const CSpell * spell, const CGHeroInstance * caster, int usedSpellPower);
	CStack * generateNewStack(const CStackInstance &base, int stackID, bool attackerOwned, int slot, THex position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	CStack * generateNewStack(const CStackBasicDescriptor &base, int stackID, bool attackerOwned, int slot, THex position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	ui32 getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	int hexToWallPart(THex hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	int lineToWallHex(int line) const; //returns hex with wall in given line
	std::pair<const CStack *, THex> getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const; //if attackerOwned is indetermnate, returened stack is of any owner; hex is the number of hex we should be looking from; returns (nerarest creature, predecessorHex)
	ui32 calculateSpellBonus(ui32 baseDamage, const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature) const;
	ui32 calculateSpellDmg(const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower) const; //calculates damage inflicted by spell
	ui32 calculateHealedHP(const CGHeroInstance * caster, const CSpell * spell, const CStack * stack) const;
	si8 hasDistancePenalty(const CStack * stackID, THex destHex) const; //determines if given stack has distance penalty shooting given pos
	si8 sameSideOfWall(int pos1, int pos2) const; //determines if given positions are on the same side of wall
	si8 hasWallPenalty(const CStack * stack, THex destHex) const; //determines if given stack has wall penalty shooting given pos
	si8 canTeleportTo(const CStack * stack, THex destHex, int telportLevel) const; //determines if given stack can teleport to given place
	bool battleCanShoot(const CStack * stack, THex dest) const; //determines if stack with given ID shoot at the selected destination
	const CGHeroInstance * getHero(int player) const; //returns fighting hero that belongs to given player

	SpellCasting::ESpellCastProblem battleCanCastSpell(int player, SpellCasting::ECastingMode mode) const; //returns true if there are no general issues preventing from casting a spell
	SpellCasting::ESpellCastProblem battleCanCastThisSpell(int player, const CSpell * spell, SpellCasting::ECastingMode mode) const; //checks if given player can cast given spell
	SpellCasting::ESpellCastProblem battleIsImmune(int player, const CSpell * spell, SpellCasting::ECastingMode mode, THex dest) const; //checks for creature immunity / anything that prevent casting *at given hex* - doesn't take into acount general problems such as not having spellbook or mana points etc.
	SpellCasting::ESpellCastProblem battleCanCastThisSpellHere(int player, const CSpell * spell, SpellCasting::ECastingMode mode, THex dest); //checks if given player can cast given spell at given tile in given mode

	std::vector<ui32> calculateResistedStacks(const CSpell * sp, const CGHeroInstance * caster, const CGHeroInstance * hero2, const std::set<CStack*> affectedCreatures, int casterSideOwner, SpellCasting::ECastingMode mode) const;


	bool battleCanFlee(int player) const; //returns true if player can flee from the battle
	const CStack * battleGetStack(THex pos, bool onlyAlive); //returns stack at given tile
	const CGHeroInstance * battleGetOwner(const CStack * stack) const; //returns hero that owns given stack; NULL if none
	si8 battleMinSpellLevel() const; //calculates minimum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, 0 is returned
	void localInit();
	static BattleInfo * setupBattle( int3 tile, int terrain, int terType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town );
	bool isInTacticRange( THex dest ) const;
};

class DLL_EXPORT CStack : public CBonusSystemNode, public CStackBasicDescriptor
{ 
public:
	const CStackInstance *base;

	ui32 ID; //unique ID of stack
	ui32 baseAmount;
	ui32 firstHPleft; //HP of first creature in stack
	ui8 owner, slot;  //owner - player colour (255 for neutrals), slot - position in garrison (may be 255 for neutrals/called creatures)
	ui8 attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	THex position; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower
	ui8 counterAttacks; //how many counter attacks can be performed more in this turn (by default set at the beginning of the round to 1)
	si16 shots; //how many shots left

	std::set<ECombatInfo> state;
	//overrides
	const CCreature* getCreature() const {return type;}

	CStack(const CStackInstance *base, int O, int I, bool AO, int S); //c-tor
	CStack(const CStackBasicDescriptor *stack, int O, int I, bool AO, int S = 255); //c-tor
	CStack(); //c-tor
	~CStack();
	std::string nodeName() const OVERRIDE;

	void init(); //set initial (invalid) values
	void postInit(); //used to finish initialization when inheriting creature parameters is working
	const Bonus * getEffect(ui16 id, int turn = 0) const; //effect id (SP)
	ui8 howManyEffectsSet(ui16 id) const; //returns amount of effects with given id set for this stack
	bool willMove(int turn = 0) const; //if stack has remaining move this turn
	bool moved(int turn = 0) const; //if stack was already moved this turn
	bool canMove(int turn = 0) const; //if stack can move
	ui32 Speed(int turn = 0) const; //get speed of creature with all modificators
	BonusList getSpellBonuses() const;
	static void stackEffectToFeature(std::vector<Bonus> & sf, const Bonus & sse);
	std::vector<si32> activeSpells() const; //returns vector of active spell IDs sorted by time of cast
	const CGHeroInstance *getMyHero() const; //if stack belongs to hero (directly or was by him summoned) returns hero, NULL otherwise

	static inline Bonus featureGenerator(Bonus::BonusType type, si16 subtype, si32 value, ui16 turnsRemain, si32 additionalInfo = 0, si32 limit = Bonus::NO_LIMIT)
	{
		Bonus hb = makeFeatureVal(type, Bonus::N_TURNS, subtype, value, Bonus::SPELL_EFFECT, turnsRemain, additionalInfo);
		hb.effectRange = limit;
		hb.source = Bonus::SPELL_EFFECT;
		return hb;
	}

	static inline Bonus featureGeneratorVT(Bonus::BonusType type, si16 subtype, si32 value, ui16 turnsRemain, ui8 valType)
	{
		Bonus ret = makeFeatureVal(type, Bonus::N_TURNS, subtype, value, Bonus::SPELL_EFFECT, turnsRemain);
		ret.valType = valType;
		ret.source = Bonus::SPELL_EFFECT;
		return ret;
	}

	static bool isMeleeAttackPossible(const CStack * attacker, const CStack * defender, THex attackerPos = THex::INVALID, THex defenderPos = THex::INVALID);

	bool doubleWide() const;
	THex occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1

	void prepareAttacked(BattleStackAttacked &bsa) const; //requires bsa.damageAmout filled

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		assert(isIndependentNode());
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CStackBasicDescriptor&>(*this);
		h & ID & baseAmount & firstHPleft & owner & slot & attackerOwned & position & state & counterAttacks
			& shots & count;

		TSlot slot = (base ? base->armyObj->findStack(base) : -1);
		const CArmedInstance *army = (base ? base->armyObj : NULL);
		if(h.saving)
		{
			h & army & slot;
		}
		else
		{
			h & army & slot;
			if(!army || slot == -1 || !army->hasStackAtSlot(slot))
			{
				base = NULL;
				tlog3 << type->nameSing << " don't have a base stack!\n";
			}
			else
			{
				base = &army->getStack(slot);
			}
		}

	}
	bool alive() const //determines if stack is alive
	{
		return vstd::contains(state,ALIVE);
	}
};

class DLL_EXPORT CMP_stack
{
	int phase; //rules of which phase will be used
	int turn;
public:

	bool operator ()(const CStack* a, const CStack* b);
	CMP_stack(int Phase = 1, int Turn = 0);
};

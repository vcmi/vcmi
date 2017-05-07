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
#include "SiegeInfo.h"
#include "SideInBattle.h"
#include "HeroBonus.h"
#include "CBattleCallback.h"
#include "int3.h"

class CStack;
class CStackInstance;
class CStackBasicDescriptor;

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

	ui32 calculateDmg(const CStack * attacker, const CStack * defender, bool shooting, ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg, CRandomGenerator & rand); //charge - number of hexes travelled before attack (for champion's jousting)
	void calculateCasualties(std::map<ui32,si32> *casualties) const; //casualties are array of maps size 2 (attacker, defeneder), maps are (crid => amount)

	//void getPotentiallyAttackableHexes(AttackableTiles &at, const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos); //hexes around target that could be attacked in melee
	//std::set<CStack*> getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID); //calculates range of multi-hex attacks
	//std::set<BattleHex> getAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos = BattleHex::INVALID); //calculates range of multi-hex attacks

	CStack * generateNewStack(const CStackInstance &base, bool attackerOwned, SlotID slot, BattleHex position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	CStack * generateNewStack(const CStackBasicDescriptor &base, bool attackerOwned, SlotID slot, BattleHex position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	int getIdForNewStack() const; //suggest a currently unused ID that'd suitable for generating a new stack

	const CGHeroInstance * getHero(PlayerColor player) const; //returns fighting hero that belongs to given player

	void localInit();

	void localInitStack(CStack * s);
	static BattleInfo * setupBattle( int3 tile, ETerrainType terrain, BFieldType battlefieldType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town );
	//bool hasNativeStack(ui8 side) const;

	PlayerColor theOtherPlayer(PlayerColor player) const;
	ui8 whatSide(PlayerColor player) const;

	static BattlefieldBI::BattlefieldBI battlefieldTypeToBI(BFieldType bfieldType); //converts above to ERM BI format
	static int battlefieldTypeToTerrain(int bfieldType); //converts above to ERM BI format
};


class DLL_LINKAGE CMP_stack
{
	int phase; //rules of which phase will be used
	int turn;
public:

	bool operator ()(const CStack* a, const CStack* b);
	CMP_stack(int Phase = 1, int Turn = 0);
};

/*
 * CStack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "BattleHex.h"
#include "CCreatureHandler.h"
#include "mapObjects/CGHeroInstance.h" // for commander serialization

struct BattleStackAttacked;

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
	ui32 totalHealth() const; // total health for all creatures in stack;

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

	void getCasterName(MetaString & text) const override;

	void getCastDescription(const CSpell * spell, const std::vector<const CStack *> & attacked, MetaString & text) const override;

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
		SlotID extSlot = (base ? base->armyObj->findStack(base) : SlotID());

		if(h.saving)
		{
			h & army & extSlot;
		}
		else
		{
			h & army & extSlot;
			if(extSlot == SlotID::COMMANDER_SLOT_PLACEHOLDER)
			{
				auto hero = dynamic_cast<const CGHeroInstance *>(army);
				assert (hero);
				base = hero->commander;
			}
			else if(slot == SlotID::SUMMONED_SLOT_PLACEHOLDER || slot == SlotID::ARROW_TOWERS_SLOT || slot == SlotID::WAR_MACHINES_SLOT)
			{
				//no external slot possible, so no base stack
				base = nullptr;
			}
			else if(!army || extSlot == SlotID() || !army->hasStackAtSlot(extSlot))
			{
				base = nullptr;
				logGlobal->warnStream() << type->nameSing << " doesn't have a base stack!";
			}
			else
			{
				base = &army->getStack(extSlot);
			}
		}

	}
	bool alive() const;

	bool isDead() const;
	bool isGhost() const; //determines if stack was removed
	bool isValidTarget(bool allowDead = false) const; //non-turret non-ghost stacks (can be attacked or be object of magic effect)
	bool isTurret() const;
};

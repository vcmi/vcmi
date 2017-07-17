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
#include "battle/BattleHex.h"
#include "CCreatureHandler.h"
#include "mapObjects/CGHeroInstance.h" // for commander serialization

struct BattleStackAttacked;
struct BattleInfo;
class CStack;
class CHealthInfo;

template <typename Quantity>
class DLL_LINKAGE CStackResource
{
public:
	CStackResource(const CStack * Owner):
		owner(Owner)
	{
		reset();
	}

	virtual void reset()
	{
		used = 0;
	};

protected:
	const CStack * owner;
	Quantity used;
};

class DLL_LINKAGE CAmmo : public CStackResource<int32_t>
{
public:
	CAmmo(const CStack * Owner, CSelector totalSelector);

	int32_t available() const;
	bool canUse(int32_t amount = 1) const;
	virtual void reset() override;
	virtual int32_t total() const;
	virtual void use(int32_t amount = 1);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		if(!h.saving)
			reset();
		h & used;
	}
protected:
	CBonusProxy totalProxy;
};

class DLL_LINKAGE CShots : public CAmmo
{
public:
	CShots(const CStack * Owner);
	void use(int32_t amount = 1) override;
};

class DLL_LINKAGE CCasts : public CAmmo
{
public:
	CCasts(const CStack * Owner);
};

class DLL_LINKAGE CRetaliations : public CAmmo
{
public:
	CRetaliations(const CStack * Owner);
	int32_t total() const override;
	void reset() override;
private:
	mutable int32_t totalCache;
};

class DLL_LINKAGE IUnitHealthInfo
{
public:
	virtual int32_t unitMaxHealth() const = 0;
	virtual int32_t unitBaseAmount() const = 0;
};

class DLL_LINKAGE CHealth
{
public:
	CHealth(const IUnitHealthInfo * Owner);
	CHealth(const CHealth & other);

	void init();
	void reset();

	void damage(int32_t & amount);
	void heal(int32_t & amount, EHealLevel level, EHealPower power);

	int32_t getCount() const;
	int32_t getFirstHPleft() const;
	int32_t getResurrected() const;

	int64_t available() const;
	int64_t total() const;

	void toInfo(CHealthInfo & info) const;
	void fromInfo(const CHealthInfo & info);

	void takeResurrected();

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		if(!h.saving)
			reset();
		h & firstHPleft & fullUnits & resurrected;
	}
private:
	void addResurrected(int32_t amount);
	void setFromTotal(const int64_t totalHealth);
	const IUnitHealthInfo * owner;

	int32_t firstHPleft;
	int32_t fullUnits;
	int32_t resurrected;
};

class DLL_LINKAGE CStack : public CBonusSystemNode, public ISpellCaster, public IUnitHealthInfo
{
public:
	const CStackInstance * base; //garrison slot from which stack originates (nullptr for war machines, summoned cres, etc)

	ui32 ID; //unique ID of stack
	ui32 baseAmount;
	const CCreature * type;

	PlayerColor owner; //owner - player color (255 for neutrals)
	SlotID slot;  //slot - position in garrison (may be 255 for neutrals/called creatures)
	ui8 side;
	BattleHex position; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower

	CRetaliations counterAttacks;
	CShots shots;
	CCasts casts;
	CHealth health;

	///id of alive clone of this stack clone if any
	si32 cloneID;
	std::set<EBattleStackState::EBattleStackState> state;

	CStack(const CStackInstance * base, PlayerColor O, int I, ui8 Side, SlotID S);
	CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S = SlotID(255));
	CStack();
	~CStack();

	int32_t getKilled() const;
	int32_t getCount() const;
	int32_t getFirstHPleft() const;
	const CCreature * getCreature() const;

	std::string nodeName() const override;

	void init(); //set initial (invalid) values
	void localInit(BattleInfo * battleInfo);
	std::string getName() const; //plural or singular
	bool willMove(int turn = 0) const; //if stack has remaining move this turn
	bool ableToRetaliate() const; //if stack can retaliate after attacked

	bool moved(int turn = 0) const; //if stack was already moved this turn
	bool waited(int turn = 0) const;

	bool canCast() const;
	bool isCaster() const;

	bool canMove(int turn = 0) const; //if stack can move

	bool canShoot() const;
	bool isShooter() const;

	bool canBeHealed() const; //for first aid tent - only harmed stacks that are not war machines

	ui32 level() const;
	si32 magicResistance() const override; //include aura of resistance
	std::vector<si32> activeSpells() const; //returns vector of active spell IDs sorted by time of cast
	const CGHeroInstance * getMyHero() const; //if stack belongs to hero (directly or was by him summoned) returns hero, nullptr otherwise

	static bool isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos = BattleHex::INVALID, BattleHex defenderPos = BattleHex::INVALID);

	bool doubleWide() const;
	BattleHex occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1
	BattleHex occupiedHex(BattleHex assumedPos) const; //returns number of occupied hex (not the position) if stack is double wide and would stand on assumedPos; otherwise -1
	std::vector<BattleHex> getHexes() const; //up to two occupied hexes, starting from front
	std::vector<BattleHex> getHexes(BattleHex assumedPos) const; //up to two occupied hexes, starting from front
	static std::vector<BattleHex> getHexes(BattleHex assumedPos, bool twoHex, ui8 side); //up to two occupied hexes, starting from front
	bool coversPos(BattleHex position) const; //checks also if unit is double-wide
	std::vector<BattleHex> getSurroundingHexes(BattleHex attackerPos = BattleHex::INVALID) const; // get six or 8 surrounding hexes depending on creature size

	BattleHex::EDir destShiftDir() const;

	CHealth healthAfterAttacked(int32_t & damage) const;
	CHealth healthAfterAttacked(int32_t & damage, const CHealth & customHealth) const;

	CHealth healthAfterHealed(int32_t & toHeal, EHealLevel level, EHealPower power) const;

	void prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand) const; //requires bsa.damageAmout filled
	void prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand, const CHealth & customHealth) const; //requires bsa.damageAmout filled

	///ISpellCaster

	ui8 getSpellSchoolLevel(const CSpell * spell, int * outSelectedSchool = nullptr) const override;
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

	///IUnitHealthInfo

	int32_t unitMaxHealth() const override;
	int32_t unitBaseAmount() const override;

	///MetaStrings

	void addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	void addNameReplacement(MetaString & text, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	std::string formatGeneralMessage(const int32_t baseTextId) const;

	///Non const API for NetPacks

	///stack will be ghost in next battle state update
	void makeGhost();
	void setHealth(const CHealthInfo & value);
	void setHealth(const CHealth & value);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		assert(isIndependentNode());
		h & static_cast<CBonusSystemNode &>(*this);
		h & type;
		h & ID & baseAmount & owner & slot & side & position & state;
		h & shots & casts & counterAttacks & health;

		const CArmedInstance * army = (base ? base->armyObj : nullptr);
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
				assert(hero);
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

	bool isClone() const;
	bool isDead() const;
	bool isGhost() const; //determines if stack was removed
	bool isValidTarget(bool allowDead = false) const; //non-turret non-ghost stacks (can be attacked or be object of magic effect)
	bool isTurret() const;

	friend class CShots; //for BattleInfo access
private:
	const BattleInfo * battle; //do not serialize
};

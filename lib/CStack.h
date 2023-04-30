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
#include "JsonNode.h"
#include "bonuses/HeroBonus.h"
#include "bonuses/CBonusSystemNode.h"
#include "CCreatureHandler.h" //todo: remove
#include "battle/BattleHex.h"
#include "mapObjects/CGHeroInstance.h" // for commander serialization

#include "battle/CUnitState.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleStackAttacked;
class BattleInfo;

//Represents STACK_BATTLE nodes
class DLL_LINKAGE CStack : public CBonusSystemNode, public battle::CUnitState, public battle::IUnitEnvironment
{
private:
	ui32 ID = -1; //unique ID of stack
	const CCreature * type = nullptr;
	TerrainId nativeTerrain; //tmp variable to save native terrain value on battle init
	ui32 baseAmount = -1;

	PlayerColor owner; //owner - player color (255 for neutrals)
	ui8 side = 1;

	SlotID slot;  //slot - position in garrison (may be 255 for neutrals/called creatures)

public:
	const CStackInstance * base = nullptr; //garrison slot from which stack originates (nullptr for war machines, summoned cres, etc)
	
	BattleHex initialPosition; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower

	CStack(const CStackInstance * base, const PlayerColor & O, int I, ui8 Side, const SlotID & S);
	CStack(const CStackBasicDescriptor * stack, const PlayerColor & O, int I, ui8 Side, const SlotID & S = SlotID(255));
	CStack();
	~CStack();

	std::string nodeName() const override;

	void localInit(BattleInfo * battleInfo);
	std::string getName() const; //plural or singular

	bool canBeHealed() const; //for first aid tent - only harmed stacks that are not war machines
	bool isOnNativeTerrain() const;
	bool isOnTerrain(TerrainId terrain) const;

	ui32 level() const;
	si32 magicResistance() const override; //include aura of resistance
	std::vector<si32> activeSpells() const; //returns vector of active spell IDs sorted by time of cast
	const CGHeroInstance * getMyHero() const; //if stack belongs to hero (directly or was by him summoned) returns hero, nullptr otherwise

	static std::vector<BattleHex> meleeAttackHexes(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos = BattleHex::INVALID, BattleHex defenderPos = BattleHex::INVALID);
	static bool isMeleeAttackPossible(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos = BattleHex::INVALID, BattleHex defenderPos = BattleHex::INVALID);

	BattleHex::EDir destShiftDir() const;

	void prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand) const; //requires bsa.damageAmout filled
	static void prepareAttacked(BattleStackAttacked & bsa,
								vstd::RNG & rand,
								const std::shared_ptr<battle::CUnitState> & customState); //requires bsa.damageAmout filled

	const CCreature * unitType() const override;
	int32_t unitBaseAmount() const override;

	uint32_t unitId() const override;
	ui8 unitSide() const override;
	PlayerColor unitOwner() const override;
	SlotID unitSlot() const override;

	std::string getDescription() const override;

	bool unitHasAmmoCart(const battle::Unit * unit) const override;
	PlayerColor unitEffectiveOwner(const battle::Unit * unit) const override;

	void spendMana(ServerCallback * server, const int spellCost) const override;

	const IBonusBearer* getBonusBearer() const override;
	
	PlayerColor getOwner() const override
	{
		return this->owner;
	}

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		//this assumes that stack objects is newly created
		//stackState is not serialized here
		assert(isIndependentNode());
		h & static_cast<CBonusSystemNode&>(*this);
		h & type;
		h & ID;
		h & baseAmount;
		h & owner;
		h & slot;
		h & side;
		h & initialPosition;

		const CArmedInstance * army = (base ? base->armyObj : nullptr);
		SlotID extSlot = (base ? base->armyObj->findStack(base) : SlotID());

		if(h.saving)
		{
			h & army;
			h & extSlot;
		}
		else
		{
			h & army;
			h & extSlot;

			if(extSlot == SlotID::COMMANDER_SLOT_PLACEHOLDER)
			{
				const auto * hero = dynamic_cast<const CGHeroInstance *>(army);
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
				logGlobal->warn("%s doesn't have a base stack!", type->getNameSingularTranslated());
			}
			else
			{
				base = &army->getStack(extSlot);
			}
		}
	}

private:
	const BattleInfo * battle; //do not serialize
};

VCMI_LIB_NAMESPACE_END

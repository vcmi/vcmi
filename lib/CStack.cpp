/*
 * CStack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CStack.h"

#include <vstd/RNG.h>

#include <vcmi/ServerCallback.h>

#include "CGeneralTextHandler.h"
#include "battle/BattleInfo.h"
#include "spells/CSpellHandler.h"
#include "NetPacks.h"

VCMI_LIB_NAMESPACE_BEGIN


///CStack
CStack::CStack(const CStackInstance * Base, PlayerColor O, int I, ui8 Side, SlotID S)
	: CBonusSystemNode(STACK_BATTLE),
	CUnitState(),
	base(Base),
	ID(I),
	type(Base->type),
	baseAmount(base->count),
	owner(O),
	slot(S),
	side(Side),
	initialPosition(),
	nativeTerrain()
{
	health.init(); //???
}

CStack::CStack()
	: CBonusSystemNode(STACK_BATTLE),
	CUnitState(),
	nativeTerrain()
{
	base = nullptr;
	type = nullptr;
	ID = -1;
	baseAmount = -1;
	owner = PlayerColor::NEUTRAL;
	slot = SlotID(255);
	side = 1;
	initialPosition = BattleHex();
}

CStack::CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S)
	: CBonusSystemNode(STACK_BATTLE),
	CUnitState(),
	base(nullptr),
	ID(I),
	type(stack->type),
	baseAmount(stack->count),
	owner(O),
	slot(S),
	side(Side),
	initialPosition()
{
	health.init(); //???
}

const CCreature * CStack::getCreature() const
{
	return type;
}

void CStack::localInit(BattleInfo * battleInfo)
{
	battle = battleInfo;
	assert(type);

	exportBonuses();
	if(base) //stack originating from "real" stack in garrison -> attach to it
	{
		attachTo(const_cast<CStackInstance *>(base));
	}
	else //attach directly to obj to which stack belongs and creature type
	{
		CArmedInstance * army = battle->battleGetArmyObject(side);
		attachTo(army);
		attachTo(const_cast<CCreature *>(type));
	}
	nativeTerrain = type->getNativeTerrain(); //save nativeTerrain in the variable on the battle start to avoid dead lock
	CUnitState::localInit(this); //it causes execution of the CStack::isOnNativeTerrain where nativeTerrain will be considered
	position = initialPosition;
}

ui32 CStack::level() const
{
	if(base)
		return base->getLevel(); //creature or commander
	else
		return std::max(1, (int)getCreature()->level); //war machine, clone etc
}

si32 CStack::magicResistance() const
{
	si32 magicResistance = IBonusBearer::magicResistance();

	si32 auraBonus = 0;

	for(auto one : battle->battleAdjacentUnits(this))
	{
		if(one->unitOwner() == owner)
			vstd::amax(auraBonus, one->valOfBonuses(Bonus::SPELL_RESISTANCE_AURA)); //max value
	}
	magicResistance += auraBonus;
	vstd::amin(magicResistance, 100);

	return magicResistance;
}

BattleHex::EDir CStack::destShiftDir() const
{
	if(doubleWide())
	{
		if(side == BattleSide::ATTACKER)
			return BattleHex::EDir::RIGHT;
		else
			return BattleHex::EDir::LEFT;
	}
	else
	{
		return BattleHex::EDir::NONE;
	}
}

std::vector<si32> CStack::activeSpells() const
{
	std::vector<si32> ret;

	std::stringstream cachingStr;
	cachingStr << "!type_" << Bonus::NONE << "source_" << Bonus::SPELL_EFFECT;
	CSelector selector = Selector::sourceType()(Bonus::SPELL_EFFECT)
						 .And(CSelector([](const Bonus * b)->bool
	{
		return b->type != Bonus::NONE;
	}));

	TConstBonusListPtr spellEffects = getBonuses(selector, Selector::all, cachingStr.str());
	for(const auto & it : *spellEffects)
	{
		if(!vstd::contains(ret, it->sid))  //do not duplicate spells with multiple effects
			ret.push_back(it->sid);
	}

	return ret;
}

CStack::~CStack()
{
	detachFromAll();
}

const CGHeroInstance * CStack::getMyHero() const
{
	if(base)
		return dynamic_cast<const CGHeroInstance *>(base->armyObj);
	else //we are attached directly?
		for(const CBonusSystemNode * n : getParentNodes())
			if(n->getNodeType() == HERO)
				return dynamic_cast<const CGHeroInstance *>(n);

	return nullptr;
}

std::string CStack::nodeName() const
{
	std::ostringstream oss;
	oss << owner.getStr();
	oss << " battle stack [" << ID << "]: " << getCount() << " of ";
	if(type)
		oss << type->namePl;
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << slot;
	if(base && base->armyObj)
		oss << " of armyobj=" << base->armyObj->id.getNum();
	return oss.str();
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand) const
{
	auto newState = acquireState();
	prepareAttacked(bsa, rand, newState);
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand, std::shared_ptr<battle::CUnitState> customState)
{
	auto initialCount = customState->getCount();

	customState->damage(bsa.damageAmount);

	bsa.killedAmount = initialCount - customState->getCount();

	if(!customState->alive() && customState->isClone())
	{
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
	}
	else if(!customState->alive()) //stack killed
	{
		bsa.flags |= BattleStackAttacked::KILLED;

		auto resurrectValue = customState->valOfBonuses(Bonus::REBIRTH);

		if(resurrectValue > 0 && customState->canCast()) //there must be casts left
		{
			double resurrectFactor = resurrectValue / 100.0;

			auto baseAmount = customState->unitBaseAmount();

			double resurrectedRaw = baseAmount * resurrectFactor;

			int32_t resurrectedCount = static_cast<int32_t>(floor(resurrectedRaw));

			int32_t resurrectedAdd = static_cast<int32_t>(baseAmount - (resurrectedCount/resurrectFactor));

			auto rangeGen = rand.getInt64Range(0, 99);

			for(int32_t i = 0; i < resurrectedAdd; i++)
			{
				if(resurrectValue > rangeGen())
					resurrectedCount += 1;
			}

			if(customState->hasBonusOfType(Bonus::REBIRTH, 1))
			{
				// resurrect at least one Sacred Phoenix
				vstd::amax(resurrectedCount, 1);
			}

			if(resurrectedCount > 0)
			{
				customState->casts.use();
				bsa.flags |= BattleStackAttacked::REBIRTH;
				int64_t toHeal = customState->MaxHealth() * resurrectedCount;
				//TODO: add one-battle rebirth?
				customState->heal(toHeal, EHealLevel::RESURRECT, EHealPower::PERMANENT);
				customState->counterAttacks.use(customState->counterAttacks.available());
			}
		}
	}

	customState->save(bsa.newState.data);
	bsa.newState.healthDelta = -bsa.damageAmount;
	bsa.newState.id = customState->unitId();
	bsa.newState.operation = UnitChanges::EOperation::RESET_STATE;
}

std::vector<BattleHex> CStack::meleeAttackHexes(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos, BattleHex defenderPos)
{
	int mask = 0;
	std::vector<BattleHex> res;

	if (!attackerPos.isValid())
		attackerPos = attacker->getPosition();
	if (!defenderPos.isValid())
		defenderPos = defender->getPosition();

	BattleHex otherAttackerPos = attackerPos + (attacker->unitSide() == BattleSide::ATTACKER ? -1 : 1);
	BattleHex otherDefenderPos = defenderPos + (defender->unitSide() == BattleSide::ATTACKER ? -1 : 1);

	if(BattleHex::mutualPosition(attackerPos, defenderPos) >= 0) //front <=> front
	{
		if((mask & 1) == 0)
		{
			mask |= 1;
			res.push_back(defenderPos);
		}
	}
	if (attacker->doubleWide() //back <=> front
		&& BattleHex::mutualPosition(otherAttackerPos, defenderPos) >= 0)
	{
		if((mask & 1) == 0)
		{
			mask |= 1;
			res.push_back(defenderPos);
		}
	}
	if (defender->doubleWide()//front <=> back
		&& BattleHex::mutualPosition(attackerPos, otherDefenderPos) >= 0)
	{
		if((mask & 2) == 0)
		{
			mask |= 2;
			res.push_back(otherDefenderPos);
		}
	}
	if (defender->doubleWide() && attacker->doubleWide()//back <=> back
		&& BattleHex::mutualPosition(otherAttackerPos, otherDefenderPos) >= 0)
	{
		if((mask & 2) == 0)
		{
			mask |= 2;
			res.push_back(otherDefenderPos);
		}
	}

	return res;
}

bool CStack::isMeleeAttackPossible(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos, BattleHex defenderPos)
{
	return !meleeAttackHexes(attacker, defender, attackerPos, defenderPos).empty();
}

std::string CStack::getName() const
{
	return (getCount() == 1) ? type->nameSing : type->namePl; //War machines can't use base
}

bool CStack::canBeHealed() const
{
	return getFirstHPleft() < (int32_t)MaxHealth()
		   && isValidTarget()
		   && !hasBonusOfType(Bonus::SIEGE_WEAPON);
}

bool CStack::isOnNativeTerrain() const
{
	//this code is called from CreatureTerrainLimiter::limit on battle start
	auto res = nativeTerrain == Terrain::ANY || nativeTerrain == battle->getTerrainType();
	return res;
}

bool CStack::isOnTerrain(const Terrain & terrain) const
{
	return battle->getTerrainType() == terrain;
}

const CCreature * CStack::unitType() const
{
	return type;
}

int32_t CStack::unitBaseAmount() const
{
	return baseAmount;
}

bool CStack::unitHasAmmoCart(const battle::Unit * unit) const
{
	for(const CStack * st : battle->stacks)
	{
		if(battle->battleMatchOwner(st, unit, true) && st->getCreature()->idNumber == CreatureID::AMMO_CART)
		{
			return st->alive();
		}
	}
	//ammo cart works during creature bank battle while not on battlefield
	auto ownerHero = battle->battleGetOwnerHero(unit);
	if(ownerHero && ownerHero->artifactsWorn.find(ArtifactPosition::MACH2) != ownerHero->artifactsWorn.end())
	{
		if(battle->battleGetOwnerHero(unit)->artifactsWorn.at(ArtifactPosition::MACH2).artifact->artType->id == ArtifactID::AMMO_CART)
		{
			return true;
		}
	}
	return false; //will be always false if trying to examine enemy hero in "special battle"
}

PlayerColor CStack::unitEffectiveOwner(const battle::Unit * unit) const
{
	return battle->battleGetOwner(unit);
}

uint32_t CStack::unitId() const
{
	return ID;
}

ui8 CStack::unitSide() const
{
	return side;
}

PlayerColor CStack::unitOwner() const
{
	return owner;
}

SlotID CStack::unitSlot() const
{
	return slot;
}

std::string CStack::getDescription() const
{
	return nodeName();
}

void CStack::spendMana(ServerCallback * server, const int spellCost) const
{
	if(spellCost != 1)
		logGlobal->warn("Unexpected spell cost %d for creature", spellCost);

	BattleSetStackProperty ssp;
	ssp.stackID = unitId();
	ssp.which = BattleSetStackProperty::CASTS;
	ssp.val = -spellCost;
	ssp.absolute = false;
	server->apply(&ssp);
}

VCMI_LIB_NAMESPACE_END

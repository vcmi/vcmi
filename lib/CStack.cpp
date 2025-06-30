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

#include <vcmi/Entity.h>
#include <vcmi/ServerCallback.h>

#include "texts/CGeneralTextHandler.h"
#include "battle/BattleInfo.h"
#include "GameLibrary.h"
#include "spells/CSpellHandler.h"
#include "networkPacks/PacksForClientBattle.h"

VCMI_LIB_NAMESPACE_BEGIN


///CStack
CStack::CStack(const CStackInstance * Base, const PlayerColor & O, int I, BattleSide Side, const SlotID & S):
	CBonusSystemNode(BonusNodeType::STACK_BATTLE),
	base(Base),
	ID(I),
	typeID(Base->getId()),
	baseAmount(Base->getCount()),
	owner(O),
	slot(S),
	side(Side)
{
	health.init(); //???
	doubleWideCached = battle::CUnitState::doubleWide();
}

CStack::CStack():
	CBonusSystemNode(BonusNodeType::STACK_BATTLE),
	owner(PlayerColor::NEUTRAL),
	slot(SlotID(255)),
	initialPosition(BattleHex())
{
}

CStack::CStack(const CStackBasicDescriptor * stack, const PlayerColor & O, int I, BattleSide Side, const SlotID & S):
	CBonusSystemNode(BonusNodeType::STACK_BATTLE),
	ID(I),
	typeID(stack->getId()),
	baseAmount(stack->getCount()),
	owner(O),
	slot(S),
	side(Side)
{
	health.init(); //???
	doubleWideCached = battle::CUnitState::doubleWide();
}

void CStack::localInit(BattleInfo * battleInfo)
{
	battle = battleInfo;
	assert(typeID.hasValue());

	exportBonuses();
	if(base) //stack originating from "real" stack in garrison -> attach to it
	{
		attachTo(const_cast<CStackInstance&>(*base));
	}
	else //attach directly to obj to which stack belongs and creature type
	{
		CArmedInstance * army = battle->battleGetArmyObject(side);
		assert(army);
		attachTo(*army);
		attachToSource(*typeID.toCreature());
	}
	CUnitState::localInit(this); //it causes execution of the CStack::isOnNativeTerrain where nativeTerrain will be considered
	position = initialPosition;
}

ui32 CStack::level() const
{
	if(base)
		return base->getLevel(); //creature or commander
	else
		return std::max(1, static_cast<int>(unitType()->getLevel())); //war machine, clone etc
}

si32 CStack::magicResistance() const
{
	auto magicResistance = AFactionMember::magicResistance();

	si32 auraBonus = 0;

	for(const auto * one : battle->battleAdjacentUnits(this))
	{
		if(one->unitOwner() == owner)
			vstd::amax(auraBonus, one->valOfBonuses(BonusType::SPELL_RESISTANCE_AURA)); //max value
	}
	vstd::abetween(auraBonus, 0, 100);
	vstd::abetween(magicResistance, 0, 100);
	float castChance = (100 - magicResistance) * (100 - auraBonus)/100.0;

	return static_cast<si32>(100 - castChance);
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

std::vector<SpellID> CStack::activeSpells() const
{
	std::vector<SpellID> ret;

	std::stringstream cachingStr;
	cachingStr << "!type_" << vstd::to_underlying(BonusType::NONE) << "source_" << vstd::to_underlying(BonusSource::SPELL_EFFECT);
	CSelector selector = Selector::sourceType()(BonusSource::SPELL_EFFECT)
						 .And(CSelector([](const Bonus * b)->bool
	{
		return b->type != BonusType::NONE && b->sid.as<SpellID>().toSpell() && !b->sid.as<SpellID>().toSpell()->isAdventure();
	}));

	TConstBonusListPtr spellEffects = getBonuses(selector, cachingStr.str());
	for(const auto & it : *spellEffects)
	{
		if(!vstd::contains(ret, it->sid.as<SpellID>()))  //do not duplicate spells with multiple effects
			ret.push_back(it->sid.as<SpellID>());
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
		return dynamic_cast<const CGHeroInstance *>(base->getArmy());
	else //we are attached directly?
		for(const CBonusSystemNode * n : getParentNodes())
			if(n->getNodeType() == BonusNodeType::HERO)
				return dynamic_cast<const CGHeroInstance *>(n);

	return nullptr;
}

std::string CStack::nodeName() const
{
	std::ostringstream oss;
	oss << owner.toString();
	oss << " battle stack [" << ID << "]: " << getCount() << " of ";
	if(typeID.hasValue())
		oss << typeID.toEntity(LIBRARY)->getJsonKey();
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << slot;
	if(base && base->getArmy())
		oss << " of armyobj=" << base->getArmy()->id.getNum();
	return oss.str();
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand) const
{
	auto newState = acquireState();
	prepareAttacked(bsa, rand, newState);
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand, const std::shared_ptr<battle::CUnitState> & customState)
{
	auto initialCount = customState->getCount();

	// compute damage and update bsa.damageAmount
	customState->damage(bsa.damageAmount);

	bsa.killedAmount = initialCount - customState->getCount();

	if(!customState->alive() && customState->isClone())
	{
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
	}
	else if(!customState->alive()) //stack killed
	{
		bsa.flags |= BattleStackAttacked::KILLED;

		auto resurrectValue = customState->valOfBonuses(BonusType::REBIRTH);

		if(resurrectValue > 0 && customState->canCast()) //there must be casts left
		{
			double resurrectFactor = resurrectValue / 100.0;

			auto baseAmount = customState->unitBaseAmount();

			double resurrectedRaw = baseAmount * resurrectFactor;

			auto resurrectedCount = static_cast<int32_t>(floor(resurrectedRaw));

			auto resurrectedAdd = static_cast<int32_t>(baseAmount - (resurrectedCount / resurrectFactor));

			for(int32_t i = 0; i < resurrectedAdd; i++)
			{
				if(resurrectValue > rand.nextInt(0, 99))
					resurrectedCount += 1;
			}

			if(customState->hasBonusOfType(BonusType::REBIRTH, BonusCustomSubtype::rebirthSpecial))
			{
				// resurrect at least one Sacred Phoenix
				vstd::amax(resurrectedCount, 1);
			}

			if(resurrectedCount > 0)
			{
				customState->casts.use();
				bsa.flags |= BattleStackAttacked::REBIRTH;
				int64_t toHeal = customState->getMaxHealth() * resurrectedCount;
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

BattleHexArray CStack::meleeAttackHexes(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos, BattleHex defenderPos)
{
	int mask = 0;
	BattleHexArray res;

	if (!attackerPos.isValid())
		attackerPos = attacker->getPosition();
	if (!defenderPos.isValid())
		defenderPos = defender->getPosition();

	BattleHex otherAttackerPos = attackerPos.toInt() + (attacker->unitSide() == BattleSide::ATTACKER ? -1 : 1);
	BattleHex otherDefenderPos = defenderPos.toInt() + (defender->unitSide() == BattleSide::ATTACKER ? -1 : 1);

	if(BattleHex::mutualPosition(attackerPos, defenderPos) >= 0) //front <=> front
	{
		if((mask & 1) == 0)
		{
			mask |= 1;
			res.insert(defenderPos);
		}
	}
	if (attacker->doubleWide() //back <=> front
		&& BattleHex::mutualPosition(otherAttackerPos, defenderPos) >= 0)
	{
		if((mask & 1) == 0)
		{
			mask |= 1;
			res.insert(defenderPos);
		}
	}
	if (defender->doubleWide()//front <=> back
		&& BattleHex::mutualPosition(attackerPos, otherDefenderPos) >= 0)
	{
		if((mask & 2) == 0)
		{
			mask |= 2;
			res.insert(otherDefenderPos);
		}
	}
	if (defender->doubleWide() && attacker->doubleWide()//back <=> back
		&& BattleHex::mutualPosition(otherAttackerPos, otherDefenderPos) >= 0)
	{
		if((mask & 2) == 0)
		{
			mask |= 2;
			res.insert(otherDefenderPos);
		}
	}

	return res;
}

bool CStack::isMeleeAttackPossible(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos, BattleHex defenderPos)
{
	if(defender->isInvincible())
		return false;
		
	return !meleeAttackHexes(attacker, defender, attackerPos, defenderPos).empty();
}

std::string CStack::getName() const
{
	return (getCount() == 1) ? typeID.toEntity(LIBRARY)->getNameSingularTranslated() : typeID.toEntity(LIBRARY)->getNamePluralTranslated(); //War machines can't use base
}

bool CStack::canBeHealed() const
{
	return getFirstHPleft() < static_cast<int32_t>(getMaxHealth()) && isValidTarget() && !hasBonusOfType(BonusType::SIEGE_WEAPON);
}

bool CStack::isOnNativeTerrain() const
{
	auto nativeTerrain = getNativeTerrain();
	return nativeTerrain == ETerrainId::ANY_TERRAIN || getCurrentTerrain() == nativeTerrain;
}

TerrainId CStack::getCurrentTerrain() const
{
	return battle->getTerrainType();
}

const CCreature * CStack::unitType() const
{
	return typeID.toCreature();
}

int32_t CStack::unitBaseAmount() const
{
	return baseAmount;
}

const IBonusBearer* CStack::getBonusBearer() const
{
	return this;
}

bool CStack::unitHasAmmoCart(const battle::Unit * unit) const
{
	for(const auto & st : battle->stacks)
	{
		if(battle->battleMatchOwner(st.get(), unit, true) && st->unitType()->getId() == CreatureID::AMMO_CART)
		{
			return st->alive();
		}
	}
	//ammo cart works during creature bank battle while not on battlefield
	const auto * ownerHero = battle->battleGetOwnerHero(unit);
	if(ownerHero && ownerHero->artifactsWorn.find(ArtifactPosition::MACH2) != ownerHero->artifactsWorn.end())
	{
		if(battle->battleGetOwnerHero(unit)->artifactsWorn.at(ArtifactPosition::MACH2).getArt()->getTypeId() == ArtifactID::AMMO_CART)
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

BattleSide CStack::unitSide() const
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
	ssp.battleID = battle->battleID;
	ssp.stackID = unitId();
	ssp.which = BattleSetStackProperty::CASTS;
	ssp.val = -spellCost;
	ssp.absolute = false;
	server->apply(ssp);
}

void CStack::postDeserialize(const CArmedInstance * army)
{
	if(slot == SlotID::COMMANDER_SLOT_PLACEHOLDER)
	{
		const auto * hero = dynamic_cast<const CGHeroInstance *>(army);
		assert(hero);
		base = hero->getCommander();
	}
	else if(slot == SlotID::SUMMONED_SLOT_PLACEHOLDER || slot == SlotID::ARROW_TOWERS_SLOT || slot == SlotID::WAR_MACHINES_SLOT)
	{
		//no external slot possible, so no base stack
		base = nullptr;
	}
	else if(!army || slot == SlotID() || !army->hasStackAtSlot(slot))
	{
		throw std::runtime_error(typeID.toEntity(LIBRARY)->getNameSingularTranslated() + " doesn't have a base stack!");
	}
	else
	{
		base = &army->getStack(slot);
	}

	doubleWideCached = battle::CUnitState::doubleWide();
}

VCMI_LIB_NAMESPACE_END

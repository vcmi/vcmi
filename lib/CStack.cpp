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
#include "CGeneralTextHandler.h"
#include "battle/BattleInfo.h"
#include "spells/CSpellHandler.h"
#include "CRandomGenerator.h"
#include "NetPacks.h"


///CAmmo
CAmmo::CAmmo(const CStack * Owner, CSelector totalSelector):
	CStackResource(Owner), totalProxy(Owner, totalSelector)
{

}

int32_t CAmmo::available() const
{
	return total() - used;
}

bool CAmmo::canUse(int32_t amount) const
{
	return available() - amount >= 0;
}

void CAmmo::reset()
{
	used = 0;
}

int32_t CAmmo::total() const
{
	return totalProxy->totalValue();
}

void CAmmo::use(int32_t amount)
{
	if(available() - amount < 0)
	{
		logGlobal->error("Stack ammo overuse");
		used += available();
	}
	else
		used += amount;
}

///CShots
CShots::CShots(const CStack * Owner):
	CAmmo(Owner, Selector::type(Bonus::SHOTS))
{

}

void CShots::use(int32_t amount)
{
	//don't remove ammo if we control a working ammo cart
	bool hasAmmoCart = false;

	for(const CStack * st : owner->battle->stacks)
	{
		if(owner->battle->battleMatchOwner(st, owner, true) && st->getCreature()->idNumber == CreatureID::AMMO_CART && st->alive())
		{
			hasAmmoCart = true;
			break;
		}
	}

	if(!hasAmmoCart)
		CAmmo::use(amount);
}

///CCasts
CCasts::CCasts(const CStack * Owner):
	CAmmo(Owner, Selector::type(Bonus::CASTS))
{

}

///CRetaliations
CRetaliations::CRetaliations(const CStack * Owner):
	CAmmo(Owner, Selector::type(Bonus::ADDITIONAL_RETALIATION)), totalCache(0)
{

}

int32_t CRetaliations::total() const
{
	//after dispell bonus should remain during current round
	int32_t val = 1 + totalProxy->totalValue();
	vstd::amax(totalCache, val);
	return totalCache;
}

void CRetaliations::reset()
{
	CAmmo::reset();
	totalCache = 0;
}

///CHealth
CHealth::CHealth(const IUnitHealthInfo * Owner):
	owner(Owner)
{
	reset();
}

CHealth::CHealth(const CHealth & other):
	owner(other.owner),
	firstHPleft(other.firstHPleft),
	fullUnits(other.fullUnits),
	resurrected(other.resurrected)
{

}

void CHealth::init()
{
	reset();
	fullUnits = owner->unitBaseAmount() > 1 ? owner->unitBaseAmount() - 1 : 0;
	firstHPleft = owner->unitBaseAmount() > 0 ? owner->unitMaxHealth() : 0;
}

void CHealth::addResurrected(int32_t amount)
{
	resurrected += amount;
	vstd::amax(resurrected, 0);
}

int64_t CHealth::available() const
{
	return static_cast<int64_t>(firstHPleft) + owner->unitMaxHealth() * fullUnits;
}

int64_t CHealth::total() const
{
	return static_cast<int64_t>(owner->unitMaxHealth()) * owner->unitBaseAmount();
}

void CHealth::damage(int32_t & amount)
{
	const int32_t oldCount = getCount();

	const bool withKills = amount >= firstHPleft;

	if(withKills)
	{
		int64_t totalHealth = available();
		if(amount > totalHealth)
			amount = totalHealth;
		totalHealth -= amount;
		if(totalHealth <= 0)
		{
			fullUnits = 0;
			firstHPleft = 0;
		}
		else
		{
			setFromTotal(totalHealth);
		}
	}
	else
	{
		firstHPleft -= amount;
	}

	addResurrected(getCount() - oldCount);
}

void CHealth::heal(int32_t & amount, EHealLevel level, EHealPower power)
{
	const int32_t unitHealth = owner->unitMaxHealth();
	const int32_t oldCount = getCount();

	int32_t maxHeal = std::numeric_limits<int32_t>::max();

	switch(level)
	{
	case EHealLevel::HEAL:
		maxHeal = std::max(0, unitHealth - firstHPleft);
		break;
	case EHealLevel::RESURRECT:
		maxHeal = total() - available();
		break;
	default:
		assert(level == EHealLevel::OVERHEAL);
		break;
	}

	vstd::amax(maxHeal, 0);
	vstd::abetween(amount, 0, maxHeal);

	if(amount == 0)
		return;

	int64_t availableHealth = available();

	availableHealth	+= amount;
	setFromTotal(availableHealth);

	if(power == EHealPower::ONE_BATTLE)
		addResurrected(getCount() - oldCount);
	else
		assert(power == EHealPower::PERMANENT);
}

void CHealth::setFromTotal(const int64_t totalHealth)
{
	const int32_t unitHealth = owner->unitMaxHealth();
	firstHPleft = totalHealth % unitHealth;
	fullUnits = totalHealth / unitHealth;

	if(firstHPleft == 0 && fullUnits > 1)
	{
		firstHPleft = unitHealth;
		fullUnits -= 1;
	}
}

void CHealth::reset()
{
	fullUnits = 0;
	firstHPleft = 0;
	resurrected = 0;
}

int32_t CHealth::getCount() const
{
	return fullUnits + (firstHPleft > 0 ? 1 : 0);
}

int32_t CHealth::getFirstHPleft() const
{
	return firstHPleft;
}

int32_t CHealth::getResurrected() const
{
	return resurrected;
}

void CHealth::fromInfo(const CHealthInfo & info)
{
	firstHPleft = info.firstHPleft;
	fullUnits = info.fullUnits;
	resurrected = info.resurrected;
}

void CHealth::toInfo(CHealthInfo & info) const
{
	info.firstHPleft = firstHPleft;
	info.fullUnits = fullUnits;
	info.resurrected = resurrected;
}

void CHealth::takeResurrected()
{
	if(resurrected != 0)
	{
		int64_t totalHealth = available();

		totalHealth -= resurrected * owner->unitMaxHealth();
		vstd::amax(totalHealth, 0);
		setFromTotal(totalHealth);
		resurrected = 0;
	}
}

///CStack
CStack::CStack(const CStackInstance * Base, PlayerColor O, int I, ui8 Side, SlotID S):
	base(Base), ID(I), owner(O), slot(S), side(Side),
	counterAttacks(this), shots(this), casts(this), health(this), cloneID(-1),
	position()
{
	assert(base);
	type = base->type;
	baseAmount = base->count;
	health.init(); //???
	setNodeType(STACK_BATTLE);
}

CStack::CStack():
	counterAttacks(this), shots(this), casts(this), health(this)
{
	init();
	setNodeType(STACK_BATTLE);
}

CStack::CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S):
	base(nullptr), ID(I), owner(O), slot(S), side(Side),
	counterAttacks(this), shots(this), casts(this), health(this), cloneID(-1),
	position()
{
	type = stack->type;
	baseAmount = stack->count;
	health.init(); //???
	setNodeType(STACK_BATTLE);
}

int32_t CStack::getKilled() const
{
	int32_t res = baseAmount - health.getCount() + health.getResurrected();
	vstd::amax(res, 0);
	return res;
}

int32_t CStack::getCount() const
{
	return health.getCount();
}

int32_t CStack::getFirstHPleft() const
{
	return health.getFirstHPleft();
}

const CCreature * CStack::getCreature() const
{
	return type;
}

void CStack::init()
{
	base = nullptr;
	type = nullptr;
	ID = -1;
	baseAmount = -1;
	owner = PlayerColor::NEUTRAL;
	slot = SlotID(255);
	side = 1;
	position = BattleHex();
	cloneID = -1;
}

void CStack::localInit(BattleInfo * battleInfo)
{
	battle = battleInfo;
	cloneID = -1;
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

	shots.reset();
	counterAttacks.reset();
	casts.reset();
	health.init();
}

ui32 CStack::level() const
{
	if(base)
		return base->getLevel(); //creatture or commander
	else
		return std::max(1, (int)getCreature()->level); //war machine, clone etc
}

si32 CStack::magicResistance() const
{
	si32 magicResistance;
	if(base)  //TODO: make war machines receive aura of magic resistance
	{
		magicResistance = base->magicResistance();
		int auraBonus = 0;
		for(const CStack * stack : base->armyObj->battle-> batteAdjacentCreatures(this))
		{
			if(stack->owner == owner)
			{
				vstd::amax(auraBonus, stack->valOfBonuses(Bonus::SPELL_RESISTANCE_AURA)); //max value
			}
		}
		magicResistance += auraBonus;
		vstd::amin(magicResistance, 100);
	}
	else
		magicResistance = type->magicResistance();
	return magicResistance;
}

bool CStack::willMove(int turn /*= 0*/) const
{
	return (turn ? true : !vstd::contains(state, EBattleStackState::DEFENDING))
		   && !moved(turn)
		   && canMove(turn);
}

bool CStack::canMove(int turn /*= 0*/) const
{
	return alive()
		   && !hasBonus(Selector::type(Bonus::NOT_ACTIVE).And(Selector::turns(turn))); //eg. Ammo Cart or blinded creature
}

bool CStack::canCast() const
{
	return casts.canUse(1);//do not check specific cast abilities here
}

bool CStack::isCaster() const
{
	return casts.total() > 0;//do not check specific cast abilities here
}

bool CStack::canShoot() const
{
	return shots.canUse(1) && hasBonusOfType(Bonus::SHOOTER);
}

bool CStack::isShooter() const
{
	return shots.total() > 0 && hasBonusOfType(Bonus::SHOOTER);
}

bool CStack::moved(int turn /*= 0*/) const
{
	if(!turn)
		return vstd::contains(state, EBattleStackState::MOVED);
	else
		return false;
}

bool CStack::waited(int turn /*= 0*/) const
{
	if(!turn)
		return vstd::contains(state, EBattleStackState::WAITING);
	else
		return false;
}

bool CStack::doubleWide() const
{
	return getCreature()->doubleWide;
}

BattleHex CStack::occupiedHex() const
{
	return occupiedHex(position);
}

BattleHex CStack::occupiedHex(BattleHex assumedPos) const
{
	if(doubleWide())
	{
		if(side == BattleSide::ATTACKER)
			return assumedPos - 1;
		else
			return assumedPos + 1;
	}
	else
	{
		return BattleHex::INVALID;
	}
}

std::vector<BattleHex> CStack::getHexes() const
{
	return getHexes(position);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos) const
{
	return getHexes(assumedPos, doubleWide(), side);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos, bool twoHex, ui8 side)
{
	std::vector<BattleHex> hexes;
	hexes.push_back(assumedPos);

	if(twoHex)
	{
		if(side == BattleSide::ATTACKER)
			hexes.push_back(assumedPos - 1);
		else
			hexes.push_back(assumedPos + 1);
	}

	return hexes;
}

bool CStack::coversPos(BattleHex pos) const
{
	return vstd::contains(getHexes(), pos);
}

std::vector<BattleHex> CStack::getSurroundingHexes(BattleHex attackerPos) const
{
	BattleHex hex = (attackerPos != BattleHex::INVALID) ? attackerPos : position; //use hypothetical position
	std::vector<BattleHex> hexes;
	if(doubleWide())
	{
		const int WN = GameConstants::BFIELD_WIDTH;
		if(side == BattleSide::ATTACKER)
		{
			//position is equal to front hex
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN + 2 : WN + 1), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN + 1 : WN), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN : WN - 1), hexes);
			BattleHex::checkAndPush(hex - 2, hexes);
			BattleHex::checkAndPush(hex + 1, hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN - 2 : WN - 1), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN - 1 : WN), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN : WN + 1), hexes);
		}
		else
		{
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN + 1 : WN), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN : WN - 1), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN - 1 : WN - 2), hexes);
			BattleHex::checkAndPush(hex + 2, hexes);
			BattleHex::checkAndPush(hex - 1, hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN - 1 : WN), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN : WN + 1), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN + 1 : WN + 2), hexes);
		}
		return hexes;
	}
	else
	{
		return hex.neighbouringTiles();
	}
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
	CSelector selector = Selector::sourceType(Bonus::SPELL_EFFECT)
						 .And(CSelector([](const Bonus * b)->bool
	{
		return b->type != Bonus::NONE;
	}));

	TBonusListPtr spellEffects = getBonuses(selector, Selector::all, cachingStr.str());
	for(const std::shared_ptr<Bonus> it : *spellEffects)
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
	oss << "Battle stack [" << ID << "]: " << health.getCount() << " creatures of ";
	if(type)
		oss << type->namePl;
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << slot;
	if(base && base->armyObj)
		oss << " of armyobj=" << base->armyObj->id.getNum();
	return oss.str();
}

CHealth CStack::healthAfterAttacked(int32_t & damage) const
{
	return healthAfterAttacked(damage, health);
}

CHealth CStack::healthAfterAttacked(int32_t & damage, const CHealth & customHealth) const
{
	CHealth res = customHealth;

	if(isClone())
	{
		// block ability should not kill clone (0 damage)
		if(damage > 0)
		{
			damage = 1;//??? what should be actual damage against clone?
			res.reset();
		}
	}
	else
	{
		res.damage(damage);
	}

	return res;
}

CHealth CStack::healthAfterHealed(int32_t & toHeal, EHealLevel level, EHealPower power) const
{
	CHealth res = health;

	if(level == EHealLevel::HEAL && power == EHealPower::ONE_BATTLE)
		logGlobal->error("Heal for one battle does not make sense", nodeName(), toHeal);
	else if(isClone())
		logGlobal->error("Attempt to heal clone: %s for %d HP", nodeName(), toHeal);
	else
		res.heal(toHeal, level, power);

	return res;
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand) const
{
	prepareAttacked(bsa, rand, health);
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand, const CHealth & customHealth) const
{
	CHealth afterAttack = healthAfterAttacked(bsa.damageAmount, customHealth);

	bsa.killedAmount = customHealth.getCount() - afterAttack.getCount();
	afterAttack.toInfo(bsa.newHealth);
	bsa.newHealth.stackId = ID;
	bsa.newHealth.delta = -bsa.damageAmount;

	if(afterAttack.available() <= 0 && isClone())
	{
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
		return; // no rebirth I believe
	}

	if(afterAttack.available() <= 0) //stack killed
	{
		bsa.flags |= BattleStackAttacked::KILLED;

		int resurrectFactor = valOfBonuses(Bonus::REBIRTH);
		if(resurrectFactor > 0 && canCast()) //there must be casts left
		{
			int resurrectedStackCount = baseAmount * resurrectFactor / 100;

			// last stack has proportional chance to rebirth
			//FIXME: diff is always 0
			auto diff = baseAmount * resurrectFactor / 100.0 - resurrectedStackCount;
			if(diff > rand.nextDouble(0, 0.99))
			{
				resurrectedStackCount += 1;
			}

			if(hasBonusOfType(Bonus::REBIRTH, 1))
			{
				// resurrect at least one Sacred Phoenix
				vstd::amax(resurrectedStackCount, 1);
			}

			if(resurrectedStackCount > 0)
			{
				bsa.flags |= BattleStackAttacked::REBIRTH;
				//TODO: use StackHealedOrResurrected
				bsa.newHealth.firstHPleft = MaxHealth();
				bsa.newHealth.fullUnits = resurrectedStackCount - 1;
				bsa.newHealth.resurrected = 0; //TODO: add one-battle rebirth?
			}
		}
	}
}

bool CStack::isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos /*= BattleHex::INVALID*/, BattleHex defenderPos /*= BattleHex::INVALID*/)
{
	if(!attackerPos.isValid())
		attackerPos = attacker->position;
	if(!defenderPos.isValid())
		defenderPos = defender->position;

	return
		(BattleHex::mutualPosition(attackerPos, defenderPos) >= 0)//front <=> front
		|| (attacker->doubleWide()//back <=> front
			&& BattleHex::mutualPosition(attackerPos + (attacker->side == BattleSide::ATTACKER ? -1 : 1), defenderPos) >= 0)
		|| (defender->doubleWide()//front <=> back
			&& BattleHex::mutualPosition(attackerPos, defenderPos + (defender->side == BattleSide::ATTACKER ? -1 : 1)) >= 0)
		|| (defender->doubleWide() && attacker->doubleWide()//back <=> back
			&& BattleHex::mutualPosition(attackerPos + (attacker->side == BattleSide::ATTACKER ? -1 : 1), defenderPos + (defender->side == BattleSide::ATTACKER ? -1 : 1)) >= 0);

}

bool CStack::ableToRetaliate() const
{
	return alive()
		   && (counterAttacks.canUse() || hasBonusOfType(Bonus::UNLIMITED_RETALIATIONS))
		   && !hasBonusOfType(Bonus::SIEGE_WEAPON)
		   && !hasBonusOfType(Bonus::HYPNOTIZED)
		   && !hasBonusOfType(Bonus::NO_RETALIATION);
}

std::string CStack::getName() const
{
	return (health.getCount() == 1) ? type->nameSing : type->namePl; //War machines can't use base
}

bool CStack::isValidTarget(bool allowDead/* = false*/) const
{
	return (alive() || (allowDead && isDead())) && position.isValid() && !isTurret();
}

bool CStack::isDead() const
{
	return !alive() && !isGhost();
}

bool CStack::isClone() const
{
	return vstd::contains(state, EBattleStackState::CLONED);
}

bool CStack::isGhost() const
{
	return vstd::contains(state, EBattleStackState::GHOST);
}

bool CStack::isTurret() const
{
	return type->idNumber == CreatureID::ARROW_TOWERS;
}

bool CStack::canBeHealed() const
{
	return getFirstHPleft() < MaxHealth()
		   && isValidTarget()
		   && !hasBonusOfType(Bonus::SIEGE_WEAPON);
}

void CStack::makeGhost()
{
	state.erase(EBattleStackState::ALIVE);
	state.insert(EBattleStackState::GHOST_PENDING);
}

bool CStack::alive() const //determines if stack is alive
{
	return vstd::contains(state, EBattleStackState::ALIVE);
}

ui8 CStack::getSpellSchoolLevel(const CSpell * spell, int * outSelectedSchool) const
{
	int skill = valOfBonuses(Selector::typeSubtype(Bonus::SPELLCASTER, spell->id));
	vstd::abetween(skill, 0, 3);
	return skill;
}

ui32 CStack::getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const
{
	//stacks does not have sorcery-like bonuses (yet?)
	return base;
}

int CStack::getEffectLevel(const CSpell * spell) const
{
	return getSpellSchoolLevel(spell);
}

int CStack::getEffectPower(const CSpell * spell) const
{
	return valOfBonuses(Bonus::CREATURE_SPELL_POWER) * health.getCount() / 100;
}

int CStack::getEnchantPower(const CSpell * spell) const
{
	int res = valOfBonuses(Bonus::CREATURE_ENCHANT_POWER);
	if(res <= 0)
		res = 3;//default for creatures
	return res;
}

int CStack::getEffectValue(const CSpell * spell) const
{
	return valOfBonuses(Bonus::SPECIFIC_SPELL_POWER, spell->id.toEnum()) * health.getCount();
}

const PlayerColor CStack::getOwner() const
{
	return battle->battleGetOwner(this);
}

void CStack::getCasterName(MetaString & text) const
{
	//always plural name in case of spell cast.
	addNameReplacement(text, true);
}

void CStack::getCastDescription(const CSpell * spell, const std::vector<const CStack *> & attacked, MetaString & text) const
{
	text.addTxt(MetaString::GENERAL_TXT, 565);//The %s casts %s
	//todo: use text 566 for single creature
	getCasterName(text);
	text.addReplacement(MetaString::SPELL_NAME, spell->id.toEnum());
}

int32_t CStack::unitMaxHealth() const
{
	return MaxHealth();
}

int32_t CStack::unitBaseAmount() const
{
	return baseAmount;
}

void CStack::addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		serial = VLC->generaltexth->pluralText(serial, health.getCount());
	else if(plural)
		serial = VLC->generaltexth->pluralText(serial, 2);
	else
		serial = VLC->generaltexth->pluralText(serial, 1);

	text.addTxt(type, serial);
}

void CStack::addNameReplacement(MetaString & text, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		text.addCreReplacement(type->idNumber, health.getCount());
	else if(plural)
		text.addReplacement(MetaString::CRE_PL_NAMES, type->idNumber.num);
	else
		text.addReplacement(MetaString::CRE_SING_NAMES, type->idNumber.num);
}

std::string CStack::formatGeneralMessage(const int32_t baseTextId) const
{
	const int32_t textId = VLC->generaltexth->pluralText(baseTextId, health.getCount());

	MetaString text;
	text.addTxt(MetaString::GENERAL_TXT, textId);
	text.addCreReplacement(type->idNumber, health.getCount());

	return text.toString();
}

void CStack::setHealth(const CHealthInfo & value)
{
	health.reset();
	health.fromInfo(value);
}

void CStack::setHealth(const CHealth & value)
{
	health = value;
}

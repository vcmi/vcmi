/*
 * CUnitState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CUnitState.h"

#include <vcmi/spells/Spell.h>

#include "../CCreatureHandler.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{
///CAmmo
CAmmo::CAmmo(const battle::Unit * Owner, CSelector totalSelector):
	used(0),
	owner(Owner),
	totalProxy(Owner, totalSelector)
{
	reset();
}

CAmmo & CAmmo::operator= (const CAmmo & other)
{
	used = other.used;
	return *this;
}

int32_t CAmmo::available() const
{
	return total() - used;
}

bool CAmmo::canUse(int32_t amount) const
{
	return (available() - amount >= 0) || !isLimited();
}

bool CAmmo::isLimited() const
{
	return true;
}

void CAmmo::reset()
{
	used = 0;
}

int32_t CAmmo::total() const
{
	return totalProxy.getValue();
}

void CAmmo::use(int32_t amount)
{
	if(!isLimited())
		return;

	if(available() - amount < 0)
	{
		logGlobal->error("Stack ammo overuse. total: %d, used: %d, requested: %d", total(), used, amount);
		used += available();
	}
	else
		used += amount;
}

void CAmmo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("used", used, 0);
}

///CShots
CShots::CShots(const battle::Unit * Owner)
	: CAmmo(Owner, Selector::type()(BonusType::SHOTS)),
	shooter(Owner, Selector::type()(BonusType::SHOOTER))
{
}

bool CShots::isLimited() const
{
	return !shooter.hasBonus() || !env->unitHasAmmoCart(owner);
}

void CShots::setEnv(const IUnitEnvironment * env_)
{
	env = env_;
}

int32_t CShots::total() const
{
	if(shooter.hasBonus())
		return CAmmo::total();
	else
		return 0;
}

///CCasts
CCasts::CCasts(const battle::Unit * Owner):
	CAmmo(Owner, Selector::type()(BonusType::CASTS))
{
}

///CRetaliations
CRetaliations::CRetaliations(const battle::Unit * Owner)
	: CAmmo(Owner, Selector::type()(BonusType::ADDITIONAL_RETALIATION)),
	totalCache(0),
	noRetaliation(Owner, Selector::type()(BonusType::SIEGE_WEAPON).Or(Selector::type()(BonusType::NO_RETALIATION))),
	unlimited(Owner, Selector::type()(BonusType::UNLIMITED_RETALIATIONS))
{
}

bool CRetaliations::isLimited() const
{
	return !unlimited.hasBonus() || noRetaliation.hasBonus();
}

int32_t CRetaliations::total() const
{
	if(noRetaliation.hasBonus())
		return 0;

	//after dispel bonus should remain during current round
	int32_t val = 1 + totalProxy.getValue();
	vstd::amax(totalCache, val);
	return totalCache;
}

void CRetaliations::reset()
{
	CAmmo::reset();
	totalCache = 0;
}

void CRetaliations::serializeJson(JsonSerializeFormat & handler)
{
	CAmmo::serializeJson(handler);
	//we may be serialized in the middle of turn
	handler.serializeInt("totalCache", totalCache, 0);
}

///CHealth
CHealth::CHealth(const battle::Unit * Owner):
	owner(Owner)
{
	reset();
}

CHealth & CHealth::operator=(const CHealth & other)
{
	//do not change owner
	firstHPleft = other.firstHPleft;
	fullUnits = other.fullUnits;
	resurrected = other.resurrected;
	return *this;
}

void CHealth::init()
{
	reset();
	fullUnits = owner->unitBaseAmount() > 1 ? owner->unitBaseAmount() - 1 : 0;
	firstHPleft = owner->unitBaseAmount() > 0 ? owner->getMaxHealth() : 0;
}

void CHealth::addResurrected(int32_t amount)
{
	resurrected += amount;
	vstd::amax(resurrected, 0);
}

int64_t CHealth::available() const
{
	return static_cast<int64_t>(firstHPleft) + owner->getMaxHealth() * fullUnits;
}

int64_t CHealth::total() const
{
	return static_cast<int64_t>(owner->getMaxHealth()) * owner->unitBaseAmount();
}

void CHealth::damage(int64_t & amount)
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
		firstHPleft -= static_cast<int32_t>(amount);
	}

	addResurrected(getCount() - oldCount);
}

HealInfo CHealth::heal(int64_t & amount, EHealLevel level, EHealPower power)
{
	const int32_t unitHealth = owner->getMaxHealth();
	const int32_t oldCount = getCount();

	int64_t maxHeal = std::numeric_limits<int64_t>::max();

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
	vstd::abetween(amount, int64_t(0), maxHeal);

	if(amount == 0)
		return {};

	int64_t availableHealth = available();

	availableHealth	+= amount;
	setFromTotal(availableHealth);

	if(power == EHealPower::ONE_BATTLE)
		addResurrected(getCount() - oldCount);
	else
		assert(power == EHealPower::PERMANENT);

	return HealInfo(amount, getCount() - oldCount);
}

void CHealth::setFromTotal(const int64_t totalHealth)
{
	const int32_t unitHealth = owner->getMaxHealth();
	firstHPleft = totalHealth % unitHealth;
	fullUnits = static_cast<int32_t>(totalHealth / unitHealth);

	if(firstHPleft == 0 && fullUnits >= 1)
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

void CHealth::takeResurrected()
{
	if(resurrected != 0)
	{
		int64_t totalHealth = available();

		totalHealth -= resurrected * owner->getMaxHealth();
		vstd::amax(totalHealth, 0);
		setFromTotal(totalHealth);
		resurrected = 0;
	}
}

void CHealth::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("firstHPleft", firstHPleft, 0);
	handler.serializeInt("fullUnits", fullUnits, 0);
	handler.serializeInt("resurrected", resurrected, 0);
}

///CUnitState
CUnitState::CUnitState():
	env(nullptr),
	cloned(false),
	defending(false),
	defendingAnim(false),
	drainedMana(false),
	fear(false),
	hadMorale(false),
	castSpellThisTurn(false),
	ghost(false),
	ghostPending(false),
	movedThisRound(false),
	summoned(false),
	waiting(false),
	waitedThisTurn(false),
	casts(this),
	counterAttacks(this),
	health(this),
	shots(this),
	stackSpeedPerTurn(this, Selector::type()(BonusType::STACKS_SPEED), BonusCacheMode::VALUE),
	immobilizedPerTurn(this, Selector::type()(BonusType::SIEGE_WEAPON).Or(Selector::type()(BonusType::BIND_EFFECT)), BonusCacheMode::PRESENCE),
	bonusCache(this),
	cloneID(-1)
{

}

CUnitState & CUnitState::operator=(const CUnitState & other)
{
	//do not change unit and bonus info

	cloned = other.cloned;
	defending = other.defending;
	defendingAnim = other.defendingAnim;
	drainedMana = other.drainedMana;
	fear = other.fear;
	hadMorale = other.hadMorale;
	castSpellThisTurn = other.castSpellThisTurn;
	ghost = other.ghost;
	ghostPending = other.ghostPending;
	movedThisRound = other.movedThisRound;
	summoned = other.summoned;
	waiting = other.waiting;
	waitedThisTurn = other.waitedThisTurn;
	casts = other.casts;
	counterAttacks = other.counterAttacks;
	shots = other.shots;
	health = other.health;
	cloneID = other.cloneID;
	position = other.position;
	return *this;
}

int32_t CUnitState::creatureIndex() const
{
	return static_cast<int32_t>(creatureId().toEnum());
}

CreatureID CUnitState::creatureId() const
{
	return unitType()->getId();
}

int32_t CUnitState::creatureLevel() const
{
	return static_cast<int32_t>(unitType()->getLevel());
}

bool CUnitState::doubleWide() const
{
	return unitType()->isDoubleWide();
}

int32_t CUnitState::creatureCost() const
{
	return unitType()->getRecruitCost(EGameResID::GOLD);
}

int32_t CUnitState::creatureIconIndex() const
{
	return unitType()->getIconIndex();
}

FactionID CUnitState::getFactionID() const
{
	return unitType()->getFactionID();
}

int32_t CUnitState::getCasterUnitId() const
{
	return static_cast<int32_t>(unitId());
}

const CGHeroInstance * CUnitState::getHeroCaster() const
{
	return nullptr;
}

int32_t CUnitState::getSpellSchoolLevel(const spells::Spell * spell, SpellSchool * outSelectedSchool) const
{
	int32_t skill = valOfBonuses(Selector::typeSubtype(BonusType::SPELLCASTER, BonusSubtypeID(spell->getId())));
	vstd::abetween(skill, 0, 3);
	return skill;
}

int64_t CUnitState::getSpellBonus(const spells::Spell * spell, int64_t base, const Unit * affectedStack) const
{
	//does not have sorcery-like bonuses (yet?)
	return base;
}

int64_t CUnitState::getSpecificSpellBonus(const spells::Spell * spell, int64_t base) const
{
	return base;
}

int32_t CUnitState::getEffectLevel(const spells::Spell * spell) const
{
	return getSpellSchoolLevel(spell);
}

int32_t CUnitState::getEffectPower(const spells::Spell * spell) const
{
	return valOfBonuses(BonusType::CREATURE_SPELL_POWER) * getCount() / 100;
}

int32_t CUnitState::getEnchantPower(const spells::Spell * spell) const
{
	int32_t res = valOfBonuses(BonusType::CREATURE_ENCHANT_POWER);
	if(res <= 0)
		res = 3;//default for creatures
	return res;
}

int64_t CUnitState::getEffectValue(const spells::Spell * spell) const
{
	return static_cast<int64_t>(getCount()) * valOfBonuses(BonusType::SPECIFIC_SPELL_POWER, BonusSubtypeID(spell->getId()));
}

PlayerColor CUnitState::getCasterOwner() const
{
	return env->unitEffectiveOwner(this);
}

void CUnitState::getCasterName(MetaString & text) const
{
	//always plural name in case of spell cast.
	addNameReplacement(text, true);
}

void CUnitState::getCastDescription(const spells::Spell * spell, const battle::Units & attacked, MetaString & text) const
{
	text.appendLocalString(EMetaText::GENERAL_TXT, 565);//The %s casts %s
	//todo: use text 566 for single creature
	getCasterName(text);
	text.replaceName(spell->getId());
}

int32_t CUnitState::manaLimit() const
{
	return 0; //TODO: creature casting with mana mode (for mods)
}

bool CUnitState::ableToRetaliate() const
{
	return alive()
		&& counterAttacks.canUse();
}

bool CUnitState::alive() const
{
	return health.getCount() > 0;
}

bool CUnitState::isGhost() const
{
	return ghost;
}

bool CUnitState::isFrozen() const
{
	return hasBonus(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::STONE_GAZE))), Selector::all);
}

bool CUnitState::isValidTarget(bool allowDead) const
{
	return (alive() || (allowDead && isDead())) && getPosition().isValid() && !isTurret();
}

bool CUnitState::isClone() const
{
	return cloned;
}

bool CUnitState::hasClone() const
{
	return cloneID > 0;
}

bool CUnitState::canCast() const
{
	return casts.canUse(1) && !castSpellThisTurn;//do not check specific cast abilities here
}

bool CUnitState::isCaster() const
{
	return casts.total() > 0;//do not check specific cast abilities here
}

bool CUnitState::canShootBlocked() const
{
	return bonusCache.hasBonus(UnitBonusValuesProxy::HAS_FREE_SHOOTING);
}

bool CUnitState::canShoot() const
{
	return
		shots.canUse(1) &&
		bonusCache.getBonusValue(UnitBonusValuesProxy::FORGETFULL) <= 1; //advanced+ level
}

bool CUnitState::isShooter() const
{
	return shots.total() > 0;
}

int32_t CUnitState::getKilled() const
{
	int32_t res = unitBaseAmount() - health.getCount() + health.getResurrected();
	vstd::amax(res, 0);
	return res;
}

int32_t CUnitState::getCount() const
{
	return health.getCount();
}

int32_t CUnitState::getFirstHPleft() const
{
	return health.getFirstHPleft();
}

int64_t CUnitState::getAvailableHealth() const
{
	return health.available();
}

int64_t CUnitState::getTotalHealth() const
{
	return health.total();
}

uint32_t CUnitState::getMaxHealth() const
{
	return std::max(1, bonusCache.getBonusValue(UnitBonusValuesProxy::STACK_HEALTH));
}

BattleHex CUnitState::getPosition() const
{
	return position;
}

void CUnitState::setPosition(const BattleHex & hex)
{
	position = hex;
}

int32_t CUnitState::getInitiative(int turn) const
{
	return stackSpeedPerTurn.getValue(turn);
}

ui32 CUnitState::getMovementRange(int turn) const
{
	if (immobilizedPerTurn.getValue(0) != 0)
		return 0;

	return stackSpeedPerTurn.getValue(0);
}

ui32 CUnitState::getMovementRange() const
{
	return getMovementRange(0);
}

uint8_t CUnitState::getRangedFullDamageDistance() const
{
	if(!isShooter())
		return 0;

	// overwrite full ranged damage distance with the value set in Additional info field of LIMITED_SHOOTING_RANGE bonus
	if(hasBonusOfType(BonusType::LIMITED_SHOOTING_RANGE))
	{
		auto bonus = this->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
		if(bonus != nullptr && bonus->additionalInfo != CAddInfo::NONE)
			return bonus->additionalInfo[0];
	}

	if (hasBonusOfType(BonusType::NO_DISTANCE_PENALTY))
		return GameConstants::BATTLE_SHOOTING_RANGE_DISTANCE;

	return GameConstants::BATTLE_SHOOTING_PENALTY_DISTANCE;
}

uint8_t CUnitState::getShootingRangeDistance() const
{
	if(!isShooter())
		return 0;

	uint8_t shootingRangeDistance = GameConstants::BATTLE_SHOOTING_RANGE_DISTANCE;

	// overwrite full ranged damage distance with the value set in Additional info field of LIMITED_SHOOTING_RANGE bonus
	if(hasBonusOfType(BonusType::LIMITED_SHOOTING_RANGE))
	{
		auto bonus = this->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
		if(bonus != nullptr)
			shootingRangeDistance = bonus->val;
	}

	return shootingRangeDistance;
}

bool CUnitState::canMove(int turn) const
{
	if (!alive())
		return false;

	if (turn == 0)
		return !hasBonusOfType(BonusType::NOT_ACTIVE);

	std::string cachingStr = "type_NOT_ACTIVE_turns_" + std::to_string(turn);
	return !hasBonus(Selector::type()(BonusType::NOT_ACTIVE).And(Selector::turns(turn)), cachingStr); //eg. Ammo Cart or blinded creature
}

bool CUnitState::defended(int turn) const
{
	return !turn && defending;
}

bool CUnitState::moved(int turn) const
{
	if(!turn && !waiting)
		return movedThisRound;
	else
		return false;
}

bool CUnitState::willMove(int turn) const
{
	return (turn ? true : !defending)
		   && !moved(turn)
		   && canMove(turn);
}

bool CUnitState::waited(int turn) const
{
	if(!turn)
		return waiting;
	else
		return false;
}

BattlePhases::Type CUnitState::battleQueuePhase(int turn) const
{
	if(turn <= 0 && waited()) //consider waiting state only for ongoing round
	{
		if(hadMorale)
			return BattlePhases::WAIT_MORALE;
		else
			return BattlePhases::WAIT;
	}
	else if(creatureIndex() == CreatureID::CATAPULT || isTurret()) //catapult and turrets are first
	{
		return BattlePhases::SIEGE;
	}
	else
	{
		return BattlePhases::NORMAL;
	}
}

bool CUnitState::isHypnotized() const
{
	return bonusCache.hasBonus(UnitBonusValuesProxy::HYPNOTIZED);
}

bool CUnitState::isInvincible() const
{
	return bonusCache.hasBonus(UnitBonusValuesProxy::INVINCIBLE);
}

int CUnitState::getTotalAttacks(bool ranged) const
{
	return 1 + (ranged ?
		bonusCache.getBonusValue(UnitBonusValuesProxy::TOTAL_ATTACKS_RANGED):
		bonusCache.getBonusValue(UnitBonusValuesProxy::TOTAL_ATTACKS_MELEE));
}

int CUnitState::getMinDamage(bool ranged) const
{
	return ranged ?
		bonusCache.getBonusValue(UnitBonusValuesProxy::MIN_DAMAGE_RANGED):
		bonusCache.getBonusValue(UnitBonusValuesProxy::MIN_DAMAGE_MELEE);

}

int CUnitState::getMaxDamage(bool ranged) const
{
	return ranged ?
		bonusCache.getBonusValue(UnitBonusValuesProxy::MAX_DAMAGE_RANGED):
		bonusCache.getBonusValue(UnitBonusValuesProxy::MAX_DAMAGE_MELEE);
}

int CUnitState::getAttack(bool ranged) const
{
	int attack = ranged ?
		bonusCache.getBonusValue(UnitBonusValuesProxy::ATTACK_RANGED):
		bonusCache.getBonusValue(UnitBonusValuesProxy::ATTACK_MELEE);

	int frenzy = bonusCache.getBonusValue(UnitBonusValuesProxy::IN_FRENZY);
	if(frenzy != 0)
	{
		int defence = ranged ?
			bonusCache.getBonusValue(UnitBonusValuesProxy::DEFENCE_RANGED):
			bonusCache.getBonusValue(UnitBonusValuesProxy::DEFENCE_MELEE);

		int frenzyBonus = frenzy * defence / 100;
		attack += frenzyBonus;
	}

	vstd::amax(attack, 0);
	return attack;
}

int CUnitState::getDefense(bool ranged) const
{
	int frenzy = bonusCache.getBonusValue(UnitBonusValuesProxy::IN_FRENZY);

	if(frenzy != 0)
	{
		return 0;
	}
	else
	{
		int defence = ranged ?
						  bonusCache.getBonusValue(UnitBonusValuesProxy::DEFENCE_RANGED):
						  bonusCache.getBonusValue(UnitBonusValuesProxy::DEFENCE_MELEE);
		vstd::amax(defence, 0);
		return defence;
	}
}

std::shared_ptr<Unit> CUnitState::acquire() const
{
	auto ret = std::make_shared<CUnitStateDetached>(this, this);
	ret->localInit(env);
	*ret = *this;
	return ret;
}

std::shared_ptr<CUnitState> CUnitState::acquireState() const
{
	auto ret = std::make_shared<CUnitStateDetached>(this, this);
	ret->localInit(env);
	*ret = *this;
	return ret;
}

void CUnitState::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeBool("cloned", cloned);
	handler.serializeBool("defending", defending);
	handler.serializeBool("defendingAnim", defendingAnim);
	handler.serializeBool("drainedMana", drainedMana);
	handler.serializeBool("fear", fear);
	handler.serializeBool("hadMorale", hadMorale);
	handler.serializeBool("castSpellThisTurn", castSpellThisTurn);
	handler.serializeBool("ghost", ghost);
	handler.serializeBool("ghostPending", ghostPending);
	handler.serializeBool("moved", movedThisRound);
	handler.serializeBool("summoned", summoned);
	handler.serializeBool("waiting", waiting);
	handler.serializeBool("waitedThisTurn", waitedThisTurn);

	handler.serializeStruct("casts", casts);
	handler.serializeStruct("counterAttacks", counterAttacks);
	handler.serializeStruct("health", health);
	handler.serializeStruct("shots", shots);

	handler.serializeInt("cloneID", cloneID);

	si16 posValue = position.toInt();
	handler.serializeInt("position", posValue);
	position = posValue;
}

void CUnitState::localInit(const IUnitEnvironment * env_)
{
	env = env_;

	shots.setEnv(env);
	reset();
	health.init();
}

void CUnitState::reset()
{
	cloned = false;
	defending = false;
	defendingAnim = false;
	drainedMana = false;
	fear = false;
	hadMorale = false;
	castSpellThisTurn = false;
	ghost = false;
	ghostPending = false;
	movedThisRound = false;
	summoned = false;
	waiting = false;
	waitedThisTurn = false;

	casts.reset();
	counterAttacks.reset();
	health.reset();
	shots.reset();

	cloneID = -1;

	position = BattleHex::INVALID;
}

void CUnitState::save(JsonNode & data)
{
	//TODO: use instance resolver
	data.clear();
	JsonSerializer ser(nullptr, data);
	ser.serializeStruct("state", *this);
}

void CUnitState::load(const JsonNode & data)
{
	//TODO: use instance resolver
	reset();
	JsonDeserializer deser(nullptr, data);
	deser.serializeStruct("state", *this);
}

void CUnitState::damage(int64_t & amount)
{
	if(cloned)
	{
		// block ability should not kill clone (0 damage)
		if(amount > 0)
		{
			amount = 0;
			health.reset();
		}
	}
	else
	{
		health.damage(amount);
	}

	bool disintegrate = hasBonusOfType(BonusType::DISINTEGRATE);
	if(health.available() <= 0 && (cloned || summoned || disintegrate))
		ghostPending = true;
}

HealInfo CUnitState::heal(int64_t & amount, EHealLevel level, EHealPower power)
{
	if(level == EHealLevel::HEAL && power == EHealPower::ONE_BATTLE)
		logGlobal->error("Heal for one battle does not make sense");
	else if(cloned)
		logGlobal->error("Attempt to heal clone");
	else
		return health.heal(amount, level, power);

	return {};
}

void CUnitState::afterAttack(bool ranged, bool counter)
{
	if(counter)
		counterAttacks.use();

	if(ranged)
		shots.use();
}

void CUnitState::afterNewRound()
{
	defending = false;
	waiting = false;
	waitedThisTurn = false;
	movedThisRound = false;
	hadMorale = false;
	castSpellThisTurn = false;
	fear = false;
	drainedMana = false;
	counterAttacks.reset();

	if(alive() && isClone() && !bonusCache.hasBonus(UnitBonusValuesProxy::CLONE_MARKER))
		makeGhost();
}

void CUnitState::afterGetsTurn(BattleUnitTurnReason reason)
{
	if(reason == BattleUnitTurnReason::MORALE)
	{
		hadMorale = true;
		castSpellThisTurn = false;
	}
}

void CUnitState::makeGhost()
{
	health.reset();
	ghostPending = true;
}

void CUnitState::onRemoved()
{
	health.reset();
	ghostPending = false;
	ghost = true;
}

CUnitStateDetached::CUnitStateDetached(const IUnitInfo * unit_, const IBonusBearer * bonus_):
	unit(unit_),
	bonus(bonus_)
{
}

TConstBonusListPtr CUnitStateDetached::getAllBonuses(const CSelector & selector, const CSelector & limit, const std::string & cachingStr) const
{
	return bonus->getAllBonuses(selector, limit, cachingStr);
}

int32_t CUnitStateDetached::getTreeVersion() const
{
	return bonus->getTreeVersion();
}

CUnitStateDetached & CUnitStateDetached::operator=(const CUnitState & other)
{
	CUnitState::operator=(other);
	return *this;
}

uint32_t CUnitStateDetached::unitId() const
{
	return unit->unitId();
}

BattleSide CUnitStateDetached::unitSide() const
{
	return unit->unitSide();
}

const CCreature * CUnitStateDetached::unitType() const
{
	return unit->unitType();
}

PlayerColor CUnitStateDetached::unitOwner() const
{
	return unit->unitOwner();
}

SlotID CUnitStateDetached::unitSlot() const
{
	return unit->unitSlot();
}

int32_t CUnitStateDetached::unitBaseAmount() const
{
	return unit->unitBaseAmount();
}

void CUnitStateDetached::spendMana(ServerCallback * server, const int spellCost) const
{
	if(spellCost != 1)
		logGlobal->warn("Unexpected spell cost %d for creature", spellCost);

	//this is evil, but
	//use of netpacks in detached state is an error
	//non const API is more evil for hero
	const_cast<CUnitStateDetached *>(this)->casts.use(spellCost);
}

}

VCMI_LIB_NAMESPACE_END

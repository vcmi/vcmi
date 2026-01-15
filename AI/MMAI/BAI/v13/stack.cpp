/*
 * stack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CCreatureHandler.h"
#include "GameLibrary.h"
#include "battle/IBattleInfoCallback.h"
#include "bonuses/BonusEnum.h"
#include "common.h"
#include "constants/EntityIdentifiers.h"

#include "BAI/v13/stack.h"
#include "schema/v13/constants.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13
{
namespace S13 = Schema::V13;
using SA = Schema::V13::StackAttribute;
using F1 = Schema::V13::StackFlag1;
using F2 = Schema::V13::StackFlag2;
using GA = Schema::V13::GlobalAttribute;
using CreatureValues = std::map<CreatureID, int>;

namespace
{
	int calculateSlot(const CStack * cstack)
	{
		int slot = cstack->unitSlot();
		if(slot >= 0 && slot < 7)
		{
			return slot;
		}
		else if(slot == SlotID::WAR_MACHINES_SLOT)
		{
			return S13::STACK_SLOT_WARMACHINES;
		}
		else
		{
			// "special" slot, e.g. summoned, commander, etc.
			return S13::STACK_SLOT_SPECIAL;
		}
	}

	char calculateAlias(int slot)
	{
		switch(slot)
		{
			case S13::STACK_SLOT_SPECIAL:
				return 'S';
				break;
			case S13::STACK_SLOT_WARMACHINES:
				return 'M';
				break;
			default:
				return static_cast<char>('0' + slot);
		}
	}

	int calculateValue(const CCreature * cr)
	{
		/*
		 * Formula:
		 * 10 * (A + B) * C * D1 * D2 * ... * Dn
		 *
		 * A = <offensive factor>
		 * B = <defensive factor>
		 * C = <speed factor>
		 * D* = <bonus factor>
		 */

		auto att = cr->getBaseAttack();
		auto def = cr->getBaseDefense();
		auto dmg = (cr->getBaseDamageMax() + cr->getBaseDamageMin()) / 2.0;
		auto hp = cr->getBaseHitPoints();
		auto spd = cr->getBaseSpeed();
		auto shooter = cr->hasBonusOfType(BonusType::SHOOTER);
		auto bonuses = cr->getAllBonuses(Selector::all);

		auto a = 3 * dmg * (1 + std::min(4.0, 0.05 * att));
		auto b = hp / (1 - std::min(0.7, 0.025 * def));
		auto c = spd ? std::log(spd * 2) : 0.5;
		auto d = shooter ? 1.5 : 1.0;

		for(const auto & bonus : *bonuses)
		{
			switch(bonus->type)
			{
				case BonusType::ADDITIONAL_ATTACK:
					d += (shooter ? 0.5 : 0.3);
					break;
				case BonusType::ADDITIONAL_RETALIATION:
					d += (bonus->val * 0.1);
					break;
				case BonusType::ATTACKS_ALL_ADJACENT:
					d += 0.2;
					break;
				case BonusType::BLOCKS_RETALIATION:
					d += 0.3;
					break;
				case BonusType::DEATH_STARE:
					d += (bonus->val * 0.02); // 10% = 0.2
					break;
				case BonusType::DOUBLE_DAMAGE_CHANCE:
					d += (bonus->val * 0.005); // 20% = 0.1
					break;
				case BonusType::ENEMY_DEFENCE_REDUCTION:
					d += (bonus->val * 0.0025); // 40% = 0.1
					break;
				case BonusType::FIRE_SHIELD:
					d += (bonus->val * 0.003); // 20% = 0.1
					break;
				case BonusType::FLYING:
					d += 0.1;
					break;
				case BonusType::LIFE_DRAIN:
					d += (bonus->val * 0.003); // 100% = 0.3
					break;
				case BonusType::NO_DISTANCE_PENALTY:
					d += 0.5;
					break;
				case BonusType::NO_MELEE_PENALTY:
					d += 0.1;
					break;
				case BonusType::THREE_HEADED_ATTACK:
					d += 0.05;
					break;
				case BonusType::TWO_HEX_ATTACK_BREATH:
					d += 0.1;
					break;
				case BonusType::UNLIMITED_RETALIATIONS:
					d += 0.2;
					break;
				case BonusType::SPELL_LIKE_ATTACK:
					switch(bonus->subtype.as<SpellID>())
					{
						case SpellID::DEATH_CLOUD:
							d += 0.2;
					}
					break;
				case BonusType::SPELL_AFTER_ATTACK:
					switch(bonus->subtype.as<SpellID>())
					{
						case SpellID::BLIND:
						case SpellID::STONE_GAZE:
						case SpellID::PARALYZE:
							d += (bonus->val * 0.01); // 20% = 0.2
							break;
						case SpellID::BIND:
							d += (bonus->val * 0.001); // 100% = 0.1
							break;
						case SpellID::WEAKNESS:
							d += (bonus->val * 0.001); // 100% = 0.1
							break;
						case SpellID::AGE:
							d += (bonus->val * 0.005); // 20% = 0.1
							break;
						case SpellID::CURSE:
							d += (bonus->val * 0.0025); // 20% = 0.05
					}
			}
		}

		// Multiply by 10 to reduce the integer rounding for weak units
		// (e.g. peasant 7.48 => 7 is a lot, 74.8 => 75 is OK)
		auto res = static_cast<int>(std::round(10 * (a + b) * c * d));

		if(MMAI_VERBOSE)
		{
			std::cout << "MMAI_VERBOSE: " << res << " " << cr->getId().toEntity(LIBRARY)->getJsonKey() << " (a=" << a << ", b=" << b << ", c=" << c
					  << ", d=" << d << ")\n";
		}

		return res;
	}

	CreatureValues InitCreatureValues()
	{
		CreatureValues values;

		for(const auto & creature : LIBRARY->creh->objects)
			if(creature)
				values.try_emplace(creature->getId(), calculateValue(creature.get()));

		return values;
	}
}

// static
int Stack::GetValue(const CCreature * creature)
{
	static const CreatureValues CREATURE_VALUES = InitCreatureValues();

	if(!creature)
		throw std::runtime_error("GetValue: nullptr given");

	const auto & it = CREATURE_VALUES.find(creature->getId());

	if(it == CREATURE_VALUES.end())
		throw std::runtime_error("GetValue: no value for creature with ID=" + std::to_string(creature->getId()));

	return it->second;
}

// static
std::pair<BitQueue, int> Stack::QBits(const CStack * cstack, const Queue & vec)
{
	BitQueue q;
	int pos = -1;
	if(vec.size() != S13::STACK_QUEUE_SIZE)
		throw std::runtime_error("Unexpected queue size: " + std::to_string(vec.size()));

	for(auto i = 0; i < vec.size(); ++i)
	{
		if(vec[i] == cstack->unitId())
		{
			q.set(i);
			if(pos < 0)
				pos = i;
		}
	}

	return {q, pos};
}

Stack::Stack(
	const CStack * cstack_,
	const Queue & q,
	const StatsContainer & statsContainer,
	const ReachabilityInfo & rinfo_,
	bool blocked,
	bool blocking,
	const DamageEstimation & estdmg
)
	: cstack(cstack_), rinfo(rinfo_)
{
	// XXX: NULL attrs are used only for non-existing stacks
	// => don't fill with null here (as opposed to attrs in Hex)

	const auto & oldgstats = statsContainer.oldgstats;
	const auto & gstats = statsContainer.gstats;
	const auto & stackStats = statsContainer.stackStats;

	int slot = calculateSlot(cstack);
	alias = calculateAlias(slot);

	// queue needs to be set first to determine if stack is active
	auto [qbits, pos] = QBits(cstack, q);
	qposFirst = pos; // for comparing two positions

	processBonuses();

	if(cstack->willMove())
	{
		setflag(F1::WILL_ACT);
		// XXX: do NOT use cstack->waited()
		//      (it returns cstack->waiting, which becomes false after acting)
		if(!cstack->waitedThisTurn)
			setflag(F1::CAN_WAIT);
	}

	if(cstack->ableToRetaliate())
		setflag(F1::CAN_RETALIATE);

	if(blocked)
		setflag(F1::BLOCKED);

	if(blocking)
		setflag(F1::BLOCKING);

	if(cstack->occupiedHex().isAvailable())
		setflag(F1::IS_WIDE);

	// std::bitset's public semantics are architecture-independent.
	// operator<< prints bits from MSB to LSB,
	// e.g. std::bitset<8>(1) prints 00000001
	// b[i] indexes from the least-significant bit: b[0] == 1, b[1..7] == 0
	if(qbits.test(0))
		setflag(F1::IS_ACTIVE);

	shots = cstack->shots.available();

	auto valueOne = GetValue(cstack->unitType());

	// Force a higher value clones (we want extra focus on them)
	// Force a lower value for summons (we don't care about them, they are not permanent)
	if(cstack->isClone())
		valueOne *= 5;
	else if(cstack->unitSlot() == SlotID::SUMMONED_SLOT_PLACEHOLDER)
		valueOne *= 0.2;

	auto permille = [](int v1, int v2)
	{
		// LL (long long) ensures long is 64-bit even on 32-bit systems
		return static_cast<int>((1000LL * v1) / v2);
	};

	auto bf_valueNow = gstats->attr(GA::BFIELD_VALUE_NOW_ABS);
	auto bf_valuePrev = oldgstats->attr(GA::BFIELD_VALUE_NOW_ABS);
	auto bf_valueStart = gstats->attr(GA::BFIELD_VALUE_START_ABS);
	auto bf_hpPrev = oldgstats->attr(GA::BFIELD_HP_NOW_ABS);
	auto bf_hpStart = gstats->attr(GA::BFIELD_HP_START_ABS);
	auto value = valueOne * cstack->getCount();

	setattr(SA::SIDE, EI(cstack->unitSide()));
	setattr(SA::SLOT, slot);
	setattr(SA::QUANTITY, cstack->getCount());
	setattr(SA::ATTACK, cstack->getAttack(shots > 0));
	setattr(SA::DEFENSE, cstack->getDefense(false));
	setattr(SA::SHOTS, shots);
	setattr(SA::DMG_MIN, cstack->getMinDamage(shots > 0));
	setattr(SA::DMG_MAX, cstack->getMaxDamage(shots > 0));
	setattr(SA::HP, cstack->getMaxHealth());
	setattr(SA::HP_LEFT, cstack->getFirstHPleft());
	setattr(SA::SPEED, cstack->getMovementRange());
	setattr(SA::QUEUE, qbits.to_ulong());
	setattr(SA::VALUE_ONE, valueOne);
	setattr(SA::VALUE_REL, permille(value, bf_valueNow));
	setattr(SA::VALUE_REL0, permille(value, bf_valueStart));
	setattr(SA::VALUE_KILLED_REL, permille(stackStats.valueKilledNow, bf_valuePrev));
	setattr(SA::VALUE_KILLED_ACC_REL0, permille(stackStats.valueKilledTotal, bf_valueStart));
	setattr(SA::VALUE_LOST_REL, permille(stackStats.valueLostNow, bf_valuePrev));
	setattr(SA::VALUE_LOST_ACC_REL0, permille(stackStats.valueLostTotal, bf_valueStart));
	setattr(SA::DMG_DEALT_REL, permille(stackStats.dmgDealtNow, bf_hpPrev));
	setattr(SA::DMG_DEALT_ACC_REL0, permille(stackStats.dmgDealtTotal, bf_hpStart));
	setattr(SA::DMG_RECEIVED_REL, permille(stackStats.dmgReceivedNow, bf_hpPrev));
	setattr(SA::DMG_RECEIVED_ACC_REL0, permille(stackStats.dmgReceivedTotal, bf_hpStart));

	// The attrs set above must match the total count -2 (which are the FLAGS1 and FLAGS2)
	static_assert(EI(SA::_count) == 23 + 2, "whistleblower in case attributes change");

	finalize();
}

void Stack::processBonuses()
{
	auto bonuses = cstack->getAllBonuses(Selector::all);

	// XXX: config/creatures/<faction>.json is misleading
	//      (for example, no creature has NO_MELEE_PENALTY bonus there)
	//      The source of truth is the original H3 data files
	//      (referred to as CRTRAITS.TXT file, see CCreatureHandler::loadLegacyData)
	for(const auto & bonus : *bonuses)
	{
		switch(bonus->type)
		{
			case BonusType::FLYING:
				setflag(F1::FLYING);
				break;
			case BonusType::SHOOTER:
				setflag(F1::SHOOTER);
				break;
			case BonusType::UNDEAD:
				setflag(F1::NON_LIVING);
				break;
			case BonusType::NON_LIVING:
				setflag(F1::NON_LIVING);
				break;
			case BonusType::SIEGE_WEAPON:
				setflag(F1::WAR_MACHINE);
				break;
			case BonusType::BLOCKS_RETALIATION:
				setflag(F1::BLOCKS_RETALIATION);
				break;
			case BonusType::NO_MELEE_PENALTY:
				setflag(F1::NO_MELEE_PENALTY);
				break;
			case BonusType::TWO_HEX_ATTACK_BREATH:
				setflag(F1::TWO_HEX_ATTACK_BREATH);
				break;
			case BonusType::ADDITIONAL_ATTACK:
				setflag(F1::ADDITIONAL_ATTACK);
				break;
			case BonusType::SPELL_AFTER_ATTACK:
				switch(bonus->subtype.as<SpellID>())
				{
					case SpellID::BLIND:
						setflag(F2::BLIND_ATTACK);
						break;
					case SpellID::PARALYZE:
						setflag(F2::BLIND_ATTACK);
						break;
					case SpellID::STONE_GAZE:
						setflag(F2::PETRIFY_ATTACK);
						break;
					case SpellID::BIND:
						setflag(F2::BIND_ATTACK);
						break;
					case SpellID::WEAKNESS:
						setflag(F2::WEAKNESS_ATTACK);
						break;
					case SpellID::DISPEL:
						setflag(F2::DISPEL_ATTACK);
						break;
					case SpellID::DISPEL_HELPFUL_SPELLS:
						setflag(F2::DISPEL_ATTACK);
						break;
					case SpellID::POISON:
						setflag(F2::POISON_ATTACK);
						break;
					case SpellID::CURSE:
						setflag(F2::CURSE_ATTACK);
						break;
					case SpellID::AGE:
						setflag(F2::AGE_ATTACK);
				}
				break;
			case BonusType::SPELL_LIKE_ATTACK:
				switch(bonus->subtype.as<SpellID>())
				{
					case SpellID::FIREBALL:
						setflag(F1::FIREBALL);
						break;
					case SpellID::DEATH_CLOUD:
						setflag(F1::DEATH_CLOUD);
				}
				break;
			case BonusType::THREE_HEADED_ATTACK:
				setflag(F1::THREE_HEADED_ATTACK);
				break;
			case BonusType::ATTACKS_ALL_ADJACENT:
				setflag(F1::ALL_AROUND_ATTACK);
				break;
			case BonusType::RETURN_AFTER_STRIKE:
				setflag(F1::RETURN_AFTER_STRIKE);
				break;
			case BonusType::ENEMY_DEFENCE_REDUCTION:
				setflag(F1::ENEMY_DEFENCE_REDUCTION);
				break;
			case BonusType::LIFE_DRAIN:
				setflag(F1::LIFE_DRAIN);
				break;
			case BonusType::DOUBLE_DAMAGE_CHANCE:
				setflag(F1::DOUBLE_DAMAGE_CHANCE);
				break;
			case BonusType::DEATH_STARE:
				setflag(F1::DEATH_STARE);
				break;
			case BonusType::NOT_ACTIVE:
				if(cstack->unitType()->getId() != CreatureID::AMMO_CART)
					setflag(F1::SLEEPING);
		}

		if(bonus->source == BonusSource::SPELL_EFFECT)
		{
			switch(bonus->sid.as<SpellID>())
			{
				case SpellID::AGE:
					setflag(F2::AGE);
					break;
				case SpellID::BIND:
					setflag(F2::BIND);
					break;
				case SpellID::BLIND:
					setflag(F2::BLIND);
					break;
				case SpellID::CURSE:
					setflag(F2::CURSE);
					break;
				case SpellID::PARALYZE:
					setflag(F2::BLIND);
					break;
				case SpellID::POISON:
					setflag(F2::POISON);
					break;
				case SpellID::STONE_GAZE:
					setflag(F2::PETRIFY);
					break;
				case SpellID::WEAKNESS:
					setflag(F2::WEAKNESS);
			}
		}
	}
}

const StackAttrs & Stack::getAttrs() const
{
	return attrs;
}

char Stack::getAlias() const
{
	return alias;
}

int Stack::getAttr(StackAttribute a) const
{
	return attr(a);
}

int Stack::getFlag(StackFlag1 sf) const
{
	return flag(sf);
}

int Stack::getFlag(StackFlag2 sf) const
{
	return flag(sf);
}

bool Stack::flag(StackFlag1 f) const
{
	return flags1.test(EI(f));
};

bool Stack::flag(StackFlag2 f) const
{
	return flags2.test(EI(f));
};

int Stack::attr(StackAttribute a) const
{
	return attrs.at(EI(a));
};

void Stack::setflag(StackFlag1 f)
{
	flags1.set(EI(f));
};

void Stack::setflag(StackFlag2 f)
{
	flags2.set(EI(f));
};

void Stack::setattr(StackAttribute a, int value)
{
	attrs.at(EI(a)) = value;
};

void Stack::addattr(StackAttribute a, int value)
{
	attrs.at(EI(a)) += value;
};

void Stack::finalize()
{
	setattr(SA::FLAGS1, flags1.to_ulong());
	setattr(SA::FLAGS2, flags2.to_ulong());
}
}

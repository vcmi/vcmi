#include "BAI/v15/graph/nodes/unit.h"
#include "BAI/v15/graph/util.h"

#include "AI/MMAI/common.h"
#include "GameLibrary.h"
#include "bonuses/BonusEnum.h"
#include "bonuses/Propagators.h"
#include "constants/EntityIdentifiers.h"
#include "schema/v15/constants.h"

// XXX: clangd warns these are unused, but they are in fact required
#include "bonuses/BonusParameters.h"
#include "spells/CSpellHandler.h"


namespace MMAI::BAI::V15::Graph::Nodes
{

namespace
{
	std::string CalculateAlias(const CStack & cstack)
	{
		int slot = cstack.unitSlot();

		if(slot == SlotID::WAR_MACHINES_SLOT)
			slot = S15::STACK_SLOT_WARMACHINES;
		else if(slot < 0 || slot >= 7)
			slot = S15::STACK_SLOT_SPECIAL;

		switch(slot)
		{
			case S15::STACK_SLOT_SPECIAL:
				return "S";
			case S15::STACK_SLOT_WARMACHINES:
				return "M";
			default:
				return std::to_string(slot);
		}
	}

	int CalculateValue(const CCreature * cr)
	{
		auto att = cr->getBaseAttack();
		auto def = cr->getBaseDefense();
		auto dmg = (cr->getBaseDamageMax() + cr->getBaseDamageMin()) / 2.0;
		auto hp = cr->getBaseHitPoints();
		auto spd = cr->getBaseSpeed();
		auto shooter = cr->hasBonusOfType(BonusType::SHOOTER);
		auto bonuses = cr->getAllBonuses(Selector::all);

		auto multihexAttackHexcount = [](const std::vector<int> & encodedPath)
		{
			// The bonus parameters vector contains encoded info about affected hexes.
			// The encoding is not known, but also not relevant:
			// we care only about the number of affected hexes and
			// whether they are adjacent or remote
			// (because remote hexes allow to hit "guarded" shooters)
			// The assumption about this hex encoding is:
			// "L"=1, "F"=2, "R"=3, "FL"=21, "FFF"=222, etc.
			// (exact values don't matter, but the number of digits does)
			int numAdjacentHexes = 0;
			int numDistantHexes = 0;
			for(int x : encodedPath)
				x < 10 ? ++numAdjacentHexes : ++numDistantHexes;

			return std::pair{numAdjacentHexes, numDistantHexes};
		};

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
					d += (bonus->val * 0.02);
					break;
				case BonusType::DOUBLE_DAMAGE_CHANCE:
					d += (bonus->val * 0.005);
					break;
				case BonusType::ENCHANTER:
					d += 0.5;
					break;
				case BonusType::ENEMY_ATTACK_REDUCTION:
				case BonusType::ENEMY_DEFENCE_REDUCTION:
					d += (bonus->val * 0.0025);
					break;
				case BonusType::FEROCITY:
					d += (bonus->val * 0.25);
					break;
				case BonusType::FIRE_SHIELD:
					d += (bonus->val * 0.003);
					break;
				case BonusType::FIRST_STRIKE:
					d += 0.3;
					break;
				case BonusType::FLYING:
					d += 0.1;
					break;
				case BonusType::LIFE_DRAIN:
					d += (bonus->val * 0.003);
					break;
				case BonusType::MULTIHEX_ENEMY_ATTACK:
				{
					const auto & [adj, dist] = multihexAttackHexcount(bonus->parameters->toVector());
					d += (adj * 0.03);
					d += (dist * 0.8);
				}
				break;
				case BonusType::MULTIHEX_UNIT_ATTACK:
				{
					const auto & [adj, dist] = multihexAttackHexcount(bonus->parameters->toVector());
					d += (adj * 0.05);
					d += (dist * 0.1);
				}
				break;
				case BonusType::NO_DISTANCE_PENALTY:
					d += 0.5;
					break;
				case BonusType::NO_MELEE_PENALTY:
					d += 0.1;
					break;
				case BonusType::RANGED_RETALIATION:
					d += 0.2;
					break;
				case BonusType::REVENGE:
				case BonusType::THREE_HEADED_ATTACK:
					d += 0.15; // deprecated by MULTIHEX_ENEMY_ATTACK
					break;
				case BonusType::TWO_HEX_ATTACK_BREATH:
					d += 0.1; // deprecated by MULTIHEX_UNIT_ATTACK
					break;
				case BonusType::UNLIMITED_RETALIATIONS:
					d += 0.2;
					break;
				case BonusType::SPELL_LIKE_ATTACK:
					if(bonus->subtype.as<SpellID>() == SpellID::DEATH_CLOUD)
						d += 0.2;
					break;
				case BonusType::SPELL_AFTER_ATTACK:
					switch(bonus->subtype.as<SpellID>())
					{
						case SpellID::BLIND:
						case SpellID::STONE_GAZE:
						case SpellID::PARALYZE:
							d += (bonus->val * 0.01);
							break;
						case SpellID::BIND:
						case SpellID::WEAKNESS:
							d += (bonus->val * 0.001);
							break;
						case SpellID::AGE:
							d += (bonus->val * 0.005);
							break;
						case SpellID::CURSE:
							d += (bonus->val * 0.0025);
							break;
						case SpellID::DISRUPTING_RAY:
							d += (bonus->val * 0.002);
							break;
						case SpellID::POISON:
							d += (bonus->val * 0.001);
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		}

		/*
		 * Some examples:
		 *
		 * Peasant=8            Gremlin=20          Imp=21             Pixie=24
		 * Medusa=260           OgreMage=270        Crusader=293       Monk=308
		 * BoneDragon=1425      Giant=1432          Hydra=1752         Devil=2344
		 * ArchDevil=4484       GoldDragon=4518     Archangel=4763     Titan=5341
		 * CrystalDragon=11347  RustDragon=12166    AzureDragon=17988
		 *
		 */
		auto res = static_cast<int>(std::round((a + b) * c * d));

		if(isMMAIVerbose())
		{
			std::cout << "MMAI_VERBOSE: " << res << " " << cr->getId().toEntity(LIBRARY)->getJsonKey() << " (a=" << a << ", b=" << b << ", c=" << c
					  << ", d=" << d << ")\n";
		}

		return res;
	}

	using CreatureValues = std::unordered_map<int, int>;
	CreatureValues InitCreatureValues()
	{
		CreatureValues values;

		for(const auto & creature : LIBRARY->creh->objects)
		{
			if(creature)
				values.try_emplace(creature->getIndex(), CalculateValue(creature.get()));
		}

		return values;
	}
}

// static
int Unit::GetValue(const CCreature * creature, bool isClone, bool isSummon)
{
	static const CreatureValues CREATURE_VALUES = InitCreatureValues();

	if(!creature)
		throw std::runtime_error("GetValue: nullptr given");

	const auto & it = CREATURE_VALUES.find(creature->getIndex());

	if(it == CREATURE_VALUES.end())
		throw std::runtime_error("GetValue: no value for creature with ID=" + std::to_string(creature->getIndex()));

	auto v = it->second;

	if(isClone)
		v *= 5;

	if(isSummon)
		v /= 5;

	return v;
}

Unit::Unit(const Args & args)
	: cstack(args.cstack)
	, alias(CalculateAlias(args.cstack))
	, isActive(args.isActive)
	, distances(args.distances)
	, isFlying(args.isFlying) // cache to prevent repeated bonus checks
	, speed(args.speed) // cache to prevent repeated bonus checks
	, valueOne(GetValue(cstack.unitType(), cstack.isClone(), cstack.unitSlot() == SlotID::SUMMONED_SLOT_PLACEHOLDER))
{
	guardflags.set(); // See note for attr()/setattr() in Unit.h

	auto value = valueOne * cstack.getCount();

	// damage modifiers that care if attack is ranged (e.g. archery skill)
	// would affect max and min proportionally which would have no effect
	// on uncertainty calculations
	auto dmgrange = cstack.getMaxDamage(false) - cstack.getMinDamage(false);
	auto k = std::min(cstack.getCount(), 10); // see BattleInfo::getActualDamage()
	auto dmgstd = std::sqrt((dmgrange * dmgrange) / (12.0 * k));
	auto dmgmean = (cstack.getMaxDamage(false) + cstack.getMinDamage(false)) / 2.0;
	auto dmgstdnorm = dmgstd / dmgmean; // relative uncertainty for the dmg dealt

	setattr(UA::VALUE_REL, permille(value, args.bfieldValue));
	setattr(UA::SHOTS, cstack.shots.available());
	setattr(UA::DMG_UNCERTAINTY, permille(dmgstdnorm, 1));
	setattr(UA::IS_ACTIVE, isActive);
	setattr(UA::IS_ENEMY, args.isEnemy);

	auto bonuses = cstack.getAllBonuses(Selector::all);

	for(const auto & bonus : *bonuses)
	{
		switch(bonus->type)
		{
			case BonusType::FLYING:
				setattr(UA::HAS_FLYING, 1);
				break;
			case BonusType::UNDEAD:
			case BonusType::NON_LIVING:
				setattr(UA::HAS_NON_LIVING, 1);
				break;
			case BonusType::SIEGE_WEAPON:
				setattr(UA::IS_WAR_MACHINE, 1);
				break;
			case BonusType::BLOCKS_RETALIATION:
				setattr(UA::HAS_BLOCKS_RETALIATION, 1);
				break;
			case BonusType::NO_MELEE_PENALTY:
				setattr(UA::HAS_NO_MELEE_PENALTY, 1);
				break;
			case BonusType::TWO_HEX_ATTACK_BREATH:
				setattr(UA::HAS_TWO_HEX_ATTACK_BREATH, 1);
				break;
			case BonusType::ADDITIONAL_ATTACK:
				setattr(UA::HAS_ADDITIONAL_ATTACK, 1);
				break;
			case BonusType::SPELL_AFTER_ATTACK:
				switch(bonus->subtype.as<SpellID>())
				{
					case SpellID::BLIND:
					case SpellID::PARALYZE:
						setattr(UA::HAS_BLIND_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::STONE_GAZE:
						setattr(UA::HAS_PETRIFY_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::BIND:
						setattr(UA::HAS_BIND_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::WEAKNESS:
						setattr(UA::HAS_WEAKNESS_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::DISPEL:
					case SpellID::DISPEL_HELPFUL_SPELLS:
						setattr(UA::HAS_DISPEL_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::POISON:
						setattr(UA::HAS_POISON_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::CURSE:
						setattr(UA::HAS_CURSE_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::AGE:
						setattr(UA::HAS_AGE_ATTACK, permille(bonus->val, 100));
						break;
					default:
						break;
				}
				break;
			case BonusType::SPELL_LIKE_ATTACK:
				switch(bonus->subtype.as<SpellID>())
				{
					case SpellID::FIREBALL:
						setattr(UA::HAS_FIREBALL, 1);
						break;
					case SpellID::DEATH_CLOUD:
						setattr(UA::HAS_DEATH_CLOUD, 1);
						break;
					default:
						break;
				}
				break;
			case BonusType::THREE_HEADED_ATTACK:
				setattr(UA::HAS_THREE_HEADED_ATTACK, 1);
				break;
			case BonusType::ATTACKS_ALL_ADJACENT:
				setattr(UA::HAS_ALL_AROUND_ATTACK, 1);
				break;
			case BonusType::RETURN_AFTER_STRIKE:
				setattr(UA::HAS_RETURN_AFTER_STRIKE, 1);
				break;
			case BonusType::LIFE_DRAIN:
				setattr(UA::HAS_LIFE_DRAIN, permille(bonus->val, 100));
				break;
			case BonusType::DOUBLE_DAMAGE_CHANCE:
				setattr(UA::HAS_DOUBLE_DAMAGE_CHANCE, permille(bonus->val, 100));
				break;
			case BonusType::FEARFUL:
				// Can't figure out how to properly check if this stack is inducing fear
				// In VCMI the FEARFUL bonus of azure dragons is applied ALL stacks
				// incl. the azure dragons have a second FEARFUL bonus that overrides
				// the value to 0 (i.e. "fearless" is FEARFUL with independentMin=0...)
				// This means it's not really possible to identify the source stack.
				// One workaround is to see check:
				//     propagator == BATTLE_WIDE
				//     && source == CREATURE_ABILITY
				//     && sid.as<CreatureID> == cstack.creatureID
				break;
			case BonusType::NOT_ACTIVE:
				if(cstack.unitType()->getId() != CreatureID::AMMO_CART)
					setattr(UA::IS_SLEEPING, 1);
				break;
			default:
				break;
		}

		if(bonus->source == BonusSource::SPELL_EFFECT)
		{
			switch(bonus->sid.as<SpellID>())
			{
				case SpellID::AGE:
					setattr(UA::HAS_AGE, bonus->turnsRemain);
					break;
				case SpellID::BIND:
					setattr(UA::HAS_BIND, bonus->turnsRemain);
					break;
				case SpellID::BLIND:
				case SpellID::PARALYZE:
					setattr(UA::HAS_BLIND, bonus->turnsRemain);
					break;
				case SpellID::CURSE:
					setattr(UA::HAS_CURSE, bonus->turnsRemain);
					break;
				case SpellID::POISON:
					setattr(UA::HAS_POISON, bonus->turnsRemain);
					break;
				case SpellID::STONE_GAZE:
					setattr(UA::HAS_PETRIFY, bonus->turnsRemain);
					break;
				case SpellID::WEAKNESS:
					setattr(UA::HAS_WEAKNESS, bonus->turnsRemain);
					break;
				default:
					break;
			}
		}
	}

	static_assert(EU(UA::_count) == 35, "whistleblower in case attributes change");
}

int Unit::attr(Attribute a) const
{
	return attrs.at(EU(a));
}

void Unit::setattr(Attribute a, int value)
{
	attrs.at(EU(a)) = value;
}

}

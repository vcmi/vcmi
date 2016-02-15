/*
 * ISpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ISpellMechanics.h"

#include "../BattleState.h"
#include "../NetPacks.h"

#include "CDefaultSpellMechanics.h"

#include "AdventureSpellMechanics.h"
#include "BattleSpellMechanics.h"
#include "CreatureSpellMechanics.h"

BattleSpellCastParameters::Destination::Destination(const CStack * destination):
	stackValue(destination),
	hexValue(destination->position)
{

}

BattleSpellCastParameters::Destination::Destination(const BattleHex & destination):
	stackValue(nullptr),
	hexValue(destination)
{

}

BattleSpellCastParameters::BattleSpellCastParameters(const BattleInfo * cb, const ISpellCaster * caster, const CSpell * spell)
	: cb(cb), caster(caster), casterColor(caster->getOwner()), casterSide(cb->whatSide(casterColor)),
	casterHero(nullptr),
	mode(ECastingMode::HERO_CASTING), casterStack(nullptr), selectedStack(nullptr),
	spellLvl(-1),  effectLevel(-1), effectPower(0), enchantPower(0), effectValue(0)
{
	casterStack = dynamic_cast<const CStack *>(caster);
	casterHero = dynamic_cast<const CGHeroInstance *>(caster);
	prepare(spell);
}

void BattleSpellCastParameters::aimToHex(const BattleHex& destination)
{
	destinations.push_back(Destination(destination));
}

void BattleSpellCastParameters::aimToStack(const CStack * destination)
{
	destinations.push_back(Destination(destination));
}


BattleHex BattleSpellCastParameters::getFirstDestinationHex() const
{
	return destinations.at(0).hexValue;
}

void BattleSpellCastParameters::prepare(const CSpell * spell)
{
	spellLvl = caster->getSpellSchoolLevel(spell);
	effectLevel = caster->getEffectLevel(spell);
	effectPower = caster->getEffectPower(spell);
	effectValue = caster->getEffectValue(spell);
	enchantPower = caster->getEnchantPower(spell);

	vstd::amax(spellLvl, 0);
	vstd::amax(effectLevel, 0);
	vstd::amax(enchantPower, 0);
	vstd::amax(enchantPower, 0);
	vstd::amax(effectValue, 0);
}

///ISpellMechanics
ISpellMechanics::ISpellMechanics(CSpell * s):
	owner(s)
{

}

ISpellMechanics * ISpellMechanics::createMechanics(CSpell * s)
{
	switch (s->id)
	{
	case SpellID::ANTI_MAGIC:
		return new AntimagicMechanics(s);
	case SpellID::ACID_BREATH_DAMAGE:
		return new AcidBreathDamageMechanics(s);
	case SpellID::CHAIN_LIGHTNING:
		return new ChainLightningMechanics(s);
	case SpellID::CLONE:
		return new CloneMechanics(s);
	case SpellID::CURE:
		return new CureMechanics(s);
	case SpellID::DEATH_STARE:
		return new DeathStareMechanics(s);
	case SpellID::DISPEL:
		return new DispellMechanics(s);
	case SpellID::DISPEL_HELPFUL_SPELLS:
		return new DispellHelpfulMechanics(s);
	case SpellID::EARTHQUAKE:
		return new EarthquakeMechanics(s);
	case SpellID::FIRE_WALL:
	case SpellID::FORCE_FIELD:
		return new WallMechanics(s);
	case SpellID::HYPNOTIZE:
		return new HypnotizeMechanics(s);
	case SpellID::LAND_MINE:
	case SpellID::QUICKSAND:
		return new ObstacleMechanics(s);
	case SpellID::REMOVE_OBSTACLE:
		return new RemoveObstacleMechanics(s);
	case SpellID::SACRIFICE:
		return new SacrificeMechanics(s);
	case SpellID::SUMMON_FIRE_ELEMENTAL:
		return new SummonMechanics(s, CreatureID::FIRE_ELEMENTAL);
	case SpellID::SUMMON_EARTH_ELEMENTAL:
		return new SummonMechanics(s, CreatureID::EARTH_ELEMENTAL);
	case SpellID::SUMMON_WATER_ELEMENTAL:
		return new SummonMechanics(s, CreatureID::WATER_ELEMENTAL);
	case SpellID::SUMMON_AIR_ELEMENTAL:
		return new SummonMechanics(s, CreatureID::AIR_ELEMENTAL);
	case SpellID::TELEPORT:
		return new TeleportMechanics(s);
	case SpellID::SUMMON_BOAT:
		return new SummonBoatMechanics(s);
	case SpellID::SCUTTLE_BOAT:
		return new ScuttleBoatMechanics(s);
	case SpellID::DIMENSION_DOOR:
		return new DimensionDoorMechanics(s);
	case SpellID::FLY:
	case SpellID::WATER_WALK:
	case SpellID::VISIONS:
	case SpellID::DISGUISE:
		return new DefaultSpellMechanics(s); //implemented using bonus system
	case SpellID::TOWN_PORTAL:
		return new TownPortalMechanics(s);
	case SpellID::VIEW_EARTH:
		return new ViewEarthMechanics(s);
	case SpellID::VIEW_AIR:
		return new ViewAirMechanics(s);
	default:
		if(s->isRisingSpell())
			return new SpecialRisingSpellMechanics(s);
		else
			return new DefaultSpellMechanics(s);
	}
}


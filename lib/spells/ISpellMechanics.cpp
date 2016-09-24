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

BattleSpellCastParameters::BattleSpellCastParameters(const BattleInfo * cb, const ISpellCaster * caster, const CSpell * spell_)
	: spell(spell_), cb(cb), caster(caster), casterColor(caster->getOwner()), casterSide(cb->whatSide(casterColor)),
	casterHero(nullptr),
	mode(ECastingMode::HERO_CASTING), casterStack(nullptr),
	spellLvl(0),  effectLevel(0), effectPower(0), enchantPower(0), effectValue(0)
{
	casterStack = dynamic_cast<const CStack *>(caster);
	casterHero = dynamic_cast<const CGHeroInstance *>(caster);

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

BattleSpellCastParameters::BattleSpellCastParameters(const BattleSpellCastParameters & orig, const ISpellCaster * caster)
	:spell(orig.spell), cb(orig.cb), caster(caster), casterColor(caster->getOwner()), casterSide(cb->whatSide(casterColor)),
	casterHero(nullptr), mode(ECastingMode::MAGIC_MIRROR), casterStack(nullptr),
	spellLvl(orig.spellLvl),  effectLevel(orig.effectLevel), effectPower(orig.effectPower), enchantPower(orig.enchantPower), effectValue(orig.effectValue)
{
	casterStack = dynamic_cast<const CStack *>(caster);
	casterHero = dynamic_cast<const CGHeroInstance *>(caster);
}

void BattleSpellCastParameters::aimToHex(const BattleHex& destination)
{
	destinations.push_back(Destination(destination));
}

void BattleSpellCastParameters::aimToStack(const CStack * destination)
{
	if(nullptr == destination)
		logGlobal->error("BattleSpellCastParameters::aimToStack invalid stack.");
	else
		destinations.push_back(Destination(destination));
}

void BattleSpellCastParameters::cast(const SpellCastEnvironment * env)
{
	spell->battleCast(env, *this);
}

BattleHex BattleSpellCastParameters::getFirstDestinationHex() const
{
	return destinations.at(0).hexValue;
}

int BattleSpellCastParameters::getEffectValue() const
{
	return (effectValue == 0) ? spell->calculateRawEffectValue(effectLevel, effectPower) : effectValue;
}

///ISpellMechanics
ISpellMechanics::ISpellMechanics(CSpell * s):
	owner(s)
{

}

std::unique_ptr<ISpellMechanics> ISpellMechanics::createMechanics(CSpell * s)
{
	switch (s->id)
	{
	case SpellID::ANTI_MAGIC:
		return make_unique<AntimagicMechanics>(s);
	case SpellID::ACID_BREATH_DAMAGE:
		return make_unique<AcidBreathDamageMechanics>(s);
	case SpellID::CHAIN_LIGHTNING:
		return make_unique<ChainLightningMechanics>(s);
	case SpellID::CLONE:
		return make_unique<CloneMechanics>(s);
	case SpellID::CURE:
		return make_unique<CureMechanics>(s);
	case SpellID::DEATH_STARE:
		return make_unique<DeathStareMechanics>(s);
	case SpellID::DISPEL:
		return make_unique<DispellMechanics>(s);
	case SpellID::DISPEL_HELPFUL_SPELLS:
		return make_unique<DispellHelpfulMechanics>(s);
	case SpellID::EARTHQUAKE:
		return make_unique<EarthquakeMechanics>(s);
	case SpellID::FIRE_WALL:
		return make_unique<FireWallMechanics>(s);
	case SpellID::FORCE_FIELD:
		return make_unique<ForceFieldMechanics>(s);
	case SpellID::HYPNOTIZE:
		return make_unique<HypnotizeMechanics>(s);
	case SpellID::LAND_MINE:
		return make_unique<LandMineMechanics>(s);
	case SpellID::QUICKSAND:
		return make_unique<QuicksandMechanics>(s);
	case SpellID::REMOVE_OBSTACLE:
		return make_unique<RemoveObstacleMechanics>(s);
	case SpellID::SACRIFICE:
		return make_unique<SacrificeMechanics>(s);
	case SpellID::SUMMON_FIRE_ELEMENTAL:
		return make_unique<SummonMechanics>(s, CreatureID::FIRE_ELEMENTAL);
	case SpellID::SUMMON_EARTH_ELEMENTAL:
		return make_unique<SummonMechanics>(s, CreatureID::EARTH_ELEMENTAL);
	case SpellID::SUMMON_WATER_ELEMENTAL:
		return make_unique<SummonMechanics>(s, CreatureID::WATER_ELEMENTAL);
	case SpellID::SUMMON_AIR_ELEMENTAL:
		return make_unique<SummonMechanics>(s, CreatureID::AIR_ELEMENTAL);
	case SpellID::TELEPORT:
		return make_unique<TeleportMechanics>(s);
	default:
		if(s->isRisingSpell())
			return make_unique<SpecialRisingSpellMechanics>(s);
		else
			return make_unique<DefaultSpellMechanics>(s);
	}
}

//IAdventureSpellMechanics
IAdventureSpellMechanics::IAdventureSpellMechanics(CSpell * s):
	owner(s)
{

}

std::unique_ptr<IAdventureSpellMechanics> IAdventureSpellMechanics::createMechanics(CSpell * s)
{
	switch (s->id)
	{
	case SpellID::SUMMON_BOAT:
		return make_unique<SummonBoatMechanics>(s);
	case SpellID::SCUTTLE_BOAT:
		return make_unique<ScuttleBoatMechanics>(s);
	case SpellID::DIMENSION_DOOR:
		return make_unique<DimensionDoorMechanics>(s);
	case SpellID::FLY:
	case SpellID::WATER_WALK:
	case SpellID::VISIONS:
	case SpellID::DISGUISE:
		return make_unique<AdventureSpellMechanics>(s); //implemented using bonus system
	case SpellID::TOWN_PORTAL:
		return make_unique<TownPortalMechanics>(s);
	case SpellID::VIEW_EARTH:
		return make_unique<ViewEarthMechanics>(s);
	case SpellID::VIEW_AIR:
		return make_unique<ViewAirMechanics>(s);
	default:
		return std::unique_ptr<IAdventureSpellMechanics>();
	}
}

/*
 * AbilityCaster.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AbilityCaster.h"

#include "../battle/Unit.h"

namespace spells
{

AbilityCaster::AbilityCaster(const battle::Unit * actualCaster_, int baseSpellLevel_)
	: actualCaster(actualCaster_),
	baseSpellLevel(baseSpellLevel_)
{
}

AbilityCaster::~AbilityCaster() = default;

ui8 AbilityCaster::getSpellSchoolLevel(const Spell * spell, int * outSelectedSchool) const
{
	int skill = baseSpellLevel;

	//if spell level is 0, it is not actually spell, but non magic ability
	//in ability cast spell school is ignored, but generic bonus is applicable

	if(spell->getLevel() > 0)
	{
		vstd::amax(skill, actualCaster->valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 0));
	}

	vstd::amax(skill, 0);
	vstd::amin(skill, 3);

	return static_cast<ui8>(skill); //todo: unify spell school level type
}

int AbilityCaster::getEffectLevel(const Spell * spell) const
{
	return getSpellSchoolLevel(spell);
}

int64_t AbilityCaster::getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const
{
	return actualCaster->getSpellBonus(spell, base, affectedStack);
}

int64_t AbilityCaster::getSpecificSpellBonus(const Spell * spell, int64_t base) const
{
	return actualCaster->getSpecificSpellBonus(spell, base);
}

int AbilityCaster::getEffectPower(const Spell * spell) const
{
	return actualCaster->getEffectPower(spell);
}

int AbilityCaster::getEnchantPower(const Spell * spell) const
{
	return actualCaster->getEnchantPower(spell);
}

int64_t AbilityCaster::getEffectValue(const Spell * spell) const
{
	return actualCaster->getEffectValue(spell);
}

const PlayerColor AbilityCaster::getOwner() const
{
	return actualCaster->getOwner();
}

void AbilityCaster::getCasterName(MetaString & text) const
{
	return actualCaster->getCasterName(text);
}

void AbilityCaster::getCastDescription(const Spell * spell, const std::vector<const battle::Unit*> & attacked, MetaString & text) const
{
	//do nothing
}

void AbilityCaster::spendMana(const PacketSender * server, const int spellCost) const
{
	//do nothing
}


} // namespace spells

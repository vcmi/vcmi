/*
 * ProxyCaster.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ProxyCaster.h"

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

ProxyCaster::ProxyCaster(const Caster * actualCaster_)
	: actualCaster(actualCaster_)
{

}

int32_t ProxyCaster::getCasterUnitId() const
{
	return actualCaster->getCasterUnitId();
}

int32_t ProxyCaster::getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool) const
{
	return actualCaster->getSpellSchoolLevel(spell, outSelectedSchool);
}

int32_t ProxyCaster::getEffectLevel(const Spell * spell) const
{
	return actualCaster->getEffectLevel(spell);
}

int64_t ProxyCaster::getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const
{
	return actualCaster->getSpellBonus(spell, base, affectedStack);
}

int64_t ProxyCaster::getSpecificSpellBonus(const Spell * spell, int64_t base) const
{
	return actualCaster->getSpecificSpellBonus(spell, base);
}

int32_t ProxyCaster::getEffectPower(const Spell * spell) const
{
	return actualCaster->getEffectPower(spell);
}

int32_t ProxyCaster::getEnchantPower(const Spell * spell) const
{
	return actualCaster->getEnchantPower(spell);
}

int64_t ProxyCaster::getEffectValue(const Spell * spell) const
{
	return actualCaster->getEffectValue(spell);
}

PlayerColor ProxyCaster::getCasterOwner() const
{
	return actualCaster->getCasterOwner();
}

void ProxyCaster::getCasterName(MetaString & text) const
{
	return actualCaster->getCasterName(text);
}

void ProxyCaster::getCastDescription(const Spell * spell, const std::vector<const battle::Unit*> & attacked, MetaString & text) const
{
	actualCaster->getCastDescription(spell, attacked, text);
}

void ProxyCaster::spendMana(ServerCallback * server, const int32_t spellCost) const
{
	actualCaster->spendMana(server, spellCost);
}

}

VCMI_LIB_NAMESPACE_END

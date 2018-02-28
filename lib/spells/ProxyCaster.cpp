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

namespace spells
{

ProxyCaster::ProxyCaster(const Caster * actualCaster_)
	: actualCaster(actualCaster_)
{

}

ProxyCaster::~ProxyCaster() = default;

ui8 ProxyCaster::getSpellSchoolLevel(const Spell * spell, int * outSelectedSchool) const
{
	return actualCaster->getSpellSchoolLevel(spell, outSelectedSchool);
}

int ProxyCaster::getEffectLevel(const Spell * spell) const
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

int ProxyCaster::getEffectPower(const Spell * spell) const
{
	return actualCaster->getEffectPower(spell);
}

int ProxyCaster::getEnchantPower(const Spell * spell) const
{
	return actualCaster->getEnchantPower(spell);
}

int64_t ProxyCaster::getEffectValue(const Spell * spell) const
{
	return actualCaster->getEffectValue(spell);
}

const PlayerColor ProxyCaster::getOwner() const
{
	return actualCaster->getOwner();
}

void ProxyCaster::getCasterName(MetaString & text) const
{
	return actualCaster->getCasterName(text);
}

void ProxyCaster::getCastDescription(const Spell * spell, const std::vector<const battle::Unit*> & attacked, MetaString & text) const
{
	actualCaster->getCastDescription(spell, attacked, text);
}

void ProxyCaster::spendMana(const PacketSender * server, const int spellCost) const
{
	actualCaster->spendMana(server, spellCost);
}

}

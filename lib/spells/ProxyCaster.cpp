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

#include <vcmi/spells/Spell.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

ProxyCaster::ProxyCaster(const Caster * actualCaster_)
	: actualCaster(actualCaster_)
{

}

ProxyCaster::~ProxyCaster() = default;

int32_t ProxyCaster::getCasterUnitId() const
{
	if(actualCaster)
		return actualCaster->getCasterUnitId();

	return -1;
}

int32_t ProxyCaster::getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool) const
{
	if(actualCaster)
		return actualCaster->getSpellSchoolLevel(spell, outSelectedSchool);

	return 0;
}

int32_t ProxyCaster::getEffectLevel(const Spell * spell) const
{
	if(actualCaster)
		return actualCaster->getEffectLevel(spell);

	return 0;
}

int64_t ProxyCaster::getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const
{
	if(actualCaster)
		return actualCaster->getSpellBonus(spell, base, affectedStack);

	return base;
}

int64_t ProxyCaster::getSpecificSpellBonus(const Spell * spell, int64_t base) const
{
	if(actualCaster)
		return actualCaster->getSpecificSpellBonus(spell, base);

	return base;
}

int32_t ProxyCaster::getEffectPower(const Spell * spell) const
{
	if(actualCaster)
		return actualCaster->getEffectPower(spell);

	return spell->getLevelPower(getEffectLevel(spell));
}

int32_t ProxyCaster::getEnchantPower(const Spell * spell) const
{
	if(actualCaster)
		return actualCaster->getEnchantPower(spell);

	return spell->getLevelPower(getEffectLevel(spell));
}

int64_t ProxyCaster::getEffectValue(const Spell * spell) const
{
	if(actualCaster)
		return actualCaster->getEffectValue(spell);

	return 0;
}

PlayerColor ProxyCaster::getCasterOwner() const
{
	if(actualCaster)
		return actualCaster->getCasterOwner();

	return PlayerColor::CANNOT_DETERMINE;
}

void ProxyCaster::getCasterName(MetaString & text) const
{
	if(actualCaster)
		actualCaster->getCasterName(text);
}

void ProxyCaster::getCastDescription(const Spell * spell, const std::vector<const battle::Unit*> & attacked, MetaString & text) const
{
	if(actualCaster)
		actualCaster->getCastDescription(spell, attacked, text);
}

void ProxyCaster::spendMana(ServerCallback * server, const int32_t spellCost) const
{
	if(actualCaster)
		actualCaster->spendMana(server, spellCost);
}

const CGHeroInstance * ProxyCaster::getHeroCaster() const
{
	if(actualCaster)
		return actualCaster->getHeroCaster();
	
	return nullptr;
}

int32_t ProxyCaster::manaLimit() const
{
	if(actualCaster)
		return actualCaster->manaLimit();

	return 0;
}

}

VCMI_LIB_NAMESPACE_END

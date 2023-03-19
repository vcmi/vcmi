/*
 * ObstacleCasterProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ObstacleCasterProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

ObstacleCasterProxy::ObstacleCasterProxy(PlayerColor owner_, const Caster * hero_, const SpellCreatedObstacle * obs_):
	ProxyCaster(hero_),
	owner(std::move(owner_)),
	obs(*obs_)
{
}

int32_t ObstacleCasterProxy::getCasterUnitId() const
{
	if(actualCaster)
		return actualCaster->getCasterUnitId();
	else
		return -1;
}

int32_t ObstacleCasterProxy::getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool) const
{
	return obs.spellLevel;
}

int32_t ObstacleCasterProxy::getEffectLevel(const Spell * spell) const
{
	return obs.spellLevel;
}

int64_t ObstacleCasterProxy::getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const
{
	if(actualCaster)
		return std::max<int64_t>(actualCaster->getSpellBonus(spell, base, affectedStack), obs.minimalDamage);
	else
		return std::max<int64_t>(base, obs.minimalDamage);
}

int64_t ObstacleCasterProxy::getSpecificSpellBonus(const Spell * spell, int64_t base) const
{
	if(actualCaster)
		return actualCaster->getSpecificSpellBonus(spell, base);
	else
		return base;
}

int32_t ObstacleCasterProxy::getEffectPower(const Spell * spell) const
{
	return obs.casterSpellPower;
}

int32_t ObstacleCasterProxy::getEnchantPower(const Spell * spell) const
{
	return obs.casterSpellPower;
}

int64_t ObstacleCasterProxy::getEffectValue(const Spell * spell) const
{
	if(actualCaster)
		return std::max(static_cast<int64_t>(obs.minimalDamage), actualCaster->getEffectValue(spell));
	else
		return obs.minimalDamage;
}

PlayerColor ObstacleCasterProxy::getCasterOwner() const
{
	return owner;
}

void ObstacleCasterProxy::getCasterName(MetaString & text) const
{
	logGlobal->error("Unexpected call to ObstacleCasterProxy::getCasterName");
}

void ObstacleCasterProxy::getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const
{
		//do nothing
}

void ObstacleCasterProxy::spendMana(ServerCallback * server, const int spellCost) const
{
		//do nothing
}

}
VCMI_LIB_NAMESPACE_END
/*
 * Caster.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class PlayerColor;
class MetaString;
class ServerCallback;
class CGHeroInstance;
class Spell;

namespace battle
{
	class Unit;
}

namespace spells
{

class Spell;

class DLL_LINKAGE Caster
{
public:
	virtual ~Caster() = default;

	virtual int32_t getCasterUnitId() const = 0;

	/// returns level on which given spell would be cast by this(0 - none, 1 - basic etc);
	/// caster may not know this spell at all
	/// optionally returns number of selected school by arg - 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic
	virtual int32_t getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool = nullptr) const = 0;

	///default spell school level for effect calculation
	virtual int32_t getEffectLevel(const Spell * spell) const = 0;

	///applying sorcery secondary skill etc
	virtual int64_t getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const = 0;

	///only bonus for particular spell
	virtual int64_t getSpecificSpellBonus(const Spell * spell, int64_t base) const = 0;

	///default spell-power for damage/heal calculation
	virtual int32_t getEffectPower(const Spell * spell) const = 0;

	///default spell-power for timed effects duration
	virtual int32_t getEnchantPower(const Spell * spell) const = 0;

	///damage/heal override(ignores spell configuration, effect level and effect power)
	virtual int64_t getEffectValue(const Spell * spell) const = 0;

	virtual PlayerColor getCasterOwner() const = 0;

	///only name substitution
	virtual void getCasterName(MetaString & text) const = 0;

	///full default text
	virtual void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const = 0;

	virtual void spendMana(ServerCallback * server, const int32_t spellCost) const = 0;

	virtual int32_t manaLimit() const = 0;
	
	///used to identify actual hero caster
	virtual const CGHeroInstance * getHeroCaster() const = 0;
};

}

VCMI_LIB_NAMESPACE_END

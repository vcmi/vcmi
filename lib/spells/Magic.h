/*
 * Magic.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

/**
 * High-level interface for spells subsystem
 */


class CSpell;
class CStack;
class PlayerColor;
struct MetaString;

class DLL_LINKAGE ISpellCaster
{
public:
	virtual ~ISpellCaster(){};

	/// returns level on which given spell would be cast by this(0 - none, 1 - basic etc);
	/// caster may not know this spell at all
	/// optionally returns number of selected school by arg - 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic
	virtual ui8 getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool = nullptr) const = 0;

	///applying sorcery secondary skill etc
	virtual ui32 getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const = 0;

	///default spell school level for effect calculation
	virtual int getEffectLevel(const CSpell * spell) const = 0;

	///default spell-power for damage/heal calculation
	virtual int getEffectPower(const CSpell * spell) const = 0;

	///default spell-power for timed effects duration
	virtual int getEnchantPower(const CSpell * spell) const = 0;

	///damage/heal override(ignores spell configuration, effect level and effect power)
	virtual int getEffectValue(const CSpell * spell) const = 0;

	virtual const PlayerColor getOwner() const = 0;

	///only name substitution
	virtual void getCasterName(MetaString & text) const = 0;

	///full default text
	virtual void getCastDescription(const CSpell * spell, const std::vector<const CStack *> & attacked, MetaString & text) const = 0;
};

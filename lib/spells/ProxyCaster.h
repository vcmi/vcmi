/*
 * ProxyCaster.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Magic.h"

namespace spells
{

class DLL_LINKAGE ProxyCaster : public Caster
{
public:
	ProxyCaster(const Caster * actualCaster_);
	virtual ~ProxyCaster();

	int32_t getCasterUnitId() const override;
	ui8 getSpellSchoolLevel(const Spell * spell, int * outSelectedSchool = nullptr) const override;
	int getEffectLevel(const Spell * spell) const override;
	int64_t getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const override;
	int64_t getSpecificSpellBonus(const Spell * spell, int64_t base) const override;
	int getEffectPower(const Spell * spell) const override;
	int getEnchantPower(const Spell * spell) const override;
	int64_t getEffectValue(const Spell * spell) const override;
	const PlayerColor getOwner() const override;
	void getCasterName(MetaString & text) const override;
	void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const override;
	void spendMana(const PacketSender * server, const int spellCost) const override;

private:
	const Caster * actualCaster;
};

} // namespace spells

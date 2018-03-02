/*
 * AbilityCaster.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ProxyCaster.h"

namespace spells
{

class DLL_LINKAGE AbilityCaster : public ProxyCaster
{
public:
	AbilityCaster(const battle::Unit * actualCaster_, int baseSpellLevel_);
	virtual ~AbilityCaster();

	ui8 getSpellSchoolLevel(const Spell * spell, int * outSelectedSchool = nullptr) const override;
	int getEffectLevel(const Spell * spell) const override;
	void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const override;
	void spendMana(const PacketSender * server, const int spellCost) const override;

private:
	const battle::Unit * actualCaster;
	int baseSpellLevel;
};

} // namespace spells

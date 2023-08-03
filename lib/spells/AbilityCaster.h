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

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

class DLL_LINKAGE AbilityCaster : public ProxyCaster
{
public:
	AbilityCaster(const battle::Unit * actualCaster_, int32_t baseSpellLevel_);
	virtual ~AbilityCaster();

	int32_t getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool = nullptr) const override;
	int32_t getEffectLevel(const Spell * spell) const override;
	void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const override;
	void spendMana(ServerCallback * server, const int32_t spellCost) const override;

private:
	int32_t baseSpellLevel;
};

} // namespace spells

VCMI_LIB_NAMESPACE_END

/*
 * ExternalCaster.h, part of VCMI engine
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

class DLL_LINKAGE ExternalCaster : public ProxyCaster
{
	int schoolLevel;
public:
	ExternalCaster();
	ExternalCaster(const Caster * actualCaster_, int schoolLevel_);
	
	void setActualCaster(const Caster * actualCaster);
	void setSpellSchoolLevel(int level);

	int32_t getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool = nullptr) const override;
	void spendMana(ServerCallback * server, const int32_t spellCost) const override;
};

} // namespace spells

VCMI_LIB_NAMESPACE_END

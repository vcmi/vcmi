/*
 * api/Spell.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Spell.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells { class SpellSchoolType; }

namespace scripting::api
{

class SpellProxy : public RawPointerWrapper<const ::spells::Spell, SpellProxy>
{
public:
	static constexpr std::string_view luaName = "Spell";
	static constexpr std::string_view luaDescription =
		"A spell definition (Fire Bolt, Slow, Town Portal, …). Identifies kind and metadata "
		"(schools, level). Cast resolution and per-cast state live on SpellMechanics.";

	static void registerMethods(MethodRegistrar & R);

	static std::vector<const spells::SpellSchoolType *> getSchools(const ::spells::Spell & spell);
};

}

VCMI_LIB_NAMESPACE_END

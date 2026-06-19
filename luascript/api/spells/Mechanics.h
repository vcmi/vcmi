/*
 * Mechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"
#include "../../../lib/spells/ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{
	class MechanicsProxy : public RawPointerWrapper<::spells::Mechanics, MechanicsProxy>
	{
	public:
		static constexpr std::string_view luaName = "SpellMechanics";
		static constexpr std::string_view luaDescription =
			"Per-cast spell context: which Spell is being cast, who is casting it, at what "
			"power and level. Lua spell scripts receive this as the entry point to their cast "
			"logic and consult it for caster identity, target validation, and damage formulas.";

		static void registerMethods(MethodRegistrar & R);

		static bool ownerMatchesUnit(const ::spells::Mechanics & m, const battle::Unit & unit);
		static std::string getPluralFormTextID(const ::spells::Mechanics & m, const std::string & baseTextID, int32_t count);
	};

}

VCMI_LIB_NAMESPACE_END

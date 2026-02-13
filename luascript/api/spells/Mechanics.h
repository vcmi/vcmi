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
#include "../../../lib/spells/ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

	namespace scripting
{
	namespace api
	{
		class SpellsMechanicsProxy : public SharedWrapper<::spells::Mechanics, SpellsMechanicsProxy>
		{
		public:
			using Wrapper = SharedWrapper<::spells::Mechanics, SpellsMechanicsProxy>;

			static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
		};
	}
}

VCMI_LIB_NAMESPACE_END

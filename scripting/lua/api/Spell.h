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

#include "../LuaWrapper.h"

namespace scripting
{
namespace api
{

class SpellProxy : public OpaqueWrapper<const ::spells::Spell, SpellProxy>
{
public:
	using Wrapper = OpaqueWrapper<const ::spells::Spell, SpellProxy>;
	static const std::vector<typename Wrapper::RegType> REGISTER;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}
}

/*
 * BattleSpellCastSource.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <cstdint>

VCMI_LIB_NAMESPACE_BEGIN

enum class BattleSpellCastSource : uint8_t
{
	HERO,
	CREATURE,
	ANY
};

VCMI_LIB_NAMESPACE_END

/*
 * PlayerConnectionID.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class PlayerConnectionID : int8_t
{
	INVALID = -1,
	PLAYER_AI = 0,
	FIRST_HUMAN = 1
};

VCMI_LIB_NAMESPACE_END

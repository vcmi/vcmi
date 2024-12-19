/*
 * BattleSide.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class BattleSide : int8_t
{
	NONE = -1,
	INVALID = -2,
	ALL_KNOWING = -3,

	ATTACKER = 0,
	DEFENDER = 1,

	// Aliases for convenience
	LEFT_SIDE = ATTACKER,
	RIGHT_SIDE = DEFENDER,
};

template<typename T>
class BattleSideArray : public std::array<T, 2>
{
public:
	const T & at(BattleSide side) const
	{
		return std::array<T, 2>::at(static_cast<int>(side));
	}

	T & at(BattleSide side)
	{
		return std::array<T, 2>::at(static_cast<int>(side));
	}

	const T & operator[](BattleSide side) const
	{
		return std::array<T, 2>::at(static_cast<int>(side));
	}

	T & operator[](BattleSide side)
	{
		return std::array<T, 2>::at(static_cast<int>(side));
	}
};

VCMI_LIB_NAMESPACE_END

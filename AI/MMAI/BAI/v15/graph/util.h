/*
 * util.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

namespace MMAI::BAI::V15::Graph
{
inline int permille(int v, int max)
{
	// Multiplying by 1000 might cause int32 overflow
	// => use temp long
	return static_cast<int>((1000LL * v) / max);
}

inline int permille(double v, int max)
{
	return static_cast<int>((1000 * v) / max);
}
}

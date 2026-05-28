/*
 * SpellAnimationItem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class VerticalPosition : ui8{TOP, CENTER, BOTTOM};

struct SpellAnimationItem
{
	AnimationPath resourceName;
	std::string effectName;
	VerticalPosition verticalPosition;
	float transparency;
	int pause;

	SpellAnimationItem() :
		verticalPosition(VerticalPosition::TOP),
		transparency(1),
		pause(0)
	{
	}
};

using SpellAnimationQueue = std::vector<SpellAnimationItem>;

VCMI_LIB_NAMESPACE_END

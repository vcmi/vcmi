/*
 * SObjectSounds.h, part of VCMI engine
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

struct SObjectSounds
{
	std::vector<AudioPath> ambient;
	std::vector<AudioPath> visit;
	std::vector<AudioPath> removal;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ambient;
		h & visit;
		h & removal;
	}
};

VCMI_LIB_NAMESPACE_END

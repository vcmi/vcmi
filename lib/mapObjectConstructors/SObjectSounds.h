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

VCMI_LIB_NAMESPACE_BEGIN

struct SObjectSounds
{
	std::vector<std::string> ambient;
	std::vector<std::string> visit;
	std::vector<std::string> removal;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ambient;
		h & visit;
		h & removal;
	}
};

VCMI_LIB_NAMESPACE_END

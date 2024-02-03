/*
 * ExtraOptionsInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE ExtraOptionsInfo
{
	bool cheatsAllowed = true;
	bool unlimitedReplay = false;

	bool operator == (const ExtraOptionsInfo & other) const;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & cheatsAllowed;
		h & unlimitedReplay;
	}
};

VCMI_LIB_NAMESPACE_END

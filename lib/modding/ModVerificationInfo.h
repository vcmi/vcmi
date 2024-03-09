/*
 * ModVerificationInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CModVersion.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ModVerificationInfo
{
	/// human-readable mod name
	std::string name;

	/// version of the mod
	CModVersion version;

	/// CRC-32 checksum of the mod
	ui32 checksum = 0;

	/// parent mod ID, empty if root-level mod
	TModID parent;

	/// for serialization purposes
	bool impactsGameplay = true;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & name;
		h & version;
		h & checksum;
		h & parent;
		h & impactsGameplay;
	}
};

VCMI_LIB_NAMESPACE_END

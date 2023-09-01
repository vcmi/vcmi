/*
 * CModVersion.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#ifdef __UCLIBC__
#undef major
#undef minor
#undef patch
#endif

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE CModVersion
{
	static const int Any = -1;
	
	int major = Any;
	int minor = Any;
	int patch = Any;

	CModVersion() = default;
	CModVersion(int mj, int mi, int p): major(mj), minor(mi), patch(p) {}

	static CModVersion GameVersion();
	static CModVersion fromString(std::string from);
	std::string toString() const;

	bool compatible(const CModVersion & other, bool checkMinor = false, bool checkPatch = false) const;
	bool isNull() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & major;
		h & minor;
		h & patch;
	}
};

DLL_LINKAGE bool operator < (const CModVersion & lesser, const CModVersion & greater);

VCMI_LIB_NAMESPACE_END

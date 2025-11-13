/*
 * CModVersion.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CModVersion.h"

VCMI_LIB_NAMESPACE_BEGIN

CModVersion CModVersion::GameVersion()
{
	return CModVersion(VCMI_VERSION_MAJOR, VCMI_VERSION_MINOR, VCMI_VERSION_PATCH);
}

CModVersion CModVersion::fromString(const std::string & from)
{
	std::vector<std::string> segments;
	boost::split(segments, from, boost::is_any_of("."));

	if (from.empty())
		return CModVersion();

	if (segments.size() > 3)
		return CModVersion();

	static const std::string whitelist = "1234567890.";

	for (const auto & ch : from)
		if (whitelist.find(ch) == std::string::npos)
			return CModVersion();

	try
	{
		int major = Any;
		int minor = Any;
		int patch = Any;

		major = std::stoi(segments[0]);
		if (segments.size() > 1)
			minor = std::stoi(segments[1]);
		if (segments.size() > 2)
			patch = std::stoi(segments[2]);

		return CModVersion(major, minor, patch);
	}
	catch(const std::logic_error &)
	{
		return CModVersion();
	}
}

std::string CModVersion::toString() const
{
	std::string res;
	if(major != Any)
	{
		res += std::to_string(major);
		if(minor != Any)
		{
			res += '.' + std::to_string(minor);
			if(patch != Any)
				res += '.' + std::to_string(patch);
		}
	}
	return res;
}

bool CModVersion::operator ==(const CModVersion & other) const
{
	return major == other.major && minor == other.minor && patch == other.patch;
}

bool CModVersion::operator !=(const CModVersion & other) const
{
	return major != other.major || minor != other.minor || patch != other.patch;
}

bool CModVersion::compatible(const CModVersion & other, bool checkMinor, bool checkPatch) const
{
	bool doCheckMinor = checkMinor && minor != Any && other.minor != Any;
	bool doCheckPatch = checkPatch && patch != Any && other.patch != Any;
	
	assert(!doCheckPatch || (doCheckPatch && doCheckMinor));
		
	return  (major == other.major &&
			(!doCheckMinor || minor >= other.minor) &&
			(!doCheckPatch || minor > other.minor || (minor == other.minor && patch >= other.patch)));
}

bool CModVersion::isNull() const
{
	return major == Any;
}

bool operator < (const CModVersion & lesser, const CModVersion & greater)
{
	//specific is "greater" than non-specific, that's why do not check for Any value
	if(lesser.major == greater.major)
	{
		if(lesser.minor == greater.minor)
			return lesser.patch < greater.patch;
		return lesser.minor < greater.minor;
	}
	return lesser.major < greater.major;
}

VCMI_LIB_NAMESPACE_END

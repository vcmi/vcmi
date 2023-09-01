/*
 * CModVersion.h, part of VCMI engine
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

CModVersion CModVersion::fromString(std::string from)
{
	int major = Any;
	int minor = Any;
	int patch = Any;
	try
	{
		auto pointPos = from.find('.');
		major = std::stoi(from.substr(0, pointPos));
		if(pointPos != std::string::npos)
		{
			from = from.substr(pointPos + 1);
			pointPos = from.find('.');
			minor = std::stoi(from.substr(0, pointPos));
			if(pointPos != std::string::npos)
				patch = std::stoi(from.substr(pointPos + 1));
		}
	}
	catch(const std::invalid_argument &)
	{
		return CModVersion();
	}
	return CModVersion(major, minor, patch);
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

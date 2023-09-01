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
	int major = 0;
	int minor = 0;
	int patch = 0;
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
	return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch);
}

bool CModVersion::compatible(const CModVersion & other, bool checkMinor, bool checkPatch) const
{
	return  (major == other.major &&
			(!checkMinor || minor >= other.minor) &&
			(!checkPatch || minor > other.minor || (minor == other.minor && patch >= other.patch)));
}

bool CModVersion::isNull() const
{
	return major == 0 && minor == 0 && patch == 0;
}

bool operator < (const CModVersion & lesser, const CModVersion & greater)
{
	if(lesser.major == greater.major)
	{
		if(lesser.minor == greater.minor)
			return lesser.patch < greater.patch;
		return lesser.minor < greater.minor;
	}
	return lesser.major < greater.major;
}

VCMI_LIB_NAMESPACE_END

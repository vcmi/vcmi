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

class JsonNode;
struct ModVerificationInfo;
using ModCompatibilityInfo = std::map<std::string, ModVerificationInfo>;

enum class ModVerificationStatus
{
	NOT_INSTALLED, /// Mod is not installed locally
	DISABLED, /// Mod is installed locally but not enabled
	EXCESSIVE, /// Mod is enabled locally but must be disabled
	VERSION_MISMATCH, /// Mod is present on both sides, but has different version
	FULL_MATCH, /// No issues detected, everything matches
};

using ModListVerificationStatus = std::map<std::string, ModVerificationStatus>;

struct DLL_LINKAGE ModVerificationInfo
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

	static JsonNode jsonSerializeList(const ModCompatibilityInfo & input);
	static ModCompatibilityInfo jsonDeserializeList(const JsonNode & input);
	static ModListVerificationStatus verifyListAgainstLocalMods(const ModCompatibilityInfo & input);

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

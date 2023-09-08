/*
 * CModInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../JsonNode.h"
#include "CModVersion.h"

VCMI_LIB_NAMESPACE_BEGIN

using TModID = std::string;

class DLL_LINKAGE CModInfo
{
	/// cached result of checkModGameplayAffecting() call
	/// Do not serialize - depends on local mod version, not server/save mod version
	mutable std::optional<bool> modGameplayAffecting;

public:
	enum EValidationStatus
	{
		PENDING,
		FAILED,
		PASSED
	};

	/// identifier, identical to name of folder with mod
	std::string identifier;

	/// human-readable strings
	std::string name;
	std::string description;

	/// version of the mod
	CModVersion version;

	/// Base language of mod, all mod strings are assumed to be in this language
	std::string baseLanguage;

	/// vcmi versions compatible with the mod
	CModVersion vcmiCompatibleMin, vcmiCompatibleMax;

	/// list of mods that should be loaded before this one
	std::set <TModID> dependencies;

	/// list of mods that can't be used in the same time as this one
	std::set <TModID> conflicts;

	/// CRC-32 checksum of the mod
	ui32 checksum;

	EValidationStatus validation;

	JsonNode config;

	CModInfo();
	CModInfo(const std::string & identifier, const JsonNode & local, const JsonNode & config);

	JsonNode saveLocalData() const;
	void updateChecksum(ui32 newChecksum);

	bool isEnabled() const;
	void setEnabled(bool on);

	static std::string getModDir(const std::string & name);
	static JsonPath getModFile(const std::string & name);

	/// return true if this mod can affect gameplay, e.g. adds or modifies any game objects
	bool checkModGameplayAffecting() const;

private:
	/// true if mod is enabled by user, e.g. in Launcher UI
	bool explicitlyEnabled;

	/// true if mod can be loaded - compatible and has no missing deps
	bool implicitlyEnabled;

	void loadLocalData(const JsonNode & data);
};

VCMI_LIB_NAMESPACE_END

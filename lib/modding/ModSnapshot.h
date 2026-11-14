/*
 * ModSnapshot.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class ModDescription;
class ModsStorage;

using TModID = std::string;
using TModList = std::vector<TModID>;

/// Immutable mod list of one game library: installed mods, active mods and checksums validated by previous loads. Created by ModManager
class DLL_LINKAGE ModSnapshot
{
	std::shared_ptr<const ModsStorage> storage;
	TModList activeMods;
	std::map<TModID, uint32_t> validatedChecksums;

public:
	ModSnapshot(std::shared_ptr<const ModsStorage> storage, TModList activeMods, std::map<TModID, uint32_t> validatedChecksums);

	const ModDescription & getModDescription(const TModID & modID) const;

	/// Returns all installed mods, active or not
	TModList getAllMods() const;

	/// Returns active mods in their load order
	const TModList & getActiveMods() const;
	bool isModActive(const TModID & modID) const;

	/// Returns checksum with which mod has passed validation, if any
	std::optional<uint32_t> getValidatedChecksum(const TModID & modID) const;
};

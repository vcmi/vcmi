/*
 * ModSnapshot.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ModSnapshot.h"

#include "ModManager.h"

ModSnapshot::ModSnapshot(std::shared_ptr<const ModsStorage> storage, TModList activeMods, std::map<TModID, uint32_t> validatedChecksums)
	: storage(std::move(storage))
	, activeMods(std::move(activeMods))
	, validatedChecksums(std::move(validatedChecksums))
{
}

const ModDescription & ModSnapshot::getModDescription(const TModID & modID) const
{
	return storage->getMod(modID);
}

TModList ModSnapshot::getInstalledMods() const
{
	return storage->getAllMods();
}

const TModList & ModSnapshot::getActiveMods() const
{
	return activeMods;
}

bool ModSnapshot::isModActive(const TModID & modID) const
{
	return vstd::contains(activeMods, modID);
}

std::optional<uint32_t> ModSnapshot::getValidatedChecksum(const TModID & modID) const
{
	auto it = validatedChecksums.find(modID);
	if(it == validatedChecksums.end())
		return std::nullopt;
	return it->second;
}

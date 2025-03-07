/*
 * ModVerificationInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ModVerificationInfo.h"

#include "CModHandler.h"
#include "ModDescription.h"
#include "ModIncompatibility.h"
#include "ModScope.h"

#include "../json/JsonNode.h"
#include "../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

JsonNode ModVerificationInfo::jsonSerializeList(const ModCompatibilityInfo & input)
{
	JsonNode output;

	for(const auto & mod : input)
	{
		JsonNode modWriter;
		modWriter["modId"].String() = mod.first;
		modWriter["name"].String() = mod.second.name;
		if (!mod.second.parent.empty())
			modWriter["parent"].String() = mod.second.parent;
		modWriter["version"].String() = mod.second.version.toString();
		output.Vector().push_back(modWriter);
	}

	return output;
}

ModCompatibilityInfo ModVerificationInfo::jsonDeserializeList(const JsonNode & input)
{
	ModCompatibilityInfo output;

	for(const auto & mod : input.Vector())
	{
		ModVerificationInfo info;
		info.version = CModVersion::fromString(mod["version"].String());
		info.name = mod["name"].String();
		info.parent = mod["parent"].String();
		info.checksum = 0;
		info.impactsGameplay = true;

		if(!mod["modId"].isNull())
			output[mod["modId"].String()] = info;
		else
			output[mod["name"].String()] = info;
	}

	return output;
}

ModListVerificationStatus ModVerificationInfo::verifyListAgainstLocalMods(const ModCompatibilityInfo & modList)
{
	ModListVerificationStatus result;

	for(const auto & m : LIBRARY->modh->getActiveMods())
	{
		if(modList.count(m))
			continue;

		if (m == ModScope::scopeBuiltin())
			continue;

		if(LIBRARY->modh->getModInfo(m).affectsGameplay())
			result[m] = ModVerificationStatus::EXCESSIVE;
	}

	for(const auto & infoPair : modList)
	{
		auto & remoteModId = infoPair.first;
		auto & remoteModInfo = infoPair.second;

		if (remoteModId == ModScope::scopeBuiltin())
			continue;

		bool modAffectsGameplay = remoteModInfo.impactsGameplay;
		//parent mod affects gameplay if child affects too
		for(const auto & subInfoPair : modList)
			modAffectsGameplay |= (subInfoPair.second.impactsGameplay && subInfoPair.second.parent == remoteModId);

		if(!vstd::contains(LIBRARY->modh->getAllMods(), remoteModId))
		{
			result[remoteModId] = ModVerificationStatus::NOT_INSTALLED;
			continue;
		}

		const auto & localVersion = LIBRARY->modh->getModInfo(remoteModId).getVersion();
		modAffectsGameplay |= LIBRARY->modh->getModInfo(remoteModId).affectsGameplay();

		// skip it. Such mods should only be present in old saves or if mod changed and no longer affects gameplay
		if (!modAffectsGameplay)
			continue;

		if (!vstd::contains(LIBRARY->modh->getActiveMods(), remoteModId))
		{
			result[remoteModId] = ModVerificationStatus::DISABLED;
			continue;
		}

		if(remoteModInfo.version != localVersion)
		{
			result[remoteModId] = ModVerificationStatus::VERSION_MISMATCH;
			continue;
		}

		result[remoteModId] = ModVerificationStatus::FULL_MATCH;
	}

	return result;
}

VCMI_LIB_NAMESPACE_END

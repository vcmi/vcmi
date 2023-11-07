/*
 * ActiveModsInSaveList.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ActiveModsInSaveList.h"

#include "../VCMI_Lib.h"
#include "CModInfo.h"
#include "CModHandler.h"
#include "ModIncompatibility.h"

VCMI_LIB_NAMESPACE_BEGIN

std::vector<TModID> ActiveModsInSaveList::getActiveMods()
{
	return VLC->modh->getActiveMods();
}

const ModVerificationInfo & ActiveModsInSaveList::getVerificationInfo(TModID mod)
{
	return VLC->modh->getModInfo(mod).getVerificationInfo();
}

void ActiveModsInSaveList::verifyActiveMods(const std::vector<std::pair<TModID, ModVerificationInfo>> & modList)
{
	auto searchVerificationInfo = [&modList](const TModID & m) -> const ModVerificationInfo*
	{
		for(auto & i : modList)
			if(i.first == m)
				return &i.second;
		return nullptr;
	};

	std::vector<TModID> missingMods, excessiveMods;
	ModIncompatibility::ModListWithVersion missingModsResult;
	ModIncompatibility::ModList excessiveModsResult;

	for(const auto & m : VLC->modh->getActiveMods())
	{
		if(searchVerificationInfo(m))
			continue;

		//TODO: support actual disabling of these mods
		if(VLC->modh->getModInfo(m).checkModGameplayAffecting())
			excessiveMods.push_back(m);
	}

	for(const auto & infoPair : modList)
	{
		auto & remoteModId = infoPair.first;
		auto & remoteModInfo = infoPair.second;

		bool modAffectsGameplay = remoteModInfo.impactsGameplay;
		//parent mod affects gameplay if child affects too
		for(const auto & subInfoPair : modList)
			modAffectsGameplay |= (subInfoPair.second.impactsGameplay && subInfoPair.second.parent == remoteModId);

		if(!vstd::contains(VLC->modh->getAllMods(), remoteModId))
		{
			if(modAffectsGameplay)
				missingMods.push_back(remoteModId); //mod is not installed
			continue;
		}

		auto & localModInfo = VLC->modh->getModInfo(remoteModId).getVerificationInfo();
		modAffectsGameplay |= VLC->modh->getModInfo(remoteModId).checkModGameplayAffecting();
		bool modVersionCompatible = localModInfo.version.isNull()
			|| remoteModInfo.version.isNull()
			|| localModInfo.version.compatible(remoteModInfo.version);
		bool modLocalyEnabled = vstd::contains(VLC->modh->getActiveMods(), remoteModId);

		if(modVersionCompatible && modAffectsGameplay && modLocalyEnabled)
			continue;

		if(modAffectsGameplay)
			missingMods.push_back(remoteModId); //incompatible mod impacts gameplay
	}

	//filter mods
	for(auto & m : missingMods)
	{
		if(auto * vInfo = searchVerificationInfo(m))
		{
			assert(vInfo->parent != m);
			if(!vInfo->parent.empty() && vstd::contains(missingMods, vInfo->parent))
				continue;
			missingModsResult.push_back({vInfo->name, vInfo->version.toString()});
		}
	}
	for(auto & m : excessiveMods)
	{
		auto & vInfo = VLC->modh->getModInfo(m).getVerificationInfo();
		assert(vInfo.parent != m);
		if(!vInfo.parent.empty() && vstd::contains(excessiveMods, vInfo.parent))
			continue;
		excessiveModsResult.push_back(vInfo.name);
	}

	if(!missingModsResult.empty() || !excessiveModsResult.empty())
		throw ModIncompatibility(missingModsResult, excessiveModsResult);

	//TODO: support actual enabling of required mods
}


VCMI_LIB_NAMESPACE_END

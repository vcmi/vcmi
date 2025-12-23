/*
 * ArtifactUtils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CArtifactSet;
struct ArtSlotInfo;

namespace ArtifactUtils
{
	DLL_LINKAGE bool checkIfSlotValid(const CArtifactSet & artSet, const ArtifactPosition & slot);
	DLL_LINKAGE ArtifactPosition getArtAnyPosition(const CArtifactSet * target, const ArtifactID & aid);
	DLL_LINKAGE ArtifactPosition getArtEquippedPosition(const CArtifactSet * target, const ArtifactID & aid);
	DLL_LINKAGE ArtifactPosition getArtBackpackPosition(const CArtifactSet * target, const ArtifactID & aid);
	// TODO: Make this constexpr when the toolset is upgraded
	DLL_LINKAGE const std::vector<ArtifactPosition> & unmovableSlots();
	DLL_LINKAGE const std::vector<ArtifactPosition> & commonWornSlots();
	DLL_LINKAGE const std::vector<ArtifactPosition> & allWornSlots();
	DLL_LINKAGE const std::vector<ArtifactPosition> & commanderSlots();
	DLL_LINKAGE bool isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot);
	DLL_LINKAGE bool checkSpellbookIsNeeded(const CGHeroInstance * heroPtr, const ArtifactID & artID, const ArtifactPosition & slot);
	DLL_LINKAGE bool isSlotBackpack(const ArtifactPosition & slot);
	DLL_LINKAGE bool isSlotEquipment(const ArtifactPosition & slot);
	DLL_LINKAGE bool isBackpackFreeSlots(const CArtifactSet * target, const size_t reqSlots = 1);
	DLL_LINKAGE std::vector<const CArtifact*> assemblyPossibilities(const CArtifactSet * artSet, const ArtifactID & aid, const bool onlyEquiped = false);
	DLL_LINKAGE void insertScrrollSpellName(std::string & description, const SpellID & sid);
}

VCMI_LIB_NAMESPACE_END

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

#include "StdInc.h"

#include "GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtHandler;
class CArtifact;
class CGHeroInstance;
class CArtifactSet;
class CArtifactInstance;
struct ArtSlotInfo;
class CMap;

namespace ArtifactUtils
{
	// Calculates where an artifact gets placed when it gets transferred from one hero to another.
	DLL_LINKAGE ArtifactPosition getArtAnyPosition(const CArtifactSet * target, const ArtifactID & aid);
	DLL_LINKAGE ArtifactPosition getArtBackpackPosition(const CArtifactSet * target, const ArtifactID & aid);
	// TODO: Make this constexpr when the toolset is upgraded
	DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & unmovableSlots();
	DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & constituentWornSlots();
	DLL_LINKAGE bool isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot);
	DLL_LINKAGE bool checkSpellbookIsNeeded(const CGHeroInstance * heroPtr, const ArtifactID & artID, const ArtifactPosition & slot);
	DLL_LINKAGE bool isSlotBackpack(const ArtifactPosition & slot);
	DLL_LINKAGE bool isSlotEquipment(const ArtifactPosition & slot);
	DLL_LINKAGE bool isBackpackFreeSlots(const CArtifactSet * target, const size_t reqSlots = 1);
	DLL_LINKAGE std::vector<const CArtifact*> assemblyPossibilities(const CArtifactSet * artSet, const ArtifactID & aid, bool equipped);
	DLL_LINKAGE CArtifactInstance * createScroll(const SpellID & sid);
	DLL_LINKAGE CArtifactInstance * createNewArtifactInstance(CArtifact * art);
	DLL_LINKAGE CArtifactInstance * createNewArtifactInstance(const ArtifactID & aid);
	DLL_LINKAGE CArtifactInstance * createArtifact(CMap * map, const ArtifactID & aid, SpellID spellID = SpellID::NONE);
	DLL_LINKAGE void insertScrrollSpellName(std::string & description, const SpellID & sid);
}

VCMI_LIB_NAMESPACE_END

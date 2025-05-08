/*
 * ArtifactUtils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ArtifactUtils.h"

#include "ArtBearer.h"
#include "CArtifact.h"
#include "CArtifactFittingSet.h"

#include "../../IGameSettings.h"
#include "../../GameLibrary.h"
#include "../../mapObjects/CGHeroInstance.h"

#include <vcmi/spells/Spell.h>

VCMI_LIB_NAMESPACE_BEGIN

DLL_LINKAGE bool ArtifactUtils::checkIfSlotValid(const CArtifactSet & artSet, const ArtifactPosition & slot)
{
	if(artSet.bearerType() == ArtBearer::HERO)
	{
		if(isSlotEquipment(slot) || isSlotBackpack(slot) || slot == ArtifactPosition::TRANSITION_POS)
			return true;
	}
	else if(artSet.bearerType() == ArtBearer::ALTAR)
	{
		if(isSlotBackpack(slot))
			return true;
	}
	else if(artSet.bearerType() == ArtBearer::COMMANDER)
	{
		if(vstd::contains(commanderSlots(), slot))
			return true;
	}
	else if(artSet.bearerType() == ArtBearer::CREATURE)
	{
		if(slot == ArtifactPosition::CREATURE_SLOT)
			return true;
	}
	return false;
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtAnyPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	if(auto targetSlot = getArtEquippedPosition(target, aid); targetSlot != ArtifactPosition::PRE_FIRST)
		return targetSlot;
	return getArtBackpackPosition(target, aid);
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtEquippedPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	const auto * art = aid.toArtifact();
	for(const auto & slot : art->getPossibleSlots().at(target->bearerType()))
	{
		if(art->canBePutAt(target, slot))
			return slot;
	}
	return ArtifactPosition::PRE_FIRST;
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtBackpackPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	const auto * art = aid.toArtifact();
	if(target->bearerType() == ArtBearer::HERO)
		if(art->canBePutAt(target, ArtifactPosition::BACKPACK_START))
		{
			return ArtifactPosition::BACKPACK_START + target->artifactsInBackpack.size();
		}
	return ArtifactPosition::PRE_FIRST;
}

DLL_LINKAGE const std::vector<ArtifactPosition> & ArtifactUtils::unmovableSlots()
{
	static const std::vector<ArtifactPosition> positions =
	{
		ArtifactPosition::SPELLBOOK,
		ArtifactPosition::MACH4
	};

	return positions;
}

DLL_LINKAGE const std::vector<ArtifactPosition> & ArtifactUtils::commonWornSlots()
{
	static const std::vector<ArtifactPosition> positions =
	{
		ArtifactPosition::HEAD,
		ArtifactPosition::SHOULDERS,
		ArtifactPosition::NECK,
		ArtifactPosition::RIGHT_HAND,
		ArtifactPosition::LEFT_HAND,
		ArtifactPosition::TORSO,
		ArtifactPosition::RIGHT_RING,
		ArtifactPosition::LEFT_RING,
		ArtifactPosition::FEET,
		ArtifactPosition::MISC1,
		ArtifactPosition::MISC2,
		ArtifactPosition::MISC3,
		ArtifactPosition::MISC4,
		ArtifactPosition::MISC5,
	};

	return positions;
}

DLL_LINKAGE const std::vector<ArtifactPosition> & ArtifactUtils::allWornSlots()
{
	static const std::vector<ArtifactPosition> positions =
	{
		ArtifactPosition::HEAD,
		ArtifactPosition::SHOULDERS,
		ArtifactPosition::NECK,
		ArtifactPosition::RIGHT_HAND,
		ArtifactPosition::LEFT_HAND,
		ArtifactPosition::TORSO,
		ArtifactPosition::RIGHT_RING,
		ArtifactPosition::LEFT_RING,
		ArtifactPosition::FEET,
		ArtifactPosition::MISC1,
		ArtifactPosition::MISC2,
		ArtifactPosition::MISC3,
		ArtifactPosition::MISC4,
		ArtifactPosition::MISC5,
		ArtifactPosition::MACH1,
		ArtifactPosition::MACH2,
		ArtifactPosition::MACH3,
		ArtifactPosition::MACH4,
		ArtifactPosition::SPELLBOOK
	};

	return positions;
}

DLL_LINKAGE const std::vector<ArtifactPosition> & ArtifactUtils::commanderSlots()
{
	static const std::vector<ArtifactPosition> positions =
	{
		ArtifactPosition::COMMANDER1,
		ArtifactPosition::COMMANDER2,
		ArtifactPosition::COMMANDER3,
		ArtifactPosition::COMMANDER4,
		ArtifactPosition::COMMANDER5,
		ArtifactPosition::COMMANDER6
	};

	return positions;
}

DLL_LINKAGE bool ArtifactUtils::isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot)
{
	return slot.second.getArt()
		&& !slot.second.locked
		&& !vstd::contains(unmovableSlots(), slot.first);
}

DLL_LINKAGE bool ArtifactUtils::checkSpellbookIsNeeded(const CGHeroInstance * heroPtr, const ArtifactID & artID, const ArtifactPosition & slot)
{
	// TODO what'll happen if Titan's thunder is equipped by pickin git up or the start of game?
	// Titan's Thunder creates new spellbook on equip
	if(artID == ArtifactID::TITANS_THUNDER && slot == ArtifactPosition::RIGHT_HAND)
	{
		if(heroPtr && !heroPtr->hasSpellbook())
		{
			return true;
		}
	}
	return false;
}

DLL_LINKAGE bool ArtifactUtils::isSlotBackpack(const ArtifactPosition & slot)
{
	return slot >= ArtifactPosition::BACKPACK_START;
}

DLL_LINKAGE bool ArtifactUtils::isSlotEquipment(const ArtifactPosition & slot)
{
	return slot < ArtifactPosition::BACKPACK_START && slot >= ArtifactPosition(0);
}

DLL_LINKAGE bool ArtifactUtils::isBackpackFreeSlots(const CArtifactSet * target, const size_t reqSlots)
{
	if(target->bearerType() == ArtBearer::HERO)
	{
		const auto backpackCap = LIBRARY->engineSettings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
		if(backpackCap < 0)
			return true;
		else
			return target->artifactsInBackpack.size() + reqSlots <= backpackCap;
	}
	else
		return false;
}

DLL_LINKAGE std::vector<const CArtifact*> ArtifactUtils::assemblyPossibilities(
	const CArtifactSet * artSet, const ArtifactID & aid, const bool onlyEquiped)
{
	std::vector<const CArtifact*> arts;
	const auto * art = aid.toArtifact();
	if(art->isCombined())
		return arts;

	for(const auto combinedArt : art->getPartOf())
	{
		assert(combinedArt->isCombined());
		bool possible = true;
		CArtifactFittingSet fittingSet(*artSet);
		for(const auto part : combinedArt->getConstituents()) // check if all constituents are available
		{
			const auto slot = fittingSet.getArtPos(part->getId(), onlyEquiped, false);
			if(slot == ArtifactPosition::PRE_FIRST)
			{
				possible = false;
				break;
			}
			fittingSet.lockSlot(slot);
		}
		if(possible)
			arts.push_back(combinedArt);
	}
	return arts;
}

DLL_LINKAGE void ArtifactUtils::insertScrrollSpellName(std::string & description, const SpellID & sid)
{
	// We expect scroll description to be like this: This scroll contains the [spell name] spell which is added
	// into spell book for as long as hero carries the scroll. So we want to replace text in [...] with a spell name.
	// However other language versions don't have name placeholder at all, so we have to be careful
	auto nameStart = description.find_first_of('[');
	auto nameEnd = description.find_first_of(']', nameStart);

	if(nameStart != std::string::npos && nameEnd != std::string::npos)
	{
		if(sid.getNum() >= 0)
			description = description.replace(nameStart, nameEnd - nameStart + 1, sid.toEntity(LIBRARY)->getNameTranslated());
		else
			description = description.erase(nameStart, nameEnd - nameStart + 2); // erase "[spell name] " - including space
	}
}

VCMI_LIB_NAMESPACE_END

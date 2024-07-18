/*
 * ArtifactUtils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ArtifactUtils.h"

#include "CArtHandler.h"
#include "GameSettings.h"
#include "spells/CSpellHandler.h"

#include "mapping/CMap.h"
#include "mapObjects/CGHeroInstance.h"

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
			return ArtifactPosition::BACKPACK_START;
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
	return slot.second.artifact
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
		const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
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

	for(const auto artifact : art->getPartOf())
	{
		assert(artifact->isCombined());
		bool possible = true;

		for(const auto constituent : artifact->getConstituents()) //check if all constituents are available
		{
			if(!artSet->hasArt(constituent->getId(), onlyEquiped, false, false))
			{
				possible = false;
				break;
			}
		}
		if(possible)
			arts.push_back(artifact);
	}
	return arts;
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createScroll(const SpellID & sid)
{
	auto ret = new CArtifactInstance(ArtifactID(ArtifactID::SPELL_SCROLL).toArtifact());
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL,
		BonusSource::ARTIFACT_INSTANCE, -1, BonusSourceID(ArtifactID(ArtifactID::SPELL_SCROLL)), BonusSubtypeID(sid));
	ret->addNewBonus(bonus);
	return ret;
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createNewArtifactInstance(const CArtifact * art)
{
	assert(art);

	auto * artInst = new CArtifactInstance(art);
	if(art->isCombined())
	{
		for(const auto & part : art->getConstituents())
			artInst->addPart(ArtifactUtils::createNewArtifactInstance(part), ArtifactPosition::PRE_FIRST);
	}
	if(art->isGrowing())
	{
		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::LEVEL_COUNTER;
		bonus->val = 0;
		artInst->addNewBonus(bonus);
	}
	return artInst;
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createNewArtifactInstance(const ArtifactID & aid)
{
	return ArtifactUtils::createNewArtifactInstance(aid.toArtifact());
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createArtifact(CMap * map, const ArtifactID & aid, SpellID spellID)
{
	CArtifactInstance * art = nullptr;
	if(aid.getNum() >= 0)
	{
		if(spellID == SpellID::NONE)
		{
			art = ArtifactUtils::createNewArtifactInstance(aid);
		}
		else
		{
			art = ArtifactUtils::createScroll(spellID);
		}
	}
	else
	{
		art = new CArtifactInstance(); // random, empty
	}
	map->addNewArtifactInstance(art);
	if(art->artType && art->isCombined())
	{
		for(auto & part : art->getPartsInfo())
		{
			map->addNewArtifactInstance(part.art);
		}
	}
	return art;
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
			description = description.replace(nameStart, nameEnd - nameStart + 1, sid.toEntity(VLC->spells())->getNameTranslated());
		else
			description = description.erase(nameStart, nameEnd - nameStart + 2); // erase "[spell name] " - including space
	}
}

VCMI_LIB_NAMESPACE_END

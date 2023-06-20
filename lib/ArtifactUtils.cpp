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

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtAnyPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	const auto * art = aid.toArtifact();
	for(const auto & slot : art->possibleSlots.at(target->bearerType()))
	{
		if(art->canBePutAt(target, slot))
			return slot;
	}
	return getArtBackpackPosition(target, aid);
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtBackpackPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	const auto * art = aid.toArtifact();
	if(art->canBePutAt(target, GameConstants::BACKPACK_START))
	{
		return GameConstants::BACKPACK_START;
	}
	return ArtifactPosition::PRE_FIRST;
}

DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & ArtifactUtils::unmovableSlots()
{
	static const std::vector<ArtifactPosition::EArtifactPosition> positions =
	{
		ArtifactPosition::SPELLBOOK,
		ArtifactPosition::MACH4
	};

	return positions;
}

DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & ArtifactUtils::constituentWornSlots()
{
	static const std::vector<ArtifactPosition::EArtifactPosition> positions =
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
		if(heroPtr)
		{
			if(heroPtr && !heroPtr->hasSpellbook())
				return true;
		}
	}
	return false;
}

DLL_LINKAGE bool ArtifactUtils::isSlotBackpack(const ArtifactPosition & slot)
{
	return slot >= GameConstants::BACKPACK_START;
}

DLL_LINKAGE bool ArtifactUtils::isSlotEquipment(const ArtifactPosition & slot)
{
	return slot < GameConstants::BACKPACK_START && slot >= 0;
}

DLL_LINKAGE bool ArtifactUtils::isBackpackFreeSlots(const CArtifactSet * target, const size_t reqSlots)
{
	const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
	if(backpackCap < 0)
		return true;
	else
		return target->artifactsInBackpack.size() + reqSlots <= backpackCap;
}

DLL_LINKAGE std::vector<const CArtifact*> ArtifactUtils::assemblyPossibilities(
	const CArtifactSet * artSet, const ArtifactID & aid, bool equipped)
{
	std::vector<const CArtifact*> arts;
	const auto * art = aid.toArtifact();
	if(art->canBeDisassembled())
		return arts;

	for(const auto artifact : art->constituentOf)
	{
		assert(artifact->constituents);
		bool possible = true;

		for(const auto constituent : *artifact->constituents) //check if all constituents are available
		{
			if(equipped)
			{
				// Search for equipped arts
				if(!artSet->hasArt(constituent->getId(), true, false, false))
				{
					possible = false;
					break;
				}
			}
			else
			{
				// Search in backpack
				if(!artSet->hasArtBackpack(constituent->getId()))
				{
					possible = false;
					break;
				}
			}
		}
		if(possible)
			arts.push_back(artifact);
	}
	return arts;
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createScroll(const SpellID & sid)
{
	auto ret = new CArtifactInstance(VLC->arth->objects[ArtifactID::SPELL_SCROLL]);
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL,
		BonusSource::ARTIFACT_INSTANCE, -1, ArtifactID::SPELL_SCROLL, sid);
	ret->addNewBonus(bonus);
	return ret;
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createNewArtifactInstance(CArtifact * art)
{
	assert(art);

	auto * artInst = new CArtifactInstance(art);
	if(art->canBeDisassembled())
	{
		assert(art->constituents);
		for(const auto & part : *art->constituents)
			artInst->addArtInstAsPart(ArtifactUtils::createNewArtifactInstance(part), ArtifactPosition::PRE_FIRST);
	}
	if(dynamic_cast<CGrowingArtifact*>(art))
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
	return ArtifactUtils::createNewArtifactInstance(VLC->arth->objects[aid]);
}

DLL_LINKAGE CArtifactInstance * ArtifactUtils::createArtifact(CMap * map, const ArtifactID & aid, int spellID)
{
	CArtifactInstance * art = nullptr;
	if(aid >= 0)
	{
		if(spellID < 0)
		{
			art = ArtifactUtils::createNewArtifactInstance(aid);
		}
		else
		{
			art = ArtifactUtils::createScroll(SpellID(spellID));
		}
	}
	else
	{
		art = new CArtifactInstance(); // random, empty
	}
	map->addNewArtifactInstance(art);
	if(art->artType && art->canBeDisassembled())
	{
		for(auto & part : art->partsInfo)
		{
			map->addNewArtifactInstance(part.art);
		}
	}
	return art;
}

DLL_LINKAGE void ArtifactUtils::insertScrrollSpellName(std::string & description, SpellID & sid)
{
	// We expect scroll description to be like this: This scroll contains the [spell name] spell which is added
	// into spell book for as long as hero carries the scroll. So we want to replace text in [...] with a spell name.
	// However other language versions don't have name placeholder at all, so we have to be careful
	auto nameStart = description.find_first_of('[');
	auto nameEnd = description.find_first_of(']', nameStart);
	if(sid.getNum() >= 0)
	{
		if(nameStart != std::string::npos && nameEnd != std::string::npos)
			description = description.replace(nameStart, nameEnd - nameStart + 1, sid.toSpell(VLC->spells())->getNameTranslated());
	}
}

VCMI_LIB_NAMESPACE_END

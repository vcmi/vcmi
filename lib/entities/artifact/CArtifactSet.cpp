/*
 * CArtifactSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtifactSet.h"

#include "CArtifact.h"
#include "ArtifactUtils.h"

#include "../../constants/StringConstants.h"
#include "../../gameState/CGameState.h"
#include "../../mapping/CMap.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

	const CArtifactInstance * CArtifactSet::getArt(const ArtifactPosition & pos, bool excludeLocked) const
{
	if(const ArtSlotInfo * si = getSlot(pos))
	{
		if(si->getArt() && (!excludeLocked || !si->locked))
			return si->getArt();
	}
	return nullptr;
}

ArtifactInstanceID CArtifactSet::getArtID(const ArtifactPosition & pos, bool excludeLocked) const
{
	if(const ArtSlotInfo * si = getSlot(pos))
	{
		if(si->getArt() && (!excludeLocked || !si->locked))
			return si->getArt()->getId();
	}
	return {};
}

ArtifactPosition CArtifactSet::getArtPos(const ArtifactID & aid, bool onlyWorn, bool allowLocked) const
{
	for(const auto & [slot, slotInfo] : artifactsWorn)
	{
		if(slotInfo.getArt()->getTypeId() == aid && (allowLocked || !slotInfo.locked))
			return slot;
	}
	if(!onlyWorn)
	{
		size_t backpackPositionIdx = ArtifactPosition::BACKPACK_START;
		for(const auto & artInfo : artifactsInBackpack)
		{
			const auto art = artInfo.getArt();
			if(art && art->getType()->getId() == aid)
				return ArtifactPosition(backpackPositionIdx);
			backpackPositionIdx++;
		}
	}
	return ArtifactPosition::PRE_FIRST;
}

bool CArtifactSet::hasScroll(const SpellID & spellID, bool onlyWorn) const
{
	return getScrollPos(spellID, onlyWorn) != ArtifactPosition::PRE_FIRST;
}

ArtifactPosition CArtifactSet::getScrollPos(const SpellID & spellID, bool onlyWorn) const
{
	for (const auto & slot : artifactsWorn)
		if (slot.second.getArt() && slot.second.getArt()->isScroll() && slot.second.getArt()->getScrollSpellID() == spellID)
			return slot.first;

	if (!onlyWorn)
	{
		size_t backpackPositionIdx = ArtifactPosition::BACKPACK_START;

		for (const auto & slot : artifactsInBackpack)
		{
			if (slot.getArt() && slot.getArt()->isScroll() && slot.getArt()->getScrollSpellID() == spellID)
				return ArtifactPosition(backpackPositionIdx);
			backpackPositionIdx++;
		}
	}

	return ArtifactPosition::PRE_FIRST;
}

const CArtifactInstance * CArtifactSet::getArtByInstanceId(const ArtifactInstanceID & artInstId) const
{
	for(const auto & i : artifactsWorn)
		if(i.second.getArt()->getId() == artInstId)
			return i.second.getArt();

	for(const auto & i : artifactsInBackpack)
		if(i.getArt()->getId() == artInstId)
			return i.getArt();

	return nullptr;
}

ArtifactPosition CArtifactSet::getArtPos(const CArtifactInstance * artInst) const
{
	if(artInst)
	{
		for(const auto & slot : artInst->getType()->getPossibleSlots().at(bearerType()))
			if(getArt(slot) == artInst)
				return slot;

		ArtifactPosition backpackSlot = ArtifactPosition::BACKPACK_START;
		for(auto & slotInfo : artifactsInBackpack)
		{
			if(slotInfo.getArt() == artInst)
				return backpackSlot;
			backpackSlot = ArtifactPosition(backpackSlot + 1);
		}
	}
	return ArtifactPosition::PRE_FIRST;
}

bool CArtifactSet::hasArt(const ArtifactID & aid, bool onlyWorn, bool searchCombinedParts) const
{
	if(searchCombinedParts && getCombinedArtWithPart(aid))
		return true;
	if(getArtPos(aid, onlyWorn, searchCombinedParts) != ArtifactPosition::PRE_FIRST)
		return true;
	return false;
}

CArtifactSet::ArtPlacementMap CArtifactSet::putArtifact(const ArtifactPosition & slot, const CArtifactInstance * art)
{
	ArtPlacementMap resArtPlacement;
	const auto putToSlot = [this](const ArtifactPosition & targetSlot, const CArtifactInstance * targetArt, bool locked)
	{
		ArtSlotInfo newSlot(targetArt, locked);

		if(targetSlot == ArtifactPosition::TRANSITION_POS)
		{
			artifactsTransitionPos = newSlot;
		}
		else if(ArtifactUtils::isSlotEquipment(targetSlot))
		{
			artifactsWorn.insert_or_assign(targetSlot, newSlot);
		}
		else
		{
			auto position = artifactsInBackpack.begin() + targetSlot - ArtifactPosition::BACKPACK_START;
			artifactsInBackpack.insert(position, newSlot);
		}
	};

	putToSlot(slot, art, false);
	if(art->getType()->isCombined() && ArtifactUtils::isSlotEquipment(slot))
	{
		const CArtifactInstance * mainPart = nullptr;
		for(const auto & part : art->getPartsInfo())
			if(vstd::contains(part.getArtifact()->getType()->getPossibleSlots().at(bearerType()), slot)
			   && (part.slot == ArtifactPosition::PRE_FIRST))
			{
				mainPart = part.getArtifact();
				break;
			}

		for(const auto & part : art->getPartsInfo())
		{
			if(part.getArtifact() != mainPart)
			{
				auto partSlot = part.slot;
				if(!part.getArtifact()->getType()->canBePutAt(this, partSlot))
					partSlot = ArtifactUtils::getArtAnyPosition(this, part.getArtifact()->getTypeId());

				assert(ArtifactUtils::isSlotEquipment(partSlot));
				putToSlot(partSlot, part.getArtifact(), true);
				resArtPlacement.emplace(part.getArtifact(), partSlot);
			}
			else
			{
				resArtPlacement.emplace(part.getArtifact(), part.slot);
			}
		}
	}
	return resArtPlacement;
}

CArtifactSet::CArtifactSet(IGameCallback * cb)
	:artifactsTransitionPos(cb)
{}

void CArtifactSet::removeArtifact(const ArtifactPosition & slot)
{
	const auto eraseArtSlot = [this](const ArtifactPosition & slotForErase)
	{
		if(slotForErase == ArtifactPosition::TRANSITION_POS)
		{
			artifactsTransitionPos.artifactID = {};
		}
		else if(ArtifactUtils::isSlotBackpack(slotForErase))
		{
			auto backpackSlot = ArtifactPosition(slotForErase - ArtifactPosition::BACKPACK_START);

			assert(artifactsInBackpack.begin() + backpackSlot < artifactsInBackpack.end());
			artifactsInBackpack.erase(artifactsInBackpack.begin() + backpackSlot);
		}
		else
		{
			artifactsWorn.erase(slotForErase);
		}
	};

	if(const auto art = getArt(slot, false))
	{
		if(art->isCombined())
		{
			for(const auto & part : art->getPartsInfo())
			{
				if(part.slot != ArtifactPosition::PRE_FIRST)
				{
					assert(getArt(part.slot, false));
					assert(getArt(part.slot, false) == part.getArtifact());
				}
				eraseArtSlot(part.slot);
			}
		}
		eraseArtSlot(slot);
	}
}

const CArtifactInstance * CArtifactSet::getCombinedArtWithPart(const ArtifactID & partId) const
{
	for(const auto & slot : artifactsInBackpack)
	{
		auto art = slot.getArt();
		if(art->isCombined())
		{
			for(auto & ci : art->getPartsInfo())
			{
				if(ci.getArtifact()->getTypeId() == partId)
					return art;
			}
		}
	}
	return nullptr;
}

const ArtSlotInfo * CArtifactSet::getSlot(const ArtifactPosition & pos) const
{
	if(pos == ArtifactPosition::TRANSITION_POS)
		return &artifactsTransitionPos;
	if(vstd::contains(artifactsWorn, pos))
		return &artifactsWorn.at(pos);
	if(ArtifactUtils::isSlotBackpack(pos))
	{
		auto backpackPos = pos - ArtifactPosition::BACKPACK_START;
		if(backpackPos < 0 || backpackPos >= artifactsInBackpack.size())
			return nullptr;
		else
			return &artifactsInBackpack[backpackPos];
	}

	return nullptr;
}

void CArtifactSet::lockSlot(const ArtifactPosition & pos)
{
	if(pos == ArtifactPosition::TRANSITION_POS)
		artifactsTransitionPos.locked = true;
	else if(ArtifactUtils::isSlotEquipment(pos))
		artifactsWorn.at(pos).locked = true;
	else
	{
		assert(artifactsInBackpack.size() > pos - ArtifactPosition::BACKPACK_START);
		(artifactsInBackpack.begin() + pos - ArtifactPosition::BACKPACK_START)->locked = true;
	}
}

bool CArtifactSet::isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck) const
{
	if(bearerType() == ArtBearer::ALTAR)
		return artifactsInBackpack.size() < GameConstants::ALTAR_ARTIFACTS_SLOTS;

	if(const ArtSlotInfo *s = getSlot(pos))
		return (onlyLockCheck || !s->getID().hasValue()) && !s->locked;

	return true; //no slot means not used
}

void CArtifactSet::artDeserializationFix(CGameState & gs, CBonusSystemNode *node)
{
	for(auto & elem : artifactsWorn)
		if(elem.second.artifactID.hasValue() && !elem.second.locked)
			node->attachToSource(*gs.getArtInstance(elem.second.artifactID));
}

void CArtifactSet::serializeJsonArtifacts(JsonSerializeFormat & handler, const std::string & fieldName, CMap * map)
{
	//todo: creature and commander artifacts
	if(handler.saving && artifactsInBackpack.empty() && artifactsWorn.empty())
		return;

	if(!handler.saving)
	{
		artifactsInBackpack.clear();
		artifactsWorn.clear();
	}

	auto s = handler.enterStruct(fieldName);

	switch(bearerType())
	{
		case ArtBearer::HERO:
			serializeJsonHero(handler, map);
			break;
		case ArtBearer::CREATURE:
			serializeJsonCreature(handler);
			break;
		case ArtBearer::COMMANDER:
			serializeJsonCommander(handler);
			break;
		default:
			assert(false);
			break;
	}
}

void CArtifactSet::serializeJsonHero(JsonSerializeFormat & handler, CMap * map)
{
	for(const auto & slot : ArtifactUtils::allWornSlots())
	{
		serializeJsonSlot(handler, slot, map);
	}

	std::vector<ArtifactID> backpackTemp;

	if(handler.saving)
	{
		backpackTemp.reserve(artifactsInBackpack.size());
		for(const ArtSlotInfo & info : artifactsInBackpack)
			backpackTemp.push_back(info.getArt()->getTypeId());
	}
	handler.serializeIdArray(NArtifactPosition::backpack, backpackTemp);
	if(!handler.saving)
	{
		for(const ArtifactID & artifactID : backpackTemp)
		{
			auto * artifact = map->createArtifact(artifactID);
			auto slot = ArtifactPosition::BACKPACK_START + artifactsInBackpack.size();
			if(artifact->getType()->canBePutAt(this, slot))
			{
				auto artsMap = putArtifact(slot, artifact);
				artifact->addPlacementMap(artsMap);
			}
		}
	}
}

void CArtifactSet::serializeJsonCreature(JsonSerializeFormat & handler)
{
	logGlobal->error("CArtifactSet::serializeJsonCreature not implemented");
}

void CArtifactSet::serializeJsonCommander(JsonSerializeFormat & handler)
{
	logGlobal->error("CArtifactSet::serializeJsonCommander not implemented");
}

void CArtifactSet::serializeJsonSlot(JsonSerializeFormat & handler, const ArtifactPosition & slot, CMap * map)
{
	ArtifactID artifactID;

	if(handler.saving)
	{
		const ArtSlotInfo * info = getSlot(slot);

		if(info != nullptr && !info->locked)
		{
			artifactID = info->getArt()->getTypeId();
			handler.serializeId(NArtifactPosition::namesHero.at(slot.num), artifactID, ArtifactID::NONE);
		}
	}
	else
	{
		handler.serializeId(NArtifactPosition::namesHero.at(slot.num), artifactID, ArtifactID::NONE);

		if(artifactID != ArtifactID::NONE)
		{
			auto * artifact = map->createArtifact(artifactID.toEnum());

			if(artifact->getType()->canBePutAt(this, slot))
			{
				auto artsMap = putArtifact(slot, artifact);
				artifact->addPlacementMap(artsMap);
			}
			else
			{
				logGlobal->debug("Artifact can't be put at the specified location."); //TODO add more debugging information
			}
		}
	}
}

VCMI_LIB_NAMESPACE_END

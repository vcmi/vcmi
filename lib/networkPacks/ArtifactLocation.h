/*
 * ArtifactLocation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation
{
	ObjectInstanceID artHolder;
	ArtifactPosition slot;
	std::optional<SlotID> creature;

	ArtifactLocation()
		: artHolder(ObjectInstanceID::NONE)
		, slot(ArtifactPosition::PRE_FIRST)
		, creature(std::nullopt)
	{
	}
	ArtifactLocation(const ObjectInstanceID id, const ArtifactPosition & slot = ArtifactPosition::PRE_FIRST)
		: artHolder(id)
		, slot(slot)
		, creature(std::nullopt)
	{
	}
	ArtifactLocation(const ObjectInstanceID id, const std::optional<SlotID> creatureSlot)
		: artHolder(id)
		, slot(ArtifactPosition::PRE_FIRST)
		, creature(creatureSlot)
	{
	}
	ArtifactLocation(const ObjectInstanceID id, const std::optional<SlotID> creatureSlot, const ArtifactPosition & slot)
		: artHolder(id)
		, slot(slot)
		, creature(creatureSlot)
	{
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & artHolder;
		h & slot;
		h & creature;
	}
};

VCMI_LIB_NAMESPACE_END

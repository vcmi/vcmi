/*
* KeyGuardInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"
#include "../mapObjects/Quest.h"
#include "../texts/TextIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Builds keymaster-gated quest guards/gates (the H3 border guard / border gate):
/// the object's colour is its subtype, so the limiter simply requires the matching key.
/// The per-colour quest is owned here and shared by every instance and its log entry.
template<class ObjectType>
class KeyGuardInstanceConstructor final : public CDefaultObjectTypeHandler<ObjectType>
{
	Quest typeQuest;

	void initTypeData(const JsonNode & input) override
	{
		typeQuest.mission.requiredKeys.push_back(MapObjectSubID(this->getSubIndex()));
		typeQuest.defineQuestName();

		if constexpr (std::is_same_v<ObjectType, QuestGuard>)
		{
			// A border guard is torn down once the matching key is held: model the
			// demolition as a refusable first-visit reward that removes the object.
			// A border gate has no such reward - it simply stays passable.
			Rewardable::VisitInfo vinfo;
			vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			vinfo.reward.removeObject = true;
			vinfo.message.appendTextID("core.advevent", 17);
			typeQuest.reward = vinfo;
		}
	}

	void initializeObject(ObjectType * object) const override
	{
		object->addQuest() = typeQuest;
		if constexpr (std::is_same_v<ObjectType, QuestGuard>)
			object->configuration.canRefuse = true;
	}

	const Quest * getTypeQuest() const override
	{
		return &typeQuest;
	}
};

VCMI_LIB_NAMESPACE_END

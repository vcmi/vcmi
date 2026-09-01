/*
* QuestGuardInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"
#include "../callback/IGameRandomizer.h"
#include "../mapObjects/Quest.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

/// Randomizes a standalone quest guard's reward and rollover text. Keymaster border
/// guards use KeyGuardInstanceConstructor instead and deliberately skip this setup.
class QuestGuardInstanceConstructor final : public CDefaultObjectTypeHandler<QuestGuard>
{
protected:
	void randomizeObject(QuestGuard * object, IGameRandomizer & gameRandomizer) const override
	{
		vstd::RNG & rand = gameRandomizer.getDefault();
		Quest & quest = object->getQuest();

		quest.textOption = rand.nextInt(3, 5);
		quest.completedOption = rand.nextInt(4, 5);
		quest.mission.hasExtraCreatures = !object->allowsFullArmyRemoval();

		Rewardable::VisitInfo vinfo;
		vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		vinfo.reward.removeObject = object->subID.getNum() == 0;
		quest.reward = vinfo;
		object->configuration.canRefuse = true;
	}
};

VCMI_LIB_NAMESPACE_END

/*
 * Trigger.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"
#include "../serializer/Serializeable.h"
#include "BonusEnum.h"

VCMI_LIB_NAMESPACE_BEGIN

using AlternativeEventTypes = std::set<CombatEventType>;

class DLL_LINKAGE Trigger : public Serializeable
{
public:
    Trigger() = default;
	Trigger(std::vector<AlternativeEventTypes> eventSequence, bool oncePerBattle, bool continuous);
	bool triggered(const CombatEventType & eventType);
	void restart();
	template<typename Handler>
	void serialize(Handler & h)
	{
		h & eventSequence;
		h & oncePerBattle;
		h & continuous;
		h & index;
	}
	bool oncePerBattle = false;
	bool continuous = false;
	int index = 0;

private:
	void handleActionTriggered();
	bool isNextEventInSequence(const CombatEventType & eventType);
	std::vector<AlternativeEventTypes> eventSequence;
};

VCMI_LIB_NAMESPACE_END

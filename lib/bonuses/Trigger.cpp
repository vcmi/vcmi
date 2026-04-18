/*
 * Trigger.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "Trigger.h"

VCMI_LIB_NAMESPACE_BEGIN

Trigger::Trigger(std::vector<AlternativeEventTypes> triggerSequence, bool oncePerBattle, bool continuous)
	: eventSequence(triggerSequence), oncePerBattle(oncePerBattle), continuous(continuous)
{
}

bool Trigger::triggered(const CombatEventType & eventType)
{
	if(index == -1 || eventSequence.empty())
		return false;
	if(isNextEventInSequence(eventType))
	{
		if(index == eventSequence.size() - 1)
		{
			handleActionTriggered();
			return true;
		}
		else
			index++;
	}
	else if(continuous)
		restart();
	return false;
}

void Trigger::restart()
{
	index = 0;
}

void Trigger::handleActionTriggered()
{
	if(oncePerBattle)
		index = -1;
	else
		restart();
}

bool Trigger::isNextEventInSequence(const CombatEventType & eventType)
{
	AlternativeEventTypes & currentlyActiveTypes = eventSequence[index];
	return currentlyActiveTypes.contains(eventType);
}

VCMI_LIB_NAMESPACE_END

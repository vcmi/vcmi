/*
 * NewTurnProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/constants/Enumerations.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGTownInstance;
class ResourceSet;
struct SetAvailableCreatures;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

class NewTurnProcessor : boost::noncopyable
{
	CGameHandler * gameHandler;
public:
	NewTurnProcessor(CGameHandler * gameHandler);

	ResourceSet generatePlayerIncome(PlayerColor playerID, bool newWeek);
	SetAvailableCreatures generateTownGrowth(const CGTownInstance * town, EWeekType weekType, CreatureID creatureWeek, bool firstDay);

	void onPlayerTurnStarted(PlayerColor color);
	void onPlayerTurnEnded(PlayerColor color);
};

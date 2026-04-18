/*
 * BattleBonusEffectProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <StdInc.h>
#include <battle/CBattleInfoCallback.h>
#include <bonuses/BonusEnum.h>

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class CGameHandler;

namespace BattleBonusEffectsProcessor
{
void processBattleEventTriggers(
	const CBattleInfoCallback & battle,
	CGameHandler * gameHandler,
	CombatEventType event,
	const CStack * target,
	const CStack * secondary
);
}

VCMI_LIB_NAMESPACE_END

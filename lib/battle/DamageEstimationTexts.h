/*
 * DamageEstimationTexts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IBattleInfoCallback.h"
#include "../texts/MetaString.h"

namespace battle
{
class Unit;
}

/// Status bar texts of attack damage estimates
namespace DamageEstimationTexts
{
/// Text of the base text id, or of its ".1" form for an amount of one, with "%d" replaced by the amount or range
DLL_LINKAGE MetaString plural(DamageRange amount, const std::string & baseTextID);
/// "Attack %CREATURE (%DAMAGE, %KILLS)."
DLL_LINKAGE MetaString meleeAttack(const DamageEstimation & estimation, const battle::Unit & target);
/// "Shoot %CREATURE (%SHOTS, %DAMAGE, %KILLS).", with the target name given as a text id
DLL_LINKAGE MetaString rangedAttack(const DamageEstimation & estimation, const std::string & targetNameTextID, int shotsLeft);
DLL_LINKAGE MetaString rangedAttack(const DamageEstimation & estimation, const battle::Unit & target, int shotsLeft);
/// "May retaliate (%DAMAGE, %KILLS)." or "Will not retaliate."
DLL_LINKAGE MetaString retaliation(const DamageEstimation & estimation, bool mayBeKilled);
/// Text of the "<baseTextID>.<index>" form for the amount of health, with "%d" replaced by the amount
DLL_LINKAGE MetaString healthGain(int64_t amount, const std::string & baseTextID);
}

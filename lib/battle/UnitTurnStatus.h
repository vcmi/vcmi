/*
 * UnitTurnStatus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

namespace battle
{

/// How far a unit is through its turn in the current round
enum class UnitTurnStatus : int8_t
{
	READY, ///< still has to act
	WAITING, ///< postponed its action to the end of the round
	ACTED,
	DEFENDED
};

}

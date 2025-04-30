/*
 * BattleUnitTurnReason.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class BattleUnitTurnReason : int8_t
{
	/// Unit gained turn due to becoming first unit in turn queue
	TURN_QUEUE,
	/// Unit gained turn due to morale triggering
	MORALE,
	/// Unit (re)gained	turn due to hero casting a spell while this unit is active
	HERO_SPELLCAST,
	/// Unit gained turn due to casting a spell while having ability to cast spells without spending turn
	UNIT_SPELLCAST,
	/// Unit gained turn for automatic action, player can not select action for this unit
	AUTOMATIC_ACTION
};

VCMI_LIB_NAMESPACE_END

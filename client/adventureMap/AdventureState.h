/*
 * AdventureState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

enum class EAdventureState
{
	NOT_INITIALIZED,
	HOTSEAT_WAIT,
	MAKING_TURN,
	AI_PLAYER_TURN,
	OTHER_HUMAN_PLAYER_TURN,
	CASTING_SPELL,
	WORLD_VIEW
};

/*
 * ExtraOptionsInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

struct DLL_LINKAGE ExtraOptionsInfo
{
	bool cheatsAllowed = true;
	bool unlimitedReplay = false;
	/// if set, client stores every received netpack on disk so the whole game can be replayed later
	bool recordGame = false;
	/// if set, town popup shows spells of every mage guild level, including levels that are not built yet
	bool revealMageGuildSpells = false;
	/// if set, rewards that are normally hidden (Pandora's Box, creature banks) are shown on right click
	bool revealHiddenRewards = false;

	bool operator == (const ExtraOptionsInfo & other) const = default;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & cheatsAllowed;
		h & unlimitedReplay;
		if(h.hasFeature(Handler::Version::GAME_REPLAY_RECORDING))
			h & recordGame;
		else
			recordGame = false;

		if(h.hasFeature(Handler::Version::REVEAL_EXTRA_OPTIONS))
		{
			h & revealMageGuildSpells;
			h & revealHiddenRewards;
		}
		else
		{
			revealMageGuildSpells = false;
			revealHiddenRewards = false;
		}
	}
};

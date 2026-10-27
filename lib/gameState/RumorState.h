/*
 * RumorState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct DLL_LINKAGE RumorState
{
	enum ERumorType : ui8
	{
		TYPE_NONE = 0, TYPE_RAND, TYPE_SPECIAL, TYPE_MAP, TYPE_MOD
	};

	enum ERumorTypeSpecial : ui8
	{
		RUMOR_OBELISKS = 208,
		RUMOR_ARTIFACTS = 209,
		RUMOR_ARMY = 210,
		RUMOR_INCOME = 211,
		RUMOR_GRAIL = 212
	};

	ERumorType type;
	std::map<ERumorType, std::pair<int, int>> last;
	std::pair<std::string, std::string> lastModRumor;

	RumorState(){type = TYPE_NONE;};
	bool update(int id, int extra);
	bool updateModRumor(const std::string & modID, const std::string & rumorID);

	template <typename Handler> void serialize(Handler &h)
	{
		h & type;
		h & last;

		// Replay pack blobs have no per-pack version. The marker keeps old blobs from reading bytes
		// belonging to the next NewTurn field as mod rumor IDs.
		if (h.hasFeature(Handler::Version::RUMOR_MOD_ID) && last.contains(TYPE_MOD))
			h & lastModRumor;
		else if (!h.saving)
			lastModRumor = {};
	}
};

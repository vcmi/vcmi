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

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE RumorState
{
	enum ERumorType : ui8
	{
		TYPE_NONE = 0, TYPE_RAND, TYPE_SPECIAL, TYPE_MAP
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

	RumorState(){type = TYPE_NONE;};
	bool update(int id, int extra);

	template <typename Handler> void serialize(Handler &h)
	{
		h & type;
		h & last;
	}
};

VCMI_LIB_NAMESPACE_END

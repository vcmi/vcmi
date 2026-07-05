/*
 * RumorState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RumorState.h"

bool RumorState::update(int id, int extra)
{
	if(vstd::contains(last, type))
	{
		if(last[type].first != id)
		{
			last[type].first = id;
			last[type].second = extra;
		}
		else
			return false;
	}
	else
		last[type] = std::make_pair(id, extra);

	return true;
}

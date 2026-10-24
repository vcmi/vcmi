/*
 * Enumerations.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Enumerations.h"

int8_t MasteryLevel::decode(const std::string & identifier)
{
	for (auto const & spellToName : masteryIdToName)
	{
		if (spellToName.second == identifier)
			return spellToName.first;
	}
	return MasteryLevel::ANY;
}

std::string MasteryLevel::encode(const int8_t index)
{
	if (masteryIdToName.contains(index))
		return masteryIdToName.at(index);
	return "ANY";
}
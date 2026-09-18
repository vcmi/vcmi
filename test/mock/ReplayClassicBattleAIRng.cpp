/*
 * ReplayClassicBattleAIRng.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ReplayClassicBattleAIRng.h"

ReplayClassicBattleAIRng::ReplayClassicBattleAIRng(std::vector<int32_t> values)
	: values(std::move(values))
{
}

int32_t ReplayClassicBattleAIRng::nextIntInclusive(int32_t lower, int32_t upper)
{
	if(cursor >= values.size())
		throw std::runtime_error(
			"Classic BattleAI random tape exhausted at request " + std::to_string(cursor)
			+ " [" + std::to_string(lower) + "," + std::to_string(upper) + "]");

	const int32_t result = values[cursor++];
	if(result < lower || result > upper)
	{
		throw std::runtime_error(
			"Classic BattleAI random value " + std::to_string(result)
			+ " is outside requested range [" + std::to_string(lower) + ", " + std::to_string(upper)
			+ "]"
		);
	}
	requests.push_back({lower, upper, result});
	return result;
}

size_t ReplayClassicBattleAIRng::consumed() const
{
	return cursor;
}

bool ReplayClassicBattleAIRng::exhausted() const
{
	return cursor == values.size();
}

const std::vector<ClassicRngRequest> & ReplayClassicBattleAIRng::getRequests() const
{
	return requests;
}

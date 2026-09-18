/*
 * ReplayClassicBattleAIRng.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../AI/BattleAI/Classic/ClassicBattleRng.h"

struct ClassicRngRequest
{
	int32_t lower;
	int32_t upper;
	int32_t value;
};

/// Strict random tape used by deterministic tests.
class ReplayClassicBattleAIRng final : public IClassicBattleAIRng
{
	std::vector<int32_t> values;
	std::vector<ClassicRngRequest> requests;
	size_t cursor = 0;

public:
	explicit ReplayClassicBattleAIRng(std::vector<int32_t> values);
	int32_t nextIntInclusive(int32_t lower, int32_t upper) override;
	size_t consumed() const;
	bool exhausted() const;
	const std::vector<ClassicRngRequest> & getRequests() const;
};

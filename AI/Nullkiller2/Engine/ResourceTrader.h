/*
* ResourceTrader.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*/
#pragma once

#include "Nullkiller.h"

namespace NK2AI
{

class ResourceTrader
{
public:
	// TODO: Mircea: Maybe include based on how close danger is: X as default + proportion of close danger or something around that
	static constexpr float ARMY_GOLD_RATIO_PER_MAKE_TURN_PASS = 0.1f;
	static constexpr float EXPENDABLE_BULK_RATIO = 0.5f;

	static bool trade(BuildAnalyzer & buildAnalyzer, CCallback & cc, const TResources & freeResources);
	static bool tradeHelper(
		float expendableBulkRatio,
		const IMarket & market,
		TResources missingNow,
		TResources income,
		TResources freeAfterMissingTotal,
		const BuildAnalyzer & buildAnalyzer,
		CCallback & cc
	);
};

}

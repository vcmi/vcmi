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
	static bool trade(const std::unique_ptr<BuildAnalyzer> & buildAnalyzer, std::shared_ptr<CCallback> cc, TResources freeResources);
	static bool tradeHelper(
		float EXPENDABLE_BULK_RATIO,
		const IMarket * market,
		TResources missingNow,
		TResources income,
		TResources freeAfterMissingTotal,
		const std::unique_ptr<BuildAnalyzer> & buildAnalyzer,
		std::shared_ptr<CCallback> cc
	);
};

}

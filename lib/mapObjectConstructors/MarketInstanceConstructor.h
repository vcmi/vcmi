/*
* MarketInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"
#include "../mapObjects/CGMarket.h"
#include "../json/JsonNode.h"

class MarketInstanceConstructor : public CDefaultObjectTypeHandler<CGMarket>
{
	std::string descriptionTextID;
	std::string speechTextID;

	std::set<EMarketMode> marketModes;
	JsonNode predefinedOffer;
	int marketEfficiency;
	/// resources that this market can trade. If empty, all resources can be traded
	std::vector<GameResID> tradeableResources;
	/// if set, overrides exchange effectiveness that is normally derived from market efficiency
	double exchangeEffectiveness = 0;

	void initTypeData(const JsonNode & config) override;
public:
	std::shared_ptr<CGMarket> createObject(IGameInfoCallback * cb) const override;
	void randomizeObject(CGMarket * object, IGameRandomizer & gameRandomizer) const override;

	const std::set<EMarketMode> & availableModes() const;
	bool hasDescription() const;
	std::string getDescriptionTextID() const;

	std::string getSpeechTranslated() const;
	int getMarketEfficiency() const;
	const std::vector<GameResID> & getTradeableResources() const;
	double getMarketExchangeEffectiveness() const;
};

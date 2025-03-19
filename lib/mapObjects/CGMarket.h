/*
 * CGMarket.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGObjectInstance.h"
#include "IMarket.h"
#include "../CArtHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class MarketInstanceConstructor;

class DLL_LINKAGE CGMarket : public CGObjectInstance, public IMarket
{
protected:
	std::shared_ptr<MarketInstanceConstructor> getMarketHandler() const;

public:
	CGMarket(IGameCallback *cb);
	///IObjectInterface
	void onHeroVisit(const CGHeroInstance * h) const override; //open trading window
	void initObj(vstd::RNG & rand) override;//set skills for trade

	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;

	///IMarket
	ObjectInstanceID getObjInstanceID() const override;
	int getMarketEfficiency() const override;
	int availableUnits(EMarketMode mode, int marketItemSerial) const override; //-1 if unlimited
	std::set<EMarketMode> availableModes() const override;
};

class DLL_LINKAGE CGBlackMarket : public CGMarket
{
public:
	using CGMarket::CGMarket;

	std::vector<ArtifactID> artifacts; //available artifacts

	void newTurn(vstd::RNG & rand) const override; //reset artifacts for black market every month
	std::vector<TradeItemBuy> availableItemsIds(EMarketMode mode) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGMarket&>(*this);
		h & artifacts;
	}
};

class DLL_LINKAGE CGUniversity : public CGMarket
{
public:
	using CGMarket::CGMarket;

	std::string getSpeechTranslated() const;

	std::vector<TradeItemBuy> skills; //available skills

	std::vector<TradeItemBuy> availableItemsIds(EMarketMode mode) const override;
	void onHeroVisit(const CGHeroInstance * h) const override; //open window

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGMarket&>(*this);
		h & skills;
	}
};

VCMI_LIB_NAMESPACE_END

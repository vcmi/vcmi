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

class DLL_LINKAGE CGMarket : public CGObjectInstance, public IMarket
{
public:
	int marketEfficiency;
	
	CGMarket(IGameCallback *cb);
	///IObjectInterface
	void onHeroVisit(const CGHeroInstance * h) const override; //open trading window
	void initObj(vstd::RNG & rand) override;//set skills for trade

	///IMarket
	ObjectInstanceID getObjInstanceID() const override;
	int getMarketEfficiency() const override;
	int availableUnits(EMarketMode mode, int marketItemSerial) const override; //-1 if unlimited
	std::set<EMarketMode> availableModes() const override;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		if (h.version < Handler::Version::NEW_MARKETS)
		{
			std::set<EMarketMode> marketModes;
			h & marketModes;
		}

		h & marketEfficiency;
		if (h.version < Handler::Version::NEW_MARKETS)
		{
			std::string speech;
			std::string title;
			h & speech;
			h & title;
		}
	}

	template <typename Handler> void serializeArtifactsAltar(Handler &h)
	{
		serialize(h);
		IMarket::serializeArtifactsAltar(h);
	}
};

class DLL_LINKAGE CGBlackMarket : public CGMarket
{
public:
	using CGMarket::CGMarket;

	std::vector<const CArtifact *> artifacts; //available artifacts

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
	std::string speech; //currently shown only in university
	std::string title;

	std::vector<TradeItemBuy> skills; //available skills

	std::vector<TradeItemBuy> availableItemsIds(EMarketMode mode) const override;
	void onHeroVisit(const CGHeroInstance * h) const override; //open window

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGMarket&>(*this);
		h & skills;
		if (h.version >= Handler::Version::NEW_MARKETS)
		{
			h & speech;
			h & title;
		}
	}
};

VCMI_LIB_NAMESPACE_END

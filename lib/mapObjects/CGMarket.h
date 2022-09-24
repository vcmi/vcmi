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

#include "CObjectHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE IMarket
{
public:
	const CGObjectInstance *o;

	IMarket(const CGObjectInstance *O);
	virtual ~IMarket() {}

	virtual int getMarketEfficiency() const =0;
	virtual bool allowsTrade(EMarketMode::EMarketMode mode) const;
	virtual int availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const; //-1 if unlimited
	virtual std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const;

	bool getOffer(int id1, int id2, int &val1, int &val2, EMarketMode::EMarketMode mode) const; //val1 - how many units of id1 player has to give to receive val2 units
	std::vector<EMarketMode::EMarketMode> availableModes() const;

	static const IMarket *castFrom(const CGObjectInstance *obj, bool verbose = true);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & o;
	}
};

class DLL_LINKAGE CGMarket : public CGObjectInstance, public IMarket
{
public:
	CGMarket();
	///IObjectIntercae
	void onHeroVisit(const CGHeroInstance * h) const override; //open trading window

	///IMarket
	int getMarketEfficiency() const override;
	bool allowsTrade(EMarketMode::EMarketMode mode) const override;
	int availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const override; //-1 if unlimited
	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<IMarket&>(*this);
	}
};

class DLL_LINKAGE CGBlackMarket : public CGMarket
{
public:
	std::vector<const CArtifact *> artifacts; //available artifacts

	void newTurn(CRandomGenerator & rand) const override; //reset artifacts for black market every month
	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGMarket&>(*this);
		h & artifacts;
	}
};

class DLL_LINKAGE CGUniversity : public CGMarket
{
public:
	std::vector<int> skills; //available skills

	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const override;
	void initObj(CRandomGenerator & rand) override;//set skills for trade
	void onHeroVisit(const CGHeroInstance * h) const override; //open window

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGMarket&>(*this);
		h & skills;
	}
};

VCMI_LIB_NAMESPACE_END

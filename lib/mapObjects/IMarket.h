/*
 * IMarket.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;

class DLL_LINKAGE IMarket
{
public:
	IMarket();
	virtual ~IMarket() {}

	virtual int getMarketEfficiency() const = 0;
	virtual bool allowsTrade(EMarketMode mode) const;
	virtual int availableUnits(EMarketMode mode, int marketItemSerial) const; //-1 if unlimited
	virtual std::vector<int> availableItemsIds(EMarketMode mode) const;

	bool getOffer(int id1, int id2, int &val1, int &val2, EMarketMode mode) const; //val1 - how many units of id1 player has to give to receive val2 units
	std::vector<EMarketMode> availableModes() const;

	static const IMarket *castFrom(const CGObjectInstance *obj, bool verbose = true);
};

VCMI_LIB_NAMESPACE_END

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

#include "../networkPacks/TradeItem.h"
#include "../constants/Enumerations.h"
#include "../entities/artifact/CArtifactSet.h"

class DLL_LINKAGE IMarket : public virtual Serializeable, boost::noncopyable
{
public:
	explicit IMarket(IGameInfoCallback *cb);
	~IMarket();

	class CArtifactSetAltar : public CArtifactSet
	{
		IGameInfoCallback *cb;
	public:
		CArtifactSetAltar(IGameInfoCallback *cb)
			: CArtifactSet(cb)
			, cb(cb)
		{}

		IGameInfoCallback * getCallback() const override {return cb;};
		ArtBearer bearerType() const override {return ArtBearer::ALTAR;};
	};

	virtual ObjectInstanceID getObjInstanceID() const = 0;	// The market is always an object on the map
	virtual int getMarketEfficiency() const = 0; // marketCount in CGTownInstance::getMarketEfficiency()
	virtual bool allowsTrade(const EMarketMode mode) const;
	virtual int availableUnits(const EMarketMode mode, const int marketItemSerial) const; //-1 if unlimited
	virtual std::vector<TradeItemBuy> availableItemsIds(const EMarketMode mode) const;
	virtual std::set<EMarketMode> availableModes() const = 0;
	/// how effectively resources are exchanged - 1.0 for trade at nominal price of resources, e.g. Warlock's Lab
	/// by default derived from market efficiency and never better than 0.5, giving 2:1 rate for resources of same price
	virtual double getMarketExchangeEffectiveness() const;
	CArtifactSet * getArtifactsStorage() const;
	virtual bool getOffer(int id1, int id2, int &val1, int &val2, EMarketMode mode) const; //val1 - how many units of id1 player has to give to receive val2 units

private:
	std::unique_ptr<CArtifactSetAltar> altarArtifactsStorage;
};

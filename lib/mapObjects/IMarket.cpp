/*
 * IMarket.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "IMarket.h"

#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CGObjectInstance.h"
#include "CObjectHandler.h"

#include "../VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

bool IMarket::getOffer(int id1, int id2, int &val1, int &val2, EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		{
			double effectiveness = std::min((getMarketEfficiency() + 1.0) / 20.0, 0.5);

			double r = VLC->objh->resVals[id1]; //value of given resource
			double g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = static_cast<int>(ceil(r / g));
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = static_cast<int>((g / r) + 0.5);
				val2 = 1;
			}
		}
		break;
	case EMarketMode::CREATURE_RESOURCE:
		{
			const double effectivenessArray[] = {0.0, 0.3, 0.45, 0.50, 0.65, 0.7, 0.85, 0.9, 1.0};
			double effectiveness = effectivenessArray[std::min(getMarketEfficiency(), 8)];

			double r = VLC->creatures()->getByIndex(id1)->getRecruitCost(EGameResID::GOLD); //value of given creature in gold
			double g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = static_cast<int>(ceil(r / g));
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = static_cast<int>((g / r) + 0.5);
				val2 = 1;
			}
		}
		break;
	case EMarketMode::RESOURCE_PLAYER:
		val1 = 1;
		val2 = 1;
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		{
			double effectiveness = std::min((getMarketEfficiency() + 3.0) / 20.0, 0.6);
			double r = VLC->objh->resVals[id1]; //value of offered resource
			double g = VLC->artifacts()->getByIndex(id2)->getPrice() / effectiveness; //value of bought artifact in gold

			if(id1 != 6) //non-gold prices are doubled
				r /= 2;

			val1 = std::max(1, static_cast<int>((g / r) + 0.5)); //don't sell arts for less than 1 resource
			val2 = 1;
		}
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		{
			double effectiveness = std::min((getMarketEfficiency() + 3.0) / 20.0, 0.6);
			double r = VLC->artifacts()->getByIndex(id1)->getPrice() * effectiveness;
			double g = VLC->objh->resVals[id2];

// 			if(id2 != 6) //non-gold prices are doubled
// 				r /= 2;

			val1 = 1;
			val2 = std::max(1, static_cast<int>((r / g) + 0.5)); //at least one resource is given in return
		}
		break;
	case EMarketMode::CREATURE_EXP:
		{
			val1 = 1;
			val2 = (CreatureID(id1).toEntity(VLC)->getAIValue() / 40) * 5;
		}
		break;
	case EMarketMode::ARTIFACT_EXP:
		{
			val1 = 1;

			int givenClass = ArtifactID(id1).toArtifact()->getArtClassSerial();
			if(givenClass < 0 || givenClass > 3)
			{
				val2 = 0;
				return false;
			}

			static constexpr int expPerClass[] = {1000, 1500, 3000, 6000};
			val2 = expPerClass[givenClass];
		}
		break;
	default:
		assert(0);
		return false;
	}

	return true;
}

bool IMarket::allowsTrade(EMarketMode mode) const
{
	return false;
}

int IMarket::availableUnits(EMarketMode mode, int marketItemSerial) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
			return -1;
	default:
			return 1;
	}
}

std::vector<TradeItemBuy> IMarket::availableItemsIds(EMarketMode mode) const
{
	std::vector<TradeItemBuy> ret;
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
		for (auto res : GameResID::ALL_RESOURCES())
			ret.push_back(res);
	}
	return ret;
}

IMarket::IMarket()
{
}

std::vector<EMarketMode> IMarket::availableModes() const
{
	std::vector<EMarketMode> ret;
	for (EMarketMode i = static_cast<EMarketMode>(0); i < EMarketMode::MARKET_AFTER_LAST_PLACEHOLDER; i = vstd::next(i, 1))
	if(allowsTrade(i))
		ret.push_back(i);

	return ret;
}

VCMI_LIB_NAMESPACE_END

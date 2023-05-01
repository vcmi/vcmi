/*
 * CGMarket.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGMarket.h"

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../CCreatureHandler.h"
#include "../CGameState.h"
#include "CGTownInstance.h"
#include "../GameSettings.h"
#include "../CSkillHandler.h"
#include "CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

bool IMarket::getOffer(int id1, int id2, int &val1, int &val2, EMarketMode::EMarketMode mode) const
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
			val2 = (VLC->creh->objects[id1]->getAIValue() / 40) * 5;
		}
		break;
	case EMarketMode::ARTIFACT_EXP:
		{
			val1 = 1;

			int givenClass = VLC->arth->objects[id1]->getArtClassSerial();
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

bool IMarket::allowsTrade(EMarketMode::EMarketMode mode) const
{
	return false;
}

int IMarket::availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const
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

std::vector<int> IMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	std::vector<int> ret;
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
		for (int i = 0; i < 7; i++)
			ret.push_back(i);
	}
	return ret;
}

const IMarket * IMarket::castFrom(const CGObjectInstance *obj, bool verbose)
{
	auto * imarket = dynamic_cast<const IMarket *>(obj);
	if(verbose && !imarket)
		logGlobal->error("Cannot cast to IMarket object type %s", obj->typeName);
	return imarket;
}

IMarket::IMarket()
{
}

std::vector<EMarketMode::EMarketMode> IMarket::availableModes() const
{
	std::vector<EMarketMode::EMarketMode> ret;
	for (int i = 0; i < EMarketMode::MARTKET_AFTER_LAST_PLACEHOLDER; i++)
	if(allowsTrade(static_cast<EMarketMode::EMarketMode>(i)))
		ret.push_back(static_cast<EMarketMode::EMarketMode>(i));

	return ret;
}

void CGMarket::initObj(CRandomGenerator & rand)
{
	VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, rand);
}

void CGMarket::onHeroVisit(const CGHeroInstance * h) const
{
	openWindow(EOpenWindowMode::MARKET_WINDOW, id.getNum(), h->id.getNum());
}

int CGMarket::getMarketEfficiency() const
{
	return marketEfficiency;
}

bool CGMarket::allowsTrade(EMarketMode::EMarketMode mode) const
{
	return marketModes.count(mode);
}

int CGMarket::availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::vector<int> CGMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	if(allowsTrade(mode))
		return IMarket::availableItemsIds(mode);
	return std::vector<int>();
}

CGMarket::CGMarket()
{
}

std::vector<int> CGBlackMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::ARTIFACT_RESOURCE:
		return IMarket::availableItemsIds(mode);
	case EMarketMode::RESOURCE_ARTIFACT:
		{
			std::vector<int> ret;
			for(const CArtifact *a : artifacts)
				if(a)
					ret.push_back(a->getId());
				else
					ret.push_back(-1);
			return ret;
		}
	default:
		return std::vector<int>();
	}
}

void CGBlackMarket::newTurn(CRandomGenerator & rand) const
{
	int resetPeriod = VLC->settings()->getInteger(EGameSettings::MARKETS_BLACK_MARKET_RESTOCK_PERIOD);

	bool isFirstDay = cb->getDate(Date::DAY) == 1;
	bool regularResetTriggered = resetPeriod != 0 && ((cb->getDate(Date::DAY)-1) % resetPeriod) != 0;

	if (!isFirstDay && !regularResetTriggered)
		return;

	SetAvailableArtifacts saa;
	saa.id = id.getNum();
	cb->pickAllowedArtsSet(saa.arts, rand);
	cb->sendAndApply(&saa);
}

void CGUniversity::initObj(CRandomGenerator & rand)
{
	CGMarket::initObj(rand);
	
	std::vector<int> toChoose;
	for(int i = 0; i < VLC->skillh->size(); ++i)
	{
		if(!vstd::contains(skills, i) && cb->isAllowed(2, i))
		{
			toChoose.push_back(i);
		}
	}
}

std::vector<int> CGUniversity::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch (mode)
	{
		case EMarketMode::RESOURCE_SKILL:
			return skills;

		default:
			return std::vector<int>();
	}
}

void CGUniversity::onHeroVisit(const CGHeroInstance * h) const
{
	openWindow(EOpenWindowMode::UNIVERSITY_WINDOW,id.getNum(),h->id.getNum());
}

VCMI_LIB_NAMESPACE_END

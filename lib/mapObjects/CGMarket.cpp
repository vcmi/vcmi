/*
 *
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

///helpers
static void openWindow(const OpenWindow::EWindow type, const int id1, const int id2 = -1)
{
	OpenWindow ow;
	ow.window = type;
	ow.id1 = id1;
	ow.id2 = id2;
	IObjectInterface::cb->sendAndApply(&ow);
}

bool IMarket::getOffer(int id1, int id2, int &val1, int &val2, EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		{
			double effectiveness = std::min((getMarketEfficiency() + 1.0) / 20.0, 0.5);

			double r = VLC->objh->resVals[id1], //value of given resource
				g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = ceil(r / g);
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = (g / r) + 0.5;
				val2 = 1;
			}
		}
		break;
	case EMarketMode::CREATURE_RESOURCE:
		{
			const double effectivenessArray[] = {0.0, 0.3, 0.45, 0.50, 0.65, 0.7, 0.85, 0.9, 1.0};
			double effectiveness = effectivenessArray[std::min(getMarketEfficiency(), 8)];

			double r = VLC->creh->creatures[id1]->cost[6], //value of given creature in gold
				g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = ceil(r / g);
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = (g / r) + 0.5;
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
			double r = VLC->objh->resVals[id1], //value of offered resource
				g = VLC->arth->artifacts[id2]->price / effectiveness; //value of bought artifact in gold

			if(id1 != 6) //non-gold prices are doubled
				r /= 2;

			val1 = std::max(1, (int)((g / r) + 0.5)); //don't sell arts for less than 1 resource
			val2 = 1;
		}
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		{
			double effectiveness = std::min((getMarketEfficiency() + 3.0) / 20.0, 0.6);
			double r = VLC->arth->artifacts[id1]->price * effectiveness,
				g = VLC->objh->resVals[id2];

// 			if(id2 != 6) //non-gold prices are doubled
// 				r /= 2;

			val1 = 1;
			val2 = std::max(1, (int)((r / g) + 0.5)); //at least one resource is given in return
		}
		break;
	case EMarketMode::CREATURE_EXP:
		{
			val1 = 1;
			val2 = (VLC->creh->creatures[id1]->AIValue / 40) * 5;
		}
		break;
	case EMarketMode::ARTIFACT_EXP:
		{
			val1 = 1;

			int givenClass = VLC->arth->artifacts[id1]->getArtClassSerial();
			if(givenClass < 0 || givenClass > 3)
			{
				val2 = 0;
				return false;
			}

			static const int expPerClass[] = {1000, 1500, 3000, 6000};
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

const IMarket * IMarket::castFrom(const CGObjectInstance *obj, bool verbose /*= true*/)
{
	switch(obj->ID)
	{
	case Obj::TOWN:
		return static_cast<const CGTownInstance*>(obj);
	case Obj::ALTAR_OF_SACRIFICE:
	case Obj::BLACK_MARKET:
	case Obj::TRADING_POST:
	case Obj::TRADING_POST_SNOW:
	case Obj::FREELANCERS_GUILD:
		return static_cast<const CGMarket*>(obj);
	case Obj::UNIVERSITY:
		return static_cast<const CGUniversity*>(obj);
	default:
		if(verbose)
			logGlobal->errorStream() << "Cannot cast to IMarket object with ID " << obj->ID;
		return nullptr;
	}
}

IMarket::IMarket(const CGObjectInstance *O)
	:o(O)
{

}

std::vector<EMarketMode::EMarketMode> IMarket::availableModes() const
{
	std::vector<EMarketMode::EMarketMode> ret;
	for (int i = 0; i < EMarketMode::MARTKET_AFTER_LAST_PLACEHOLDER; i++)
		if(allowsTrade((EMarketMode::EMarketMode)i))
			ret.push_back((EMarketMode::EMarketMode)i);

	return ret;
}

void CGMarket::onHeroVisit(const CGHeroInstance * h) const
{
	openWindow(OpenWindow::MARKET_WINDOW,id.getNum(),h->id.getNum());
}

int CGMarket::getMarketEfficiency() const
{
	return 5;
}

bool CGMarket::allowsTrade(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
		switch(ID)
		{
		case Obj::TRADING_POST:
		case Obj::TRADING_POST_SNOW:
			return true;
		default:
			return false;
		}
	case EMarketMode::CREATURE_RESOURCE:
		return ID == Obj::FREELANCERS_GUILD;
	//case ARTIFACT_RESOURCE:
	case EMarketMode::RESOURCE_ARTIFACT:
		return ID == Obj::BLACK_MARKET;
	case EMarketMode::ARTIFACT_EXP:
	case EMarketMode::CREATURE_EXP:
		return ID == Obj::ALTAR_OF_SACRIFICE; //TODO? check here for alignment of visiting hero? - would not be coherent with other checks here
	case EMarketMode::RESOURCE_SKILL:
		return ID == Obj::UNIVERSITY;
	default:
		return false;
	}
}

int CGMarket::availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::vector<int> CGMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
		return IMarket::availableItemsIds(mode);
	default:
		return std::vector<int>();
	}
}

CGMarket::CGMarket()
	:IMarket(this)
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
					ret.push_back(a->id);
				else
					ret.push_back(-1);
			return ret;
		}
	default:
		return std::vector<int>();
	}
}

void CGBlackMarket::newTurn() const
{
	if(cb->getDate(Date::DAY_OF_MONTH) != 1) //new month
		return;

	SetAvailableArtifacts saa;
	saa.id = id.getNum();
	cb->pickAllowedArtsSet(saa.arts);
	cb->sendAndApply(&saa);
}

void CGUniversity::initObj()
{
	std::vector<int> toChoose;
	for(int i = 0; i < GameConstants::SKILL_QUANTITY; ++i)
	{
		if(cb->isAllowed(2, i))
		{
			toChoose.push_back(i);
		}
	}
	if(toChoose.size() < 4)
	{
		logGlobal->warnStream()<<"Warning: less then 4 available skills was found by University initializer!";
		return;
	}

	// get 4 skills
	for(int i = 0; i < 4; ++i)
	{
		// move randomly one skill to selected and remove from list
		auto it = RandomGeneratorUtil::nextItem(toChoose, cb->gameState()->getRandomGenerator());
		skills.push_back(*it);
		toChoose.erase(it);
	}
}

std::vector<int> CGUniversity::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch (mode)
	{
		case EMarketMode::RESOURCE_SKILL:
			return skills;

		default:
			return std::vector <int> ();
	}
}

void CGUniversity::onHeroVisit(const CGHeroInstance * h) const
{
	openWindow(OpenWindow::UNIVERSITY_WINDOW,id.getNum(),h->id.getNum());
}

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
#include "CGTownInstance.h"
#include "../GameSettings.h"
#include "../CSkillHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

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

bool CGMarket::allowsTrade(EMarketMode mode) const
{
	return marketModes.count(mode);
}

int CGMarket::availableUnits(EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::vector<int> CGMarket::availableItemsIds(EMarketMode mode) const
{
	if(allowsTrade(mode))
		return IMarket::availableItemsIds(mode);
	return std::vector<int>();
}

CGMarket::CGMarket()
{
}

std::vector<int> CGBlackMarket::availableItemsIds(EMarketMode mode) const
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

std::vector<int> CGUniversity::availableItemsIds(EMarketMode mode) const
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

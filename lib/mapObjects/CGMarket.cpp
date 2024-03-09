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

#include "../CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../CCreatureHandler.h"
#include "CGTownInstance.h"
#include "../GameSettings.h"
#include "../CSkillHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

void CGMarket::initObj(CRandomGenerator & rand)
{
	getObjectHandler()->configureObject(this, rand);
}

void CGMarket::onHeroVisit(const CGHeroInstance * h) const
{
	cb->showObjectWindow(this, EOpenWindowMode::MARKET_WINDOW, h, true);
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

std::vector<TradeItemBuy> CGMarket::availableItemsIds(EMarketMode mode) const
{
	if(allowsTrade(mode))
		return IMarket::availableItemsIds(mode);
	return std::vector<TradeItemBuy>();
}

CGMarket::CGMarket(IGameCallback *cb):
	CGObjectInstance(cb)
{}

std::vector<TradeItemBuy> CGBlackMarket::availableItemsIds(EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::ARTIFACT_RESOURCE:
		return IMarket::availableItemsIds(mode);
	case EMarketMode::RESOURCE_ARTIFACT:
		{
			std::vector<TradeItemBuy> ret;
			for(const CArtifact *a : artifacts)
				if(a)
					ret.push_back(a->getId());
				else
					ret.push_back(ArtifactID{});
			return ret;
		}
	default:
		return std::vector<TradeItemBuy>();
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
	saa.id = id;
	cb->pickAllowedArtsSet(saa.arts, rand);
	cb->sendAndApply(&saa);
}

std::vector<TradeItemBuy> CGUniversity::availableItemsIds(EMarketMode mode) const
{
	switch (mode)
	{
		case EMarketMode::RESOURCE_SKILL:
			return skills;

		default:
			return std::vector<TradeItemBuy>();
	}
}

void CGUniversity::onHeroVisit(const CGHeroInstance * h) const
{
	cb->showObjectWindow(this, EOpenWindowMode::UNIVERSITY_WINDOW, h, true);
}

ArtBearer::ArtBearer CGArtifactsAltar::bearerType() const
{
	return ArtBearer::ALTAR;
}

VCMI_LIB_NAMESPACE_END

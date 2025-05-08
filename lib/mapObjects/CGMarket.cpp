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

#include "../texts/CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../CCreatureHandler.h"
#include "CGTownInstance.h"
#include "../IGameSettings.h"
#include "../CSkillHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/CommonConstructors.h"
#include "../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

ObjectInstanceID CGMarket::getObjInstanceID() const
{
	return id;
}

void CGMarket::initObj(vstd::RNG & rand)
{
	getObjectHandler()->configureObject(this, rand);
}

void CGMarket::onHeroVisit(const CGHeroInstance * h) const
{
	cb->showObjectWindow(this, EOpenWindowMode::MARKET_WINDOW, h, true);
}

std::string CGMarket::getPopupText(PlayerColor player) const
{
	if (!getMarketHandler()->hasDescription())
		return getHoverText(player);

	MetaString message = MetaString::createFromRawString("{%s}\r\n\r\n%s");
	message.replaceName(ID, subID);
	message.replaceTextID(TextIdentifier(getObjectHandler()->getBaseTextID(), "description").get());
	return message.toString();
}

std::string CGMarket::getPopupText(const CGHeroInstance * hero) const
{
	return getPopupText(hero->getOwner());
}

int CGMarket::getMarketEfficiency() const
{
	return getMarketHandler()->getMarketEfficiency();
}

int CGMarket::availableUnits(EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::shared_ptr<MarketInstanceConstructor> CGMarket::getMarketHandler() const
{
	const auto & baseHandler = getObjectHandler();
	const auto & ourHandler = std::dynamic_pointer_cast<MarketInstanceConstructor>(baseHandler);
	return ourHandler;
}

std::set<EMarketMode> CGMarket::availableModes() const
{
	return getMarketHandler()->availableModes();
}

CGMarket::CGMarket(IGameCallback *cb)
	: CGObjectInstance(cb)
	, IMarket(cb)
{}

std::vector<TradeItemBuy> CGBlackMarket::availableItemsIds(EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_ARTIFACT:
		{
			std::vector<TradeItemBuy> ret;
			for(const auto & a : artifacts)
				ret.push_back(a);
			return ret;
		}
	default:
		return std::vector<TradeItemBuy>();
	}
}

void CGBlackMarket::newTurn(vstd::RNG & rand) const
{
	int resetPeriod = cb->getSettings().getInteger(EGameSettings::MARKETS_BLACK_MARKET_RESTOCK_PERIOD);

	bool isFirstDay = cb->getDate(Date::DAY) == 1;
	bool regularResetTriggered = resetPeriod != 0 && ((cb->getDate(Date::DAY)-1) % resetPeriod) == 0;

	if (!isFirstDay && !regularResetTriggered)
		return;

	SetAvailableArtifacts saa;
	saa.id = id;
	cb->pickAllowedArtsSet(saa.arts, rand);
	cb->sendAndApply(saa);
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

std::string CGUniversity::getSpeechTranslated() const
{
	return getMarketHandler()->getSpeechTranslated();
}

void CGUniversity::onHeroVisit(const CGHeroInstance * h) const
{
	cb->showObjectWindow(this, EOpenWindowMode::UNIVERSITY_WINDOW, h, true);
}

VCMI_LIB_NAMESPACE_END

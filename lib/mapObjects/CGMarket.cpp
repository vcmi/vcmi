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

#include "CGHeroInstance.h"
#include "CGTownInstance.h"

//#include "../CCreatureHandler.h"
#include "../CPlayerState.h"
//#include "../CSkillHandler.h"
#include "../IGameSettings.h"
#include "../callback/IGameEventCallback.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/MarketInstanceConstructor.h"
#include "../networkPacks/PacksForClient.h"
#include "../texts/TextIdentifier.h"

ObjectInstanceID CGMarket::getObjInstanceID() const
{
	return id;
}

void CGMarket::initObj(IGameRandomizer & gameRandomizer)
{
	getObjectHandler()->configureObject(this, gameRandomizer);
}

void CGMarket::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	gameEvents.showObjectWindow(this, EOpenWindowMode::MARKET_WINDOW, h, true);
}

MetaString CGMarket::getPopupText(PlayerColor player) const
{
	const auto handler = getMarketHandler();
	if (!handler || !handler->hasDescription())
		return getHoverText(player);

	MetaString message = MetaString::createFromRawString("{%s}\r\n\r\n%s");
	message.replaceTextID(getObjectNameTextID());
	message.replaceTextID(handler->getDescriptionTextID());
	return message;
}

MetaString CGMarket::getPopupText(const CGHeroInstance * hero) const
{
	return getPopupText(hero->getOwner());
}

int CGMarket::getMarketEfficiency() const
{
	const auto handler = getMarketHandler();
	return handler ? handler->getMarketEfficiency() : 0;
}

int CGMarket::availableUnits(EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::vector<TradeItemBuy> CGMarket::availableItemsIds(EMarketMode mode) const
{
	const auto handler = getMarketHandler();
	if(!handler)
		return {};

	const auto & tradeableResources = handler->getTradeableResources();

	// market that trades only some of the resources, e.g. Warlock's Lab
	if(!tradeableResources.empty() && mode == EMarketMode::RESOURCE_RESOURCE)
		return std::vector<TradeItemBuy>(tradeableResources.begin(), tradeableResources.end());

	return IMarket::availableItemsIds(mode);
}

double CGMarket::getMarketExchangeEffectiveness() const
{
	// market with exchange effectiveness set explicitly in its config, e.g. Warlock's Lab
	const auto handler = getMarketHandler();
	if(double effectiveness = handler ? handler->getMarketExchangeEffectiveness() : 0; effectiveness > 0)
		return effectiveness;

	return IMarket::getMarketExchangeEffectiveness();
}

std::shared_ptr<MarketInstanceConstructor> CGMarket::getMarketHandler() const
{
	// null if object type was changed to non-market since the save was made, e.g. by mod update
	const auto & baseHandler = getObjectHandler();
	const auto & ourHandler = std::dynamic_pointer_cast<MarketInstanceConstructor>(baseHandler);
	return ourHandler;
}

std::set<EMarketMode> CGMarket::availableModes() const
{
	const auto handler = getMarketHandler();
	return handler ? handler->availableModes() : std::set<EMarketMode>();
}

CGMarket::CGMarket(IGameInfoCallback *cb)
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

void CGBlackMarket::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{
	int resetPeriod = cb->getSettings().getInteger(EGameSettings::MARKETS_BLACK_MARKET_RESTOCK_PERIOD);

	int currentDay = cb->getCalendar().getCurrentDay();
	bool isFirstDay = currentDay == 1;
	bool regularResetTriggered = resetPeriod != 0 && ((currentDay-1) % resetPeriod) == 0;

	if (!isFirstDay && !regularResetTriggered)
		return;

	SetAvailableArtifacts saa;
	saa.id = id;
	saa.arts = gameRandomizer.rollMarketArtifactSet();
	gameEvents.sendAndApply(saa);
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
	const auto handler = getMarketHandler();
	return handler ? handler->getSpeechTranslated() : std::string();
}

void CGUniversity::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	ChangeObjectVisitors cow;
	cow.object = id;
	cow.mode = ChangeObjectVisitors::VISITOR_ADD_PLAYER;
	cow.hero = h->id;
	gameEvents.sendAndApply(cow);

	gameEvents.showObjectWindow(this, EOpenWindowMode::UNIVERSITY_WINDOW, h, true);
}

bool CGUniversity::wasVisited (PlayerColor player) const
{
	return cb->getPlayerState(player)->visitedObjects.count(id) != 0;
}

std::vector<Component> CGUniversity::getPopupComponents(PlayerColor player) const
{
	std::vector<Component> result;

	if (!wasVisited(player))
		return result;

	for (auto const & skill : skills)
		result.emplace_back(ComponentType::SEC_SKILL, skill.as<SecondarySkill>());

	return result;
}

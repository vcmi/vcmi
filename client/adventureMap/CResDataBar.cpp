/*
 * CResDataBar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CResDataBar.h"

#include "../CPlayerInterface.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/TextAlignment.h"
#include "../widgets/Images.h"
#include "../widgets/CComponent.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/ResourceSet.h"
#include "../../lib/StartInfo.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/mapObjects/IOwnableObject.h"
#include "../../lib/networkPacks/Component.h"

CResDataBar::CResDataBar(const ImagePath & imageName, const Point & position)
{
	addUsedEvents(SHOW_POPUP);

	pos.x += position.x;
	pos.y += position.y;

	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(imageName, 0, 0);
	background->setPlayerColor(GAME->interface()->playerID);

	pos.w = background->pos.w;
	pos.h = background->pos.h;
}

CResDataBar::CResDataBar(const ImagePath & defname, int x, int y, int offx, int offy, int resdist, int datedist):
	CResDataBar(defname, Point(x,y))
{
	for (int i = 0; i < 7 ; i++)
		resourcePositions[GameResID(i)] = Point( offx + resdist*i, offy );

	datePosition = resourcePositions[EGameResID::GOLD] + Point(datedist, 0);
}

void CResDataBar::setDatePosition(const Point & position)
{
	datePosition = position;
}

void CResDataBar::setResourcePosition(const GameResID & resource, const Point & position)
{
	resourcePositions[resource] = position;
}

std::string CResDataBar::buildDateString()
{
	std::string pattern = "%s: %d, %s: %d, %s: %d";

	auto formatted = boost::format(pattern)
		% LIBRARY->generaltexth->translate("core.genrltxt.62") % GAME->interface()->cb->getDate(Date::MONTH)
		% LIBRARY->generaltexth->translate("core.genrltxt.63") % GAME->interface()->cb->getDate(Date::WEEK)
		% LIBRARY->generaltexth->translate("core.genrltxt.64") % GAME->interface()->cb->getDate(Date::DAY_OF_WEEK);

	return boost::str(formatted);
}

void CResDataBar::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	//TODO: all this should be labels, but they require proper text update on change
	for (auto & entry : resourcePositions)
	{
		std::string text = std::to_string(GAME->interface()->cb->getResourceAmount(entry.first));

		to.drawText(pos.topLeft() + entry.second, FONT_SMALL, Colors::WHITE, ETextAlignment::TOPLEFT, text);
	}

	if (datePosition)
		to.drawText(pos.topLeft() + *datePosition, FONT_SMALL, Colors::WHITE, ETextAlignment::TOPLEFT, buildDateString());
}

void CResDataBar::setPlayerColor(PlayerColor player)
{
	background->setPlayerColor(player);
}

void CResDataBar::showPopupWindow(const Point & cursorPosition)
{
	if((cursorPosition.x - pos.x) > 600)
		return;

	// only daily income
	ResourceSet income;
	auto playerState = GAME->interface()->cb->getPlayerState(GAME->interface()->playerID);
	auto playerSettings = GAME->interface()->cb->getPlayerSettings(GAME->interface()->playerID);
	for(auto & k : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		income += playerState->valOfBonuses(BonusType::RESOURCES_CONSTANT_BOOST, BonusSubtypeID(k));
		income += playerState->valOfBonuses(BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST, BonusSubtypeID(k)) * playerState->getTowns().size();
	}
	TResources incomeHandicapped = income;
	incomeHandicapped.applyHandicap(playerSettings->handicap.percentIncome);
	for(auto & mapObject : playerState->getOwnedObjects())
		incomeHandicapped += mapObject->asOwnable()->dailyIncome();

	std::vector<std::shared_ptr<CComponent>> comp;
	for (auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		std::string text = std::to_string(GAME->interface()->cb->getResourceAmount(i));
		if(incomeHandicapped[i])
			text += "\n{lightgreen|(+" + std::to_string(incomeHandicapped[i]) + ")}";
		comp.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, i, text));
	}

	CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("core.genrltxt.270"), comp);
}

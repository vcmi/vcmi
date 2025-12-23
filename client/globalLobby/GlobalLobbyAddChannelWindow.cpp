/*
 * GlobalLobbyAddChannelWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyAddChannelWindow.h"

#include "GlobalLobbyClient.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"

#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/Languages.h"

GlobalLobbyAddChannelWindowCard::GlobalLobbyAddChannelWindowCard(const std::string & languageID)
	: languageID(languageID)
{
	pos.w = 200;
	pos.h = 40;
	addUsedEvents(LCLICK);

	OBJECT_CONSTRUCTION;
	const auto & language = Languages::getLanguageOptions(languageID);

	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);
	labelNameNative = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, language.nameNative);

	if (language.nameNative != language.nameEnglish)
		labelNameTranslated = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, language.nameEnglish);
}

void GlobalLobbyAddChannelWindowCard::clickPressed(const Point & cursorPosition)
{
	GAME->server().getGlobalLobby().addChannel(languageID);
	ENGINE->windows().popWindows(1);
}

GlobalLobbyAddChannelWindow::GlobalLobbyAddChannelWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 236;
	pos.h = 420;

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));
	labelTitle = std::make_shared<CLabel>(
		pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, MetaString::createFromTextID("vcmi.lobby.channel.add").toString()
		);

	const auto & allLanguages = Languages::getLanguageList();
	std::vector<std::string> newLanguages;
	for (const auto & language : allLanguages)
		if (!vstd::contains(GAME->server().getGlobalLobby().getActiveChannels(), language.identifier))
			newLanguages.push_back(language.identifier);

	const auto & createChannelCardCallback = [newLanguages](size_t index) -> std::shared_ptr<CIntObject>
	{
		if(index < newLanguages.size())
			return std::make_shared<GlobalLobbyAddChannelWindowCard>(newLanguages[index]);
		return std::make_shared<CIntObject>();
	};

	listBackground = std::make_shared<TransparentFilledRectangle>(Rect(8, 48, 220, 324), ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1);
	languageList = std::make_shared<CListBox>(createChannelCardCallback, Point(10, 50), Point(0, 40), 8, newLanguages.size(), 0, 1 | 4, Rect(200, 0, 320, 320));
	languageList->setRedrawParent(true);

	buttonClose = std::make_shared<CButton>(Point(86, 384), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this]() { close(); }, EShortcut::GLOBAL_RETURN );

	center();
}

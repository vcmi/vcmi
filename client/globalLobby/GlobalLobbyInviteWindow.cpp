/*
 * GlobalLobbyInviteWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyInviteWindow.h"

#include "GlobalLobbyClient.h"

#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"

#include "../../lib/MetaString.h"
#include "../../lib/json/JsonNode.h"

GlobalLobbyInviteAccountCard::GlobalLobbyInviteAccountCard(const GlobalLobbyAccount & accountDescription)
	: accountID(accountDescription.accountID)
{
	pos.w = 200;
	pos.h = 40;
	addUsedEvents(LCLICK);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);
	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, accountDescription.displayName);
	labelStatus = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, accountDescription.status);
}

void GlobalLobbyInviteAccountCard::clickPressed(const Point & cursorPosition)
{
	JsonNode message;
	message["type"].String() = "sendInvite";
	message["accountID"].String() = accountID;

	CSH->getGlobalLobby().sendMessage(message);
}

GlobalLobbyInviteWindow::GlobalLobbyInviteWindow()
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.w = 236;
	pos.h = 400;

	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	filledBackground->playerColored(PlayerColor(1));
	labelTitle = std::make_shared<CLabel>(
		pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, MetaString::createFromTextID("vcmi.lobby.invite.header").toString()
	);

	const auto & createAccountCardCallback = [](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & accounts = CSH->getGlobalLobby().getActiveAccounts();

		if(index < accounts.size())
			return std::make_shared<GlobalLobbyInviteAccountCard>(accounts[index]);
		return std::make_shared<CIntObject>();
	};

	listBackground = std::make_shared<TransparentFilledRectangle>(Rect(8, 48, 220, 304), ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1);
	accountList = std::make_shared<CListBox>(createAccountCardCallback, Point(10, 50), Point(0, 40), 8, 0, 0, 1 | 4, Rect(200, 0, 300, 300));

	buttonClose = std::make_shared<CButton>(Point(86, 364), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this]() { close(); } );

	center();
}

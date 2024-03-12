/*
 * GlobalLobbyWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyWidget.h"

#include "GlobalLobbyClient.h"
#include "GlobalLobbyWindow.h"

#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"

#include "../../lib/MetaString.h"

GlobalLobbyWidget::GlobalLobbyWidget(GlobalLobbyWindow * window)
	: window(window)
{
	addCallback("closeWindow", [](int) { GH.windows().popWindows(1); });
	addCallback("sendMessage", [this](int) { this->window->doSendChatMessage(); });
	addCallback("createGameRoom", [this](int) { if (!CSH->inGame()) this->window->doCreateGameRoom(); else GH.windows().popWindows(1); });

	REGISTER_BUILDER("accountList", &GlobalLobbyWidget::buildAccountList);
	REGISTER_BUILDER("roomList", &GlobalLobbyWidget::buildRoomList);

	const JsonNode config(JsonPath::builtin("config/widgets/lobbyWindow.json"));
	build(config);
}

std::shared_ptr<CIntObject> GlobalLobbyWidget::buildAccountList(const JsonNode & config) const
{
	const auto & createCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & accounts = CSH->getGlobalLobby().getActiveAccounts();

		if(index < accounts.size())
			return std::make_shared<GlobalLobbyAccountCard>(this->window, accounts[index]);
		return std::make_shared<CIntObject>();
	};

	auto position = readPosition(config["position"]);
	auto itemOffset = readPosition(config["itemOffset"]);
	auto sliderPosition = readPosition(config["sliderPosition"]);
	auto sliderSize = readPosition(config["sliderSize"]);
	size_t visibleAmount = config["visibleAmount"].Integer();
	size_t totalAmount = 0; // Will be set later, on server netpack
	int sliderMode = 1 | 4; //  present, vertical, blue
	int initialPos = 0;

	return std::make_shared<CListBox>(createCallback, position, itemOffset, visibleAmount, totalAmount, initialPos, sliderMode, Rect(sliderPosition, sliderSize));
}

std::shared_ptr<CIntObject> GlobalLobbyWidget::buildRoomList(const JsonNode & config) const
{
	const auto & createCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & rooms = CSH->getGlobalLobby().getActiveRooms();

		if(index < rooms.size())
			return std::make_shared<GlobalLobbyRoomCard>(this->window, rooms[index]);
		return std::make_shared<CIntObject>();
	};

	auto position = readPosition(config["position"]);
	auto itemOffset = readPosition(config["itemOffset"]);
	auto sliderPosition = readPosition(config["sliderPosition"]);
	auto sliderSize = readPosition(config["sliderSize"]);
	size_t visibleAmount = config["visibleAmount"].Integer();
	size_t totalAmount = 0; // Will be set later, on server netpack
	int sliderMode = 1 | 4; //  present, vertical, blue
	int initialPos = 0;

	return std::make_shared<CListBox>(createCallback, position, itemOffset, visibleAmount, totalAmount, initialPos, sliderMode, Rect(sliderPosition, sliderSize));
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getAccountNameLabel()
{
	return widget<CLabel>("accountNameLabel");
}

std::shared_ptr<CTextInput> GlobalLobbyWidget::getMessageInput()
{
	return widget<CTextInput>("messageInput");
}

std::shared_ptr<CTextBox> GlobalLobbyWidget::getGameChat()
{
	return widget<CTextBox>("gameChat");
}

std::shared_ptr<CListBox> GlobalLobbyWidget::getAccountList()
{
	return widget<CListBox>("accountList");
}

std::shared_ptr<CListBox> GlobalLobbyWidget::getRoomList()
{
	return widget<CListBox>("roomList");
}

GlobalLobbyAccountCard::GlobalLobbyAccountCard(GlobalLobbyWindow * window, const GlobalLobbyAccount & accountDescription)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.w = 130;
	pos.h = 40;

	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));
	labelName = std::make_shared<CLabel>(5, 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, accountDescription.displayName);
	labelStatus = std::make_shared<CLabel>(5, 20, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, accountDescription.status);
}

GlobalLobbyRoomCard::GlobalLobbyRoomCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & roomDescription)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const auto & onJoinClicked = [window, roomID = roomDescription.gameRoomID]()
	{
		window->doJoinRoom(roomID);
	};

	auto roomSizeText = MetaString::createFromRawString("%d/%d");
	roomSizeText.replaceNumber(roomDescription.playersCount);
	roomSizeText.replaceNumber(roomDescription.playerLimit);

	auto roomStatusText = MetaString::createFromTextID("vcmi.lobby.room.state." + roomDescription.statusID);

	pos.w = 230;
	pos.h = 40;

	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));
	labelName = std::make_shared<CLabel>(5, 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, roomDescription.hostAccountDisplayName);
	labelDescription = std::make_shared<CLabel>(5, 20, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, roomDescription.description);
	labelRoomSize = std::make_shared<CLabel>(178, 2, FONT_SMALL, ETextAlignment::TOPRIGHT, Colors::YELLOW, roomSizeText.toString());
	labelRoomStatus = std::make_shared<CLabel>(190, 20, FONT_SMALL, ETextAlignment::TOPRIGHT, Colors::YELLOW, roomStatusText.toString());
	iconRoomSize = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconPlayer"), Point(180, 5));

	if(!CSH->inGame() && roomDescription.statusID == "public")
	{
		buttonJoin = std::make_shared<CButton>(Point(194, 4), AnimationPath::builtin("lobbyJoinRoom"), CButton::tooltip(), onJoinClicked);
		buttonJoin->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/iconEnter")));
	}
}

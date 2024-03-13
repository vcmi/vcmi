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

#include "../../lib/Languages.h"
#include "../../lib/MetaString.h"

GlobalLobbyWidget::GlobalLobbyWidget(GlobalLobbyWindow * window)
	: window(window)
{
	addCallback("closeWindow", [](int) { GH.windows().popWindows(1); });
	addCallback("sendMessage", [this](int) { this->window->doSendChatMessage(); });
	addCallback("createGameRoom", [this](int) { if (!CSH->inGame()) this->window->doCreateGameRoom(); });//TODO: button should be blocked instead

	REGISTER_BUILDER("lobbyItemList", &GlobalLobbyWidget::buildItemList);

	const JsonNode config(JsonPath::builtin("config/widgets/lobbyWindow.json"));
	build(config);
}

GlobalLobbyWidget::CreateFunc GlobalLobbyWidget::getItemListConstructorFunc(const std::string & callbackName) const
{
	const auto & createAccountCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & accounts = CSH->getGlobalLobby().getActiveAccounts();

		if(index < accounts.size())
			return std::make_shared<GlobalLobbyAccountCard>(this->window, accounts[index]);
		return std::make_shared<CIntObject>();
	};

	const auto & createRoomCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & rooms = CSH->getGlobalLobby().getActiveRooms();

		if(index < rooms.size())
			return std::make_shared<GlobalLobbyRoomCard>(this->window, rooms[index]);
		return std::make_shared<CIntObject>();
	};

	const auto & createChannelCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & channels = CSH->getGlobalLobby().getActiveChannels();

		if(index < channels.size())
			return std::make_shared<GlobalLobbyChannelCard>(this->window, channels[index]);
		return std::make_shared<CIntObject>();
	};

	const auto & createMatchCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & matches = CSH->getGlobalLobby().getMatchesHistory();

		if(index < matches.size())
			return std::make_shared<GlobalLobbyMatchCard>(this->window, matches[index]);
		return std::make_shared<CIntObject>();
	};

	if (callbackName == "room")
		return createRoomCardCallback;

	if (callbackName == "account")
		return createAccountCardCallback;

	if (callbackName == "channel")
		return createChannelCardCallback;

	if (callbackName == "match")
		return createMatchCardCallback;

	throw std::runtime_error("Unknown item type in lobby widget constructor: " + callbackName);
}

std::shared_ptr<CIntObject> GlobalLobbyWidget::buildItemList(const JsonNode & config) const
{
	auto callback = getItemListConstructorFunc(config["itemType"].String());
	auto position = readPosition(config["position"]);
	auto itemOffset = readPosition(config["itemOffset"]);
	auto sliderPosition = readPosition(config["sliderPosition"]);
	auto sliderSize = readPosition(config["sliderSize"]);
	size_t visibleAmount = config["visibleAmount"].Integer();
	size_t totalAmount = 0; // Will be set later, on server netpack
	int sliderMode = config["sliderSize"].isNull() ? 0 : (1 | 4); //  present, vertical, blue
	int initialPos = 0;

	return std::make_shared<CListBox>(callback, position, itemOffset, visibleAmount, totalAmount, initialPos, sliderMode, Rect(sliderPosition, sliderSize));
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

GlobalLobbyChannelCardBase::GlobalLobbyChannelCardBase(GlobalLobbyWindow * window, const std::string & channelType, const std::string & channelName)
	: window(window)
	, channelType(channelType)
	, channelName(channelName)
{
	addUsedEvents(LCLICK);
}

void GlobalLobbyChannelCardBase::clickPressed(const Point & cursorPosition)
{
	window->doOpenChannel(channelType, channelName);
}

GlobalLobbyAccountCard::GlobalLobbyAccountCard(GlobalLobbyWindow * window, const GlobalLobbyAccount & accountDescription)
	: GlobalLobbyChannelCardBase(window, "player", accountDescription.accountID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.w = 130;
	pos.h = 40;

	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));
	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, accountDescription.displayName);
	labelStatus = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, accountDescription.status);
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
	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, roomDescription.hostAccountDisplayName);
	labelDescription = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, roomDescription.description);
	labelRoomSize = std::make_shared<CLabel>(178, 10, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, roomSizeText.toString());
	labelRoomStatus = std::make_shared<CLabel>(190, 30, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, roomStatusText.toString());
	iconRoomSize = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconPlayer"), Point(180, 5));

	if(!CSH->inGame() && roomDescription.statusID == "public")
	{
		buttonJoin = std::make_shared<CButton>(Point(194, 4), AnimationPath::builtin("lobbyJoinRoom"), CButton::tooltip(), onJoinClicked);
		buttonJoin->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/iconEnter")));
	}
}

GlobalLobbyChannelCard::GlobalLobbyChannelCard(GlobalLobbyWindow * window, const std::string & channelName)
	: GlobalLobbyChannelCardBase(window, "global", channelName)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.w = 146;
	pos.h = 40;

	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));
	labelName = std::make_shared<CLabel>(5, 20, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, Languages::getLanguageOptions(channelName).nameNative);
}

GlobalLobbyMatchCard::GlobalLobbyMatchCard(GlobalLobbyWindow * window, const GlobalLobbyHistoryMatch & matchDescription)
	: GlobalLobbyChannelCardBase(window, "match", matchDescription.gameRoomID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.w = 130;
	pos.h = 40;

	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));
	labelMatchDate = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, matchDescription.startDateFormatted);

	MetaString opponentDescription;

	if (matchDescription.opponentDisplayNames.empty())
		opponentDescription.appendRawString("Solo game"); // or "Singleplayer game" is better?

	if (matchDescription.opponentDisplayNames.size() == 1)
		opponentDescription.appendRawString(matchDescription.opponentDisplayNames[0]);

	if (matchDescription.opponentDisplayNames.size() > 1)
	{
		opponentDescription.appendRawString("%d players");
		opponentDescription.replaceNumber(matchDescription.opponentDisplayNames.size());
	}

	labelMatchOpponent = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, opponentDescription.toString());
}

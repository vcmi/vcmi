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

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"

#include "../../lib/CConfigHandler.h"
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

	auto result = std::make_shared<CListBox>(callback, position, itemOffset, visibleAmount, totalAmount, initialPos, sliderMode, Rect(sliderPosition, sliderSize));

	result->setRedrawParent(true);
	return result;
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

std::shared_ptr<CListBox> GlobalLobbyWidget::getChannelList()
{
	return widget<CListBox>("channelList");
}

std::shared_ptr<CListBox> GlobalLobbyWidget::getMatchList()
{
	return widget<CListBox>("matchList");
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getGameChatHeader()
{
	return widget<CLabel>("headerGameChat");
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getAccountListHeader()
{
	return widget<CLabel>("headerAccountList");
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getRoomListHeader()
{
	return widget<CLabel>("headerRoomList");
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getChannelListHeader()
{
	return widget<CLabel>("headerChannelList");
}

std::shared_ptr<CLabel> GlobalLobbyWidget::getMatchListHeader()
{
	return widget<CLabel>("headerMatchList");
}

GlobalLobbyChannelCardBase::GlobalLobbyChannelCardBase(GlobalLobbyWindow * window, const Point & dimensions, const std::string & channelType, const std::string & channelName, const std::string & channelDescription)
	: window(window)
	, channelType(channelType)
	, channelName(channelName)
	, channelDescription(channelDescription)
{
	pos.w = dimensions.x;
	pos.h = dimensions.y;
	addUsedEvents(LCLICK);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	if (window->isChannelOpen(channelType, channelName))
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), Colors::YELLOW, 2);
	else if (window->isChannelUnread(channelType, channelName))
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), Colors::WHITE, 1);
	else
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);
}

void GlobalLobbyChannelCardBase::clickPressed(const Point & cursorPosition)
{
	CCS->soundh->playSound(soundBase::button);
	window->doOpenChannel(channelType, channelName, channelDescription);
}

GlobalLobbyAccountCard::GlobalLobbyAccountCard(GlobalLobbyWindow * window, const GlobalLobbyAccount & accountDescription)
	: GlobalLobbyChannelCardBase(window, Point(130, 40), "player", accountDescription.accountID, accountDescription.displayName)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
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

	bool publicRoom = roomDescription.statusID == "public";
	bool hasInvite = CSH->getGlobalLobby().isInvitedToRoom(roomDescription.gameRoomID);
	bool canJoin = publicRoom || hasInvite;

	auto roomSizeText = MetaString::createFromRawString("%d/%d");
	roomSizeText.replaceNumber(roomDescription.participants.size());
	roomSizeText.replaceNumber(roomDescription.playerLimit);

	MetaString roomStatusText;
	if (roomDescription.statusID == "private" && hasInvite)
		roomStatusText.appendTextID("vcmi.lobby.room.state.invited");
	else
		roomStatusText.appendTextID("vcmi.lobby.room.state." + roomDescription.statusID);

	pos.w = 230;
	pos.h = 40;

	if (window->isInviteUnread(roomDescription.gameRoomID))
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), Colors::WHITE, 1);
	else
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);

	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, roomDescription.hostAccountDisplayName);
	labelDescription = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, roomDescription.description);
	labelRoomSize = std::make_shared<CLabel>(178, 10, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, roomSizeText.toString());
	labelRoomStatus = std::make_shared<CLabel>(190, 30, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, roomStatusText.toString());
	iconRoomSize = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconPlayer"), Point(180, 5));

	if(!CSH->inGame() && canJoin)
	{
		buttonJoin = std::make_shared<CButton>(Point(194, 4), AnimationPath::builtin("lobbyJoinRoom"), CButton::tooltip(), onJoinClicked);
		buttonJoin->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/iconEnter")));
	}
}

GlobalLobbyChannelCard::GlobalLobbyChannelCard(GlobalLobbyWindow * window, const std::string & channelName)
	: GlobalLobbyChannelCardBase(window, Point(146, 40), "global", channelName, Languages::getLanguageOptions(channelName).nameNative)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	labelName = std::make_shared<CLabel>(5, 20, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, Languages::getLanguageOptions(channelName).nameNative);
}

GlobalLobbyMatchCard::GlobalLobbyMatchCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & matchDescription)
	: GlobalLobbyChannelCardBase(window, Point(130, 40), "match", matchDescription.gameRoomID, matchDescription.startDateFormatted)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	labelMatchDate = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, matchDescription.startDateFormatted);

	MetaString opponentDescription;

	if (matchDescription.participants.size() == 1)
		opponentDescription.appendTextID("vcmi.lobby.match.solo");

	if (matchDescription.participants.size() == 2)
	{
		std::string ourAccountID = CSH->getGlobalLobby().getAccountID();

		opponentDescription.appendTextID("vcmi.lobby.match.duel");
		// Find display name of our one and only opponent in this game
		if (matchDescription.participants[0].accountID == ourAccountID)
			opponentDescription.replaceRawString(matchDescription.participants[1].displayName);
		else
			opponentDescription.replaceRawString(matchDescription.participants[0].displayName);
	}

	if (matchDescription.participants.size() > 2)
	{
		opponentDescription.appendTextID("vcmi.lobby.match.multi");
		opponentDescription.replaceNumber(matchDescription.participants.size());
	}

	labelMatchOpponent = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, opponentDescription.toString());
}

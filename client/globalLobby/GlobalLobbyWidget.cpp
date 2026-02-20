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

#include "GlobalLobbyAddChannelWindow.h"
#include "GlobalLobbyClient.h"
#include "GlobalLobbyRoomWindow.h"
#include "GlobalLobbyWindow.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../media/ISoundPlayer.h"
#include "../render/Colors.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/texts/MetaString.h"

GlobalLobbyWidget::GlobalLobbyWidget(GlobalLobbyWindow * window)
	: window(window)
{
	addCallback("closeWindow", [](int) { ENGINE->windows().popWindows(1); });
	addCallback("sendMessage", [this](int) { this->window->doSendChatMessage(); });
	addCallback("createGameRoom", [this](int) { if (!GAME->server().inGame()) this->window->doCreateGameRoom(); });//TODO: button should be blocked instead

	REGISTER_BUILDER("lobbyItemList", &GlobalLobbyWidget::buildItemList);

	const JsonNode config(JsonPath::builtin(ENGINE->screenDimensions().x >= 1024 ? "config/widgets/lobbyWindowWide.json" : "config/widgets/lobbyWindow.json"));
	build(config);
}

GlobalLobbyWidget::CreateFunc GlobalLobbyWidget::getItemListConstructorFunc(const std::string & callbackName) const
{
	const auto & createAccountCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & accounts = GAME->server().getGlobalLobby().getActiveAccounts();

		if(index < accounts.size())
			return std::make_shared<GlobalLobbyAccountCard>(this->window, accounts[index]);
		return std::make_shared<CIntObject>();
	};

	const auto & createRoomCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & rooms = GAME->server().getGlobalLobby().getActiveRooms();

		if(index < rooms.size())
			return std::make_shared<GlobalLobbyRoomCard>(this->window, rooms[index]);
		return std::make_shared<CIntObject>();
	};

	const auto & createChannelCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & channels = GAME->server().getGlobalLobby().getActiveChannels();

		if(index < channels.size())
			return std::make_shared<GlobalLobbyChannelCard>(this->window, channels[index]);

		if(index == channels.size())
		{
			const auto buttonCallback = [](){
				ENGINE->windows().createAndPushWindow<GlobalLobbyAddChannelWindow>();
			};

			auto result = std::make_shared<CButton>(Point(0,0), AnimationPath::builtin("lobbyAddChannel"), CButton::tooltip(), buttonCallback);
			result->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/addChannel")));
			return result;
		}

		return std::make_shared<CIntObject>();
	};

	const auto & createMatchCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		const auto & matches = GAME->server().getGlobalLobby().getMatchesHistory();

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

	if (result->getSlider())
	{
		Point scrollBoundsDimensions(sliderPosition.x + result->getSlider()->pos.w, result->getSlider()->pos.h);
		Point scrollBoundsOffset = -sliderPosition;

		result->getSlider()->setScrollBounds(Rect(scrollBoundsOffset, scrollBoundsDimensions));
		result->getSlider()->setPanningStep(itemOffset.length());
	}

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

	OBJECT_CONSTRUCTION;

	if (window->isChannelOpen(channelType, channelName))
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), Colors::YELLOW, 2);
	else if (window->isChannelUnread(channelType, channelName))
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), Colors::WHITE, 1);
	else
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);
}

void GlobalLobbyChannelCardBase::clickPressed(const Point & cursorPosition)
{
	ENGINE->sound().playSound(soundBase::button);
	window->doOpenChannel(channelType, channelName, channelDescription);
}

GlobalLobbyAccountCard::GlobalLobbyAccountCard(GlobalLobbyWindow * window, const GlobalLobbyAccount & accountDescription)
	: GlobalLobbyChannelCardBase(window, Point(130, 40), "player", accountDescription.accountID, accountDescription.displayName)
{
	OBJECT_CONSTRUCTION;
	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, accountDescription.displayName, 120);
	labelStatus = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, accountDescription.status);
}

GlobalLobbyRoomCard::GlobalLobbyRoomCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & roomDescription)
	: window(window)
	, roomUUID(roomDescription.gameRoomID)
{
	OBJECT_CONSTRUCTION;
	addUsedEvents(LCLICK);

	bool hasInvite = GAME->server().getGlobalLobby().isInvitedToRoom(roomDescription.gameRoomID);

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

	if (hasInvite)
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), Colors::WHITE, 1);
	else
		backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);

	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, roomDescription.hostAccountDisplayName, 180);
	labelDescription = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, roomDescription.description, 160);
	labelRoomSize = std::make_shared<CLabel>(212, 10, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, roomSizeText.toString());
	labelRoomStatus = std::make_shared<CLabel>(225, 30, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, roomStatusText.toString());
	iconRoomSize = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconPlayer"), Point(214, 5));
}

void GlobalLobbyRoomCard::clickPressed(const Point & cursorPosition)
{
	ENGINE->windows().createAndPushWindow<GlobalLobbyRoomWindow>(window, roomUUID);
}

GlobalLobbyChannelCard::GlobalLobbyChannelCard(GlobalLobbyWindow * window, const std::string & channelName)
	: GlobalLobbyChannelCardBase(window, Point(146, 40), "global", channelName, Languages::getLanguageOptions(channelName).nameNative)
{
	OBJECT_CONSTRUCTION;
	labelName = std::make_shared<CLabel>(5, 20, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, Languages::getLanguageOptions(channelName).nameNative);

	if (GAME->server().getGlobalLobby().getActiveChannels().size() > 1)
	{
		pos.w = 110;
		buttonClose = std::make_shared<CButton>(Point(113, 7), AnimationPath::builtin("lobbyCloseChannel"), CButton::tooltip(), [channelName](){GAME->server().getGlobalLobby().closeChannel(channelName);});
		buttonClose->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/closeChannel")));
	}
}

GlobalLobbyMatchCard::GlobalLobbyMatchCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & matchDescription)
	: GlobalLobbyChannelCardBase(window, Point(130, 40), "match", matchDescription.gameRoomID, matchDescription.startDateFormatted)
{
	OBJECT_CONSTRUCTION;
	labelMatchDate = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, matchDescription.startDateFormatted);

	MetaString opponentDescription;

	if (matchDescription.participants.size() == 1)
		opponentDescription.appendTextID("vcmi.lobby.match.solo");

	if (matchDescription.participants.size() == 2)
	{
		std::string ourAccountID = GAME->server().getGlobalLobby().getAccountID();

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

	labelMatchOpponent = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, opponentDescription.toString(), 120);
}

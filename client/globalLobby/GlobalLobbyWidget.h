/*
 * GlobalLobbyWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/InterfaceObjectConfigurable.h"

class GlobalLobbyWindow;
struct GlobalLobbyAccount;
struct GlobalLobbyRoom;
class CListBox;

class GlobalLobbyWidget : public InterfaceObjectConfigurable
{
	GlobalLobbyWindow * window;

	using CreateFunc = std::function<std::shared_ptr<CIntObject>(size_t)>;

	std::shared_ptr<CIntObject> buildItemList(const JsonNode &) const;
	CreateFunc getItemListConstructorFunc(const std::string & callbackName) const;

public:
	explicit GlobalLobbyWidget(GlobalLobbyWindow * window);

	std::shared_ptr<CLabel> getAccountNameLabel();
	std::shared_ptr<CTextInput> getMessageInput();
	std::shared_ptr<CTextBox> getGameChat();
	std::shared_ptr<CListBox> getAccountList();
	std::shared_ptr<CListBox> getRoomList();
	std::shared_ptr<CListBox> getChannelList();
	std::shared_ptr<CListBox> getMatchList();
};

class GlobalLobbyChannelCardBase : public CIntObject
{
	GlobalLobbyWindow * window;
	std::string channelType;
	std::string channelName;

	void clickPressed(const Point & cursorPosition) override;
public:
	GlobalLobbyChannelCardBase(GlobalLobbyWindow * window, const std::string & channelType, const std::string & channelName);
};

class GlobalLobbyAccountCard : public GlobalLobbyChannelCardBase
{
	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelStatus;
	std::shared_ptr<CButton> buttonInvite;

public:
	GlobalLobbyAccountCard(GlobalLobbyWindow * window, const GlobalLobbyAccount & accountDescription);
};

class GlobalLobbyRoomCard : public CIntObject
{
	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelRoomSize;
	std::shared_ptr<CLabel> labelRoomStatus;
	std::shared_ptr<CLabel> labelDescription;
	std::shared_ptr<CButton> buttonJoin;
	std::shared_ptr<CPicture> iconRoomSize;

public:
	GlobalLobbyRoomCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & roomDescription);
};

class GlobalLobbyChannelCard : public GlobalLobbyChannelCardBase
{
	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;

public:
	GlobalLobbyChannelCard(GlobalLobbyWindow * window, const std::string & channelName);
};

class GlobalLobbyMatchCard : public GlobalLobbyChannelCardBase
{
	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelMatchDate;
	std::shared_ptr<CLabel> labelMatchOpponent;

public:
	GlobalLobbyMatchCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & matchDescription);
};

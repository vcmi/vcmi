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

	std::shared_ptr<CIntObject> buildAccountList(const JsonNode &) const;
	std::shared_ptr<CIntObject> buildRoomList(const JsonNode &) const;

public:
	explicit GlobalLobbyWidget(GlobalLobbyWindow * window);

	std::shared_ptr<CLabel> getAccountNameLabel();
	std::shared_ptr<CTextInput> getMessageInput();
	std::shared_ptr<CTextBox> getGameChat();
	std::shared_ptr<CListBox> getAccountList();
	std::shared_ptr<CListBox> getRoomList();
};

class GlobalLobbyAccountCard : public CIntObject
{
public:
	GlobalLobbyAccountCard(GlobalLobbyWindow * window, const GlobalLobbyAccount & accountDescription);

	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelStatus;
	std::shared_ptr<CButton> buttonInvite;
};

class GlobalLobbyRoomCard : public CIntObject
{
public:
	GlobalLobbyRoomCard(GlobalLobbyWindow * window, const GlobalLobbyRoom & roomDescription);

	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelRoomSize;
	std::shared_ptr<CLabel> labelStatus;
	std::shared_ptr<CButton> buttonJoin;
	std::shared_ptr<CPicture> iconRoomSize;
};

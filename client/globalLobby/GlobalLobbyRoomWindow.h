/*
 * GlobalLobbyRoomWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../../lib/modding/ModVerificationInfo.h"

class CLabel;
class CTextBox;
class FilledTexturePlayerColored;
class CButton;
class CToggleGroup;
class GlobalLobbyWindow;
class TransparentFilledRectangle;
class CListBox;

struct GlobalLobbyAccount;
struct GlobalLobbyRoom;

VCMI_LIB_NAMESPACE_BEGIN
struct ModVerificationInfo;
VCMI_LIB_NAMESPACE_END

struct GlobalLobbyRoomModInfo
{
	std::string modName;
	std::string version;
	ModVerificationStatus status;
};

class GlobalLobbyRoomAccountCard : public CIntObject
{
	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelStatus;

public:
	GlobalLobbyRoomAccountCard(const GlobalLobbyAccount & accountDescription);
};

class GlobalLobbyRoomModCard : public CIntObject
{
	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelStatus;
	std::shared_ptr<CLabel> labelVersion;

public:
	GlobalLobbyRoomModCard(const GlobalLobbyRoomModInfo & modInfo);
};

class GlobalLobbyRoomWindow : public CWindowObject
{
	std::vector<GlobalLobbyRoomModInfo> modVerificationList;
	GlobalLobbyWindow * window;
	std::string roomUUID;

	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CLabel> labelSubtitle;
	std::shared_ptr<CLabel> labelVersionTitle;
	std::shared_ptr<CLabel> labelVersionValue;
	std::shared_ptr<CTextBox> labelJoinStatus;

	std::shared_ptr<CLabel> accountListTitle;
	std::shared_ptr<CLabel> modListTitle;

	std::shared_ptr<TransparentFilledRectangle> accountListBackground;
	std::shared_ptr<TransparentFilledRectangle> modListBackground;

	std::shared_ptr<CListBox> accountList;
	std::shared_ptr<CListBox> modList;

	std::shared_ptr<CButton> buttonJoin;
	std::shared_ptr<CButton> buttonClose;

	void onJoin();
	void onClose();

public:
	GlobalLobbyRoomWindow(GlobalLobbyWindow * window, const std::string & roomUUID);
};

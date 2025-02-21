/*
 * GlobalLobbyRoomWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyRoomWindow.h"

#include "GlobalLobbyClient.h"
#include "GlobalLobbyDefines.h"
#include "GlobalLobbyWindow.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../mainmenu/CMainMenu.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/ObjectLists.h"

#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/ModDescription.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/GameLibrary.h"

GlobalLobbyRoomAccountCard::GlobalLobbyRoomAccountCard(const GlobalLobbyAccount & accountDescription)
{
	OBJECT_CONSTRUCTION;
	pos.w = 130;
	pos.h = 40;
	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);
	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, accountDescription.displayName, 120);
	labelStatus = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, accountDescription.status);
}

GlobalLobbyRoomModCard::GlobalLobbyRoomModCard(const GlobalLobbyRoomModInfo & modInfo)
{
	const std::map<ModVerificationStatus, std::string> statusToString = {
		{ ModVerificationStatus::NOT_INSTALLED, "missing" },
		{ ModVerificationStatus::DISABLED, "disabled" },
		{ ModVerificationStatus::EXCESSIVE, "excessive" },
		{ ModVerificationStatus::VERSION_MISMATCH, "version" },
		{ ModVerificationStatus::FULL_MATCH, "compatible" }
	};

	OBJECT_CONSTRUCTION;
	pos.w = 200;
	pos.h = 40;
	backgroundOverlay = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1);

	labelName = std::make_shared<CLabel>(5, 10, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, modInfo.modName, 190);
	labelVersion = std::make_shared<CLabel>(195, 30, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::YELLOW, modInfo.version);
	auto statusColor = Colors::RED;
	if(modInfo.status == ModVerificationStatus::FULL_MATCH)
		statusColor = ColorRGBA(128, 128, 128);
	else if(modInfo.status == ModVerificationStatus::VERSION_MISMATCH)
		statusColor = Colors::YELLOW;
	labelStatus = std::make_shared<CLabel>(5, 30, FONT_SMALL, ETextAlignment::CENTERLEFT, statusColor, LIBRARY->generaltexth->translate("vcmi.lobby.mod.state." + statusToString.at(modInfo.status)));
}

static std::string getJoinRoomErrorMessage(const GlobalLobbyRoom & roomDescription, const std::vector<GlobalLobbyRoomModInfo> & modVerificationList)
{
	bool publicRoom = roomDescription.statusID == "public";
	bool privateRoom = roomDescription.statusID == "private";
	bool gameStarted = !publicRoom && !privateRoom;
	bool hasInvite = GAME->server().getGlobalLobby().isInvitedToRoom(roomDescription.gameRoomID);
	bool alreadyInRoom = GAME->server().inGame();

	if (alreadyInRoom)
		return "vcmi.lobby.preview.error.playing";

	if (gameStarted)
		return "vcmi.lobby.preview.error.busy";

	CModVersion localVersion = CModVersion::fromString(VCMI_VERSION_STRING);
	CModVersion hostVersion = CModVersion::fromString(roomDescription.gameVersion);

	// 1.5.X can play with each other, but not with 1.X.Y
	if (localVersion.major != hostVersion.major || localVersion.minor != hostVersion.minor)
		return "vcmi.lobby.preview.error.version";

	if (roomDescription.playerLimit == roomDescription.participants.size())
		return "vcmi.lobby.preview.error.full";

	if (privateRoom && !hasInvite)
		return "vcmi.lobby.preview.error.invite";

	for(const auto & mod : modVerificationList)
	{
		switch (mod.status)
		{
			case ModVerificationStatus::NOT_INSTALLED:
			case ModVerificationStatus::DISABLED:
			case ModVerificationStatus::EXCESSIVE:
				return "vcmi.lobby.preview.error.mods";
				break;
			case ModVerificationStatus::VERSION_MISMATCH:
			case ModVerificationStatus::FULL_MATCH:
				break;
			default:
				assert(0);
		}
	}
	return "";
}

GlobalLobbyRoomWindow::GlobalLobbyRoomWindow(GlobalLobbyWindow * window, const std::string & roomUUID)
	: CWindowObject(BORDERED)
	, window(window)
	, roomUUID(roomUUID)
{
	OBJECT_CONSTRUCTION;

	pos.w = 400;
	pos.h = 400;

	GlobalLobbyRoom roomDescription = GAME->server().getGlobalLobby().getActiveRoomByName(roomUUID);
	for(const auto & modEntry : ModVerificationInfo::verifyListAgainstLocalMods(roomDescription.modList))
	{
		GlobalLobbyRoomModInfo modInfo;
		modInfo.status = modEntry.second;
		if (modEntry.second == ModVerificationStatus::EXCESSIVE)
			modInfo.version = LIBRARY->modh->getModInfo(modEntry.first).getVersion().toString();
		else
			modInfo.version = roomDescription.modList.at(modEntry.first).version.toString();

		if (modEntry.second == ModVerificationStatus::NOT_INSTALLED)
			modInfo.modName = roomDescription.modList.at(modEntry.first).name;
		else
			modInfo.modName = LIBRARY->modh->getModInfo(modEntry.first).getName();

		modVerificationList.push_back(modInfo);
	}
	std::sort(modVerificationList.begin(), modVerificationList.end(), [](const GlobalLobbyRoomModInfo &a, const GlobalLobbyRoomModInfo &b)
	{ 
		if(a.status == b.status)
			return a.modName < b.modName;

		return a.status < b.status; 
	});

	MetaString subtitleText;
	subtitleText.appendTextID("vcmi.lobby.preview.subtitle");
	subtitleText.replaceRawString(roomDescription.description);
	subtitleText.replaceRawString(roomDescription.hostAccountDisplayName);

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>( pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, MetaString::createFromTextID("vcmi.lobby.preview.title").toString());
	labelSubtitle = std::make_shared<CLabel>( pos.w / 2, 40, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, subtitleText.toString(), 400);

	labelVersionTitle = std::make_shared<CLabel>( 10, 60, FONT_MEDIUM, ETextAlignment::CENTERLEFT, Colors::YELLOW, MetaString::createFromTextID("vcmi.lobby.preview.version").toString());
	labelVersionValue = std::make_shared<CLabel>( 10, 80, FONT_MEDIUM, ETextAlignment::CENTERLEFT, Colors::WHITE, roomDescription.gameVersion);

	buttonJoin = std::make_shared<CButton>(Point(10, 360), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ onJoin(); }, EShortcut::GLOBAL_ACCEPT);
	buttonClose = std::make_shared<CButton>(Point(100, 360), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ onClose(); }, EShortcut::GLOBAL_CANCEL);

	MetaString joinStatusText;
	std::string errorMessage = getJoinRoomErrorMessage(roomDescription, modVerificationList);
	if (!errorMessage.empty())
	{
		joinStatusText.appendTextID("vcmi.lobby.preview.error.header");
		joinStatusText.appendRawString("\n");
		joinStatusText.appendTextID(errorMessage);
	}
	else
		joinStatusText.appendTextID("vcmi.lobby.preview.allowed");

	labelJoinStatus = std::make_shared<CTextBox>(joinStatusText.toString(), Rect(10, 280, 150, 70), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	const auto & createAccountCardCallback = [participants = roomDescription.participants](size_t index) -> std::shared_ptr<CIntObject>
	{
		if(index < participants.size())
			return std::make_shared<GlobalLobbyRoomAccountCard>(participants[index]);
		return std::make_shared<CIntObject>();
	};

	accountListBackground = std::make_shared<TransparentFilledRectangle>(Rect(8, 98, 150, 180), ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1);
	accountList = std::make_shared<CListBox>(createAccountCardCallback, Point(10, 116), Point(0, 40), 4, roomDescription.participants.size(), 0, 1 | 4, Rect(130, 0, 160, 160));
	accountList->setRedrawParent(true);
	accountListTitle = std::make_shared<CLabel>( 12, 109, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, MetaString::createFromTextID("vcmi.lobby.preview.players").toString());

	const auto & createModCardCallback = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		if(index < modVerificationList.size())
			return std::make_shared<GlobalLobbyRoomModCard>(modVerificationList[index]);
		return std::make_shared<CIntObject>();
	};

	modListBackground = std::make_shared<TransparentFilledRectangle>(Rect(178, 48, 220, 340), ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1);
	modList = std::make_shared<CListBox>(createModCardCallback, Point(180, 66), Point(0, 40), 8, modVerificationList.size(), 0, 1 | 4, Rect(200, 0, 320, 320));
	modList->setRedrawParent(true);
	modListTitle = std::make_shared<CLabel>( 182, 59, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, MetaString::createFromTextID("vcmi.lobby.preview.mods").toString());

	buttonJoin->block(!errorMessage.empty());
	filledBackground->setPlayerColor(PlayerColor(1));
	center();
}

void GlobalLobbyRoomWindow::onJoin()
{
	window->doJoinRoom(roomUUID);
}

void GlobalLobbyRoomWindow::onClose()
{
	close();
}

/*
 * CSavingScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CSavingScreen.h"
#include "SelectionTab.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../CServerHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../windows/InfoWindows.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/filesystem/SavegamePath.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/networkPacks/PacksForLobby.h"

CSavingScreen::CSavingScreen()
	: CSelectionBase(ESelectionScreen::saveGame)
{
	OBJECT_CONSTRUCTION;
	center(pos);
	localMi = std::make_shared<CMapInfo>();
	localMi->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*GAME->interface()->cb->getMapHeader()));

	tabSel = std::make_shared<SelectionTab>(screenType);
	tabSel->curFolder = SavegamePath::getGameDirectoryName(
		*GAME->interface()->cb->getStartInfo(),
		*GAME->interface()->cb->getMapHeader());
	tabSel->callOnSelect = std::bind(&CSavingScreen::changeSelection, this, _1);
	curTab = tabSel;
		
	buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRSAV.DEF"), LIBRARY->generaltexth->zelp[103], std::bind(&CSavingScreen::saveGame, this), EShortcut::LOBBY_SAVE_GAME);
	// after all widgets are created, so that input of all of them is blocked until the list is received
	tabSel->toggleMode();
	
	GAME->interface()->gamePause(true);
}

const CMapInfo * CSavingScreen::getMapInfo()
{
	return localMi.get();
}

const StartInfo * CSavingScreen::getStartInfo()
{
	if (localMi)
		return localMi->scenarioOptionsOfSave.get();
	return GAME->interface()->cb->getStartInfo();
}

void CSavingScreen::changeSelection(const std::shared_ptr<ElementInfo> & element)
{
	if(element == selectedElement)
		return;

	selectedElement = element;
	localMi.reset();
	localTextOverlay.reset();

	if(element)
	{
		LobbyRequestScenarioInfo request;
		request.selection = {ScenarioSelection::EType::SAVE, element->fileURI};
		GAME->server().sendLobbyPack(request);
	}

	card->changeSelection();
	card->redraw();
}

void CSavingScreen::onScenarioInfoReceived(const LobbyScenarioInfo & pack)
{
	if(!selectedElement || pack.selection.fileURI != selectedElement->fileURI)
		return;

	if(pack.mapInfo)
	{
		localTextOverlay.emplace(pack.mapInfo->mapHeader->texts);
		localMi = pack.mapInfo;
		card->changeSelection();
		card->redraw();
	}
	else
	{
		MetaString message;
		message.appendTextID("vcmi.client.errors.invalidMap");
		message.replaceRawString(pack.error);
		CInfoWindow::showInfoDialog(message.toString(&GAME->translator()), {});
	}
}

void CSavingScreen::close()
{
	GAME->interface()->gamePause(false);
	CSelectionBase::close();
}

void CSavingScreen::saveGame()
{
	if(!(tabSel && tabSel->inputName && tabSel->inputName->getText().size()))
		return;

	std::string path = "Saves/" + tabSel->curFolder + tabSel->inputName->getText();

	auto overWrite = [this, path]() -> void
	{
		GAME->interface()->cb->save(path, true);
		close();
	};

	auto confirmOverwrite = [this, path, overWrite]()
	{
		const bool exists = std::ranges::any_of(tabSel->allItems, [&path](const auto & item)
		{
			return !item->isFolder && boost::algorithm::iequals(item->fileURI, path);
		});
		if(exists)
		{
			MetaString hlp = MetaString::createFromTextID("core.genrltxt.493"); //%s exists. Overwrite?
			hlp.replaceRawString(tabSel->inputName->getText());
			GAME->interface()->showYesNoDialog(hlp.toString(&GAME->translator()), overWrite, nullptr);
		}
		else
		{
			overWrite();
		}
	};

	if(SavegamePath::isAutosaveName(tabSel->inputName->getText()))
	{
		const std::string warning = LIBRARY->generaltexth->translate("vcmi.savingScreen.autosaveNameWarning");
		GAME->interface()->showYesNoDialog(warning, confirmOverwrite, nullptr);
	}
	else
		confirmOverwrite();
}

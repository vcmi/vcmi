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
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/SavegamePath.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/GameLibrary.h"

CSavingScreen::CSavingScreen(bool pauseGame, std::function<void()> onClose)
	: CSelectionBase(ESelectionScreen::saveGame)
	, pauseGame(pauseGame)
	, onClose(std::move(onClose))
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
	tabSel->toggleMode();
	curTab = tabSel;
		
	buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRSAV.DEF"), LIBRARY->generaltexth->zelp[103], std::bind(&CSavingScreen::saveGame, this), EShortcut::LOBBY_SAVE_GAME);
	
	if(pauseGame)
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

void CSavingScreen::changeSelection(std::shared_ptr<CMapInfo> to)
{
	if(localMi == to)
		return;

	localMi = to;
	card->changeSelection();
	card->redraw();
}

void CSavingScreen::close()
{
	if(pauseGame)
		GAME->interface()->gamePause(false);

	auto closeCallback = std::move(onClose);
	CSelectionBase::close();

	if(closeCallback)
		ENGINE->dispatchMainThread(std::move(closeCallback));
}

void CSavingScreen::saveGame()
{
	if(!(tabSel && tabSel->inputName && tabSel->inputName->getText().size()))
		return;

	std::string path = "Saves/" + tabSel->curFolder + tabSel->inputName->getText();

	auto overWrite = [this, path]() -> void
	{
		if(!onClose)
		{
			tabSel->rememberSave(path);
			GAME->interface()->cb->save(path, true);
			close();
			return;
		}

		saving = true;
		pendingSavePath = path;
		buttonStart->block(true);
		buttonBack->block(true);
		GAME->interface()->saveGame(path, [](bool success)
		{
			ENGINE->dispatchMainThread([success]()
			{
				if(auto savingScreen = ENGINE->windows().topWindow<CSavingScreen>())
					savingScreen->saveFinished(success);
			});
		});
	};

	auto confirmOverwrite = [this, path, overWrite]()
	{
		if(CResourceHandler::get("local")->existsResource(ResourcePath(path, EResType::SAVEGAME)))
		{
			std::string hlp = LIBRARY->generaltexth->allTexts[493]; //%s exists. Overwrite?
			boost::algorithm::replace_first(hlp, "%s", tabSel->inputName->getText());
			GAME->interface()->showYesNoDialog(hlp, overWrite, nullptr);
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

void CSavingScreen::saveFinished(bool success)
{
	if(!saving)
		return;

	saving = false;
	if(success)
	{
		tabSel->rememberSave(pendingSavePath);
		close();
		return;
	}

	buttonStart->block(false);
	buttonBack->block(false);

	std::string message = LIBRARY->generaltexth->allTexts[9];
	boost::algorithm::replace_first(message, "%s", tabSel->inputName->getText());
	GAME->interface()->showInfoDialog(message);
}

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
	tabSel->toggleMode();
	curTab = tabSel;
		
	buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRSAV.DEF"), LIBRARY->generaltexth->zelp[103], std::bind(&CSavingScreen::saveGame, this), EShortcut::LOBBY_SAVE_GAME);
	
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
		tabSel->rememberSave(path);
		GAME->interface()->cb->save(path, true);
		close();
	};

	auto confirmOverwrite = [this, path, overWrite]()
	{
		if(CResourceHandler::get("local")->existsResource(ResourcePath(path, EResType::SAVEGAME)))
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

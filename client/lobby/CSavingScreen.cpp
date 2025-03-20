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

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/filesystem/Filesystem.h"
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
		Settings lastSave = settings.write["general"]["lastSave"];
		lastSave->String() = path;
		GAME->interface()->cb->save(path);
		close();
	};

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
}

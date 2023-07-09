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

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"

CSavingScreen::CSavingScreen()
	: CSelectionBase(ESelectionScreen::saveGame)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	center(pos);
	localMi = std::make_shared<CMapInfo>();
	localMi->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*LOCPLINT->cb->getMapHeader()));

	tabSel = std::make_shared<SelectionTab>(screenType);
	tabSel->callOnSelect = std::bind(&CSavingScreen::changeSelection, this, _1);
	tabSel->toggleMode();
	curTab = tabSel;
		
	buttonStart = std::make_shared<CButton>(Point(411, 535), "SCNRSAV.DEF", CGI->generaltexth->zelp[103], std::bind(&CSavingScreen::saveGame, this), EShortcut::LOBBY_SAVE_GAME);
}

const CMapInfo * CSavingScreen::getMapInfo()
{
	return localMi.get();
}

const StartInfo * CSavingScreen::getStartInfo()
{
	if (localMi)
		return localMi->scenarioOptionsOfSave;
	return LOCPLINT->cb->getStartInfo();
}

void CSavingScreen::changeSelection(std::shared_ptr<CMapInfo> to)
{
	if(localMi == to)
		return;

	localMi = to;
	card->changeSelection();
}

void CSavingScreen::saveGame()
{
	if(!(tabSel && tabSel->inputName && tabSel->inputName->getText().size()))
		return;

	std::string path = "Saves/" + tabSel->inputName->getText();

	auto overWrite = [this, path]() -> void
	{
		Settings lastSave = settings.write["general"]["lastSave"];
		lastSave->String() = path;
		LOCPLINT->cb->save(path);
		close();
	};

	if(CResourceHandler::get("local")->existsResource(ResourceID(path, EResType::CLIENT_SAVEGAME)))
	{
		std::string hlp = CGI->generaltexth->allTexts[493]; //%s exists. Overwrite?
		boost::algorithm::replace_first(hlp, "%s", tabSel->inputName->getText());
		LOCPLINT->showYesNoDialog(hlp, overWrite, nullptr);
	}
	else
	{
		overWrite();
	}
}

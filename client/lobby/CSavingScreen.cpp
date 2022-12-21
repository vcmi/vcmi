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
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapInfo.h"

CSavingScreen::CSavingScreen()
	: CSelectionBase(ESelectionScreen::saveGame)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	center(pos);
	// TODO: we should really use std::shared_ptr for passing StartInfo around.
	localSi = new StartInfo(*LOCPLINT->cb->getStartInfo());
	localMi = std::make_shared<CMapInfo>();
	localMi->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*LOCPLINT->cb->getMapHeader()));

	tabSel = std::make_shared<SelectionTab>(screenType);
	curTab = tabSel;
	tabSel->toggleMode();

	tabSel->callOnSelect = std::bind(&CSavingScreen::changeSelection, this, _1);
	buttonStart = std::make_shared<CButton>(Point(411, 535), "SCNRSAV.DEF", CGI->generaltexth->zelp[103], std::bind(&CSavingScreen::saveGame, this), SDLK_s);
	buttonStart->assignedKeys.insert(SDLK_RETURN);
}

const CMapInfo * CSavingScreen::getMapInfo()
{
	return localMi.get();
}

const StartInfo * CSavingScreen::getStartInfo()
{
	return localSi;
}

void CSavingScreen::changeSelection(std::shared_ptr<CMapInfo> to)
{
	if(localMi == to)
		return;

	localMi = to;
	localSi = localMi->scenarioOptionsOfSave;
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

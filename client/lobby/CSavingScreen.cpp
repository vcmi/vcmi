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
#include "../gui/CGuiHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"
#include "../CGameInfo.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/CConfigHandler.h"

CSavingScreen::CSavingScreen()
	: CSelectionBase(ESelectionScreen::saveGame)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bordered = false;
	center(pos);
	// TODO: we should really use std::shared_ptr for passing StartInfo around.
	localSi = new StartInfo(*LOCPLINT->cb->getStartInfo());
	localMi = new CMapInfo();
	localMi->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*LOCPLINT->cb->getMapHeader()));

	tabSel = new SelectionTab(screenType); //scenario selection tab
	tabSel->recActions = 255;
	tabSel->toggleMode();
	curTab = tabSel;

	tabSel->onSelect = std::bind(&CSavingScreen::changeSelection, this, _1);
	buttonStart = new CButton(Point(411, 535), "SCNRSAV.DEF", CGI->generaltexth->zelp[103], std::bind(&CSavingScreen::saveGame, this), SDLK_s);
	buttonStart->assignedKeys.insert(SDLK_RETURN);
}

CSavingScreen::~CSavingScreen()
{
	vstd::clear_pointer(localMi);
}

const CMapInfo * CSavingScreen::getMapInfo()
{
	return localMi;
}

const StartInfo * CSavingScreen::getStartInfo()
{
	return localSi;
}

void CSavingScreen::changeSelection(std::shared_ptr<CMapInfo> to)
{
	if(localMi == to.get())
		return;

	localMi = to.get();
	localSi = localMi->scenarioOpts;
	card->changeSelection();
}

void CSavingScreen::saveGame()
{
	if(!(tabSel && tabSel->txt && tabSel->txt->text.size()))
		return;

	std::string path = "Saves/" + tabSel->txt->text;

	auto overWrite = [&]() -> void
	{
		Settings lastSave = settings.write["session"]["lastSave"];
		lastSave->String() = path;
		LOCPLINT->cb->save(path);
		GH.popIntTotally(this);
	};

	if(CResourceHandler::get("local")->existsResource(ResourceID(path, EResType::CLIENT_SAVEGAME)))
	{
		std::string hlp = CGI->generaltexth->allTexts[493]; //%s exists. Overwrite?
		boost::algorithm::replace_first(hlp, "%s", tabSel->txt->text);
		LOCPLINT->showYesNoDialog(hlp, overWrite, 0, false);
	}
	else
	{
		overWrite();
	}
}

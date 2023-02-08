/*
 * CScenarioInfoScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CScenarioInfoScreen.h"
#include "OptionsTab.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/Buttons.h"

#include "../../CCallback.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapInfo.h"

CScenarioInfoScreen::CScenarioInfoScreen() : localSi(new StartInfo(*LOCPLINT->cb->getStartInfo())), localMi(new CMapInfo())
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	localMi->mapHeader = std::make_unique<CMapHeader>(*LOCPLINT->cb->getMapHeader());

	screenType = ESelectionScreen::scenarioInfo;

	card = std::make_shared<InfoCard>();
	opt = std::make_shared<OptionsTab>();
	opt->recActions = UPDATE | SHOWALL;
	opt->recreate();
	card->changeSelection();

	card->iconDifficulty->setSelected(getCurrentDifficulty());
	buttonBack = std::make_shared<CButton>(Point(584, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], [=](){ close();}, SDLK_ESCAPE);
}

CScenarioInfoScreen::~CScenarioInfoScreen()
{
	vstd::clear_pointer(localSi);
	vstd::clear_pointer(localMi);
}

const CMapInfo * CScenarioInfoScreen::getMapInfo()
{
	return localMi;
}

const StartInfo * CScenarioInfoScreen::getStartInfo()
{
	return localSi;
}

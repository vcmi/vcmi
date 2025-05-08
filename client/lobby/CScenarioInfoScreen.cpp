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

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"

#include "../../CCallback.h"

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/GameLibrary.h"

CScenarioInfoScreen::CScenarioInfoScreen()
{
	OBJECT_CONSTRUCTION;
	pos.w = 800;
	pos.h = 600;
	pos = center();

	localSi = std::make_unique<StartInfo>(*GAME->interface()->cb->getStartInfo());
	localMi = std::make_unique<CMapInfo>();
	localMi->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*GAME->interface()->cb->getMapHeader()));

	screenType = ESelectionScreen::scenarioInfo;

	card = std::make_shared<InfoCard>();
	opt = std::make_shared<OptionsTab>();
	opt->recActions = UPDATE | SHOWALL;
	opt->recreate();
	card->changeSelection();

	card->iconDifficulty->setSelected(getCurrentDifficulty());
	buttonBack = std::make_shared<CButton>(Point(584, 535), AnimationPath::builtin("SCNRBACK.DEF"), LIBRARY->generaltexth->zelp[105], [this](){ close();}, EShortcut::GLOBAL_CANCEL);
}

CScenarioInfoScreen::~CScenarioInfoScreen() = default;

const CMapInfo * CScenarioInfoScreen::getMapInfo()
{
	return localMi.get();
}

const StartInfo * CScenarioInfoScreen::getStartInfo()
{
	return localSi.get();
}

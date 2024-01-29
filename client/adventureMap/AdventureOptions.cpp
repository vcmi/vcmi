/*
 * CAdventureOptions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AdventureOptions.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../lobby/CCampaignInfoScreen.h"
#include "../lobby/CScenarioInfoScreen.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"

#include "../../CCallback.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CGeneralTextHandler.h"

AdventureOptions::AdventureOptions()
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("ADVOPTS"))
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	viewWorld = std::make_shared<CButton>(Point(24, 23), AnimationPath::builtin("ADVVIEW.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_VIEW_WORLD);
	viewWorld->addCallback([] { LOCPLINT->viewWorldMap(); });

	puzzle = std::make_shared<CButton>(Point(24, 81), AnimationPath::builtin("ADVPUZ.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_VIEW_PUZZLE);
	puzzle->addCallback(std::bind(&CPlayerInterface::showPuzzleMap, LOCPLINT));

	dig = std::make_shared<CButton>(Point(24, 139), AnimationPath::builtin("ADVDIG.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_DIG_GRAIL);
	if(const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero())
		dig->addCallback(std::bind(&CPlayerInterface::tryDigging, LOCPLINT, h));
	else
		dig->block(true);

	scenInfo = std::make_shared<CButton>(Point(24, 198), AnimationPath::builtin("ADVINFO.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_VIEW_SCENARIO);
	scenInfo->addCallback(AdventureOptions::showScenarioInfo);
	
	replay = std::make_shared<CButton>(Point(24, 257), AnimationPath::builtin("ADVTURN.DEF"), CButton::tooltip(), [&](){ close(); });
	replay->addCallback([]{ LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.replayOpponentTurnNotImplemented")); });

	exit = std::make_shared<CButton>(Point(203, 313), AnimationPath::builtin("IOK6432.DEF"), CButton::tooltip(), std::bind(&AdventureOptions::close, this), EShortcut::GLOBAL_RETURN);
}

void AdventureOptions::showScenarioInfo()
{
	if(LOCPLINT->cb->getStartInfo()->campState)
	{
		GH.windows().createAndPushWindow<CCampaignInfoScreen>();
	}
	else
	{
		GH.windows().createAndPushWindow<CScenarioInfoScreen>();
	}
}


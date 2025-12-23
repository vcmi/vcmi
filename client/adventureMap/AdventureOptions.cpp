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

#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../lobby/CCampaignInfoScreen.h"
#include "../lobby/CScenarioInfoScreen.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"

AdventureOptions::AdventureOptions()
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("ADVOPTS"))
{
	OBJECT_CONSTRUCTION;

	viewWorld = std::make_shared<CButton>(Point(24, 23), AnimationPath::builtin("ADVVIEW.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_VIEW_WORLD);
	viewWorld->addCallback([] { GAME->interface()->viewWorldMap(); });

	puzzle = std::make_shared<CButton>(Point(24, 81), AnimationPath::builtin("ADVPUZ.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_VIEW_PUZZLE);
	puzzle->addCallback(std::bind(&CPlayerInterface::showPuzzleMap, GAME->interface()));

	dig = std::make_shared<CButton>(Point(24, 139), AnimationPath::builtin("ADVDIG.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_DIG_GRAIL);
	if(const CGHeroInstance *h = GAME->interface()->localState->getCurrentHero())
		dig->addCallback(std::bind(&CPlayerInterface::tryDigging, GAME->interface(), h));
	else
		dig->block(true);

	scenInfo = std::make_shared<CButton>(Point(24, 198), AnimationPath::builtin("ADVINFO.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_VIEW_SCENARIO);
	scenInfo->addCallback(AdventureOptions::showScenarioInfo);
	
	replay = std::make_shared<CButton>(Point(24, 257), AnimationPath::builtin("ADVTURN.DEF"), CButton::tooltip(), [&](){ close(); }, EShortcut::ADVENTURE_REPLAY_TURN);
	replay->addCallback([]{ GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.adventureMap.replayOpponentTurnNotImplemented")); });

	exit = std::make_shared<CButton>(Point(203, 313), AnimationPath::builtin("IOK6432.DEF"), CButton::tooltip(), std::bind(&AdventureOptions::close, this), EShortcut::GLOBAL_RETURN);
}

void AdventureOptions::showScenarioInfo()
{
	if(GAME->interface()->cb->getStartInfo()->campState)
	{
		ENGINE->windows().createAndPushWindow<CCampaignInfoScreen>();
	}
	else
	{
		ENGINE->windows().createAndPushWindow<CScenarioInfoScreen>();
	}
}


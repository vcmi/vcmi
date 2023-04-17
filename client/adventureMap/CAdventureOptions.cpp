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
#include "CAdventureOptions.h"

#include "CAdventureMapInterface.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../lobby/CCampaignInfoScreen.h"
#include "../lobby/CScenarioInfoScreen.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/Buttons.h"

#include "../../CCallback.h"
#include "../../lib/StartInfo.h"

CAdventureOptions::CAdventureOptions()
	: CWindowObject(PLAYER_COLORED, "ADVOPTS")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	viewWorld = std::make_shared<CButton>(Point(24, 23), "ADVVIEW.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_v);
	viewWorld->addCallback( [] { LOCPLINT->viewWorldMap(); });

	exit = std::make_shared<CButton>(Point(204, 313), "IOK6432.DEF", CButton::tooltip(), std::bind(&CAdventureOptions::close, this), SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	scenInfo = std::make_shared<CButton>(Point(24, 198), "ADVINFO.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_i);
	scenInfo->addCallback(CAdventureOptions::showScenarioInfo);

	puzzle = std::make_shared<CButton>(Point(24, 81), "ADVPUZ.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_p);
	puzzle->addCallback(std::bind(&CPlayerInterface::showPuzzleMap, LOCPLINT));

	dig = std::make_shared<CButton>(Point(24, 139), "ADVDIG.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_d);
	if(const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero())
		dig->addCallback(std::bind(&CPlayerInterface::tryDiggging, LOCPLINT, h));
	else
		dig->block(true);
}

void CAdventureOptions::showScenarioInfo()
{
	if(LOCPLINT->cb->getStartInfo()->campState)
	{
		GH.pushIntT<CCampaignInfoScreen>();
	}
	else
	{
		GH.pushIntT<CScenarioInfoScreen>();
	}
}


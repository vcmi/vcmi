/*
 * CAdvmapInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAdvmapInterface.h"

#include "CCastleInterface.h"
#include "CHeroWindow.h"
#include "CKingdomInterface.h"
#include "CSpellWindow.h"
#include "CTradeWindow.h"
#include "GUIClasses.h"
#include "InfoWindows.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../mainmenu/CMainMenu.h"
#include "../lobby/CSelectionBase.h"
#include "../lobby/CCampaignInfoScreen.h"
#include "../lobby/CSavingScreen.h"
#include "../lobby/CScenarioInfoScreen.h"
#include "../Graphics.h"
#include "../mapHandler.h"

#include "../gui/CAnimation.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/MiscWidgets.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CSoundBase.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/TerrainHandler.h"

#include <SDL_surface.h>
#include <SDL_events.h>

#define ADVOPT (conf.go()->ac)
using namespace CSDL_Ext;

std::shared_ptr<CAdvMapInt> adventureInt;

static void setScrollingCursor(ui8 direction)
{
	if(direction & CAdvMapInt::RIGHT)
	{
		if(direction & CAdvMapInt::UP)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHEAST);
		else if(direction & CAdvMapInt::DOWN)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHEAST);
		else
			CCS->curh->set(Cursor::Map::SCROLL_EAST);
	}
	else if(direction & CAdvMapInt::LEFT)
	{
		if(direction & CAdvMapInt::UP)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHWEST);
		else if(direction & CAdvMapInt::DOWN)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHWEST);
		else
			CCS->curh->set(Cursor::Map::SCROLL_WEST);
	}
	else if(direction & CAdvMapInt::UP)
		CCS->curh->set(Cursor::Map::SCROLL_NORTH);
	else if(direction & CAdvMapInt::DOWN)
		CCS->curh->set(Cursor::Map::SCROLL_SOUTH);
}

CAdventureOptions::CAdventureOptions()
	: CWindowObject(PLAYER_COLORED, "ADVOPTS")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	viewWorld = std::make_shared<CButton>(Point(24, 23), "ADVVIEW.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_v);
	viewWorld->addCallback(std::bind(&CPlayerInterface::viewWorldMap, LOCPLINT));

	exit = std::make_shared<CButton>(Point(204, 313), "IOK6432.DEF", CButton::tooltip(), std::bind(&CAdventureOptions::close, this), SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	scenInfo = std::make_shared<CButton>(Point(24, 198), "ADVINFO.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_i);
	scenInfo->addCallback(CAdventureOptions::showScenarioInfo);

	puzzle = std::make_shared<CButton>(Point(24, 81), "ADVPUZ.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_p);
	puzzle->addCallback(std::bind(&CPlayerInterface::showPuzzleMap, LOCPLINT));

	dig = std::make_shared<CButton>(Point(24, 139), "ADVDIG.DEF", CButton::tooltip(), [&](){ close(); }, SDLK_d);
	if(const CGHeroInstance *h = adventureInt->curHero())
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


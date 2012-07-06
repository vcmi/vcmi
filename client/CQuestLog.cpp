#include "StdInc.h"
#include "CQuestLog.h"

#include "CGameInfo.h"
#include "../lib/CGeneralTextHandler.h"
#include "../CCallback.h"

#include <SDL.h>
#include "UIFramework/SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "Graphics.h"
#include "CPlayerInterface.h"
#include "CConfigHandler.h"

#include "../lib/CGameState.h"
#include "../lib/CArtHandler.h"
#include "../lib/NetPacks.h"

#include "UIFramework/CGuiHandler.h"
#include "UIFramework/CIntObjectClasses.h"

struct QuestInfo;

CQuestLog::CQuestLog (std::vector<const QuestInfo> & Quests) :
	CWindowObject(PLAYER_COLORED, "QuestLog.pcx"),
	quests (Quests), slider (NULL)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
}

void CQuestLog::init()
{
	minimap = new CQuestMinimap (Rect (47, 33, 144, 144));
	description = new CTextBox ("", Rect(244, 36, 355, 350), 1);
	ok = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, boost::bind(&CQuestLog::close,this), 547, 401, "IOKAY.DEF", SDLK_RETURN);
}
/*
* EventHandlerSDLUser.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "UserEventHandler.h"

#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../mainmenu/CPrologEpilogVideo.h"
#include "../gui/EventDispatcher.h"

#include "../../lib/campaign/CampaignState.h"

#include <SDL_events.h>

void UserEventHandler::handleUserEvent(const SDL_UserEvent & user)
{
	switch(static_cast<EUserEvent>(user.code))
	{
		case EUserEvent::FORCE_QUIT:
		{
			handleQuit(false);
			return;
		}
		break;
		case EUserEvent::RETURN_TO_MAIN_MENU:
		{
			CSH->endGameplay();
			GH.defActionsDef = 63;
			CMM->menu->switchToTab("main");
		}
		break;
		case EUserEvent::RESTART_GAME:
		{
			CSH->sendRestartGame();
		}
		break;
		case EUserEvent::CAMPAIGN_START_SCENARIO:
		{
			CSH->campaignServerRestartLock.set(true);
			CSH->endGameplay();
			auto ourCampaign = std::shared_ptr<CampaignState>(reinterpret_cast<CampaignState *>(user.data1));
			auto & epilogue = ourCampaign->scenario(*ourCampaign->lastScenario()).epilog;
			auto finisher = [=]()
			{
				if(!ourCampaign->isCampaignFinished())
				{
					GH.windows().pushWindow(CMM);
					GH.windows().pushWindow(CMM->menu);
					CMM->openCampaignLobby(ourCampaign);
				}
			};
			if(epilogue.hasPrologEpilog)
			{
				GH.windows().createAndPushWindow<CPrologEpilogVideo>(epilogue, finisher);
			}
			else
			{
				CSH->campaignServerRestartLock.waitUntil(false);
				finisher();
			}
		}
		break;
		case EUserEvent::RETURN_TO_MENU_LOAD:
			CSH->endGameplay();
			GH.defActionsDef = 63;
			CMM->menu->switchToTab("load");
			break;
		case EUserEvent::FULLSCREEN_TOGGLED:
		{
			boost::unique_lock<boost::recursive_mutex> lock(*CPlayerInterface::pim);
			GH.onScreenResize();
			break;
		}
		case EUserEvent::FAKE_MOUSE_MOVE:
			GH.events().dispatchMouseMoved(Point(0, 0), GH.getCursorPosition());
			break;
		default:
			logGlobal->error("Unknown user event. Code %d", user.code);
			break;
	}
}

#include "../stdafx.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
//#include "SDL_Extensions.h"
#include "SDL_Extensions.h"
//#include "SDL_framerate.h"

#include "SDL_framerate.h"
#include "CConfigHandler.h"
#include "CCreatureAnimation.h"
#include "Graphics.h"
#include "../hch/CArtHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CObjectHandler.h"
#include "../lib/Connection.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CTownHandler.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "../lib/map.h"
#include "../mapHandler.h"
#include "../timeHandler.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <cmath>
#include <queue>
#include <sstream>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/*
 * CPlayerInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::assign;
using namespace CSDL_Ext;

void processCommand(const std::string &message, CClient *&client);

extern TTF_Font * GEOR16;
extern std::queue<SDL_Event*> events;
extern boost::mutex eventsM;

CPlayerInterface * LOCPLINT;
enum  EMoveState {STOP_MOVE, WAITING_MOVE, CONTINUE_MOVE, DURING_MOVE};
CondSh<EMoveState> stillMoveHero; //used during hero movement

struct OCM_HLP_CGIN
{
	bool inline operator ()(const std::pair<const CGObjectInstance*,SDL_Rect>  & a, const std::pair<const CGObjectInstance*,SDL_Rect> & b) const
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo_cgin ;



CPlayerInterface::CPlayerInterface(int Player, int serial)
{
	LOCPLINT = this;
	curAction = NULL;
	playerID=Player;
	serialID=serial;
	human=true;
	castleInt = NULL;
	adventureInt = NULL;
	battleInt = NULL;
	pim = new boost::recursive_mutex;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	heroMoveSpeed = 2;
	mapScrollingSpeed = 2;
	//initializing framerate keeper
	mainFPSmng = new FPSmanager;
	SDL_initFramerate(mainFPSmng);
	SDL_setFramerate(mainFPSmng, 48);
	//framerate keeper initialized
	cingconsole = new CInGameConsole;
}
CPlayerInterface::~CPlayerInterface()
{
	delete pim;
	delete showingDialog;
	delete mainFPSmng;
	delete adventureInt;
	delete cingconsole;

	for(std::map<int,SDL_Surface*>::iterator i=graphics->heroWins.begin(); i!= graphics->heroWins.end(); i++)
		SDL_FreeSurface(i->second);
	for(std::map<int,SDL_Surface*>::iterator i=graphics->townWins.begin(); i!= graphics->townWins.end(); i++)
		SDL_FreeSurface(i->second);
}
void CPlayerInterface::init(ICallback * CB)
{
	cb = dynamic_cast<CCallback*>(CB);
	adventureInt = new CAdvMapInt(playerID);
	std::vector<const CGTownInstance*> tt = cb->getTownsInfo(false);
	for(int i=0;i<tt.size();i++)
	{
		SDL_Surface * pom = infoWin(tt[i]);
		graphics->townWins.insert(std::pair<int,SDL_Surface*>(tt[i]->id,pom));
	}
}
void CPlayerInterface::yourTurn()
{
	try
	{
		LOCPLINT = this;
		makingTurn = true;

		static int autosaveCount = 0;
		LOCPLINT->cb->save("Autosave_" + boost::lexical_cast<std::string>(autosaveCount++ + 1));
		autosaveCount %= 5;

		for(std::map<int,SDL_Surface*>::iterator i=graphics->heroWins.begin(); i!=graphics->heroWins.end();i++) //redraw hero infoboxes
			SDL_FreeSurface(i->second);
		graphics->heroWins.clear();
		std::vector <const CGHeroInstance *> hh = cb->getHeroesInfo(false);
		for(int i=0;i<hh.size();i++)
		{
			SDL_Surface * pom = infoWin(hh[i]);
			graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hh[i]->subID,pom));
		}

		/* TODO: This isn't quite right. First day in game should play
		 * NEWDAY. And we don't play NEWMONTH. */
		int day = cb->getDate(1);
		if (day != 1)
			CGI->audioh->playSound(soundBase::newDay);
		else
			CGI->audioh->playSound(soundBase::newWeek);

		adventureInt->infoBar.newDay(day);

		//select first hero if available.
		//TODO: check if hero is slept
		if(adventureInt->heroList.items.size())
			adventureInt->select(adventureInt->heroList.items[0].first);
		else
			adventureInt->select(adventureInt->townList.items[0]);

		adventureInt->showAll(screen);
		pushInt(adventureInt);
		adventureInt->KeyInterested::activate();

		timeHandler th;
		th.getDif();
		while(makingTurn) // main loop
		{

			updateWater();
			pim->lock();


			//if there are any waiting dialogs, show them
			if(dialogs.size() && !showingDialog->get())
			{
				showingDialog->set(true);
				pushInt(dialogs.front());
				dialogs.pop_front();
			}

			int tv = th.getDif();
			std::list<TimeInterested*> hlp = timeinterested;
			for (std::list<TimeInterested*>::iterator i=hlp.begin(); i != hlp.end();i++)
			{
				if(!vstd::contains(timeinterested,*i)) continue;
				if ((*i)->toNextTick>=0)
					(*i)->toNextTick-=tv;
				if ((*i)->toNextTick<0)
					(*i)->tick();
			}

			while(true)
			{
				SDL_Event *ev = NULL;
				{
					boost::unique_lock<boost::mutex> lock(eventsM);
					if(!events.size())
					{
						break;
					}
					else
					{
						ev = events.front();
						events.pop();
					}
				}
				handleEvent(ev);
				delete ev;
			}

			if(!adventureInt->active && adventureInt->scrollingDir) //player force map scrolling though interface is disabled
			{
				totalRedraw();
			}
			else
			{
				//update only top interface and draw background
				if(objsToBlit.size() > 1)
					blitAt(screen2,0,0,screen); //blit background
				objsToBlit.back()->show(screen); //blit active interface/window
			}

			CGI->curh->draw1();
			CSDL_Ext::update(screen);
			CGI->curh->draw2();
			pim->unlock();
			SDL_framerateDelay(mainFPSmng);
		}

		adventureInt->KeyInterested::deactivate();
		popInt(adventureInt);

		cb->endTurn();
	} HANDLE_EXCEPTION
}

inline void subRect(const int & x, const int & y, const int & z, const SDL_Rect & r, const int & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].first->id==hid)
		{
			hlp.objects[h].second = r;
			return;
		}
}

inline void delObjRect(const int & x, const int & y, const int & z, const int & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].first->id==hid)
		{
			hlp.objects.erase(hlp.objects.begin()+h);
			return;
		}
}
int getDir(int3 src, int3 dst)
{
	int ret = -1;
	if(dst.x+1 == src.x && dst.y+1 == src.y) //tl
	{
		ret = 1;
	}
	else if(dst.x == src.x && dst.y+1 == src.y) //t
	{
		ret = 2;
	}
	else if(dst.x-1 == src.x && dst.y+1 == src.y) //tr
	{
		ret = 3;
	}
	else if(dst.x-1 == src.x && dst.y == src.y) //r
	{
		ret = 4;
	}
	else if(dst.x-1 == src.x && dst.y-1 == src.y) //br
	{
		ret = 5;
	}
	else if(dst.x == src.x && dst.y-1 == src.y) //b
	{
		ret = 6;
	}
	else if(dst.x+1 == src.x && dst.y-1 == src.y) //bl
	{
		ret = 7;
	}
	else if(dst.x+1 == src.x && dst.y == src.y) //l
	{
		ret = 8;
	}
	return ret;
}
void CPlayerInterface::heroMoved(const HeroMoveDetails & details)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	adventureInt->centerOn(details.ho->pos); //actualizing screen pos
	adventureInt->minimap.draw(screen2);
	adventureInt->heroList.draw(screen2);

	if(details.style>0  ||  details.src == details.dst)
		return;

	//initializing objects and performing first step of move
	const CGHeroInstance * ho = details.ho; //object representing this hero
	int3 hp = details.src;
	if (!details.successful) //hero failed to move
	{
		if(ho->movement > 50)
			ho->moveDir = getDir(details.src,details.dst);
		ho->isStanding = true;

		if(ho->movement)
		{
			delete adventureInt->terrain.currentPath;
			adventureInt->terrain.currentPath = NULL;
			adventureInt->heroList.items[adventureInt->heroList.getPosOfHero(ho)].second = NULL;
		}
		stillMoveHero.setn(STOP_MOVE);
		return;
	}

	if (adventureInt->terrain.currentPath) //&& hero is moving
	{
		adventureInt->terrain.currentPath->nodes.erase(adventureInt->terrain.currentPath->nodes.end()-1);
		if(!adventureInt->terrain.currentPath->nodes.size())
		{

			delete adventureInt->terrain.currentPath;
			adventureInt->terrain.currentPath = NULL;
			adventureInt->heroList.items[adventureInt->heroList.getPosOfHero(ho)].second = NULL;
		}
	}


	if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
	{
		ho->moveDir = 1;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, -31)));
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 33, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 65, -31)));

		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 33)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
	{
		ho->moveDir = 2;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
	{
		ho->moveDir = 3;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 31, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 63, -31)));
		CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 33), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 33)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
	{
		ho->moveDir = 4;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 0), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 0)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 32), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 32)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
	{
		ho->moveDir = 5;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, -1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, -1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 31), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 31)));

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 31, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 63, 63)));
		CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
	{
		ho->moveDir = 6;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31), ho->id);

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
	{
		ho->moveDir = 7;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, -1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, -1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 31)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 31), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 63)));
		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 33, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 65, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
	{
		ho->moveDir = 8;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 0)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 0), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 32)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 32), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	//first initializing done
	SDL_framerateDelay(mainFPSmng); // after first move
	//main moving
	for(int i=1; i<32; i+=2*heroMoveSpeed)
	{
		if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
		{
			//setting advmap shift
			adventureInt->terrain.moveX = i-32;
			adventureInt->terrain.moveY = i-32;

			subRect(hp.x-3, hp.y-2, hp.z, genRect(32, 32, -31+i, -31+i), ho->id);
			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 1+i, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 33+i, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 65+i, -31+i), ho->id);

			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 1+i), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 1+i), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 33+i), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 33+i), ho->id);
		}
		else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
		{
			//setting advmap shift
			adventureInt->terrain.moveY = i-32;

			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 0, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 32, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 64, -31+i), ho->id);

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1+i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33+i), ho->id);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
		{
			//setting advmap shift
			adventureInt->terrain.moveX = -i+32;
			adventureInt->terrain.moveY = i-32;

			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, -1-i, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 31-i, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 63-i, -31+i), ho->id);
			subRect(hp.x+1, hp.y-2, hp.z, genRect(32, 32, 95-i, -31+i), ho->id);

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 1+i), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 1+i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 33+i), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 33+i), ho->id);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
		{
			//setting advmap shift
			adventureInt->terrain.moveX = -i+32;

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 0), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 0), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 32), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 32), ho->id);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
		{
			
			//setting advmap shift
			adventureInt->terrain.moveX = -i+32;
			adventureInt->terrain.moveY = -i+32;

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, -1-i), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, -1-i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 31-i), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 31-i), ho->id);

			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, -1-i, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 31-i, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 63-i, 63-i), ho->id);
			subRect(hp.x+1, hp.y+1, hp.z, genRect(32, 32, 95-i, 63-i), ho->id);
		}
		else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
		{
			//setting advmap shift
			adventureInt->terrain.moveY = -i+32;

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1-i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31-i), ho->id);

			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 0, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 32, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 64, 63-i), ho->id);
		}
		else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
		{
			//setting advmap shift
			adventureInt->terrain.moveX = i-32;
			adventureInt->terrain.moveY = -i+32;

			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, -1-i), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, -1-i), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 31-i), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 31-i), ho->id);

			subRect(hp.x-3, hp.y+1, hp.z, genRect(32, 32, -31+i, 63-i), ho->id);
			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 1+i, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 33+i, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 65+i, 63-i), ho->id);
		}
		else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
		{
			//setting advmap shift
			adventureInt->terrain.moveX = i-32;

			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 0), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 0), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 32), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 32), ho->id);
		}
		adventureInt->updateScreen = true;
		adventureInt->show(screen);
		//LOCPLINT->adventureInt->show(); //updating screen
		CSDL_Ext::update(screen);

		SDL_Delay(5);
		SDL_framerateDelay(mainFPSmng); //for animation purposes
	} //for(int i=1; i<32; i+=4)
	//main moving done
	//finishing move

	//restoring adventureInt->terrain.move*
	adventureInt->terrain.moveX = adventureInt->terrain.moveY = 0;

	if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
	{
		delObjRect(hp.x, hp.y-2, hp.z, ho->id);
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
		delObjRect(hp.x-3, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
	{
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
	{
		delObjRect(hp.x-2, hp.y-2, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x+1, hp.y, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
	{
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
	{
		delObjRect(hp.x-2, hp.y+1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
		delObjRect(hp.x+1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
	}
	else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-3, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x, hp.y+1, hp.z, ho->id);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
	}

	//restoring good rects
	subRect(details.dst.x-2, details.dst.y-1, details.dst.z, genRect(32, 32, 0, 0), ho->id);
	subRect(details.dst.x-1, details.dst.y-1, details.dst.z, genRect(32, 32, 32, 0), ho->id);
	subRect(details.dst.x, details.dst.y-1, details.dst.z, genRect(32, 32, 64, 0), ho->id);

	subRect(details.dst.x-2, details.dst.y, details.dst.z, genRect(32, 32, 0, 32), ho->id);
	subRect(details.dst.x-1, details.dst.y, details.dst.z, genRect(32, 32, 32, 32), ho->id);
	subRect(details.dst.x, details.dst.y, details.dst.z, genRect(32, 32, 64, 32), ho->id);

	//restoring good order of objects
	std::stable_sort(CGI->mh->ttiles[details.dst.x-2][details.dst.y-1][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-2][details.dst.y-1][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x-1][details.dst.y-1][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-1][details.dst.y-1][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x][details.dst.y-1][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x][details.dst.y-1][details.dst.z].objects.end(), ocmptwo_cgin);

	std::stable_sort(CGI->mh->ttiles[details.dst.x-2][details.dst.y][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-2][details.dst.y][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x-1][details.dst.y][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x-1][details.dst.y][details.dst.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.dst.x][details.dst.y][details.dst.z].objects.begin(), CGI->mh->ttiles[details.dst.x][details.dst.y][details.dst.z].objects.end(), ocmptwo_cgin);

	ho->isStanding = true;
	//move finished
	adventureInt->minimap.draw(screen2);
	adventureInt->heroList.updateMove(ho);

	//check if user cancelled movement
	{
		boost::unique_lock<boost::mutex> un(eventsM);
		while(events.size())
		{
			SDL_Event *ev = events.front();
			events.pop();
			switch(ev->type)
			{
			case SDL_MOUSEBUTTONDOWN:
				stillMoveHero.setn(STOP_MOVE);
				break;
			case SDL_KEYDOWN:
				if(ev->key.keysym.sym < SDLK_F1)
					stillMoveHero.setn(STOP_MOVE);
				break;
			}
			delete ev;
		}
	}

	if(stillMoveHero.get() == 1)
		stillMoveHero.setn(DURING_MOVE);

}
void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	graphics->heroWins.erase(hero->ID);
	adventureInt->heroList.updateHList(hero);
}
void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(graphics->heroWins.find(hero->subID)==graphics->heroWins.end())
		graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hero->subID,infoWin(hero)));
	adventureInt->heroList.updateHList();
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	castleInt = new CCastleInterface(town);
	CGI->audioh->playMusic(castleInt->musicID, -1);
	LOCPLINT->pushInt(castleInt);
}

SDL_Surface * CPlayerInterface::infoWin(const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if (specific)
	{
		switch (specific->ID)
		{
		case HEROI_TYPE:
			return graphics->drawHeroInfoWin(dynamic_cast<const CGHeroInstance*>(specific));
			break;
		case TOWNI_TYPE:
			return graphics->drawTownInfoWin(dynamic_cast<const CGTownInstance*>(specific));
			break;
		default:
			return NULL;
			break;
		}
	}
	else
	{
		switch (adventureInt->selection->ID)
		{
		case HEROI_TYPE:
			{
				const CGHeroInstance * curh = (const CGHeroInstance *)adventureInt->selection;
				return graphics->drawHeroInfoWin(curh);
			}
		case TOWNI_TYPE:
			{
				return graphics->drawTownInfoWin((const CGTownInstance *)adventureInt->selection);
			}
		default:
			tlog1 << "Strange... selection is neither hero nor town\n";
			return NULL;
		}
	}
}

void CPlayerInterface::handleMouseMotion(SDL_Event *sEvent)
{
	//sending active, hovered hoverable objects hover() call
	std::vector<Hoverable*> hlp;
	for(std::list<Hoverable*>::iterator i=hoverable.begin(); i != hoverable.end();i++)
	{
		if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			if (!(*i)->hovered)
				hlp.push_back((*i));
		}
		else if ((*i)->hovered)
		{
			(*i)->hover(false);
		}
	}
	for(int i=0; i<hlp.size();i++)
		hlp[i]->hover(true);

	//sending active, MotionInterested objects mouseMoved() call
	std::list<MotionInterested*> miCopy = motioninterested;
	for(std::list<MotionInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
	{
		if ((*i)->strongInterest || isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
		{
			(*i)->mouseMoved(sEvent->motion);
		}
	}

	//adventure map scrolling with mouse
	if(!SDL_GetKeyState(NULL)[SDLK_LCTRL])
	{
		if(sEvent->motion.x<15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::LEFT;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::LEFT;
		}
		if(sEvent->motion.x>screen->w-15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::RIGHT;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::RIGHT;
		}
		if(sEvent->motion.y<15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::UP;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::UP;
		}
		if(sEvent->motion.y>screen->h-15)
		{
			adventureInt->scrollingDir |= CAdvMapInt::DOWN;
		}
		else
		{
			adventureInt->scrollingDir &= ~CAdvMapInt::DOWN;
		}
	}
}
void CPlayerInterface::handleEvent(SDL_Event *sEvent)
{
	current = sEvent;

	if (sEvent->type==SDL_KEYDOWN || sEvent->type==SDL_KEYUP)
	{
		SDL_KeyboardEvent key = sEvent->key;

		//translate numpad keys
		if (key.keysym.sym >= SDLK_KP0  && key.keysym.sym <= SDLK_KP9)
		{
			key.keysym.sym = (SDLKey) (key.keysym.sym - SDLK_KP0 + SDLK_0);
		}
		else if(key.keysym.sym == SDLK_KP_ENTER)
		{
			key.keysym.sym = (SDLKey)SDLK_RETURN;
		}

		bool keysCaptured = false;
		for(std::list<KeyInterested*>::iterator i=keyinterested.begin(); i != keyinterested.end();i++)
		{
			if((*i)->captureAllKeys)
			{
				keysCaptured = true;
				break;
			}
		}

		std::list<KeyInterested*> miCopy = keyinterested;
		for(std::list<KeyInterested*>::iterator i=miCopy.begin(); i != miCopy.end();i++)
			if(vstd::contains(keyinterested,*i) && (!keysCaptured || (*i)->captureAllKeys))
				(**i).keyPressed(key);
	}
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		CGI->curh->cursorMove(sEvent->motion.x, sEvent->motion.y);
		handleMouseMotion(sEvent);
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		std::list<ClickableL*> hlp = lclickable;
		for(std::list<ClickableL*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		std::list<ClickableL*> hlp = lclickable;
		for(std::list<ClickableL*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(lclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickLeft(false);
			}
			else
				(*i)->clickLeft(boost::logic::indeterminate);
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<ClickableR*> hlp = rclickable;
		for(std::list<ClickableR*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_RIGHT))
	{
		std::list<ClickableR*> hlp = rclickable;
		for(std::list<ClickableR*>::iterator i=hlp.begin(); i != hlp.end();i++)
		{
			if(!vstd::contains(rclickable,*i)) continue;
			if (isItIn(&(*i)->pos,sEvent->motion.x,sEvent->motion.y))
			{
				(*i)->clickRight(false);
			}
			else
				(*i)->clickRight(boost::logic::indeterminate);
		}
	}
	current = NULL;

} //event end

int3 CPlayerInterface::repairScreenPos(int3 pos)
{
	if(pos.x<=-Woff)
		pos.x = -Woff+1;
	if(pos.y<=-Hoff)
		pos.y = -Hoff+1;
	if(pos.x>CGI->mh->map->width - this->adventureInt->terrain.tilesw + Woff)
		pos.x = CGI->mh->map->width - this->adventureInt->terrain.tilesw + Woff;
	if(pos.y>CGI->mh->map->height - this->adventureInt->terrain.tilesh + Hoff)
		pos.y = CGI->mh->map->height - this->adventureInt->terrain.tilesh + Hoff;
	return pos;
}
void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	if(which >= PRIMARY_SKILLS) //no need to redraw infowin if this is experience (exp is treated as prim skill with id==4)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}
void CPlayerInterface::heroManaPointsChanged(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}
void CPlayerInterface::heroMovePointsChanged(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	//adventureInt->heroList.draw();
}
void CPlayerInterface::receivedResource(int type, int val)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	LOCPLINT->totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16>& skills, boost::function<void(ui32)> &callback)
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	CGI->audioh->playSound(soundBase::heroNewLevel);

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CLevelWindow *lw = new CLevelWindow(hero,pskill,skills,callback);
	LOCPLINT->pushInt(lw);
}
void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	//redraw infowindow
	SDL_FreeSurface(graphics->townWins[town->id]);
	graphics->townWins[town->id] = infoWin(town);
	if(town->garrisonHero)
	{
		CGI->mh->hideObject(town->garrisonHero);
	}
	if(town->visitingHero)
	{
		CGI->mh->printObject(town->visitingHero);
	}
	adventureInt->heroList.updateHList();

	CCastleInterface *c = castleInt;
	if(c)
	{
		c->garr->highlighted = NULL;
		c->hslotup.hero = town->garrisonHero;
		c->garr->odown = c->hslotdown.hero = town->visitingHero;
		c->garr->set2 = town->visitingHero ? &town->visitingHero->army : NULL;
		c->garr->recreateSlots();
	}
	LOCPLINT->totalRedraw();
}
void CPlayerInterface::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	if(hero->tempOwner != town->tempOwner)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	openTownWindow(town);
}
void CPlayerInterface::garrisonChanged(const CGObjectInstance * obj)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(obj->ID == HEROI_TYPE) //hero
	{
		const CGHeroInstance * hh;
		if(hh = dynamic_cast<const CGHeroInstance*>(obj))
		{
			SDL_FreeSurface(graphics->heroWins[hh->subID]);
			graphics->heroWins[hh->subID] = infoWin(hh);
		}
	}
	else if (obj->ID == TOWNI_TYPE) //town
	{
		const CGTownInstance * tt;
		if(tt = static_cast<const CGTownInstance*>(obj))
		{
			SDL_FreeSurface(graphics->townWins[tt->id]);
			graphics->townWins[tt->id] = infoWin(tt);
		}
		if(tt->visitingHero)
		{
			SDL_FreeSurface(graphics->heroWins[tt->visitingHero->subID]);
			graphics->heroWins[tt->visitingHero->subID] = infoWin(tt->visitingHero);
		}

	}

	bool wasGarrison = false;
	for(std::list<IShowActivable*>::iterator i = listInt.begin(); i != listInt.end(); i++)
	{
		if((*i)->type & IShowActivable::WITH_GARRISON)
		{
			CWindowWithGarrison *wwg = static_cast<CWindowWithGarrison*>(*i);
			wwg->garr->recreateSlots();
			wasGarrison = true;
		}
	}

	LOCPLINT->totalRedraw();
}

void CPlayerInterface::buildChanged(const CGTownInstance *town, int buildingID, int what) //what: 1 - built, 2 - demolished
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	switch (buildingID)
	{
	case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 15:
		{
			SDL_FreeSurface(graphics->townWins[town->id]);
			graphics->townWins[town->id] = infoWin(town);
			break;
		}
	}
	if(!castleInt)
		return;
	if(castleInt->town!=town)
		return;
	switch(what)
	{
	case 1:
		CGI->audioh->playSound(soundBase::newBuilding);
		castleInt->addBuilding(buildingID);
		break;
	case 2:
		castleInt->removeBuilding(buildingID);
		break;
	}
}

void CPlayerInterface::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side) //called by engine when battle starts; side=0 - left, side=1 - right
{
	while(showingDialog->get())
		SDL_Delay(20);

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt = new CBattleInterface(army1, army2, hero1, hero2, genRect(600, 800, (conf.cc.resx - 800)/2, (conf.cc.resy - 600)/2));
	CGI->audioh->playMusicFromSet(CGI->audioh->battleMusics, -1);
	pushInt(battleInt);
}

void CPlayerInterface::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles) //called when battlefield is prepared, prior the battle beginning
{
}

void CPlayerInterface::battleNewRound(int round) //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->newRound(round);
}

void CPlayerInterface::actionStarted(const BattleAction* action)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	curAction = new BattleAction(*action);
	if( (action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber))) )
	{
		battleInt->moveStarted = true;
		if(battleInt->creAnims[action->stackNumber]->framesInGroup(20))
		{
			battleInt->creAnims[action->stackNumber]->setType(20);
		}
	}


	battleInt->deactivate();

	CStack *stack = cb->battleGetStackByID(action->stackNumber);
	char txt[400];

	if(action->actionType == 1)
	{
		if(action->side)
			battleInt->defendingHero->setPhase(4);
		else
			battleInt->attackingHero->setPhase(4);
		return;
	}
	if(!stack)
	{
		tlog1<<"Something wrong with stackNumber in actionStarted. Stack number: "<<action->stackNumber<<std::endl;
		return;
	}

	int txtid = 0;
	switch(action->actionType)
	{
	case 3: //defend
		txtid = 120;
		break;
	case 8: //wait
		txtid = 136;
		break;
	case 11: //bad morale
		txtid = -34; //negative -> no separate singular/plural form		
		battleInt->displayEffect(30,stack->position);
		break;
	}

	if(txtid > 0  &&  stack->amount != 1)
		txtid++; //move to plural text
	else if(txtid < 0)
		txtid = -txtid;

	if(txtid)
	{
		sprintf(txt, CGI->generaltexth->allTexts[txtid].c_str(),  (stack->amount != 1) ? stack->creature->namePl.c_str() : stack->creature->nameSing.c_str(), 0);
		LOCPLINT->battleInt->console->addText(txt);
	}
}

void CPlayerInterface::actionFinished(const BattleAction* action)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	delete curAction;
	curAction = NULL;
	//if((action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber)))) //activating interface when move is finished
	{
		battleInt->activate();
	}
	if(action->actionType == 1)
	{
		if(action->side)
			battleInt->defendingHero->setPhase(0);
		else
			battleInt->attackingHero->setPhase(0);
	}
}

BattleAction CPlayerInterface::activeStack(int stackID) //called when it's turn of that stack
{
	CBattleInterface *b = battleInt;
	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);

		CStack *stack = cb->battleGetStackByID(stackID);
		if(vstd::contains(stack->state,MOVED)) //this stack has moved and makes second action -> high morale
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->amount != 1) ? stack->creature->namePl : stack->creature->nameSing);
			battleInt->displayEffect(20,stack->position);
			battleInt->console->addText(hlp);
		}

		b->stackActivated(stackID);
	}
	//wait till BattleInterface sets its command
	boost::unique_lock<boost::mutex> lock(b->givenCommand->mx);
	while(!b->givenCommand->data)
		b->givenCommand->cond.wait(lock);

	//tidy up
	BattleAction ret = *(b->givenCommand->data);
	delete b->givenCommand->data;
	b->givenCommand->data = NULL;

	//return command
	return ret;
}

void CPlayerInterface::battleEnd(BattleResult *br)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->battleFinished(*br);
}

void CPlayerInterface::battleStackMoved(int ID, int dest, int distance, bool end)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->stackMoved(ID, dest, end, distance);
}
void CPlayerInterface::battleSpellCast(SpellCast *sc)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->spellCast(sc);
}
void CPlayerInterface::battleStacksEffectsSet(SetStackEffect & sse)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->battleStacksEffectsSet(sse);
}
void CPlayerInterface::battleStacksAttacked(std::set<BattleStackAttacked> & bsa)
{
	tlog5 << "CPlayerInterface::battleStackAttacked - locking...";
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	tlog5 << "done!\n";


	std::vector<CBattleInterface::SStackAttackedInfo> arg;
	for(std::set<BattleStackAttacked>::iterator i = bsa.begin(); i != bsa.end(); i++)
	{
		if(i->isEffect() && i->effect != 12) //and not armageddon
		{
			battleInt->displayEffect(i->effect, cb->battleGetStackByID(i->stackAttacked)->position);
		}
		CBattleInterface::SStackAttackedInfo to_put = {i->stackAttacked, i->damageAmount, i->killedAmount, LOCPLINT->curAction->stackNumber, LOCPLINT->curAction->actionType==7, i->killed()};
		arg.push_back(to_put);
	}

	if(bsa.begin()->isEffect() && bsa.begin()->effect == 12) //for armageddon - I hope this condition is enough
	{
		battleInt->displayEffect(bsa.begin()->effect, -1);
	}

	battleInt->stacksAreAttacked(arg);
}
void CPlayerInterface::battleAttack(BattleAttack *ba)
{
	tlog5 << "CPlayerInterface::battleAttack - locking...";
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	tlog5 << "done!\n";
	if(ba->lucky()) //lucky hit
	{
		CStack *stack = cb->battleGetStackByID(ba->stackAttacking);
		std::string hlp = CGI->generaltexth->allTexts[45];
		boost::algorithm::replace_first(hlp,"%s",(stack->amount != 1) ? stack->creature->namePl.c_str() : stack->creature->nameSing.c_str());
		battleInt->console->addText(hlp);
		battleInt->displayEffect(18,stack->position);
	}
	//TODO: bad luck?

	if(ba->shot())
	{
		for(std::set<BattleStackAttacked>::iterator i = ba->bsa.begin(); i != ba->bsa.end(); i++)
			battleInt->stackIsShooting(ba->stackAttacking,cb->battleGetPos(i->stackAttacked));
	}
	else
	{
		CStack * attacker = cb->battleGetStackByID(ba->stackAttacking);
		int shift = 0;		
		if(ba->counter() && BattleInfo::mutualPosition(curAction->destinationTile, attacker->position) < 0)		
		{			
			if(attacker->attackerOwned)				
				shift = 1;			
			else				
				shift = -1;		
		}		
		battleInt->stackAttacking( ba->stackAttacking, ba->counter() ? curAction->destinationTile + shift : curAction->additionalInfo );	
	}
}

void CPlayerInterface::showComp(SComponent comp)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	CGI->audioh->playSoundFromSet(CGI->audioh->pickupSounds);

	adventureInt->infoBar.showComp(&comp,4000);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID)
{
	std::vector<SComponent*> intComps;
	for(int i=0;i<components.size();i++)
		intComps.push_back(new SComponent(*components[i]));
	showInfoDialog(text,intComps,soundID);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<SComponent*> & components, int soundID)
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	
	if(stillMoveHero.get() == DURING_MOVE)//if we are in the middle of hero movement
		stillMoveHero.setn(STOP_MOVE); //after showing dialog movement will be stopped

	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text,playerID,36,components,pom,false);

	if(makingTurn && listInt.size())
	{
		CGI->audioh->playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->set(true);
		pushInt(temp);
	}
	else
	{
		dialogs.push_back(temp);
	}
}

void CPlayerInterface::showYesNoDialog(const std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	LOCPLINT->showingDialog->setn(true);
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text,playerID,36,components,pom,DelComps);
	temp->delComps = DelComps;
	for(int i=0;i<onYes.funcs.size();i++)
		temp->buttons[0]->callback += onYes.funcs[i];
	for(int i=0;i<onNo.funcs.size();i++)
		temp->buttons[1]->callback += onNo.funcs[i];

	LOCPLINT->pushInt(temp);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	CGI->audioh->playSound(static_cast<soundBase::soundID>(soundID));

	if(!selection && cancel) //simple yes/no dialog
	{
		std::vector<SComponent*> intComps;
		for(int i=0;i<components.size();i++)
			intComps.push_back(new SComponent(components[i])); //will be deleted by close in window

		showYesNoDialog(text,intComps,boost::bind(&CCallback::selectionMade,cb,1,askID),boost::bind(&CCallback::selectionMade,cb,0,askID),true);
	}
	else if(selection)
	{
		std::vector<CSelectableComponent*> intComps;
		for(int i=0;i<components.size();i++)
			intComps.push_back(new CSelectableComponent(components[i])); //will be deleted by CSelWindow::close

		std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
		pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
		if(cancel)
		{
			pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
		}

		CSelWindow * temp = new CSelWindow(text,playerID,35,intComps,pom,askID);
		pushInt(temp);
		intComps[0]->clickLeft(true);
	}

}

void CPlayerInterface::tileRevealed(const std::set<int3> &pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::set<int3>::const_iterator i=pos.begin(); i!=pos.end();i++)
		adventureInt->minimap.showTile(*i);
}

void CPlayerInterface::tileHidden(const std::set<int3> &pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::set<int3>::const_iterator i=pos.begin(); i!=pos.end();i++)
		adventureInt->minimap.hideTile(*i);
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->heroWindow->setHero(hero);
	adventureInt->heroWindow->quitButton->callback = boost::bind(&CHeroWindow::quit,adventureInt->heroWindow);
	pushInt(adventureInt->heroWindow);
}

void CPlayerInterface::heroArtifactSetChanged(const CGHeroInstance*hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(adventureInt->heroWindow->curHero)
	{
		adventureInt->heroWindow->deactivate();
		adventureInt->heroWindow->setHero(adventureInt->heroWindow->curHero);
		adventureInt->heroWindow->activate();
	}
}

void CPlayerInterface::updateWater()
{

}

void CPlayerInterface::availableCreaturesChanged( const CGTownInstance *town )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(castleInt)
	{
		CFortScreen *fs = dynamic_cast<CFortScreen*>(listInt.front());
		if(fs)
			fs->draw(castleInt,false);
	}
}

void CPlayerInterface::heroBonusChanged( const CGHeroInstance *hero, const HeroBonus &bonus, bool gain )
{
	if(bonus.type == HeroBonus::NONE)	return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}

template <typename Handler> void CPlayerInterface::serializeTempl( Handler &h, const int version )
{
	h & playerID & serialID;
	h & heroMoveSpeed & mapScrollingSpeed;
	h & CBattleInterface::settings;
}

void CPlayerInterface::serialize( COSer<CSaveFile> &h, const int version )
{
	serializeTempl(h,version);
}

void CPlayerInterface::serialize( CISer<CLoadFile> &h, const int version )
{
	serializeTempl(h,version);
}

void CPlayerInterface::redrawHeroWin(const CGHeroInstance * hero)
{
	if(!vstd::contains(graphics->heroWins,hero->subID))
	{
		tlog1 << "Cannot redraw infowindow for hero with subID=" << hero->subID << " - not present in our map\n";
		return;
	}
	SDL_FreeSurface(graphics->heroWins[hero->subID]);
	graphics->heroWins[hero->subID] = infoWin(hero); 
	if (adventureInt->selection == hero)
		adventureInt->infoBar.draw(screen);
}

bool CPlayerInterface::moveHero( const CGHeroInstance *h, CPath path )
{
	if (!h)
		return false; //can't find hero

	bool result = false;
	path.convert(0);
	boost::unique_lock<boost::mutex> un(stillMoveHero.mx);
	stillMoveHero.data = CONTINUE_MOVE;

	enum TerrainTile::EterrainType currentTerrain = TerrainTile::border; // not init yet
	enum TerrainTile::EterrainType newTerrain;
	int sh = -1;

	for(int i=path.nodes.size()-1; i>0 && stillMoveHero.data == CONTINUE_MOVE; i--)
	{
		// Start a new sound for the hero movement or let the existing one carry on.
#if 0
		// TODO
		if (hero is flying && sh == -1)
			sh = CGI->audioh->playSound(soundBase::horseFlying, -1);
		} 
		else if (hero is in a boat && sh = -1) {
			sh = CGI->audioh->playSound(soundBase::sound_todo, -1);
		} else
#endif
		{
			newTerrain = cb->getTileInfo(path.nodes[i].coord)->tertype;

			if (newTerrain != currentTerrain) {
				CGI->audioh->stopSound(sh);
				sh = CGI->audioh->playSound(CGI->audioh->horseSounds[newTerrain], -1);
				currentTerrain = newTerrain;
			}
		}

		stillMoveHero.data = WAITING_MOVE;

		int3 endpos(path.nodes[i-1].coord.x, path.nodes[i-1].coord.y, h->pos.z);
		cb->moveHero(h,endpos);
		while(stillMoveHero.data != STOP_MOVE  &&  stillMoveHero.data != CONTINUE_MOVE)
			stillMoveHero.cond.wait(un);
	}

	CGI->audioh->stopSound(sh);

	//stillMoveHero = false;
	return result;
}

bool CPlayerInterface::shiftPressed() const
{
	return SDL_GetKeyState(NULL)[SDLK_LSHIFT]  ||  SDL_GetKeyState(NULL)[SDLK_RSHIFT];
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd )
{
	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	while(dialogs.size())
	{
		pim->unlock();
		SDL_Delay(20);
		pim->lock();
	}
	CGarrisonWindow *cgw = new CGarrisonWindow(up,down);
	cgw->quit->callback += onEnd;
	pushInt(cgw);
}

void CPlayerInterface::popInt( IShowActivable *top )
{
	assert(listInt.front() == top);
	top->deactivate();
	listInt.pop_front();
	objsToBlit -= top;
	if(listInt.size())
		listInt.front()->activate();
	totalRedraw();
}

void CPlayerInterface::popIntTotally( IShowActivable *top )
{
	assert(listInt.front() == top);
	popInt(top);
	delete top;
}

void CPlayerInterface::pushInt( IShowActivable *newInt )
{
	if(listInt.size())
		listInt.front()->deactivate();
	listInt.push_front(newInt);
	newInt->activate();
	objsToBlit += newInt;
	LOCPLINT->totalRedraw();
}

void CPlayerInterface::popInts( int howMany )
{
	if(!howMany) return; //senseless but who knows...

	assert(listInt.size() > howMany);
	listInt.front()->deactivate();
	for(int i=0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		delete listInt.front();
		listInt.pop_front();
	}
	listInt.front()->activate();
	totalRedraw();
}

IShowActivable * CPlayerInterface::topInt()
{
	if(!listInt.size())
		return NULL;
	else 
		return listInt.front();
}

void CPlayerInterface::totalRedraw()
{
	for(int i=0;i<objsToBlit.size();i++)
		objsToBlit[i]->showAll(screen2);

	blitAt(screen2,0,0,screen);

	if(objsToBlit.size())
		objsToBlit.back()->showAll(screen);
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	if(stillMoveHero.get() == DURING_MOVE)
		stillMoveHero.setn(CONTINUE_MOVE);
}


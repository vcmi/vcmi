#include "stdafx.h"
#include "CGameInterface.h"
#include "CAdvMapInterface.h"
#include "CMessage.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include "CScreenHandler.h"
#include "CCursorHandler.h"
#include "CCallback.h"
#include "SDL_Extensions.h"
#include "hch/CLodHandler.h"
#include <sstream>
using namespace CSDL_Ext;
class OCM_HLP_CGIN
{
public:
	bool operator ()(const std::pair<CObjectInstance *, SDL_Rect> & a, const std::pair<CObjectInstance *, SDL_Rect> & b) const
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo_cgin ;
CButtonBase::CButtonBase()
{
	curimg=0;
	type=-1;
	abs=false;
	active=false;
	ourObj=NULL;
	state=0;
}
void CButtonBase::show()
{
	if (abs)
	{
		blitAt(imgs[curimg][state],pos.x,pos.y);
		updateRect(&pos);
	}
	else
	{
		blitAt(imgs[curimg][state],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y);
		updateRect(&genRect(pos.h,pos.w,pos.x+ourObj->pos.x,pos.y+ourObj->pos.y));
		
	}
}
ClickableL::ClickableL()
{
	pressedL=false;
}
void ClickableL::clickLeft(tribool down)
{
	if (down)
		pressedL=true;
	else
		pressedL=false;
}
void ClickableL::activate()
{
	LOCPLINT->lclickable.push_back(this);
}
void ClickableL::deactivate()
{
	LOCPLINT->lclickable.erase
		(std::find(LOCPLINT->lclickable.begin(),LOCPLINT->lclickable.end(),this));
}
ClickableR::ClickableR()
{
	pressedR=false;
}
void ClickableR::activate()
{
	LOCPLINT->rclickable.push_back(this);
}
void ClickableR::deactivate()
{
	LOCPLINT->rclickable.erase(std::find(LOCPLINT->rclickable.begin(),LOCPLINT->rclickable.end(),this));
}
void Hoverable::activate()
{
	LOCPLINT->hoverable.push_back(this);
}
void Hoverable::deactivate()
{
	LOCPLINT->hoverable.erase(std::find(LOCPLINT->hoverable.begin(),LOCPLINT->hoverable.end(),this));
}
void Hoverable::hover(bool on)
{
	hovered=on;
}
void KeyInterested::activate()
{
	LOCPLINT->keyinterested.push_back(this);
}
void KeyInterested::deactivate()
{
	LOCPLINT->
		keyinterested.erase(std::find(LOCPLINT->keyinterested.begin(),LOCPLINT->keyinterested.end(),this));
}
void MotionInterested::activate()
{
	LOCPLINT->motioninterested.push_back(this);
}
void MotionInterested::deactivate()
{
	LOCPLINT->
		motioninterested.erase(std::find(LOCPLINT->motioninterested.begin(),LOCPLINT->motioninterested.end(),this));
}
CPlayerInterface::CPlayerInterface(int Player, int serial)
{
	playerID=Player;
	serialID=serial;
	CGI->localPlayer = playerID;
	human=true;
	hInfo = CGI->bitmaph->loadBitmap("HEROQVBK.bmp");
}
void CPlayerInterface::init(CCallback * CB)
{
	cb = CB;
	CGI->localPlayer = serialID;
	adventureInt = new CAdvMapInt(playerID);
}
void CPlayerInterface::yourTurn()
{
	CGI->localPlayer = serialID;
	unsigned char & animVal = LOCPLINT->adventureInt->anim; //for animations handling
	adventureInt->show();
	//show rest of things

	//initializing framerate keeper
	mainFPSmng = new FPSmanager;
	SDL_initFramerate(mainFPSmng);
	SDL_setFramerate(mainFPSmng, 24);
	SDL_Event sEvent;
	//framerate keeper initialized
	for(;;) // main loop
	{
		CGI->screenh->updateScreen();

		LOCPLINT->adventureInt->updateScreen = false;
		while (SDL_PollEvent(&sEvent))  //wait for event...
		{
			handleEvent(&sEvent);
		}
		++LOCPLINT->adventureInt->animValHitCount; //for animations
		if(LOCPLINT->adventureInt->animValHitCount == 2)
		{
			LOCPLINT->adventureInt->animValHitCount = 0;
			++animVal;
			LOCPLINT->adventureInt->updateScreen = true;

		}
		if(LOCPLINT->adventureInt->scrollingLeft)
		{
			if(LOCPLINT->adventureInt->position.x>-Woff)
			{
				LOCPLINT->adventureInt->position.x--;
				LOCPLINT->adventureInt->updateScreen = true;
			}
		}
		if(LOCPLINT->adventureInt->scrollingRight)
		{
			if(LOCPLINT->adventureInt->position.x<CGI->ac->map.width-19+4)
			{
				LOCPLINT->adventureInt->position.x++;
				LOCPLINT->adventureInt->updateScreen = true;
			}
		}
		if(LOCPLINT->adventureInt->scrollingUp)
		{
			if(LOCPLINT->adventureInt->position.y>-Hoff)
			{
				LOCPLINT->adventureInt->position.y--;
				LOCPLINT->adventureInt->updateScreen = true;
			}
		}
		if(LOCPLINT->adventureInt->scrollingDown)
		{
			if(LOCPLINT->adventureInt->position.y<CGI->ac->map.height-18+4)
			{
				LOCPLINT->adventureInt->position.y++;
				LOCPLINT->adventureInt->updateScreen = true;
			}
		}
		if(LOCPLINT->adventureInt->updateScreen)
		{
			adventureInt->update();
			LOCPLINT->adventureInt->updateScreen=false;
		}
		SDL_Delay(5); //give time for other apps
		SDL_framerateDelay(mainFPSmng);
	}
}

inline void subRect(const int & x, const int & y, const int & z, SDL_Rect & r, const int & hid)
{
	for(int h=0; h<CGI->mh->ttiles[x][y][z].objects.size(); ++h)
		if(CGI->mh->ttiles[x][y][z].objects[h].first->id==hid)
			CGI->mh->ttiles[x][y][z].objects[h].second = r;
}

inline void delObjRect(const int & x, const int & y, const int & z, const int & hid)
{
	for(int h=0; h<CGI->mh->ttiles[x][y][z].objects.size(); ++h)
		if(CGI->mh->ttiles[x][y][z].objects[h].first->id==hid)
			CGI->mh->ttiles[x][y][z].objects.erase(CGI->mh->ttiles[x][y][z].objects.begin()+h);
}

void CPlayerInterface::heroMoved(const HeroMoveDetails & details)
{
	//initializing objects and performing first step of move
	CObjectInstance * ho = CGI->heroh->heroInstances[details.heroID]->ourObject; //object representing this hero
	int3 hp = CGI->heroh->heroInstances[details.heroID]->pos;
	if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
	{
		ho->moveDir = 1;
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
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
	{
		ho->moveDir = 2;
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

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
	{
		ho->moveDir = 3;
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

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
	{
		ho->moveDir = 4;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 0), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 0)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 32), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 32)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
	{
		ho->moveDir = 5;
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

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
	{
		ho->moveDir = 6;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31), ho->id);

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
	{
		ho->moveDir = 7;
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
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
	{
		ho->moveDir = 8;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 0)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 0), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 32)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 32), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	//first initializing done
	SDL_framerateDelay(mainFPSmng); // after first move
	//main moving
	for(int i=1; i<32; i+=4)
	{
		if(details.dst.x+1 == details.src.x && details.dst.y+1 == details.src.y) //tl
		{
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

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x == details.src.x && details.dst.y+1 == details.src.y) //t
		{
			subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 0, -31+i), ho->id);
			subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 32, -31+i), ho->id);
			subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 64, -31+i), ho->id);

			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1+i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1+i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1+i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33+i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33+i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33+i), ho->id);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y+1 == details.src.y) //tr
		{
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

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y == details.src.y) //r
		{
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 0), ho->id);
			subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 0), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 32), ho->id);
			subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 32), ho->id);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x-1 == details.src.x && details.dst.y-1 == details.src.y) //br
		{
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

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x == details.src.x && details.dst.y-1 == details.src.y) //b
		{
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1-i), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1-i), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1-i), ho->id);

			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31-i), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31-i), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31-i), ho->id);

			subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 0, 63-i), ho->id);
			subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 32, 63-i), ho->id);
			subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 64, 63-i), ho->id);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x+1 == details.src.x && details.dst.y-1 == details.src.y) //bl
		{
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

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		}
		else if(details.dst.x+1 == details.src.x && details.dst.y == details.src.y) //l
		{
			subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 0), ho->id);
			subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 0), ho->id);
			subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 0), ho->id);
			subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 0), ho->id);

			subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 32), ho->id);
			subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 32), ho->id);
			subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 32), ho->id);
			subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 32), ho->id);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

			std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
			std::stable_sort(CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y][hp.z].objects.end(), ocmptwo_cgin);
		}
		LOCPLINT->adventureInt->update(); //updating screen
		CGI->screenh->updateScreen();
		SDL_framerateDelay(mainFPSmng); //for animation purposes
	}
	//main moving done
	//finishing move
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
	CGI->heroh->heroInstances[details.heroID]->pos = details.dst; //actualizing hero position
	CGI->heroh->heroInstances[details.heroID]->ourObject->pos = details.dst; //copy of hero's position
	ho->moveDir = 0; //move ended
	//move finished
}
void CPlayerInterface::heroKilled(const CHeroInstance * hero)
{
}
void CPlayerInterface::heroCreated(const CHeroInstance * hero)
{
}

SDL_Surface * CPlayerInterface::infoWin(void * specific) //specific=0 => draws info about selected town/hero //TODO - gdy sie dorobi sensowna hierarchie klas ins. to wywalic tego brzydkiego void*
{
	if (specific)
		;//TODO: dorobic, ale w ogole to moze lepiej najpierw zastapic tego voida czym innym
	else
	{
		if (adventureInt->selection.type == HEROI_TYPE)
		{
			char * buf = new char[10];
			SDL_Surface * ret = copySurface(hInfo);
			SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
			blueToPlayersAdv(ret,playerID);
			const CHeroInstance * curh = (const CHeroInstance *)adventureInt->selection.selected;
			printAt(curh->name,15,75,GEOR13,zwykly,ret);
			for (int i=0;i<PRIMARY_SKILLS;i++)
			{
				itoa(curh->primSkills[i],buf,10);
				printAtMiddle(buf,87+27*i,69,GEOR13,zwykly,ret);
			}
			itoa(curh->mana,buf,10);
			printAtMiddle(buf,169,109,GEORM,zwykly,ret);
			delete buf;
			return ret;
		}
		else if (adventureInt->selection.type == TOWNI_TYPE)
		{
			//TODO: do it
			return NULL;
		}
		else
			return NULL;
	}
	return NULL;
}

void CPlayerInterface::handleEvent(SDL_Event *sEvent)
{
	current = sEvent;

	if(sEvent->type == SDL_MOUSEMOTION)
	{
		CGI->curh->cursorMove(sEvent->motion.x, sEvent->motion.y);
	}

	if(sEvent->type==SDL_QUIT) 
		exit(0);
	else if (sEvent->type==SDL_KEYDOWN)
	{
		switch (sEvent->key.keysym.sym)
		{
		case SDLK_LEFT:
			{
				LOCPLINT->adventureInt->scrollingLeft = true;
				break;
			}
		case (SDLK_RIGHT):
			{
				LOCPLINT->adventureInt->scrollingRight = true;
				break;
			}
		case (SDLK_UP):
			{
				LOCPLINT->adventureInt->scrollingUp = true;
				break;
			}
		case (SDLK_DOWN):
			{
				LOCPLINT->adventureInt->scrollingDown = true;
				break;
			}
		case (SDLK_q):
			{
				exit(0);
				break;
			}
		case (SDLK_u):
			{
				adventureInt->underground.clickLeft(true);
			}
		}
	} //keydown end
	else if(sEvent->type==SDL_KEYUP) 
	{
		switch (sEvent->key.keysym.sym)
		{
		case SDLK_LEFT:
			{
				LOCPLINT->adventureInt->scrollingLeft = false;
				break;
			}
		case (SDLK_RIGHT):
			{
				LOCPLINT->adventureInt->scrollingRight = false;
				break;
			}
		case (SDLK_UP):
			{
				LOCPLINT->adventureInt->scrollingUp = false;
				break;
			}
		case (SDLK_DOWN):
			{
				LOCPLINT->adventureInt->scrollingDown = false;
				break;
			}
		case (SDLK_u):
			{
				adventureInt->underground.clickLeft(false);
			}
		}
	}//keyup end
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		for (int i=0; i<hoverable.size();i++)
		{
			if (isItIn(&hoverable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				if (!hoverable[i]->hovered)
					hoverable[i]->hover(true);
			}
			else if (hoverable[i]->hovered)
			{
				hoverable[i]->hover(false);
			}
		}
		for(int i=0; i<motioninterested.size();i++)
		{
			if (isItIn(&motioninterested[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				motioninterested[i]->mouseMoved(sEvent->motion);
			}
		}
		if(sEvent->motion.x<15)
		{
			LOCPLINT->adventureInt->scrollingLeft = true;
		}
		else
		{
			LOCPLINT->adventureInt->scrollingLeft = false;
		}
		if(sEvent->motion.x>ekran->w-15)
		{
			LOCPLINT->adventureInt->scrollingRight = true;
		}
		else
		{
			LOCPLINT->adventureInt->scrollingRight = false;
		}
		if(sEvent->motion.y<15)
		{
			LOCPLINT->adventureInt->scrollingUp = true;
		}
		else
		{
			LOCPLINT->adventureInt->scrollingUp = false;
		}
		if(sEvent->motion.y>ekran->h-15)
		{
			LOCPLINT->adventureInt->scrollingDown = true;
		}
		else
		{
			LOCPLINT->adventureInt->scrollingDown = false;
		}
	} //mousemotion end

	else if ((sEvent->type==SDL_MOUSEBUTTONDOWN) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		for(int i=0; i<lclickable.size();i++)
		{
			if (isItIn(&lclickable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				lclickable[i]->clickLeft(true);
			}
		}
	}
	else if ((sEvent->type==SDL_MOUSEBUTTONUP) && (sEvent->button.button == SDL_BUTTON_LEFT))
	{
		for(int i=0; i<lclickable.size();i++)
		{
			if (isItIn(&lclickable[i]->pos,sEvent->motion.x,sEvent->motion.y))
			{
				lclickable[i]->clickLeft(false);
			}
			else
				lclickable[i]->clickLeft(boost::logic::indeterminate);
		}
	}
	current = NULL;

} //event end
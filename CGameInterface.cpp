#include "stdafx.h"
#include "CGameInterface.h"
#include "CAdvMapInterface.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
using namespace CSDL_Ext;
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
CPlayerInterface::CPlayerInterface(int Player, int serial)
{
	playerID=Player;
	serialID=serial;
	CGI->localPlayer = playerID;
	human=true;
}
void CPlayerInterface::init()
{
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
	FPSmanager * mainLoopFramerateKeeper = new FPSmanager;
	SDL_initFramerate(mainLoopFramerateKeeper);
	SDL_setFramerate(mainLoopFramerateKeeper, 24);
	SDL_Event sEvent;
	//framerate keeper initialized
	for(;;) // main loop
	{
		CGI->screenh->updateScreen();

		LOCPLINT->adventureInt->updateScreen = false;
		if(SDL_PollEvent(&sEvent))  //wait for event...
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
		SDL_framerateDelay(mainLoopFramerateKeeper);
	}




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
	}

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
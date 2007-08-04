#include "stdafx.h"
#include "CGameInterface.h"
#include "CAdvMapInterface.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
using namespace CSDL_Ext;
CButtonBase::CButtonBase()
{
	type=-1;
	abs=false;
	active=false;
	ourObj=NULL;
	state=0;
}
void CButtonBase::show()
{
	if (!abs)
	{
		blitAt(imgs[state],pos.x,pos.y);
		updateRect(&pos);
	}
	else
	{
		blitAt(imgs[state],pos.x+ourObj->pos.x,pos.y+ourObj->pos.y);
		updateRect(&genRect(pos.h,pos.w,pos.x+ourObj->pos.x,pos.y+ourObj->pos.y));
		
	}
}
void ClickableL::activate()
{
	CURPLINT->lclickable.push_back(this);
}
void ClickableL::deactivate()
{
	CURPLINT->lclickable.erase
		(std::find(CURPLINT->lclickable.begin(),CURPLINT->lclickable.end(),this));
}
void ClickableR::activate()
{
	CURPLINT->rclickable.push_back(this);
}
void ClickableR::deactivate()
{
	CURPLINT->rclickable.erase(std::find(CURPLINT->rclickable.begin(),CURPLINT->rclickable.end(),this));
}
void Hoverable::activate()
{
	CURPLINT->hoverable.push_back(this);
}
void Hoverable::deactivate()
{
	CURPLINT->hoverable.erase(std::find(CURPLINT->hoverable.begin(),CURPLINT->hoverable.end(),this));
}
void KeyInterested::activate()
{
	CURPLINT->keyinterested.push_back(this);
}
void KeyInterested::deactivate()
{
	CURPLINT->
		keyinterested.erase(std::find(CURPLINT->keyinterested.begin(),CURPLINT->keyinterested.end(),this));
}
CPlayerInterface::CPlayerInterface(int Player)
{
	playerID=Player;
	human=true;
	adventureInt = new CAdvMapInt(Player);
}
void CPlayerInterface::yourTurn()
{
	unsigned char & animVal = CURPLINT->adventureInt->anim; //for animations handling
	adventureInt->show();
	//show rest of things

	//initializing framerate keeper
	FPSmanager * mainLoopFramerateKeeper = new FPSmanager;
	SDL_initFramerate(mainLoopFramerateKeeper);
	SDL_setFramerate(mainLoopFramerateKeeper, 30);
	SDL_Event sEvent;
	//framerate keeper initialized
	for(;;) // main loop
	{
		
		CURPLINT->adventureInt->updateScreen = false;
		if(SDL_PollEvent(&sEvent))  //wait for event...
		{
			handleEvent(&sEvent);
		}
		++CURPLINT->adventureInt->animValHitCount; //for animations
		if(CURPLINT->adventureInt->animValHitCount == 2)
		{
			CURPLINT->adventureInt->animValHitCount = 0;
			++animVal;
			CURPLINT->adventureInt->updateScreen = true;

		}
		if(CURPLINT->adventureInt->scrollingLeft)
		{
			if(CURPLINT->adventureInt->position.x>0)
			{
				CURPLINT->adventureInt->position.x--;
				CURPLINT->adventureInt->updateScreen = true;
			}
		}
		if(CURPLINT->adventureInt->scrollingRight)
		{
			if(CURPLINT->adventureInt->position.x<CGI->ac->map.width-19+8)
			{
				CURPLINT->adventureInt->position.x++;
				CURPLINT->adventureInt->updateScreen = true;
			}
		}
		if(CURPLINT->adventureInt->scrollingUp)
		{
			if(CURPLINT->adventureInt->position.y>0)
			{
				CURPLINT->adventureInt->position.y--;
				CURPLINT->adventureInt->updateScreen = true;
			}
		}
		if(CURPLINT->adventureInt->scrollingDown)
		{
			if(CURPLINT->adventureInt->position.y<CGI->ac->map.height-18+8)
			{
				CURPLINT->adventureInt->position.y++;
				CURPLINT->adventureInt->updateScreen = true;
			}
		}
		if(CURPLINT->adventureInt->updateScreen)
		{
			adventureInt->update();
			CURPLINT->adventureInt->updateScreen=false;
		}
		SDL_Delay(5); //give time for other apps
		SDL_framerateDelay(mainLoopFramerateKeeper);
	}




}

void CPlayerInterface::handleEvent(SDL_Event *sEvent)
{
	if(sEvent->type==SDL_QUIT) 
		exit(0);
	else if (sEvent->type==SDL_KEYDOWN)
	{
		switch (sEvent->key.keysym.sym)
		{
		case SDLK_LEFT:
			{
				CURPLINT->adventureInt->scrollingLeft = true;
				break;
			}
		case (SDLK_RIGHT):
			{
				CURPLINT->adventureInt->scrollingRight = true;
				break;
			}
		case (SDLK_UP):
			{
				CURPLINT->adventureInt->scrollingUp = true;
				break;
			}
		case (SDLK_DOWN):
			{
				CURPLINT->adventureInt->scrollingDown = true;
				break;
			}
		case (SDLK_q):
			{
				exit(0);
				break;
			}
		case (SDLK_u):
			{
				if(!CGI->ac->map.twoLevel)
					break;
				if (adventureInt->position.z)
					adventureInt->position.z--;
				else adventureInt->position.z++;
				CURPLINT->adventureInt->updateScreen = true;
				break;
			}
		}
	} //keydown end
	else if(sEvent->type==SDL_KEYUP) 
	{
		switch (sEvent->key.keysym.sym)
		{
		case SDLK_LEFT:
			{
				CURPLINT->adventureInt->scrollingLeft = false;
				break;
			}
		case (SDLK_RIGHT):
			{
				CURPLINT->adventureInt->scrollingRight = false;
				break;
			}
		case (SDLK_UP):
			{
				CURPLINT->adventureInt->scrollingUp = false;
				break;
			}
		case (SDLK_DOWN):
			{
				CURPLINT->adventureInt->scrollingDown = false;
				break;
			}
		}
	}//keyup end
	else if(sEvent->type==SDL_MOUSEMOTION)
	{
		if(sEvent->motion.x<15)
		{
			CURPLINT->adventureInt->scrollingLeft = true;
		}
		else
		{
			CURPLINT->adventureInt->scrollingLeft = false;
		}
		if(sEvent->motion.x>ekran->w-15)
		{
			CURPLINT->adventureInt->scrollingRight = true;
		}
		else
		{
			CURPLINT->adventureInt->scrollingRight = false;
		}
		if(sEvent->motion.y<15)
		{
			CURPLINT->adventureInt->scrollingUp = true;
		}
		else
		{
			CURPLINT->adventureInt->scrollingUp = false;
		}
		if(sEvent->motion.y>ekran->h-15)
		{
			CURPLINT->adventureInt->scrollingDown = true;
		}
		else
		{
			CURPLINT->adventureInt->scrollingDown = false;
		}
	}
} //event end
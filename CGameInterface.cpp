#include "stdafx.h"
#include "CGameInterface.h"
#include "CMessage.h"
#include "SDL_Extensions.h"
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
	CGI->playerint->lclickable.push_back(this);
}
void ClickableL::deactivate()
{
	CGI->playerint->lclickable.erase(std::find(CGI->playerint->lclickable.begin(),CGI->playerint->lclickable.end(),this));
}
void ClickableR::activate()
{
	CGI->playerint->rclickable.push_back(this);
}
void ClickableR::deactivate()
{
	CGI->playerint->rclickable.erase(std::find(CGI->playerint->rclickable.begin(),CGI->playerint->rclickable.end(),this));
}
void Hoverable::activate()
{
	CGI->playerint->hoverable.push_back(this);
}
void Hoverable::deactivate()
{
	CGI->playerint->hoverable.erase(std::find(CGI->playerint->hoverable.begin(),CGI->playerint->hoverable.end(),this));
}
void KeyInterested::activate()
{
	CGI->playerint->keyinterested.push_back(this);
}
void KeyInterested::deactivate()
{
	CGI->playerint->
		keyinterested.erase(std::find(CGI->playerint->keyinterested.begin(),CGI->playerint->keyinterested.end(),this));
}
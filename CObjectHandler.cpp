#include "stdafx.h"
#include "CObjectHandler.h"
#include "CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CAmbarCendamo.h"

void CObjectHandler::loadObjects()
{
	int ID=0;
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("OBJNAMES.TXT");
	int it=0;
	while (it<buf.length()-1)
	{
		CObject nobj;
		CGeneralTextHandler::loadToIt(nobj.name,buf,it,3);
		objects.push_back(nobj);
	}
}

bool CObjectInstance::operator <(const CObjectInstance &cmp) const
{
	if(CGI->ac->map.defy[this->defNumber].printPriority==1 && CGI->ac->map.defy[cmp.defNumber].printPriority==0)
		return true;
	if(CGI->ac->map.defy[cmp.defNumber].printPriority==1 && CGI->ac->map.defy[this->defNumber].printPriority==0)
		return false;
	if(this->pos.y<cmp.pos.y)
		return true;
	if(this->pos.y>cmp.pos.y)
		return false;
	if(CGI->ac->map.defy[this->defNumber].isOnDefList && !(CGI->ac->map.defy[cmp.defNumber].isOnDefList))
		return true;
	if(CGI->ac->map.defy[cmp.defNumber].isOnDefList && !(CGI->ac->map.defy[this->defNumber].isOnDefList))
		return false;
	if(!CGI->ac->map.defy[this->defNumber].isVisitable() && CGI->ac->map.defy[cmp.defNumber].isVisitable())
		return true;
	if(!CGI->ac->map.defy[cmp.defNumber].isVisitable() && CGI->ac->map.defy[this->defNumber].isVisitable())
		return false;
	if(this->pos.x<cmp.pos.x)
		return true;
	return false;
}

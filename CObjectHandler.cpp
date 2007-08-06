#include "stdafx.h"
#include "CObjectHandler.h"
#include "CGameInfo.h"
#include "CGeneralTextHandler.h"

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
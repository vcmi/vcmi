#include "stdafx.h"
#include "CDefObjInfoHandler.h"
#include "CGameInfo.h"
#include <sstream>

bool DefObjInfo::operator==(const std::string & por) const
{
	return this->defName == por;
}

void CDefObjInfoHandler::load()
{
	std::istringstream inp(CGameInfo::mainObj->bitmaph->getTextFile("ZOBJCTS.TXT"));
	int objNumber;
	inp>>objNumber;
	for(int hh=0; hh<objNumber; ++hh)
	{
		DefObjInfo nobj;
		std::string dump;
		inp>>nobj.defName;
		for(int yy=0; yy<4; ++yy)
			inp>>dump;
		inp>>nobj.type;
		inp>>nobj.subtype;
		inp>>nobj.objType;
		inp>>nobj.priority;
		objs.push_back(nobj);
	}
}

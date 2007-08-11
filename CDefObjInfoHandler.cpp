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

		for(int o=0; o<6; ++o)
		{
			nobj.blockMap[o] = 0xff;
			nobj.visitMap[o] = 0x00;
		}
		std::string mapStr;
		inp>>mapStr;
		std::reverse(mapStr.begin(), mapStr.end());
		for(int v=0; v<mapStr.size(); ++v)
		{
			if(mapStr[v]=='0')
			{
				nobj.blockMap[v/8] &= 255 - (128 >> (v%8));
			}
		}
		inp>>mapStr;
		std::reverse(mapStr.begin(), mapStr.end());
		for(int v=0; v<mapStr.size(); ++v)
		{
			if(mapStr[v]=='1')
			{
				nobj.visitMap[v/8] |= (128 >> (v%8));
			}
		}

		for(int yy=0; yy<2; ++yy)
			inp>>dump;
		inp>>nobj.type;
		inp>>nobj.subtype;
		inp>>nobj.objType;
		inp>>nobj.priority;
		objs.push_back(nobj);
	}
}

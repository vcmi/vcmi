#include "../stdafx.h"
#include "CDefObjInfoHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <sstream>

bool CGDefInfo::isVisitable()
{
	for (int i=0; i<6; i++)
	{
		if (visitMap[i])
			return true;
	}
	return false;
}
bool DefObjInfo::operator==(const std::string & por) const
{
	return this->defName == por;
}

void CDefObjInfoHandler::load()
{
	nodrze<int> ideki;
	std::istringstream inp(CGameInfo::mainObj->bitmaph->getTextFile("ZOBJCTS.TXT"));
	int objNumber;
	inp>>objNumber;
	for(int hh=0; hh<objNumber; ++hh)
	{
		CGDefInfo* nobj = new CGDefInfo();
		nobj->handler = NULL;
		std::string dump;
		inp>>nobj->name;
		
		std::transform(nobj->name.begin(), nobj->name.end(), nobj->name.begin(), (int(*)(int))toupper);

		for(int o=0; o<6; ++o)
		{
			nobj->blockMap[o] = 0xff;
			nobj->visitMap[o] = 0x00;
		}
		std::string mapStr;
		inp>>mapStr;
		std::reverse(mapStr.begin(), mapStr.end());
		for(int v=0; v<mapStr.size(); ++v)
		{
			if(mapStr[v]=='0')
			{
				nobj->blockMap[v/8] &= 255 - (128 >> (v%8));
			}
		}
		inp>>mapStr;
		std::reverse(mapStr.begin(), mapStr.end());
		for(int v=0; v<mapStr.size(); ++v)
		{
			if(mapStr[v]=='1')
			{
				nobj->visitMap[v/8] |= (128 >> (v%8));
			}
		}

		for(int yy=0; yy<2; ++yy)
			inp>>dump;
		inp>>nobj->id;
		inp>>nobj->subid;
		inp>>nobj->type;
		inp>>nobj->printPriority;
		gobjs[nobj->id][nobj->subid] = nobj;
		if(nobj->id==98)
			castles[nobj->subid]=nobj;
	}
}

bool DefObjInfo::isVisitable() const
{
	for(int g=0; g<6; ++g)
	{
		if(visitMap[g]!=0)
			return true;
	}
	return false;
}

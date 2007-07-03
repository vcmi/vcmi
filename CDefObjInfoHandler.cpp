#include "stdafx.h"
#include "CDefObjInfoHandler.h"

bool DefObjInfo::operator==(const std::string & por) const
{
	return this->defName == por;
}

void CDefObjInfoHandler::load()
{
	std::ifstream inp("H3bitmap.lod\\ZOBJCTS.TXT", std::ios::in | std::ios::binary);
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
	inp.close();
}

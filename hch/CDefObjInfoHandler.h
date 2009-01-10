#ifndef __CDEFOBJINFOHANDLER_H__
#define __CDEFOBJINFOHANDLER_H__
#include <vector>
#include <map>
#include "../global.h"
class CDefHandler;
class CLodHandler;
class DLL_EXPORT CGDefInfo
{
public:
	std::string name;

	ui8 visitMap[6];
	ui8 blockMap[6];
	ui8 visitDir; //directions from which object can be entered, format same as for moveDir in CGHeroInstance(but 0 - 7)
	si32 id, subid; //of object described by this defInfo
	si32 serial;
	si32 terrainAllowed, //on which terrain it is possible to place object
		 terrainMenu; //in which menus in map editor object will be showed
	si32 width, height; //tiles
	si32 type; //(0- ground, 1- towns, 2-creatures, 3- heroes, 4-artifacts, 5- resources)   
	CDefHandler * handler;
	si32 printPriority;
	bool isVisitable();
	bool operator<(const CGDefInfo& por)
	{
		if(id!=por.id)
			return id<por.id;
		else
			return subid<por.subid;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & serial & visitMap & blockMap & visitDir & id & subid &terrainAllowed
			& terrainMenu & width & height & type & printPriority;
	}
	CGDefInfo();
};
class DLL_EXPORT CDefObjInfoHandler
{
public:
	std::map<int,std::map<int,CGDefInfo*> > gobjs;
	std::map<int,CGDefInfo*> castles;

	void load();
	~CDefObjInfoHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & gobjs;

		if(!h.saving) //recrete castles map
			for(std::map<int,std::map<int,CGDefInfo*> >::iterator i=gobjs.begin(); i!=gobjs.end(); i++)
				for(std::map<int,CGDefInfo*>::iterator j=i->second.begin(); j!=i->second.end(); j++)
					if(j->second->id == 98)
						castles[j->second->subid]=j->second;
	}
};

#endif // __CDEFOBJINFOHANDLER_H__

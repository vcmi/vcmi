#ifndef COBJINFOECTHANDLER_H
#define COBJINFOECTHANDLER_H
#include <vector>
#include <map>
#include "../global.h"
class CDefHandler;
class CLodHandler;
class DLL_EXPORT CGDefInfo
{
public:
	std::string name;

	unsigned char visitMap[6];
	unsigned char blockMap[6];
	unsigned char visitDir; //directions from which object can be entered, format same as for moveDir in CGHeroInstance(but 0 - 7)
	int id, subid; //of object described by this defInfo
	int terrainAllowed, //on which terrain it is possible to place object
		 terrainMenu; //in which menus in map editor object will be showed
	int width, height; //tiles
	int type; //(0- ground, 1- towns, 2-creatures, 3- heroes, 4-artifacts, 5- resources)   
	CDefHandler * handler;
	int printPriority;
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
		h & name & visitMap & blockMap & visitDir & id & subid &terrainAllowed & terrainMenu & width & height & type & printPriority;
	}
	CGDefInfo();
};
class DLL_EXPORT CDefObjInfoHandler
{
public:
	std::map<int,std::map<int,CGDefInfo*> > gobjs;
	std::map<int,CGDefInfo*> castles;
	//std::vector<DefObjInfo> objs;
	void load();


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

#endif //COBJINFOECTHANDLER_H

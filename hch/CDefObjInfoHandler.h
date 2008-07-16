#ifndef COBJINFOECTHANDLER_H
#define COBJINFOECTHANDLER_H
#include <vector>
#include <map>
class CDefHandler;
class CGDefInfo
{
public:
	std::string name;

	unsigned char visitMap[6];
	unsigned char blockMap[6];
	unsigned char visitDir; //directions from which object can be entered, format same as for moveDir in CGHeroInstance(but 0 - 7)
	int id, subid; //of object described by this defInfo
	int terrainAllowed, //on which terrain it is possible to place object
		 terrainMenu; //in which menus in map editor object will be showed
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
	CGDefInfo();
};
struct DefObjInfo
{
	std::string defName;
	int priority;
	int type, subtype;
	int objType;
	unsigned char visitMap[6];
	unsigned char blockMap[6];
	bool operator==(const std::string & por) const;
	bool isVisitable() const;
};

class CDefObjInfoHandler
{
public:
	std::map<int,std::map<int,CGDefInfo*> > gobjs;
	std::map<int,CGDefInfo*> castles;
	//std::vector<DefObjInfo> objs;
	void load();
};

#endif //COBJINFOECTHANDLER_H

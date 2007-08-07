#ifndef COBJINFOECTHANDLER_H
#define COBJINFOECTHANDLER_H

#include <vector>

struct DefObjInfo
{
	std::string defName;
	int priority;
	int type, subtype;
	int objType;
	unsigned char visitMap[6];
	unsigned char blockMap[6];
	bool operator==(const std::string & por) const;
};

class CDefObjInfoHandler
{
public:
	std::vector<DefObjInfo> objs;
	void load();
};

#endif //COBJINFOECTHANDLER_H
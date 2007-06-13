#ifndef CCASTLEHANDLER_H
#define CCASTLEHANDLER_H

#include "CBuildingHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"

class CCastle : public CSpecObjInfo //castle class
{
public:
	int x, y, z; //posiotion
	std::vector<CBuilding> buildings; //buildings we can build in this castle
	std::vector<bool> isBuild; //isBuild[i] is true, when building buildings[i] has been built
	std::vector<bool> isLocked; //isLocked[i] is true, when building buildings[i] canot be built
	CHero * visitingHero;
	CHero * garnisonHero;
	//TODO: dokoñczyæ
};

#endif //CCASTLEHANDLER_H
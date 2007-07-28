#ifndef CGAMESTATE_H
#define CGAMESTATE_H

#include "CSpellHandler.h"
#include "CAbilityHandler.h"
#include "CCreaturehandler.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CAmbarCendamo.h"
#include "CBuildingHandler.h"
#include "CObjectHandler.h"
#include "CMusicHandler.h"
#include "CSemiLodHandler.h"
#include "CDefObjInfoHandler.h"
#include "CLodHandler.h"
#include "CTownHandler.h"

struct PlayerState
{
	int color;
	vector<vector<bool> > fogOfWarMap;
	std::vector<int> resources;
	std::vector<CHeroInstance> heroes;
	std::vector<CTownInstance> heroes;
}

class CGameState
{

}

#endif //CGAMESTATE_H
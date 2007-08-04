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
public:
	int color;
	std::vector<std::vector<std::vector<bool> > >fogOfWarMap;
	std::vector<int> resources;
	std::vector<CHeroInstance> heroes;
	std::vector<CTownInstance> towns;
};

class CGameState
{
public:
	int currentPlayer;
	std::map<int,PlayerState> players; //color <-> playerstate
};

#endif //CGAMESTATE_H
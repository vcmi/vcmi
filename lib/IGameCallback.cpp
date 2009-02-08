#define VCMI_DLL
#include "IGameCallback.h"
#include "../CGameState.h"
#include "../map.h"
#include "../hch/CObjectHandler.h"
#include "../StartInfo.h"

const CGObjectInstance* IGameCallback::getObj(int objid)
{
	return gs->map->objects[objid];
}
const CGHeroInstance* IGameCallback::getHero(int objid)
{
	return dynamic_cast<const CGHeroInstance*>(gs->map->objects[objid]);
}
const CGTownInstance* IGameCallback::getTown(int objid)
{
	return dynamic_cast<const CGTownInstance*>(gs->map->objects[objid]);
}

int IGameCallback::getOwner(int heroID)
{
	return gs->map->objects[heroID]->tempOwner;
}
int IGameCallback::getResource(int player, int which)
{
	return gs->players.find(player)->second.resources[which];
}
int IGameCallback::getDate(int mode)
{
	return gs->getDate(mode);
}

const CGHeroInstance* IGameCallback::getSelectedHero( int player )
{
	return getHero(gs->players.find(player)->second.currentSelection);
}

const PlayerSettings * IGameCallback::getPlayerSettings( int color )
{
	return &gs->scenarioOps->getIthPlayersSettings(color);
}
#define VCMI_DLL
#include "IGameCallback.h"
#include "../CGameState.h"
#include "../map.h"
#include "../hch/CObjectHandler.h"
#include "../StartInfo.h"

const CGObjectInstance* IGameCallback::getObj(int objid)
{
	if(objid < 0  ||  objid >= gs->map->objects.size())
	{
		tlog1 << "Cannot get object with id " << objid << std::endl;
		return NULL;
	}
	else if (!gs->map->objects[objid])
	{
		tlog1 << "Cannot get object with id " << objid << ". Object was removed.\n";
		return NULL;
	}
	return gs->map->objects[objid];
}
const CGHeroInstance* IGameCallback::getHero(int objid)
{
	const CGObjectInstance *obj = getObj(objid);
	if(obj)
		return dynamic_cast<const CGHeroInstance*>(obj);
	else
		return NULL;
}
const CGTownInstance* IGameCallback::getTown(int objid)
{
	const CGObjectInstance *obj = getObj(objid);
	if(obj)
		return dynamic_cast<const CGTownInstance*>(gs->map->objects[objid]);
	else
		return NULL;
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

int IGameCallback::getHeroCount( int player, bool includeGarrisoned )
{
	int ret = 0;
	if(includeGarrisoned)
		return gs->getPlayer(player)->heroes.size();
	else
		for(int i=0; i < gs->getPlayer(player)->heroes.size(); i++)
			if(!gs->getPlayer(player)->heroes[i]->inTownGarrison)
				ret++;
	return ret;
}
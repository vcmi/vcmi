#include "stdafx.h"
#include "CCallback.h"
#include "CPathfinder.h"
#include "CHeroHandler.h"
#include "CGameInfo.h"
#include "CAmbarCendamo.h"
#include "mapHandler.h"

bool CCallback::moveHero(int ID, int3 destPoint)
{
	if(ID<0 || ID>CGI->heroh->heroInstances.size())
		return false;
	if(destPoint.x<0 || destPoint.x>CGI->ac->map.width)
		return false;
	if(destPoint.y<0 || destPoint.y>CGI->ac->map.height)
		return false;
	if(destPoint.z<0 || destPoint.z>CGI->mh->ttiles[0][0].size()-1)
		return false;
	CPath * ourPath = CGI->pathf->getPath(CGI->heroh->heroInstances[ID]->pos, destPoint, CGI->heroh->heroInstances[ID]);
	if(!ourPath)
		return false;
	return false;
}

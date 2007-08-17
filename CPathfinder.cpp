#include "stdafx.h"
#include "global.h"
#include "CPathfinder.h"
#include "CGameInfo.h"
CPath * CPathfinder::getPath(int3 &src, int3 &dest, CHeroInstance * hero) //TODO: test it (seems to be finished, but relies on unwritten functions :()
{
	if(src.z!=dest.z) //first check
		return NULL;

	//graph initialization
	graph.resize(CGI->ac->map.width);
	for(int i=0; i<graph.size(); ++i)
	{
		graph[i].resize(CGI->ac->map.height);
		for(int j=0; j<graph[i].size(); ++j)
		{
			graph[i][j] = new CPathNode;
			graph[i][j]->accesible = !CGI->mh->ttiles[i][j][src.z].blocked;
			graph[i][j]->dist = -1;
			graph[i][j]->theNodeBefore = NULL;
			graph[i][j]->visited = false;
			graph[i][j]->x = i;
			graph[i][j]->y = j;
		}
	}

	//graph initialized

	graph[src.x][src.y]->dist = 0;

	std::queue<CPathNode *> mq;
	mq.push(graph[src.x][src.y]);

	unsigned int curDist = 4000000000;

	while(!mq.empty())
	{
		CPathNode * cp = mq.front();
		mq.pop();
		if ((cp->x == dest.x) && (cp->y==dest.y))
		{
			if (cp->dist < curDist)
				curDist=cp->dist;
		}
		else
		{
			if (cp->dist > curDist)
				continue;
		}
		if(cp->x>0)
		{
			CPathNode * dp = graph[cp->x-1][cp->y];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->y>0)
		{
			CPathNode * dp = graph[cp->x][cp->y-1];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x, cp->y-1, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->x>0 && cp->y>0)
		{
			CPathNode * dp = graph[cp->x-1][cp->y-1];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y-1, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y-1, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->x<graph.size()-1)
		{
			CPathNode * dp = graph[cp->x+1][cp->y];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x+1, cp->y, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x+1, cp->y, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->y<graph[0].size()-1)
		{
			CPathNode * dp = graph[cp->x][cp->y+1];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x, cp->y+1, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x, cp->y+1, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->x<graph.size()-1 && cp->y<graph[0].size()-1)
		{
			CPathNode * dp = graph[cp->x+1][cp->y+1];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x+1, cp->y+1, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x+1, cp->y+1, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->x>0 && cp->y<graph[0].size()-1)
		{
			CPathNode * dp = graph[cp->x-1][cp->y+1];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y+1, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x-1, cp->y+1, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
		if(cp->x<graph.size()-1 && cp->y>0)
		{
			CPathNode * dp = graph[cp->x+1][cp->y-1];
			if((dp->dist==-1 || (dp->dist > cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x+1, cp->y-1, src.z), hero))) && dp->accesible)
			{
				dp->dist = cp->dist + CGI->mh->getCost(int3(cp->x, cp->y, src.z), int3(cp->x+1, cp->y-1, src.z), hero);
				dp->theNodeBefore = cp;
				mq.push(dp);
			}
		}
	}

	CPathNode * curNode = graph[dest.x][dest.y];
	CPath * ret = new CPath;

	while(curNode!=graph[src.x][src.y] && curNode != NULL)
	{
		ret->nodes.push_back(*curNode);
		curNode = curNode->theNodeBefore;
	}

	ret->nodes.push_back(*graph[src.x][src.y]);

	for(int i=0; i<graph.size(); ++i)
	{
		for(int j=0; j<graph[0].size(); ++j)
			delete graph[i][j];
	}
	return ret;
}

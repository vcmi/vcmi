#include "stdafx.h"
#include "global.h"
#include "CPathfinder.h"
#include "CGameInfo.h"
#include "hch\CAmbarCendamo.h"
#include "mapHandler.h"
using namespace boost::logic;
int3 CPath::startPos()
{
	return int3(nodes[nodes.size()-1].coord.x,nodes[nodes.size()-1].coord.y,nodes[nodes.size()-1].coord.z);
}
int3 CPath::endPos()
{
	return int3(nodes[0].coord.x,nodes[0].coord.y,nodes[0].coord.z);
}

CPath * CPathfinder::getPath(int3 src, int3 dest, const CGHeroInstance * hero, unsigned char type) //TODO: test it (seems to be finished, but relies on unwritten functions :()
{
	//check input
	if ((src.x < 0)||(src.y < 0)||(src.z < 0)||(dest.x < 0)||(dest.y < 0)||(dest.z < 0))
	{
		return NULL;
	}
	if ((src.x >= CGI->mh->sizes.x)||(src.y >= CGI->mh->sizes.y)||(src.z >= CGI->mh->sizes.z)
			||(dest.x >= CGI->mh->sizes.x)||(dest.y >= CGI->mh->sizes.y)||(dest.z >= CGI->mh->sizes.z))
	{
		return NULL;
	}

	int3 hpos = hero->getPosition(false);

	tribool blockLandSea; //true - blocks sea, false - blocks land, indeterminate - allows all
	if (!hero->canWalkOnSea())
	{
		if (CGI->mh->ttiles[hpos.x][hpos.y][hpos.z].terType==EterrainType::water)
			blockLandSea=false;
		else
			blockLandSea=true;
	}
	else
	{
		blockLandSea = indeterminate;
	}

	//graph initialization
	graph.resize(CGI->ac->map.width);
	for(int i=0; i<graph.size(); ++i)
	{
		graph[i].resize(CGI->ac->map.height);
		for(int j=0; j<graph[i].size(); ++j)
		{
			graph[i][j].accesible = !CGI->mh->ttiles[i][j][src.z].blocked;
			if(i==dest.x && j==dest.y && CGI->mh->ttiles[i][j][src.z].visitable)
				graph[i][j].accesible = true; //for allowing visiting objects
			graph[i][j].dist = -1;
			graph[i][j].theNodeBefore = NULL;
			graph[i][j].visited = false;
			graph[i][j].coord.x = i;
			graph[i][j].coord.y = j;
			graph[i][j].coord.z = dest.z;
			if (CGI->mh->ttiles[i][j][src.z].terType==EterrainType::rock)
				graph[i][j].accesible = false;
			if ((blockLandSea) && (CGI->mh->ttiles[i][j][src.z].terType==EterrainType::water))
				graph[i][j].accesible = false;
			else if ((!blockLandSea) && (CGI->mh->ttiles[i][j][src.z].terType!=EterrainType::water))
				graph[i][j].accesible = false;
		}
	}

	//graph initialized

	graph[src.x][src.y].dist = 0;

	std::queue<CPathNode> mq;
	mq.push(graph[src.x][src.y]);

	unsigned int curDist = 4000000000;

	while(!mq.empty())
	{
		CPathNode cp = mq.front();
		mq.pop();
		if ((cp.coord.x == dest.x) && (cp.coord.y==dest.y))
		{
			if (cp.dist < curDist)
				curDist=cp.dist;
		}
		else
		{
			if (cp.dist > curDist)
				continue;
		}
		if(cp.coord.x>0)
		{
			CPathNode & dp = graph[cp.coord.x-1][cp.coord.y];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.y>0)
		{
			CPathNode & dp = graph[cp.coord.x][cp.coord.y-1];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x, cp.coord.y-1, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.x>0 && cp.coord.y>0)
		{
			CPathNode & dp = graph[cp.coord.x-1][cp.coord.y-1];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y-1, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y-1, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.x<graph.size()-1)
		{
			CPathNode & dp = graph[cp.coord.x+1][cp.coord.y];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x+1, cp.coord.y, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x+1, cp.coord.y, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.y<graph[0].size()-1)
		{
			CPathNode & dp = graph[cp.coord.x][cp.coord.y+1];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x, cp.coord.y+1, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x, cp.coord.y+1, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.x<graph.size()-1 && cp.coord.y<graph[0].size()-1)
		{
			CPathNode & dp = graph[cp.coord.x+1][cp.coord.y+1];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x+1, cp.coord.y+1, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x+1, cp.coord.y+1, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.x>0 && cp.coord.y<graph[0].size()-1)
		{
			CPathNode & dp = graph[cp.coord.x-1][cp.coord.y+1];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y+1, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x-1, cp.coord.y+1, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
		if(cp.coord.x<graph.size()-1 && cp.coord.y>0)
		{
			CPathNode & dp = graph[cp.coord.x+1][cp.coord.y-1];
			if((dp.dist==-1 || (dp.dist > cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x+1, cp.coord.y-1, src.z), hero))) && dp.accesible)
			{
				dp.dist = cp.dist + CGI->mh->getCost(int3(cp.coord.x, cp.coord.y, src.z), int3(cp.coord.x+1, cp.coord.y-1, src.z), hero);
				dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
				mq.push(dp);
			}
		}
	}

	CPathNode curNode = graph[dest.x][dest.y];
	if(!curNode.theNodeBefore)
		return NULL;

	CPath * ret = new CPath;

	while(curNode.coord!=graph[src.x][src.y].coord)
	{
		ret->nodes.push_back(curNode);
		curNode = *(curNode.theNodeBefore);
	}

	ret->nodes.push_back(graph[src.x][src.y]);

	return ret;
}

void CPathfinder::convertPath(CPath * path, unsigned int mode) //mode=0 -> from 'manifest' to 'object'
{
	if (mode==0)
	{
		for (int i=0;i<path->nodes.size();i++)
		{
			path->nodes[i].coord = CGHeroInstance::convertPosition(path->nodes[i].coord,true);
		}
	}
}
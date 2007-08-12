#ifndef CPATHFINDER_H
#define CPATHFINDER_H
#include "CGameInfo.h"
#include "int3.h"
#include <queue>
#include <vector>

struct CPathNode
{
	bool accesible; //true if a hero can be on this node
	int dist; //distance from the first node of searching; -1 is infinity
	CPathNode * theNodeBefore;
	int x, y; //coordiantes
	bool visited;
};

struct CPath
{
	std::queue<CPathNode> nodes; //just get node by node
};

/**
 * main pathfinder class
 */
class CPathfinder
{
private:
	std::vector< std::vector<CPathNode *> > graph;
public:
	CPath * getPath(int3 & src, int3 & dest, CHeroInstance * hero); //calculates path between src and dest; returns pointer to CPath or NULL if path does not exists
	CPath * getPath(int3 & src, int3 & dest, CHeroInstance * hero, int (*getDist)(int3 & a, int3 b)); //calculates path between src and dest; returns pointer to CPath or NULL if path does not exists; uses getDist to calculate distance
};

#endif //CPATHFINDER_H
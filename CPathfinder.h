#ifndef CPATHFINDER_H
#define CPATHFINDER_H
#include "int3.h"
#include <queue>
#include <vector>
class CHeroInstance;
struct CPathNode
{
	bool accesible; //true if a hero can be on this node
	int dist; //distance from the first node of searching; -1 is infinity
	CPathNode * theNodeBefore;
	int3 coord; //coordiantes
	bool visited;
};

struct CPath
{
	std::vector<CPathNode> nodes; //just get node by node

	int3 startPos(); // start point
	int3 endPos(); //destination point
};

/**
 * main pathfinder class
 */
class CPathfinder
{
private:
	std::vector< std::vector<CPathNode *> > graph;
public:
	CPath * getPath(const int3 & src, const int3 & dest, const CHeroInstance * hero); //calculates path between src and dest; returns pointer to CPath or NULL if path does not exists
	CPath * getPath(const int3 & src, const int3 & dest, const CHeroInstance * hero, int (*getDist)(int3 & a, int3 & b)); //calculates path between src and dest; returns pointer to CPath or NULL if path does not exists; uses getDist to calculate distance
};

#endif //CPATHFINDER_H
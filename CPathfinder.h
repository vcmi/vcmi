#ifndef CPATHFINDER_H
#define CPATHFINDER_H
#include "global.h"
#include <queue>
#include <vector>

class CGHeroInstance;

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
	std::vector< std::vector<CPathNode> > graph;
	void processNode(CPathNode & dp, const CGHeroInstance * hero, std::queue<CPathNode> & mq, const CPathNode & cp, const int3 & src, bool diagonal); //helper function for getPath
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const;
public:
	CPath * getPath(int3 src, int3 dest, const CGHeroInstance * hero, unsigned char type=0); //calculates path between src and dest; returns pointer to CPath or NULL if path does not exists; type - type of calculation: 0 - positions are normal positions of hero; 1 - given places are tiles blocked by hero
	CPath * getPath(const int3 & src, const int3 & dest, const CGHeroInstance * hero, int (*getDist)(int3 & a, int3 & b), unsigned char type=0); //calculates path between src and dest; returns pointer to CPath or NULL if path does not exists; uses getDist to calculate distance; type - type of calculation: 0 - positions are normal positions of hero; 1 - given places are tiles blocked by hero

	static void convertPath(CPath * path, unsigned int mode); //mode=0 -> from 'manifest' to 'object'
};

#endif //CPATHFINDER_H

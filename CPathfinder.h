#ifndef CPathfinder_H
#define CPathfinder_H
#include "global.h"
#include <vector>
#include <queue>
#include <math.h>
class CGHeroInstance;

using namespace std;

class Coordinate
{
public:
	int x;
	int y;
	int z;
	float g; //Distance from goal
	int h; //Movement cost
	bool diagonal; //if true, this tile will be crossed by diagonal

	Coordinate() : x(NULL), y(NULL), z(NULL), g(NULL), h(NULL), diagonal(false) {}
	Coordinate(int3 xyz) : x(xyz.x), y(xyz.y), z(xyz.z), diagonal(false){}
	Coordinate(int xCor, int yCor, int zCor) : x(xCor), y(yCor), z(zCor), diagonal(false){}
	Coordinate(int xCor, int yCor, int zCor, int dist, int penalty) : x(xCor), y(yCor), z(zCor), g(dist), h(penalty), diagonal(false){}
	void operator=(const Coordinate& other);
};

//Class used in priority queue to determine order.
class Compare
{
public:
	bool operator()(const vector<Coordinate>& a, const vector<Coordinate>& b);
};

//Included for old format
struct CPathNode
{
	bool accesible; //true if a hero can be on this node
	int dist; //distance from the first node of searching; -1 is infinity
	CPathNode * theNodeBefore;
	int3 coord; //coordiantes
	bool visited;
};

//Included for old format
struct CPath
{
	std::vector<CPathNode> nodes; //just get node by node

	int3 startPos(); // start point
	int3 endPos(); //destination point
};

class CPathfinder
{
private:
	boost::logic::tribool blockLandSea; //true - blocks sea, false - blocks land, indeterminate - allows all

	/*
	 * Does the actual path calculation.  Don't call this directly, call GetPath instead.
	 */
	vector<Coordinate>* CalcPath();

	/*
 	 * Determines if the given node exists in the closed list, returns true if it does.  This is
	 * used to ensure you dont check the same node twice.
	 */
	bool ExistsInClosed(Coordinate node);

	/*
	 * Adds the neighbors of the current node to the open cue so they can be considered in the
	 * path creation.  If the node has a cost (f = g + h) less than zero, it isn't added to Open.
	 */
	void AddNeighbors(vector<Coordinate>* node);

	/*
	 * Calculates the movement cost of the node.  Returns -1 if it is impossible to travel on.
	 */
	void CalcH(Coordinate* node);

	/*
	 * Calculates distance from node to end node.
	 */
	void CalcG(Coordinate* node);

public:
	//Contains nodes to be searched
	priority_queue < vector<Coordinate>, vector<vector<Coordinate> >, Compare > Open;

	//History of nodes you have been to before
	vector<Coordinate> Closed;

	//The start, or source position.
	Coordinate Start;

	//The end, or destination position
	Coordinate End;

	//A reference to the Hero.
	const CGHeroInstance* Hero;

	/*
	 * Does basic input checking and setup for the path calculation.
	 */
	vector<Coordinate>* GetPath(const CGHeroInstance* hero);
	vector<Coordinate>* GetPath(Coordinate start, Coordinate end, const CGHeroInstance* hero);

	//Convert to oldformat
	CPath* ConvertToOldFormat(vector<Coordinate>* p);

	static void convertPath(CPath * path, unsigned int mode); //mode=0 -> from 'manifest' to 'object'

};

#endif //CPATHFINDER_H

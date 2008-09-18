#include "stdafx.h"
#include "global.h"
#include "CPathfinder.h"
#include "CGameInfo.h"
#include "hch/CAmbarCendamo.h"
#include "mapHandler.h"
#include "CGameState.h"
#include "hch/CObjectHandler.h"
#include "hch/CDefObjInfoHandler.h"

using namespace std;

vector<Coordinate>* CPathfinder::GetPath(Coordinate start, Coordinate end, const CGHeroInstance* hero)
{
	Start = start;
	End = end;
	return GetPath(hero);
}

/*
 * Does basic input checking and setup for the path calculation.
*/
vector<Coordinate>* CPathfinder::GetPath(const CGHeroInstance* hero)
{
	//check input
	if ((Start.x < 0)||(Start.y < 0)||(Start.z < 0)||(End.x < 0)||(End.y < 0)||(End.z < 0))
	{
		return NULL;
	}
	if ((Start.x >= CGI->mh->sizes.x)||(Start.y >= CGI->mh->sizes.y)||(Start.z >= CGI->mh->sizes.z)
			||(End.x >= CGI->mh->sizes.x)||(End.y >= CGI->mh->sizes.y)||(End.z >= CGI->mh->sizes.z))
	{
		return NULL;
	}

	Hero = hero;

	//Reset the queues
	Open = priority_queue < vector<Coordinate>, vector<vector<Coordinate> >, Compare>();
	Closed.clear();

	//Determine if the hero can move on water
	int3 hpos = Hero->getPosition(false);
	if (!Hero->canWalkOnSea())
	{
		if (CGI->mh->ttiles[hpos.x][hpos.y][hpos.z].tileInfo->tertype==water)
			blockLandSea=false;
		else
			blockLandSea=true;
	}
	else
	{
		blockLandSea = boost::logic::indeterminate;
	}

	CalcG(&Start);
	Start.h = 0;
	CalcG(&End);
	CalcH(&End);

	//If it is impossible to get to where the user clicked, dont return a path.
	if(End.h == -1)
	{
		return NULL;
	}

	//Add the Start node to the open list.
	vector<Coordinate> startPath;
	startPath.push_back(Start);
	Open.push(startPath);

	//Calculate the path.
	vector<Coordinate>* toReturn =  CalcPath();

	if(toReturn != NULL)
	{
		//Flip the route since it is calculated in reverse
		int size = toReturn->size();
		for(int i = 0; i < size/2; i++)
		{
			Coordinate t = toReturn->at(i);
			(*toReturn)[i] = toReturn->at(size-1-i);
			(*toReturn)[size-1-i] = t;
		}
	}

	return toReturn;
}

/*
 * Does the actual path calculation.  Don't call this directly, call GetPath instead.
*/
vector<Coordinate>* CPathfinder::CalcPath()
{
	if(Open.empty())
	{
		return NULL;
	}

	//Find the path in open with the smallest cost (f = g + h)
	//and remove it from open
	vector<Coordinate>* branch = new vector<Coordinate>();
	*branch = Open.top();
	Open.pop();

	//Don't search elements in the closed list, for they have been visited already.
	if(!ExistsInClosed(branch->back()))
	{
		//Add it to the closed list.
		Closed.push_back(branch->back());

		//If it is the destination
		if(branch->back().x == End.x && branch->back().y == End.y && branch->back().z == End.z)
		{
			return branch;
		}

		//Add neighbors to the open list
		AddNeighbors(branch);
	}
	delete branch;

	return CalcPath();
}

/*
 * Determines if the given node exists in the closed list, returns true if it does.  This is
 * used to ensure you dont check the same node twice.
*/
bool CPathfinder::ExistsInClosed(Coordinate node)
{
	for(int i = 0; i < Closed.size(); i++)
	{
		if(node.x == Closed[i].x && node.y == Closed[i].y && node.z == Closed[i].z)
			return true;
	}

	return false;
}

/*
 * Adds the neighbors of the current node to the open cue so they can be considered in the
 * path creation.  If the node has a cost (f = g + h) less than zero, it isn't added to Open.
*/
void CPathfinder::AddNeighbors(vector<Coordinate>* branch)
{
	//8 possible Nodes to add
	//
	//   1  2  3
	//   4  X  5
	//   6  7  8

	Coordinate node = branch->back();
	Coordinate c;

	for(int i = node.x-1; i<node.x+2;i++)
	{
		for(int j = node.y-1; j < node.y+2; j++)
		{
			if(i >= 0 && j >= 0 && i < CGI->mh->sizes.x && j < CGI->mh->sizes.y)
			{
				c = Coordinate(i,j,node.z);

				//Calculate the distance from the end node
				CalcG(&c);

				//Calculate the movement cost
				CalcH(&c);

				if(c.g != -1 && c.h != -1)
				{
					bool pass = true; //checking for allowed visiting direction
					for(int b=0; b<CGI->mh->ttiles[i][j][node.z].tileInfo->visitableObjects.size(); ++b) //checking destination tile
					{
						CGDefInfo * di = CGI->mh->ttiles[i][j][node.z].tileInfo->visitableObjects[b]->defInfo;
						if( (i == node.x-1 && j == node.y-1) && !(di->visitDir & (1<<4)) )
						{
							pass = false; break;
						}
						if( (i == node.x && j == node.y-1) && !(di->visitDir & (1<<5)) )
						{
							pass = false; break;
						}
						if( (i == node.x+1 && j == node.y-1) && !(di->visitDir & (1<<6)) )
						{
							pass = false; break;
						}
						if( (i == node.x+1 && j == node.y) && !(di->visitDir & (1<<7)) )
						{
							pass = false; break;
						}
						if( (i == node.x+1 && j == node.y+1) && !(di->visitDir & (1<<0)) )
						{
							pass = false; break;
						}
						if( (i == node.x && j == node.y+1) && !(di->visitDir & (1<<1)) )
						{
							pass = false; break;
						}
						if( (i == node.x-1 && j == node.y+1) && !(di->visitDir & (1<<2)) )
						{
							pass = false; break;
						}
						if( (i == node.x-1 && j == node.y) && !(di->visitDir & (1<<3)) )
						{
							pass = false; break;
						}
					}
					for(int b=0; b<CGI->mh->ttiles[node.x][node.y][node.z].tileInfo->visitableObjects.size(); ++b) //checking source tile
					{
						CGDefInfo * di = CGI->mh->ttiles[node.x][node.y][node.z].tileInfo->visitableObjects[b]->defInfo;
						if( (i == node.x-1 && j == node.y-1) && !(di->visitDir & (1<<0)) )
						{
							pass = false; break;
						}
						if( (i == node.x && j == node.y-1) && !(di->visitDir & (1<<1)) )
						{
							pass = false; break;
						}
						if( (i == node.x+1 && j == node.y-1) && !(di->visitDir & (1<<2)) )
						{
							pass = false; break;
						}
						if( (i == node.x+1 && j == node.y) && !(di->visitDir & (1<<3)) )
						{
							pass = false; break;
						}
						if( (i == node.x+1 && j == node.y+1) && !(di->visitDir & (1<<4)) )
						{
							pass = false; break;
						}
						if( (i == node.x && j == node.y+1) && !(di->visitDir & (1<<5)) )
						{
							pass = false; break;
						}
						if( (i == node.x-1 && j == node.y+1) && !(di->visitDir & (1<<6)) )
						{
							pass = false; break;
						}
						if( (i == node.x-1 && j == node.y) && !(di->visitDir & (1<<7)) )
						{
							pass = false; break;
						}
					}
					if(pass)
					{
						vector<Coordinate> toAdd = *branch;
						toAdd.push_back(c);
						Open.push(toAdd);
					}
				}
				//delete c;
			}
		}
	}

}

/*
 * Calculates the movement cost of the node.  Returns -1 if it is impossible to travel on.
*/
void CPathfinder::CalcH(Coordinate* node)
{
	/*
	 * If the terrain is blocked
	 * If the terrain is rock
	 * If the Hero cant walk on water and node is in water
	 * If the Hero is on water and the node is not.
	 * If there is fog of war on the node.
	 *  => Impossible to move there.
	 */
	if( (CGI->mh->ttiles[node->x][node->y][node->z].tileInfo->blocked && !(node->x==End.x && node->y==End.y && CGI->mh->ttiles[node->x][node->y][node->z].tileInfo->visitable)) ||
		(CGI->mh->ttiles[node->x][node->y][node->z].tileInfo->tertype==rock) ||
		((blockLandSea) && (CGI->mh->ttiles[node->x][node->y][node->z].tileInfo->tertype==water)) ||
		(!CGI->state->players[Hero->tempOwner].fogOfWarMap[node->x][node->y][node->z]) ||
		((!blockLandSea) && (CGI->mh->ttiles[node->x][node->y][node->z].tileInfo->tertype!=water)))
	{
		//Impossible.

		node->h = -1;
		return;
	}

	int ret=-1;
	int x = node->x;
	int y = node->y;
	if(node->x>=CGI->mh->map->width)
		x = CGI->mh->map->width-1;
	if(node->y>=CGI->mh->map->height)
		y = CGI->mh->map->height-1;

	//Get the movement cost.
	ret = Hero->getTileCost(CGI->mh->ttiles[x][y][node->z].tileInfo->tertype, CGI->mh->map->terrain[x][y][node->z].malle,CGI->mh->map->terrain[x][y][node->z].nuine);

	node->h = ret;
}

/*
 * Calculates distance from node to end node.
*/
void CPathfinder::CalcG(Coordinate* node)
{
	float dist = (End.x-node->x) * (End.x-node->x) + (End.y-node->y) * (End.y-node->y);
	node->g = sqrt(dist);
	return;
}

/*
 * Converts the old Pathfinder format for compatibility reasons. This should be replaced
 * eventually by making the need for it obsolete.
*/
CPath* CPathfinder::ConvertToOldFormat(vector<Coordinate>* p)
{
	if(p == NULL)
		return NULL;

	CPath* path = new CPath();

	std::vector<int> costs; //vector with costs of tiles

	for(int i = 0; i < p->size(); i++)
	{
		CPathNode temp;

		//Set coord
		temp.coord = int3(p->at(i).x,p->at(i).y,p->at(i).z);

		//Set accesible
		if(p->at(i).h == -1)
		{
			temp.accesible = false;
		}
		else
		{
			temp.accesible = true;
		}
		//set diagonality
		float diagonal = 1.0f; //by default
		if(i>0)
		{
			if(p->at(i-1).x != temp.coord.x && p->at(i-1).y != temp.coord.y)
			{
				diagonal = sqrt(2.0f);
			}
		}

		//Set distance
		costs.push_back( i==0 ? 0 : p->at(i - 1).h * diagonal );

		//theNodeBefore is never used outside of pathfinding?

		//Set visited
		temp.visited = false;

		path->nodes.push_back(temp);
	}

	costs.push_back(0);
	for(int i=path->nodes.size()-1; i>=0; --i)
	{
		path->nodes[i].dist = costs[i+1] + ((i == path->nodes.size()-1) ? 0 : path->nodes[i+1].dist);
	}

	return path;
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

int3 CPath::startPos()
{
	return int3(nodes[nodes.size()-1].coord.x,nodes[nodes.size()-1].coord.y,nodes[nodes.size()-1].coord.z);
}
int3 CPath::endPos()
{
	return int3(nodes[0].coord.x,nodes[0].coord.y,nodes[0].coord.z);
}

/*
 * The function used by the priority cue to determine which node to put at the top.
*/
bool Compare::operator()(const vector<Coordinate>& a, const vector<Coordinate>& b)
{
	double aTotal=0;
	double bTotal=0;

	for(int i = 0; i < a.size(); i++)
	{
		aTotal += a[i].g*a[i].h;
	}
	for(int i = 0; i < b.size(); i++)
	{
		bTotal += b[i].g*b[i].h;
	}

	return aTotal > bTotal;
}

void Coordinate::operator =(const Coordinate &other)
{
	this->x = other.x;
	this->y = other.y;
	this->z = other.z;
	this->g = other.g;
	this->h = other.h;
}

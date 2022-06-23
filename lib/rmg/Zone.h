//
//  Zone.hpp
//  vcmi
//
//  Created by nordsoft on 22.06.2022.
//

#pragma once

#include "../GameConstants.h"
#include "float3.h"
#include "../int3.h"
#include "CRmgTemplate.h"

class CMapGenerator;

class Modificator
{
	
};

class DLL_LINKAGE Zone : public rmg::ZoneOptions
{
public:
	Zone(CMapGenerator * Gen);
	
	void setOptions(const rmg::ZoneOptions & options);
	bool isUnderground() const;
	
	float3 getCenter() const;
	void setCenter(const float3 &f);
	int3 getPos() const;
	void setPos(const int3 &pos);
	
	void addTile(const int3 & pos);
	void removeTile(const int3 & pos);
	void initFreeTiles();
	const std::set<int3> & getTileInfo() const;
	const std::set<int3> & getPossibleTiles() const;
	void clearTiles();
	
	const std::set<int3>& getFreePaths() const;
	void addFreePath(const int3 &);
	
	si32 getTownType() const;
	const Terrain & getTerrainType() const;
	
	bool crunchPath(const int3 & src, const int3 & dst, bool onlyStraight, std::set<int3> * clearedTiles = nullptr);
	bool connectPath(const int3 & src, bool onlyStraight);
	bool connectWithCenter(const int3 & src, bool onlyStraight, bool passTroughBlocked = false);

protected:
	CMapGenerator * gen;
	
	//placement info
	int3 pos;
	float3 center;
	std::set<int3> tileinfo; //irregular area assined to zone
	std::set<int3> possibleTiles; //optimization purposes for treasure generation
	std::set<int3> freePaths; //core paths of free tiles that all other objects will be linked to
	std::set<int3> coastTiles; //tiles bordered to water
	
	//template info
	si32 townType;
	Terrain terrainType;
	
};

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

class RmgMap;

class Modificator
{
public:
	virtual void process() = 0;
};

class DLL_LINKAGE Zone : public rmg::ZoneOptions
{
public:
	Zone(RmgMap & map, CRandomGenerator & generator);
	
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
	void addPossibleTile(const int3 & pos);
	void removePossibleTile(const int3 & pos);
	void clearTiles();
	
	void fractalize();
	
	const std::set<int3>& getFreePaths() const;
	void addFreePath(const int3 &);
	
	si32 getTownType() const;
	void setTownType(si32 town);
	const Terrain & getTerrainType() const;
	void setTerrainType(const Terrain & terrain);
	
	bool crunchPath(const int3 & src, const int3 & dst, bool onlyStraight, std::set<int3> * clearedTiles = nullptr);
	bool connectPath(const int3 & src, bool onlyStraight);
	bool connectWithCenter(const int3 & src, bool onlyStraight, bool passTroughBlocked = false);
	void addToConnectLater(const int3& src);
	void connectLater();
	
	template<class T>
	T* getModificator()
	{
		for(auto & m : modificators)
			if(auto * mm = dynamic_cast<T*>(m.get()))
				return mm;
		return nullptr;
	}
	
	template<class T>
	void addModificator(CRandomGenerator & generator)
	{
		modificators.push_back(std::make_unique<T>(*this, map, generator));
	}
	
protected:
	CRandomGenerator & generator;
	RmgMap & map;
	std::list<std::unique_ptr<Modificator>> modificators;
	
	//placement info
	int3 pos;
	float3 center;
	std::set<int3> tileinfo; //irregular area assined to zone
	std::set<int3> possibleTiles; //optimization purposes for treasure generation
	std::set<int3> freePaths; //core paths of free tiles that all other objects will be linked to
	std::set<int3> coastTiles; //tiles bordered to water
	std::set<int3> tilesToConnectLater; //will be connected after paths are fractalized
	
	//template info
	si32 townType;
	Terrain terrainType;
	
};

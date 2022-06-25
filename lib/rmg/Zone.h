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
#include "CRmgArea.h"
#include "CRmgObject.h"

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
	
	const Rmg::Area & getArea() const;
	Rmg::Area & area();
	Rmg::Area & areaPossible();
	Rmg::Area & freePaths();
	
	void initFreeTiles();
	void clearTiles();
	void fractalize();
	
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
	Rmg::Area dArea; //irregular area assined to zone
	Rmg::Area dAreaPossible;
	Rmg::Area dAreaFree; //core paths of free tiles that all other objects will be linked to
	Rmg::Area dTilesToConnectLater; //will be connected after paths are fractalized
	std::vector<Rmg::Object> dObjects;
	
	//template info
	si32 townType;
	Terrain terrainType;
	
};

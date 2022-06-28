/*
 * Zone.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "float3.h"
#include "../int3.h"
#include "CRmgTemplate.h"
#include "RmgArea.h"
#include "RmgObject.h"

#define MODIFICATOR(x) x(Zone & z, RmgMap & m, CMapGenerator & g): Modificator(z, m, g) {setName(#x);}

class RmgMap;
class CMapGenerator;
class Zone;

class Modificator
{
public:
	Modificator() = delete;
	Modificator(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	virtual void process() = 0;
	virtual void init() {/*override to add dependencies*/}
	virtual char dump(const int3 &);
	virtual ~Modificator() {};
	
	void setName(const std::string & n);
	const std::string & getName() const;
	
	void run();
	void dependency(Modificator * modificator);
	void postfunction(Modificator * modificator);

protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
private:
	std::string name;
	bool started = false;
	bool finished = false;
	std::set<Modificator*> preceeders;
	void dump();
};

class DLL_LINKAGE Zone : public rmg::ZoneOptions
{
public:
	Zone(RmgMap & map, CMapGenerator & generator);
	
	void setOptions(const rmg::ZoneOptions & options);
	bool isUnderground() const;
	
	float3 getCenter() const;
	void setCenter(const float3 &f);
	int3 getPos() const;
	void setPos(const int3 &pos);
	
	const rmg::Area & getArea() const;
	rmg::Area & area();
	rmg::Area & areaPossible();
	rmg::Area & freePaths();
	rmg::Area & areaUsed();
	
	void initFreeTiles();
	void clearTiles();
	void fractalize();
	
	si32 getTownType() const;
	void setTownType(si32 town);
	const Terrain & getTerrainType() const;
	void setTerrainType(const Terrain & terrain);
	
	bool crunchPath(const int3 & src, const int3 & dst, bool onlyStraight, std::set<int3> * clearedTiles = nullptr);
	bool connectPath(const int3 & src, bool onlyStraight);
	bool connectPath(const rmg::Area & src, bool onlyStraight);
	
	template<class T>
	T* getModificator()
	{
		for(auto & m : modificators)
			if(auto * mm = dynamic_cast<T*>(m.get()))
				return mm;
		return nullptr;
	}
	
	template<class T>
	void addModificator() //name is used for debug purposes
	{
		modificators.push_back(std::make_unique<T>(*this, map, generator));
	}
	
	void initModificators();
	void processModificators();
	
protected:
	CMapGenerator & generator;
	RmgMap & map;
	std::list<std::unique_ptr<Modificator>> modificators;
	
	//placement info
	int3 pos;
	float3 center;
	rmg::Area dArea; //irregular area assined to zone
	rmg::Area dAreaPossible;
	rmg::Area dAreaFree; //core paths of free tiles that all other objects will be linked to
	rmg::Area dAreaUsed;
	
	//template info
	si32 townType;
	Terrain terrainType;
	
};

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
#include "RmgPath.h"
#include "RmgObject.h"

//uncomment to generate dumps afger every step of map generation
//#define RMG_DUMP

#define MODIFICATOR(x) x(Zone & z, RmgMap & m, CMapGenerator & g): Modificator(z, m, g) {setName(#x);}
#define DEPENDENCY(x) 		dependency(zone.getModificator<x>());
#define POSTFUNCTION(x)		postfunction(zone.getModificator<x>());
#define DEPENDENCY_ALL(x) 	for(auto & z : map.getZones()) \
							{ \
								dependency(z.second->getModificator<x>()); \
							}
#define POSTFUNCTION_ALL(x) for(auto & z : map.getZones()) \
							{ \
								postfunction(z.second->getModificator<x>()); \
							}

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
	virtual ~Modificator();
	
	void setName(const std::string & n);
	const std::string & getName() const;
	
	void run();
	void dependency(Modificator * modificator);
	void postfunction(Modificator * modificator);

protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	bool isFinished() const;
	
private:
	std::string name;
	bool started = false;
	bool finished = false;
	std::list<Modificator*> preceeders; //must be ordered container
	void dump();
};

extern std::function<bool(const int3 &)> AREA_NO_FILTER;

class Zone : public rmg::ZoneOptions
{
public:
	Zone(RmgMap & map, CMapGenerator & generator);
	Zone(const Zone &) = delete;
	
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
		
	void connectPath(const rmg::Path & path);
	rmg::Path searchPath(const rmg::Area & src, bool onlyStraight, std::function<bool(const int3 &)> areafilter = AREA_NO_FILTER) const;
	rmg::Path searchPath(const int3 & src, bool onlyStraight, std::function<bool(const int3 &)> areafilter = AREA_NO_FILTER) const;
	
	template<class T>
	T* getModificator()
	{
		for(auto & m : modificators)
			if(auto * mm = dynamic_cast<T*>(m.get()))
				return mm;
		return nullptr;
	}
	
	template<class T>
	void addModificator()
	{
		modificators.emplace_back(new T(*this, map, generator));
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

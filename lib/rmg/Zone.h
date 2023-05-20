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
#include "../CRandomGenerator.h"
#include "CRmgTemplate.h"
#include "RmgArea.h"
#include "RmgPath.h"
#include "RmgObject.h"
#include "modificators/Modificator.h"

//uncomment to generate dumps afger every step of map generation
//#define RMG_DUMP

VCMI_LIB_NAMESPACE_BEGIN

class RmgMap;
class CMapGenerator;
class Modificator;

extern std::function<bool(const int3 &)> AREA_NO_FILTER;

typedef std::list<std::shared_ptr<Modificator>> TModificators;

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
	
	FactionID getTownType() const;
	void setTownType(si32 town);
	TerrainId getTerrainType() const;
	void setTerrainType(TerrainId terrain);
		
	void connectPath(const rmg::Path & path);
	rmg::Path searchPath(const rmg::Area & src, bool onlyStraight, const std::function<bool(const int3 &)> & areafilter = AREA_NO_FILTER) const;
	rmg::Path searchPath(const int3 & src, bool onlyStraight, const std::function<bool(const int3 &)> & areafilter = AREA_NO_FILTER) const;

	TModificators getModificators();

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

public:
	boost::recursive_mutex areaMutex;
	using Lock = boost::unique_lock<boost::recursive_mutex>;
	
protected:
	CMapGenerator & generator;
	RmgMap & map;
	TModificators modificators;
	bool finished;
	
	//placement info
	int3 pos;
	float3 center;
	rmg::Area dArea; //irregular area assined to zone
	rmg::Area dAreaPossible;
	rmg::Area dAreaFree; //core paths of free tiles that all other objects will be linked to
	rmg::Area dAreaUsed;
	std::vector<int3> possibleQuestArtifactPos;
	
	//template info
	si32 townType;
	TerrainId terrainType;
};

VCMI_LIB_NAMESPACE_END

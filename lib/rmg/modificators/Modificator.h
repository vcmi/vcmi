/*
 * Modificator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../GameConstants.h"
#include "../../int3.h"
#include "../Zone.h"
#include "../MapProxy.h"

class RmgMap;
class CMapGenerator;
class Zone;
class MapProxy;

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

VCMI_LIB_NAMESPACE_BEGIN

class Modificator
{
public:
	Modificator() = delete;
	Modificator(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	virtual void init() {/*override to add dependencies*/}
	virtual char dump(const int3 &);
	virtual ~Modificator() = default;

	void setName(const std::string & n);
	const std::string & getName() const;

	bool isReady();
	bool isFinished();
	
	void run();
	void dependency(Modificator * modificator);
	void postfunction(Modificator * modificator);

protected:
	RmgMap & map;
	std::shared_ptr<MapProxy> mapProxy;
	CMapGenerator & generator;
	Zone & zone;

	bool finished = false;
	
	mutable boost::recursive_mutex externalAccessMutex; //Used to communicate between Modificators
	using RecursiveLock = boost::unique_lock<boost::recursive_mutex>;
	using Lock = boost::unique_lock<boost::shared_mutex>;

private:
	virtual void process() = 0;

	std::string name;

	std::list<Modificator*> preceeders; //must be ordered container

	mutable boost::shared_mutex mx; //Used only for task scheduling

	void dump();
};

VCMI_LIB_NAMESPACE_END

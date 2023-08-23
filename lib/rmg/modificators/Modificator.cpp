/*
 * Modificator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Modificator.h"
#include "../Functions.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../CStopWatch.h"
#include "../../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

Modificator::Modificator(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
	mapProxy = map.getMapProxy();
}

void Modificator::setName(const std::string & n)
{
	name = n;
}

const std::string & Modificator::getName() const
{
	return name;
}

bool Modificator::isReady()
{
	Lock lock(mx, boost::try_to_lock_t{});
	if (!lock.owns_lock())
	{
		return false;
	}
	else
	{
		//Check prerequisites
		for (auto it = preceeders.begin(); it != preceeders.end();)
		{
			if ((*it)->isFinished()) //OK
			{
				//This preceeder won't be checked in the future
				it = preceeders.erase(it);
			}
			else
			{
				return false;
			}
		}

		//If a job is finished, it should be already erased from a queue
		return !finished;
	}
}

bool Modificator::isFinished()
{
	Lock lock(mx, boost::try_to_lock_t{});
	if (!lock.owns_lock())
	{
		return false;
	}
	else
	{
		return finished;
	}
}

void Modificator::run()
{
	Lock lock(mx);

	if(!finished)
	{
		logGlobal->info("Modificator zone %d - %s - started", zone.getId(), getName());
		CStopWatch processTime;
		try
		{
			process();
		}
		catch(rmgException &e)
		{
			logGlobal->error("Modificator %s, exception: %s", getName(), e.what());
		}
#ifdef RMG_DUMP
		dump();
#endif
		finished = true;
		logGlobal->info("Modificator zone %d - %s - done (%d ms)", zone.getId(), getName(), processTime.getDiff());
	}
}

void Modificator::dependency(Modificator * modificator)
{
	if(modificator && modificator != this)
	{
		//TODO: use vstd::contains
		if(std::find(preceeders.begin(), preceeders.end(), modificator) == preceeders.end())
			preceeders.push_back(modificator);
	}
}

void Modificator::postfunction(Modificator * modificator)
{
	if(modificator && modificator != this)
	{
		if(std::find(modificator->preceeders.begin(), modificator->preceeders.end(), this) == modificator->preceeders.end())
			modificator->preceeders.push_back(this);
	}
}

void Modificator::dump()
{
	std::ofstream out(boost::str(boost::format("seed_%d_modzone_%d_%s.txt") % generator.getRandomSeed() % zone.getId() % getName()));
	int levels = map.levels();
	int width =  map.width();
	int height = map.height();
	for(int z = 0; z < levels; z++)
	{
		for(int j=0; j<height; j++)
		{
			for(int i=0; i<width; i++)
			{
				out << dump(int3(i, j, z));
			}
			out << std::endl;
		}
		out << std::endl;
	}
	out << std::endl;
}

char Modificator::dump(const int3 & t)
{
	if(zone.freePaths().contains(t))
		return '.'; //free path
	if(zone.areaPossible().contains(t))
		return ' '; //possible
	if(zone.areaUsed().contains(t))
		return 'U'; //used
	if(zone.area().contains(t))
	{
		if(map.shouldBeBlocked(t))
			return '#'; //obstacle
		else
			return '^'; //visitable points?
	}
	return '?';
}

VCMI_LIB_NAMESPACE_END
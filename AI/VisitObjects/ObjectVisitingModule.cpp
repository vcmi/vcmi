#include "StdInc.h"
#include "ObjectVisitingModule.h"
#include "..\..\CCallback.h"
#include "..\..\lib\CObjectHandler.h"
#include "..\..\lib\CGameState.h"


ObjectVisitingModule::ObjectVisitingModule(void)
{
}


ObjectVisitingModule::~ObjectVisitingModule(void)
{
}

void ObjectVisitingModule::receivedMessage(const boost::any &msg)
{

	if(auto txt = boost::any_cast<std::string>(&msg))
	{
		std::istringstream hlp(*txt);
		std::string w1, w2;
		hlp >> w1 >> w2;
		if(w1 != "visit")
			return;

		if(w2 == "all")
		{
			destinations = getDestinations();
			logGlobal->debugStream() << "Added as destination all possible objects:";
			printObjects(destinations);
		}
		if(w2 == "list")
		{
			auto dsts = getDestinations();
			logGlobal->debugStream() << "Possible visit destinations:";
			printObjects(dsts);
		}
		if(w2 == "clear")
		{
			destinations.clear();
		}
		if(w2 == "add")
		{
			auto dsts = getDestinations();

			int no;
			hlp >> no;
			if(no < 0 || no >= dsts.size() || !hlp)
			{
				logGlobal->warnStream() << "Invalid number!";
			}
			else if(vstd::contains(destinations, dsts[no]))
			{
				logGlobal->warnStream() << "Object already present on destinations list!";
			}
			else
			{
				destinations.push_back(dsts[no]);
			}
		}
	}
	
	if(auto objs = boost::any_cast<std::vector<const CGObjectInstance*>>(&msg))
	{
		destinations = *objs;
	}
}

bool compareNodes(const CGPathNode *lhs, const CGPathNode *rhs)
{
	if(lhs->turns != rhs->turns)
		return lhs->turns < rhs->turns;

	return lhs->moveRemains > rhs->moveRemains;
}

void ObjectVisitingModule::executeInternal()
{
	int weekNow = cb->getDate(Date::WEEK);
	if(weekNow != week)
	{
		visitedThisWeek.clear();
		week = weekNow;
	}

	if(auto h = myHero())
	{
		cb->setSelection(h);

		while(true)
		{
			auto leftToVisit = destinations;
			vstd::erase_if(leftToVisit, [this](const CGObjectInstance *obj) 
				{ return vstd::contains(visitedThisWeek, obj); });

			if(leftToVisit.empty())
				return;

			auto isCloser = [this] (const CGObjectInstance *lhs, const CGObjectInstance *rhs) -> bool
			{
				return compareNodes(cb->getPathInfo(lhs->visitablePos()), cb->getPathInfo(lhs->visitablePos()));
			};

			const auto toVisit = *range::min_element(leftToVisit, isCloser);

			try
			{
				if(!moveHero(h, toVisit->visitablePos()))
					break;
			}
			catch(...)
			{
				logAi->errorStream() <<boost::format("Something wrong with %s at %s, removing") % toVisit->getHoverText() % toVisit->pos;
				destinations -= toVisit;
			}
		}
	}
}

void ObjectVisitingModule::heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start)
{
	if(vstd::contains(destinations, visitedObj))
		visitedThisWeek.insert(visitedObj);
}

bool ObjectVisitingModule::isInterestingObject(const CGObjectInstance *obj) const
{
	switch(obj->ID)
	{
	case Obj::WINDMILL:
	case Obj::WATER_WHEEL:
	case Obj::MYSTICAL_GARDEN:
		return true;
	default:
		return false;
	}
}

std::vector<const CGObjectInstance *> ObjectVisitingModule::getDestinations() const
{
	std::vector<const CGObjectInstance *> ret;

	auto foreach_tile_pos = [this](boost::function<void(const int3& pos)> foo)
	{
		for(int i = 0; i < cb->getMapSize().x; i++)
			for(int j = 0; j < cb->getMapSize().y; j++)
				for(int k = 0; k < cb->getMapSize().z; k++)
					foo(int3(i,j,k));
	};


	foreach_tile_pos([&](const int3 &pos)
	{
		BOOST_FOREACH(const CGObjectInstance *obj, cb->getVisitableObjs(pos, false))
		{
			if(isInterestingObject(obj))
				ret.push_back(obj);
		}
	});

	return ret;
}

void ObjectVisitingModule::printObjects(const std::vector<const CGObjectInstance *> &objs) const
{
	logGlobal->debugStream() << objs.size() << " objects:";
	for(int i =  0; i < objs.size(); i++)
		logGlobal->debugStream() << boost::format("\t%d. %s at %s.") % i % objs[i]->getHoverText() % objs[i]->pos;
}

const CGHeroInstance * ObjectVisitingModule::myHero() const
{
	auto heroes = getMyHeroes();
	if(heroes.empty())
		return nullptr;
	return heroes.front();
}

bool ObjectVisitingModule::moveHero(const CGHeroInstance *h, int3 dst)
{
	int3 startHpos = h->visitablePos();
	bool ret = false;
	if(startHpos == dst)
	{
		assert(cb->getVisitableObjs(dst).size() > 1); //there's no point in revisiting tile where there is no visitable object
		cb->moveHero(h, CGHeroInstance::convertPosition(dst, true));
		ret = true;
	}
	else
	{
		CGPath path;
		cb->getPath2(dst, path);
		if(path.nodes.empty())
		{
			logAi->errorStream() << "Hero " << h->name << " cannot reach " << dst;
			cb->recalculatePaths();
			throw std::runtime_error("Wrong move order!");
		}

		int i=path.nodes.size()-1;
		for(; i>0; i--)
		{
			//stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
			if(path.nodes[i-1].turns)
			{
				//blockedHeroes.insert(h); //to avoid attempts of moving heroes with very little MPs
				break;
			}

			int3 endpos = path.nodes[i-1].coord;
			if(endpos == h->visitablePos())
					continue;

			logAi->debugStream() << "Moving " << h->name << " from " << h->getPosition() << " to " << endpos;
			cb->moveHero(h, CGHeroInstance::convertPosition(endpos, true));
		}
		ret = !i;
	}

	return ret;
}

/*
 * Goals.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Goal.h"
#include "../VCAI.h"
#include "../Fuzzy.h"
#include "../SectorMap.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Tasks/VisitTile.h"
#include "../Tasks/BuildStructure.h"
#include "../Tasks/RecruitHero.h"
#include "CaptureObjects.h"
#include "Conquer.h"
#include "Explore.h"
#include "Build.h"
#include "GatherArmy.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh; //TODO: this logic should be moved inside VCAI

using namespace Goals;

TSubgoal Goals::sptr(const AbstractGoal & tmp)
{
	std::shared_ptr<AbstractGoal> ptr;
	ptr.reset(tmp.clone());

	return ptr;
}

std::string Goals::AbstractGoal::toString() const //TODO: virtualize
{
	std::string desc;
	switch(goalType)
	{
	case INVALID:
		return "INVALID";
	case WIN:
		return "WIN";
	case DO_NOT_LOSE:
		return "DO NOT LOOSE";
	case CONQUER:
		return "CONQUER";
	case BUILD:
		return "BUILD";
	case EXPLORE:
		desc = "EXPLORE";
		break;
	case GATHER_ARMY:
		desc = "GATHER ARMY";
		break;
	case BOOST_HERO:
		desc = "BOOST_HERO (unsupported)";
		break;
	case RECRUIT_HERO:
		return "RECRUIT HERO";
	case BUILD_STRUCTURE:
		return "BUILD STRUCTURE";
	case BUY_RESOURCES:
		desc = "COLLECT RESOURCE";
		break;
	case GATHER_TROOPS:
		desc = "GATHER TROOPS";
		break;
	case GET_OBJ:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if(obj)
			desc = "GET OBJ " + obj->getObjectName();
	}
	break;
	case FIND_OBJ:
		desc = "FIND OBJ " + boost::lexical_cast<std::string>(objid);
		break;
	case VISIT_HERO:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if(obj)
			desc = "VISIT HERO " + obj->getObjectName();
	}
	break;
	case GET_ART_TYPE:
		desc = "GET ARTIFACT OF TYPE " + VLC->arth->artifacts[aid]->Name();
		break;
	case ISSUE_COMMAND:
		return "ISSUE COMMAND (unsupported)";
	case VISIT_TILE:
		desc = "VISIT TILE " + tile.toString();
		break;
	case CLEAR_WAY_TO:
		desc = "CLEAR WAY TO " + tile.toString();
		break;
	case DIG_AT_TILE:
		desc = "DIG AT TILE " + tile.toString();
		break;
	default:
		return boost::lexical_cast<std::string>(goalType);
	}
	if(hero.get(true)) //FIXME: used to crash when we lost hero and failed goal
		desc += " (" + hero->name + ")";
	return desc;
}

//TODO: virtualize if code gets complex?
bool Goals::AbstractGoal::operator==(AbstractGoal & g)
{
	if(g.goalType != goalType)
		return false;
	if(g.isElementar != isElementar) //elementar goals fulfill long term non-elementar goals (VisitTile)
		return false;

	switch(goalType)
	{
	//no parameters
	case INVALID:
	case WIN:
	case DO_NOT_LOSE:
	case RECRUIT_HERO: //recruit any hero, as yet
		return true;
		break;

	//assigned to hero, no parameters
	case CONQUER:
	case EXPLORE:
	case GATHER_ARMY: //actual value is indifferent
	case BOOST_HERO:
		return g.hero.h == hero.h; //how comes HeroPtrs are equal for different heroes?
		break;

	//assigned hero and tile
	case VISIT_TILE:
	case CLEAR_WAY_TO:
		return (g.hero.h == hero.h && g.tile == tile);
		break;

	//assigned hero and object
	case GET_OBJ:
	case FIND_OBJ: //TODO: use subtype?
	case VISIT_HERO:
	case GET_ART_TYPE:
	case DIG_AT_TILE:
		return (g.hero.h == hero.h && g.objid == objid);
		break;

	//no check atm
	case BUY_RESOURCES:
	case GATHER_TROOPS:
	case ISSUE_COMMAND:
	case BUILD: //TODO: should be decomposed to build specific structures
	case BUILD_STRUCTURE:
	default:
		return false;
	}
}

//TODO: find out why the following are not generated automatically on MVS?

void AbstractGoal::accept(VCAI * ai)
{
	ai->tryRealize(*this);
}

float AbstractGoal::accept(FuzzyHelper * f)
{
	return f->evaluate(*this);
}

//TSubgoal AbstractGoal::whatToDoToAchieve()
//{
//	logAi->debug("Decomposing goal of type %s",name());
//	return sptr (Goals::Explore());
//}

TSubgoal FindObj::whatToDoToAchieve()
{
	const CGObjectInstance * o = nullptr;
	if(resID > -1) //specified
	{
		for(const CGObjectInstance * obj : ai->visitableObjs)
		{
			if(obj->ID == objid && obj->subID == resID)
			{
				o = obj;
				break; //TODO: consider multiple objects and choose best
			}
		}
	}
	else
	{
		for(const CGObjectInstance * obj : ai->visitableObjs)
		{
			if(obj->ID == objid)
			{
				o = obj;
				break; //TODO: consider multiple objects and choose best
			}
		}
	}
	if(o && ai->isAccessible(o->pos)) //we don't use isAccessibleForHero as we don't know which hero it is
		return sptr(Goals::GetObj(o->id.getNum()));
	else
		return sptr(Goals::Explore());
}

std::string GetObj::completeMessage() const
{
	return "hero " + hero.get()->name + " captured Object ID = " + boost::lexical_cast<std::string>(objid);
}

TSubgoal GetObj::whatToDoToAchieve()
{
	const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(objid));
	if(!obj)
		return sptr(Goals::Explore());
	if(obj->tempOwner == ai->playerID) //we can't capture our own object -> move to Win codition
		throw cannotFulfillGoalException("Cannot capture my own object " + obj->getObjectName());

	int3 pos = obj->visitablePos();
	if(hero)
	{
		if(ai->isAccessibleForHero(pos, hero))
			return sptr(Goals::VisitTile(pos).sethero(hero));
	}
	else
	{
		for(auto h : cb->getHeroesInfo())
		{
			if(ai->isAccessibleForHero(pos, h))
				return sptr(Goals::VisitTile(pos).sethero(h)); //we must visit object with same hero, if any
		}
	}
	return sptr(Goals::ClearWayTo(pos).sethero(hero));
}

bool GetObj::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType == Goals::VISIT_TILE)
	{
		auto obj = cb->getObj(ObjectInstanceID(objid));
		if(obj && obj->visitablePos() == goal->tile) //object could be removed
			return true;
	}

	return false;
}

std::string VisitHero::completeMessage() const
{
	return "hero " + hero.get()->name + " visited hero " + boost::lexical_cast<std::string>(objid);
}

TSubgoal VisitHero::whatToDoToAchieve()
{
	const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(objid));
	if(!obj)
		return sptr(Goals::Explore());
	int3 pos = obj->visitablePos();

	if(hero && ai->isAccessibleForHero(pos, hero, true) && isSafeToVisit(hero, pos)) //enemy heroes can get reinforcements
	{
		if(hero->pos == pos)
			logAi->error("Hero %s tries to visit himself.", hero.name);
		else
		{
			//can't use VISIT_TILE here as tile appears blocked by target hero
			//FIXME: elementar goal should not be abstract
			return sptr(Goals::VisitHero(objid).sethero(hero).settile(pos).setisElementar(true));
		}
	}
	return sptr(Goals::Invalid());
}

bool VisitHero::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType != Goals::VISIT_TILE)
	{
		return false;
	}
	auto obj = cb->getObj(ObjectInstanceID(objid));
	if(!obj)
	{
		logAi->error("Hero %s: VisitHero::fulfillsMe at %s: object %d not found", hero.name, goal->tile.toString(), objid);
		return false;
	}
	return obj->visitablePos() == goal->tile;
}

TSubgoal GetArtOfType::whatToDoToAchieve()
{
	TSubgoal alternativeWay = CGoal::lookForArtSmart(aid); //TODO: use
	if(alternativeWay->invalid())
		return sptr(Goals::FindObj(Obj::ARTIFACT, aid));
	return sptr(Goals::Invalid());
}

TSubgoal ClearWayTo::whatToDoToAchieve()
{
	assert(cb->isInTheMap(tile)); //set tile
	if(!cb->isVisible(tile))
	{
		logAi->error("Clear way should be used with visible tiles!");
		return sptr(Goals::Explore());
	}

	return (fh->chooseSolution(getAllPossibleSubgoals()));
}

TGoalVec ClearWayTo::getAllPossibleSubgoals()
{
	TGoalVec ret;

	std::vector<const CGHeroInstance *> heroes;
	if(hero)
		heroes.push_back(hero.h);
	else
		heroes = cb->getHeroesInfo();

	for(auto h : heroes)
	{
		//TODO: handle clearing way to allied heroes that are blocked
		//if ((hero && hero->visitablePos() == tile && hero == *h) || //we can't free the way ourselves
		//	h->visitablePos() == tile) //we are already on that tile! what does it mean?
		//	continue;

		//if our hero is trapped, make sure we request clearing the way from OUR perspective

		auto sm = ai->getCachedSectorMap(h);

		int3 tileToHit = sm->firstTileToGet(h, tile);
		if(!tileToHit.valid())
			continue;

		if(isBlockedBorderGate(tileToHit))
		{
			//FIXME: this way we'll not visit gate and activate quest :?
			ret.push_back(sptr(Goals::FindObj(Obj::KEYMASTER, cb->getTile(tileToHit)->visitableObjects.back()->subID)));
		}

		auto topObj = cb->getTopObj(tileToHit);
		if(topObj)
		{
			if(topObj->ID == Obj::HERO && cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
			{
				if(topObj != hero.get(true)) //the hero we want to free
					logAi->error("%s stands in the way of %s", topObj->getObjectName(), h->getObjectName());
			}
			if(topObj->ID == Obj::QUEST_GUARD || topObj->ID == Obj::BORDERGUARD)
			{
				if(shouldVisit(h, topObj))
				{
					//do NOT use VISIT_TILE, as tile with quets guard can't be visited
					ret.push_back(sptr(Goals::GetObj(topObj->id.getNum()).sethero(h)));
					continue; //do not try to visit tile or gather army
				}
				else
				{
					//TODO: we should be able to return apriopriate quest here (VCAI::striveToQuest)
					logAi->debug("Quest guard blocks the way to %s", tile.toString());
					continue; //do not access quets guard if we can't complete the quest
				}
			}
		}
		if(isSafeToVisit(h, tileToHit)) //this makes sense only if tile is guarded, but there i no quest object
		{
			ret.push_back(sptr(Goals::VisitTile(tileToHit).sethero(h)));
		}
		else
		{
			ret.push_back(sptr(Goals::GatherArmy(evaluateDanger(tileToHit, h) * SAFE_ATTACK_CONSTANT).
					   sethero(h).setisAbstract(true)));
		}
	}

	if(ret.empty())
	{
		logAi->warn("There is no known way to clear the way to tile %s", tile.toString());
		throw goalFulfilledException(sptr(Goals::ClearWayTo(tile))); //make sure asigned hero gets unlocked
	}

	return ret;
}

Tasks::TaskList RecruitHero::getTasks() {
	Tasks::TaskList tasks;
	auto heroes = cb->getHeroesInfo();
	auto towns = cb->getTownsInfo();

	if (cb->getResourceAmount(Res::GOLD) > GameConstants::HERO_GOLD_COST * 3
		&& heroes.size() < towns.size() + 1) {
		tasks.push_back(Tasks::sptr(Tasks::RecruitHero()));
	}

	return tasks;
}

TSubgoal DigAtTile::whatToDoToAchieve()
{
	const CGObjectInstance * firstObj = vstd::frontOrNull(cb->getVisitableObjs(tile));
	if(firstObj && firstObj->ID == Obj::HERO && firstObj->tempOwner == ai->playerID) //we have hero at dest
	{
		const CGHeroInstance * h = dynamic_cast<const CGHeroInstance *>(firstObj);
		sethero(h).setisElementar(true);
		return sptr(*this);
	}

	return sptr(Goals::VisitTile(tile));
}

TSubgoal BuildThis::whatToDoToAchieve()
{
	//TODO check res
	//look for town
	//prerequisites?
	return iAmElementar();
}

TSubgoal GetResources::whatToDoToAchieve()
{
	std::vector<const IMarket *> markets;

	std::vector<const CGObjectInstance *> visObjs;
	ai->retrieveVisitableObjs(visObjs, true);
	for(const CGObjectInstance * obj : visObjs)
	{
		if(const IMarket * m = IMarket::castFrom(obj, false))
		{
			if(obj->ID == Obj::TOWN && obj->tempOwner == ai->playerID && m->allowsTrade(EMarketMode::RESOURCE_RESOURCE))
				markets.push_back(m);
			else if(obj->ID == Obj::TRADING_POST) //TODO a moze po prostu test na pozwalanie handlu?
				markets.push_back(m);
		}
	}

	boost::sort(markets, [](const IMarket * m1, const IMarket * m2) -> bool
	{
		return m1->getMarketEfficiency() < m2->getMarketEfficiency();
	});

	markets.erase(boost::remove_if(markets, [](const IMarket * market) -> bool
	{
		if(!(market->o->ID == Obj::TOWN && market->o->tempOwner == ai->playerID))
		{
			if(!ai->isAccessible(market->o->visitablePos()))
				return true;
		}
		return false;
	}), markets.end());

	if(!markets.size())
	{
		for(const CGTownInstance * t : cb->getTownsInfo())
		{
			if(cb->canBuildStructure(t, BuildingID::MARKETPLACE) == EBuildingState::ALLOWED)
				return sptr(Goals::BuildThis(BuildingID::MARKETPLACE, t));
		}
	}
	else
	{
		const IMarket * m = markets.back();
		//attempt trade at back (best prices)
		int howManyCanWeBuy = 0;
		for(Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, 1))
		{
			if(i == resID)
				continue;
			int toGive = -1, toReceive = -1;
			m->getOffer(i, resID, toGive, toReceive, EMarketMode::RESOURCE_RESOURCE);
			assert(toGive > 0 && toReceive > 0);
			howManyCanWeBuy += toReceive * (cb->getResourceAmount(i) / toGive);
		}

		if(howManyCanWeBuy + cb->getResourceAmount(static_cast<Res::ERes>(resID)) >= value)
		{
			auto backObj = cb->getTopObj(m->o->visitablePos()); //it'll be a hero if we have one there; otherwise marketplace
			assert(backObj);
			if(backObj->tempOwner != ai->playerID)
			{
				return sptr(Goals::GetObj(m->o->id.getNum()));
			}
			else
			{
				return sptr(Goals::GetObj(m->o->id.getNum()).setisElementar(true));
			}
		}
	}
	return sptr(setisElementar(true)); //all the conditions for trade are met
}

TSubgoal GatherTroops::whatToDoToAchieve()
{
	std::vector<const CGDwelling *> dwellings;
	for(const CGTownInstance * t : cb->getTownsInfo())
	{
		auto creature = VLC->creh->creatures[objid];
		if(t->subID == creature->faction) //TODO: how to force AI to build unupgraded creatures? :O
		{
			auto creatures = vstd::tryAt(t->town->creatures, creature->level - 1);
			if(!creatures)
				continue;

			int upgradeNumber = vstd::find_pos(*creatures, creature->idNumber);
			if(upgradeNumber < 0)
				continue;

			BuildingID bid(BuildingID::DWELL_FIRST + creature->level - 1 + upgradeNumber * GameConstants::CREATURES_PER_TOWN);
			if(t->hasBuilt(bid)) //this assumes only creatures with dwellings are assigned to faction
			{
				dwellings.push_back(t);
			}
			else
			{
				return sptr(Goals::BuildThis(bid, t));
			}
		}
	}
	for(auto obj : ai->visitableObjs)
	{
		if(obj->ID != Obj::CREATURE_GENERATOR1) //TODO: what with other creature generators?
			continue;

		auto d = dynamic_cast<const CGDwelling *>(obj);
		for(auto creature : d->creatures)
		{
			if(creature.first) //there are more than 0 creatures avaliabe
			{
				for(auto type : creature.second)
				{
					if(type == objid && ai->freeResources().canAfford(VLC->creh->creatures[type]->cost))
						dwellings.push_back(d);
				}
			}
		}
	}
	if(dwellings.size())
	{
		typedef std::map<const CGHeroInstance *, const CGDwelling *> TDwellMap;

		// sorted helper
		auto comparator = [](const TDwellMap::value_type & a, const TDwellMap::value_type & b) -> bool
		{
			const CGPathNode * ln = ai->myCb->getPathsInfo(a.first)->getPathInfo(a.second->visitablePos());
			const CGPathNode * rn = ai->myCb->getPathsInfo(b.first)->getPathInfo(b.second->visitablePos());

			if(ln->turns != rn->turns)
				return ln->turns < rn->turns;

			return (ln->moveRemains > rn->moveRemains);
		};

		// for all owned heroes generate map <hero -> nearest dwelling>
		TDwellMap nearestDwellings;
		for(const CGHeroInstance * hero : cb->getHeroesInfo(true))
		{
			nearestDwellings[hero] = *boost::range::min_element(dwellings, CDistanceSorter(hero));
		}
		if(nearestDwellings.size())
		{
			// find hero who is nearest to a dwelling
			const CGDwelling * nearest = boost::range::min_element(nearestDwellings, comparator)->second;
			if(!nearest)
				throw cannotFulfillGoalException("Cannot find nearest dwelling!");

			return sptr(Goals::GetObj(nearest->id.getNum()));
		}
		else
			return sptr(Goals::Explore());
	}
	else
	{
		return sptr(Goals::Explore());
	}
	//TODO: exchange troops between heroes
}

TSubgoal Invalid::whatToDoToAchieve()
{
	return iAmElementar();
}

//TSubgoal AbstractGoal::whatToDoToAchieve()
//{
//	logAi->debug("Decomposing goal of type %s",name());
//	return sptr (Goals::Explore());
//}

TSubgoal AbstractGoal::goVisitOrLookFor(const CGObjectInstance * obj)
{
	if(obj)
		return sptr(Goals::GetObj(obj->id.getNum()));
	else
		return sptr(Goals::Explore());
}

TSubgoal AbstractGoal::lookForArtSmart(int aid)
{
	return sptr(Goals::Invalid());
}

bool AbstractGoal::invalid() const
{
	return goalType == INVALID;
}

void AbstractGoal::addTasks(Tasks::TaskList &target, TSubgoal subgoal, double priority) {
	auto tasks = subgoal->getTasks();

	for (Tasks::TaskPtr t : tasks) {
		t->addAncestorPriority(priority);
		target.push_back(t);
	}
}

void AbstractGoal::addTask(Tasks::TaskList &target, const Tasks::CTask &task, double priority) {
	auto taskPtr = Tasks::sptr(task);
	if (taskPtr->canExecute()) {
		taskPtr->addAncestorPriority(priority);
		target.push_back(taskPtr);
	}
}

void AbstractGoal::sortByPriority(Tasks::TaskList &tasks) {
	std::sort(tasks.begin(), tasks.end(), [](Tasks::TaskPtr t1, Tasks::TaskPtr t2) -> bool {
		return t1->getPriority() > t2->getPriority();
	});
}
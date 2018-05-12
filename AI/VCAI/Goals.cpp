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
#include "Goals.h"
#include "VCAI.h"
#include "Fuzzy.h"
#include "SectorMap.h"
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"
#include "Tasks/VisitTile.h"
#include "Tasks/BuildStructure.h"
#include "Tasks/RecruitHero.h"

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

namespace Goals
{
template<>
void CGoal<Win>::accept(VCAI * ai)
{
	ai->tryRealize(static_cast<Win &>(*this));
}

template<>
void CGoal<Build>::accept(VCAI * ai)
{
	ai->tryRealize(static_cast<Build &>(*this));
}
template<>
float CGoal<Win>::accept(FuzzyHelper * f)
{
	return f->evaluate(static_cast<Win &>(*this));
}

template<>
float CGoal<Build>::accept(FuzzyHelper * f)
{
	return f->evaluate(static_cast<Build &>(*this));
}
}

//TSubgoal AbstractGoal::whatToDoToAchieve()
//{
//	logAi->debug("Decomposing goal of type %s",name());
//	return sptr (Goals::Explore());
//}

TSubgoal Win::whatToDoToAchieve()
{
	auto toBool = [=](const EventCondition &)
	{
		// TODO: proper implementation
		// Right now even already fulfilled goals will be included into generated list
		// Proper check should test if event condition is already fulfilled
		// Easiest way to do this is to call CGameState::checkForVictory but this function should not be
		// used on client side or in AI code
		return false;
	};

	std::vector<EventCondition> goals;

	for(const TriggeredEvent & event : cb->getMapHeader()->triggeredEvents)
	{
		//TODO: try to eliminate human player(s) using loss conditions that have isHuman element

		if(event.effect.type == EventEffect::VICTORY)
		{
			boost::range::copy(event.trigger.getFulfillmentCandidates(toBool), std::back_inserter(goals));
		}
	}

	//TODO: instead of returning first encountered goal AI should generate list of possible subgoals
	for(const EventCondition & goal : goals)
	{
		switch(goal.condition)
		{
		case EventCondition::HAVE_ARTIFACT:
			return sptr(Goals::GetArtOfType(goal.objectType));
		case EventCondition::DESTROY:
		{
			if(goal.object)
			{
				auto obj = cb->getObj(goal.object->id);
				if(obj)
					if(obj->getOwner() == ai->playerID) //we can't capture our own object
						return sptr(Goals::Conquer());


				return sptr(Goals::GetObj(goal.object->id.getNum()));
			}
			else
			{
				// TODO: destroy all objects of type goal.objectType
				// This situation represents "kill all creatures" condition from H3
				break;
			}
		}
		case EventCondition::HAVE_BUILDING:
		{
			// TODO build other buildings apart from Grail
			// goal.objectType = buidingID to build
			// goal.object = optional, town in which building should be built
			// Represents "Improve town" condition from H3 (but unlike H3 it consists from 2 separate conditions)

			if(goal.objectType == BuildingID::GRAIL)
			{
				if(auto h = ai->getHeroWithGrail())
				{
					//hero is in a town that can host Grail
					if(h->visitedTown && !vstd::contains(h->visitedTown->forbiddenBuildings, BuildingID::GRAIL))
					{
						const CGTownInstance * t = h->visitedTown;
						return sptr(Goals::BuildThis(BuildingID::GRAIL, t));
					}
					else
					{
						auto towns = cb->getTownsInfo();
						towns.erase(boost::remove_if(towns,
									     [](const CGTownInstance * t) -> bool
							{
								return vstd::contains(t->forbiddenBuildings, BuildingID::GRAIL);
							}),
							    towns.end());
						boost::sort(towns, CDistanceSorter(h.get()));
						if(towns.size())
						{
							return sptr(Goals::VisitTile(towns.front()->visitablePos()).sethero(h));
						}
					}
				}
				double ratio = 0;
				// maybe make this check a bit more complex? For example:
				// 0.75 -> dig randomly within 3 tiles radius
				// 0.85 -> radius now 2 tiles
				// 0.95 -> 1 tile radius, position is fully known
				// AFAIK H3 AI does something like this
				int3 grailPos = cb->getGrailPos(&ratio);
				if(ratio > 0.99)
				{
					return sptr(Goals::DigAtTile(grailPos));
				} //TODO: use FIND_OBJ
				else if(const CGObjectInstance * obj = ai->getUnvisitedObj(objWithID<Obj::OBELISK>)) //there are unvisited Obelisks
					return sptr(Goals::GetObj(obj->id.getNum()));
				else
					return sptr(Goals::Explore());
			}
			break;
		}
		case EventCondition::CONTROL:
		{
			if(goal.object)
			{
				return sptr(Goals::GetObj(goal.object->id.getNum()));
			}
			else
			{
				//TODO: control all objects of type "goal.objectType"
				// Represents H3 condition "Flag all mines"
				break;
			}
		}

		case EventCondition::HAVE_RESOURCES:
			//TODO mines? piles? marketplace?
			//save?
			return sptr(Goals::BuyResources(static_cast<Res::ERes>(goal.objectType), goal.value));
		case EventCondition::HAVE_CREATURES:
			return sptr(Goals::GatherTroops(goal.objectType, goal.value));
		case EventCondition::TRANSPORT:
		{
			//TODO. merge with bring Grail to town? So AI will first dig grail, then transport it using this goal and builds it
			// Represents "transport artifact" condition:
			// goal.objectType = type of artifact
			// goal.object = destination-town where artifact should be transported
			break;
		}
		case EventCondition::STANDARD_WIN:
			return sptr(Goals::Conquer());

		// Conditions that likely don't need any implementation
		case EventCondition::DAYS_PASSED:
			break; // goal.value = number of days for condition to trigger
		case EventCondition::DAYS_WITHOUT_TOWN:
			break; // goal.value = number of days to trigger this
		case EventCondition::IS_HUMAN:
			break; // Should be only used in calculation of candidates (see toBool lambda)
		case EventCondition::CONST_VALUE:
			break;

		case EventCondition::HAVE_0:
		case EventCondition::HAVE_BUILDING_0:
		case EventCondition::DESTROY_0:
			//TODO: support new condition format
			return sptr(Goals::Conquer());
		default:
			assert(0);
		}
	}
	return sptr(Goals::Invalid());
}

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
			if(vstd::contains(ai->reservedObjs, topObj) && !vstd::contains(ai->reservedHeroesMap[h], topObj))
			{
				throw goalFulfilledException(sptr(Goals::ClearWayTo(tile, h)));
				continue; //do not capure object reserved by other hero
			}

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
	if(ai->canRecruitAnyHero())
		ret.push_back(sptr(Goals::RecruitHero()));

	if(ret.empty())
	{
		logAi->warn("There is no known way to clear the way to tile %s", tile.toString());
		throw goalFulfilledException(sptr(Goals::ClearWayTo(tile))); //make sure asigned hero gets unlocked
	}

	return ret;
}

std::string CaptureObjects::toString() const {
	return "Capture objects";
}

Tasks::TaskList CaptureObjects::getTasks() {
	Tasks::TaskList tasks;

	if (!hero->movement) {
		return tasks;
	}

	auto sm = ai->getCachedSectorMap(hero);
	auto needArmy = false;
	auto pathsInfo = cb->getPathsInfo(hero.get());
	
	auto captureObjects = [&](std::vector<const CGObjectInstance*> objs) -> bool {
		if (objs.empty()) {
			return false;
		}

		boost::sort(objs, CDistanceSorter(hero.get()));

		for (auto objToVisit : objs) {
			const int3 pos = objToVisit->visitablePos();
			const int3 targetPos = sm->firstTileToGet(hero, pos);

			if (!this->shouldVisitObject(objToVisit, hero, *sm)) {
				continue;
			}

			if (targetPos.x == -1) {
				return false;
			}

			auto pathInfo = pathsInfo->getPathInfo(targetPos);
			double priority = 0;

			if (pathInfo->turns == 0) {
				priority = 1 - (hero->movement - pathInfo->moveRemains) / (double)hero->maxMovePoints(true);
			}

			if (isSafeToVisit(hero, targetPos)) {
				//TODO: check hero is the nearest one
				addTask(tasks, Tasks::VisitTile(targetPos, hero), priority);

				return true;
			}
			else {
				needArmy = true;
			}
		}

		return false;
	};

	if (captureObjects(objectsToCapture)) {
		return tasks;
	}

	if (captureObjects(sm->getNearbyObjs(hero, 1))) {
		return tasks;
	}

	captureObjects(std::vector<const CGObjectInstance*>(ai->visitableObjs.begin(), ai->visitableObjs.end()));

	if (needArmy) {
		addTasks(tasks, sptr(GatherArmy().sethero(hero)));
	}

	return tasks;
}

bool CaptureObjects::shouldVisitObject(ObjectIdRef obj, HeroPtr hero, SectorMap& sm) {
	const CGObjectInstance* objInstance = obj;

	if (!objInstance || objectTypes.size() && !vstd::contains(objectTypes, objInstance->ID.num)) {
		return false;
	}

	const int3 pos = objInstance->visitablePos();
	const int3 targetPos = sm.firstTileToGet(hero, pos);

	if (!targetPos.valid()
		|| vstd::contains(ai->alreadyVisited, objInstance)) {
		return false;
	}

	if (objInstance->wasVisited(hero.get())) {
		return false;
	}

	auto playerRelations = cb->getPlayerRelations(ai->playerID, objInstance->tempOwner);
	if (playerRelations != PlayerRelations::ENEMIES && !isWeeklyRevisitable(objInstance)) {
		return false;
	}

	if (!shouldVisit(hero, objInstance)) {
		return false;
	}
	
	if(!ai->isAccessibleForHero(targetPos, hero)) 
	{
		return false;
	}

	//it may be hero visiting this obj
	//we don't try visiting object on which allied or owned hero stands
	// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
	const CGObjectInstance * topObj = cb->getTopObj(obj->visitablePos()); 

	if (topObj->ID == Obj::HERO && cb->getPlayerRelations(hero->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
		return false;
	else
		return true; //all of the following is met
}

std::string Explore::completeMessage() const
{
	return "Hero " + hero.get()->name + " completed exploration";
}

TSubgoal Explore::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());
	if(hero) //use best step for this hero
	{
		return ret;
	}
	else
	{
		if(ret->hero.get(true))
			return sptr(sethero(ret->hero.h).setisAbstract(true)); //choose this hero and then continue with him
		else
			return ret; //other solutions, like buying hero from tavern
	}
}
Tasks::TaskList Explore::getTasks() {
	Tasks::TaskList tasks = Tasks::TaskList();

	if (!hero->movement) {
		return tasks;
	}

	addTasks(tasks, sptr(CaptureObjects(this->getExplorationHelperObjects()).sethero(hero)), 0.8);

	int3 t = whereToExplore(hero);
	if (t.valid())
	{
		addTask(tasks, Tasks::VisitTile(t, hero), 0.5);
	}
	else
	{
		t = ai->explorationDesperate(hero);

		if (t.valid()) //don't waste time if we are completely blocked
			addTasks(tasks, sptr(Goals::ClearWayTo(t, hero)), 0); // TODO: Implement ClearWayTo.getTasks
	}

	return tasks;
}

TGoalVec Explore::getAllPossibleSubgoals()
{
	TGoalVec ret;
	std::vector<const CGHeroInstance *> heroes;

	if(hero)
	{
		heroes.push_back(hero.h);
	}
	else
	{
		//heroes = ai->getUnblockedHeroes();
		vstd::erase_if(heroes, [](const HeroPtr h)
		{
			if(ai->getGoal(h)->goalType == Goals::EXPLORE) //do not reassign hero who is already explorer
				return true;

			if(!ai->isAbleToExplore(h))
				return true;

			return !h->movement; //saves time, immobile heroes are useless anyway
		});
	}

	//try to use buildings that uncover map
	std::vector<const CGObjectInstance *> objs = getExplorationHelperObjects();

	for(auto h : heroes)
	{
		auto sm = ai->getCachedSectorMap(h);

		for(auto obj : objs) //double loop, performance risk?
		{
			auto t = sm->firstTileToGet(h, obj->visitablePos()); //we assume that no more than one tile on the way is guarded
			if(ai->isTileNotReserved(h, t))
				ret.push_back(sptr(Goals::ClearWayTo(obj->visitablePos(), h).setisAbstract(true)));
		}

		int3 t = whereToExplore(h);
		if(t.valid())
		{
			ret.push_back(sptr(Goals::VisitTile(t).sethero(h)));
		}
		else
		{
			ai->markHeroUnableToExplore(h); //there is no freely accessible tile, do not poll this hero anymore
			//possible issues when gathering army to break

			if(hero.h == h || (!hero && h == ai->primaryHero().h)) //check this only ONCE, high cost
			{
				t = ai->explorationDesperate(h);
				if(t.valid()) //don't waste time if we are completely blocked
					ret.push_back(sptr(Goals::ClearWayTo(t, h).setisAbstract(true)));
			}
		}
	}
	//we either don't have hero yet or none of heroes can explore
	/*if((!hero || ret.empty()) && ai->canRecruitAnyHero())
		ret.push_back(sptr(Goals::RecruitHero()));*/

	if(ret.empty())
	{
		throw goalFulfilledException(sptr(Goals::Explore().sethero(hero)));
	}
	//throw cannotFulfillGoalException("Cannot explore - no possible ways found!");

	return ret;
}

std::vector<const CGObjectInstance *> Explore::getExplorationHelperObjects() {
	//try to use buildings that uncover map
	std::vector<const CGObjectInstance *> objs;
	for (auto obj : ai->visitableObjs)
	{
		if (!vstd::contains(ai->alreadyVisited, obj))
		{
			switch (obj->ID.num)
			{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			case Obj::CARTOGRAPHER:
				objs.push_back(obj);
				break;
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				assert(ai->knownTeleportChannels.find(tObj->channel) != ai->knownTeleportChannels.end());
				if (TeleportChannel::IMPASSABLE != ai->knownTeleportChannels[tObj->channel]->passability)
					objs.push_back(obj);
				break;
			}
		}
		else
		{
			switch (obj->ID.num)
			{
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				if (TeleportChannel::IMPASSABLE == ai->knownTeleportChannels[tObj->channel]->passability)
					break;
				for (auto exit : ai->knownTeleportChannels[tObj->channel]->exits)
				{
					if (!cb->getObj(exit))
					{ // Always attempt to visit two-way teleports if one of channel exits is not visible
						objs.push_back(obj);
						break;
					}
				}
				break;
			}
		}
	}

	return objs;
}

bool Explore::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType == Goals::EXPLORE)
	{
		if(goal->hero)
			return hero == goal->hero;
		else
			return true; //cancel ALL exploration
	}
	return false;
}


TSubgoal RecruitHero::whatToDoToAchieve()
{
	const CGTownInstance * t = ai->findTownWithTavern();
	if(!t)
		return sptr(Goals::BuildThis(BuildingID::TAVERN));

	if(cb->getResourceAmount(Res::GOLD) < GameConstants::HERO_GOLD_COST)
		return sptr(Goals::BuyResources(Res::GOLD, GameConstants::HERO_GOLD_COST));

	return iAmElementar();
}

Tasks::TaskList RecruitHero::getTasks() {
	Tasks::TaskList tasks;
	auto heroes = cb->getHeroesInfo();
	auto towns = cb->getTownsInfo();

	if (cb->getResourceAmount(Res::GOLD) > GameConstants::HERO_GOLD_COST * 3
		&& heroes.size() < towns.size() + 2) {
		tasks.push_back(Tasks::sptr(Tasks::RecruitHero()));
	}

	return tasks;
}

std::string VisitTile::completeMessage() const
{
	return "Hero " + hero.get()->name + " visited tile " + tile.toString();
}

TSubgoal VisitTile::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());

	if(ret->hero)
	{
		if(isSafeToVisit(ret->hero, tile) && ai->isAccessibleForHero(tile, ret->hero))
		{
			if(cb->getTile(tile)->topVisitableId().num == Obj::TOWN) //if target is town, fuzzy system will use additional "estimatedReward" variable to increase priority a bit
				ret->objid = Obj::TOWN; //TODO: move to getObj eventually and add appropiate logic there
			ret->setisElementar(true);
			return ret;
		}
		else
		{
			return sptr(Goals::GatherArmy(evaluateDanger(tile, *ret->hero) * SAFE_ATTACK_CONSTANT)
				    .sethero(ret->hero).setisAbstract(true));
		}
	}
	return ret;
}

TGoalVec VisitTile::getAllPossibleSubgoals()
{
	assert(cb->isInTheMap(tile));

	TGoalVec ret;
	if(!cb->isVisible(tile))
		ret.push_back(sptr(Goals::Explore())); //what sense does it make?
	else
	{
		std::vector<const CGHeroInstance *> heroes;
		if(hero)
			heroes.push_back(hero.h); //use assigned hero if any
		else
			heroes = cb->getHeroesInfo(); //use most convenient hero

		for(auto h : heroes)
		{
			if(ai->isAccessibleForHero(tile, h))
				ret.push_back(sptr(Goals::VisitTile(tile).sethero(h)));
		}
		if(ai->canRecruitAnyHero())
			ret.push_back(sptr(Goals::RecruitHero()));
	}
	if(ret.empty())
	{
		auto obj = vstd::frontOrNull(cb->getVisitableObjs(tile));
		if(obj && obj->ID == Obj::HERO && obj->tempOwner == ai->playerID) //our own hero stands on that tile
		{
			if(hero.get(true) && hero->id == obj->id) //if it's assigned hero, visit tile. If it's different hero, we can't visit tile now
				ret.push_back(sptr(Goals::VisitTile(tile).sethero(dynamic_cast<const CGHeroInstance *>(obj)).setisElementar(true)));
			else
				throw cannotFulfillGoalException("Tile is already occupied by another hero "); //FIXME: we should give up this tile earlier
		}
		else
			ret.push_back(sptr(Goals::ClearWayTo(tile)));
	}

	//important - at least one sub-goal must handle case which is impossible to fulfill (unreachable tile)
	return ret;
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

TSubgoal BuyResources::whatToDoToAchieve()
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

Tasks::TaskList Conquer::getTasks() {
	auto tasks = Tasks::TaskList();
	auto heroes = cb->getHeroesInfo();
	
	// lets process heroes according their army strength in descending order
	std::sort(heroes.rbegin(), heroes.rend(), isLevelHigher);
	auto nextHero = vstd::tryFindIf(heroes, [](const CGHeroInstance* h) -> bool { return h->movement > 0; });

	addTasks(tasks, sptr(Build()), 0.9);
	addTasks(tasks, sptr(RecruitHero()));
	addTasks(tasks, sptr(GatherArmy()), 0.9); // no hero - just pickup existing army, no buy

	if (nextHero) {
		auto heroPtr = HeroPtr(nextHero.get());
		auto heroTasks = Tasks::TaskList();

		addTasks(heroTasks, sptr(CaptureObjects().ofType(Obj::TOWN).sethero(heroPtr)), 1);
		addTasks(heroTasks, sptr(CaptureObjects().ofType(Obj::HERO).sethero(heroPtr)), 0.95);
		addTasks(heroTasks, sptr(CaptureObjects().ofType(Obj::MINE).sethero(heroPtr)), 0.7);
		addTasks(heroTasks, sptr(CaptureObjects().sethero(HeroPtr(heroPtr))), 0.5);

		auto strongestHero = vstd::maxElementByFun(heroes, [](const CGHeroInstance* h) -> bool { return h->getArmyStrength(); });

		if (cb->getDate(Date::MONTH) > 1 && nextHero.get() == strongestHero[0]) {
			addTasks(heroTasks, sptr(Explore().sethero(HeroPtr(heroPtr))), 0.7);
		}
		else {
			addTasks(heroTasks, sptr(Explore().sethero(HeroPtr(heroPtr))), 0.5);
		}

		if (heroTasks.size()) {
			sortByPriority(heroTasks);
			tasks.push_back(heroTasks.front());
		}
	}

	sortByPriority(tasks);

	return tasks;
}
TSubgoal Conquer::whatToDoToAchieve()
{
	return sptr(Invalid());
}
TGoalVec Conquer::getAllPossibleSubgoals()
{
	TGoalVec ret;

	auto conquerable = [](const CGObjectInstance * obj) -> bool
	{
		if(cb->getPlayerRelations(ai->playerID, obj->tempOwner) == PlayerRelations::ENEMIES)
		{
			switch(obj->ID.num)
			{
			case Obj::TOWN:
			case Obj::HERO:
			case Obj::CREATURE_GENERATOR1:
			case Obj::MINE: //TODO: check ai->knownSubterraneanGates
				return true;
			}
		}
		return false;
	};

	std::vector<const CGObjectInstance *> objs;
	for(auto obj : ai->visitableObjs)
	{
		if(conquerable(obj))
			objs.push_back(obj);
	}

	auto heroes = cb->getHeroesInfo();

	for(auto h : heroes)
	{
		auto sm = ai->getCachedSectorMap(h);
		std::vector<const CGObjectInstance *> ourObjs(objs); //copy common objects

		for(auto obj : ai->reservedHeroesMap[h]) //add objects reserved by this hero
		{
			if(conquerable(obj))
				ourObjs.push_back(obj);
		}
		for(auto obj : ourObjs)
		{
			int3 dest = obj->visitablePos();
			auto t = sm->firstTileToGet(h, dest); //we assume that no more than one tile on the way is guarded
			if(t.valid()) //we know any path at all
			{
				if(ai->isTileNotReserved(h, t)) //no other hero wants to conquer that tile
				{
					if(isSafeToVisit(h, dest))
					{
						if(dest != t) //there is something blocking our way
							ret.push_back(sptr(Goals::ClearWayTo(dest, h).setisAbstract(true)));
						else
						{
							if(obj->ID.num == Obj::HERO) //enemy hero may move to other position
							{
								ret.push_back(sptr(Goals::VisitHero(obj->id.getNum()).sethero(h).setisAbstract(true)));
							}
							else //just visit that tile
							{
								if(obj->ID.num == Obj::TOWN)
									//if target is town, fuzzy system will use additional "estimatedReward" variable to increase priority a bit
									ret.push_back(sptr(Goals::VisitTile(dest).sethero(h).setobjid(obj->ID.num).setisAbstract(true))); //TODO: change to getObj eventually and and move appropiate logic there
								else
									ret.push_back(sptr(Goals::VisitTile(dest).sethero(h).setisAbstract(true)));
							}
						}
					}
					else //we need to get army in order to conquer that place
						ret.push_back(sptr(Goals::GatherArmy(evaluateDanger(dest, h) * SAFE_ATTACK_CONSTANT).sethero(h).setisAbstract(true)));
				}
			}
		}

		ret.push_back(sptr(Goals::GatherArmy(1).sethero(h).setisAbstract(true)));
	}


	auto townCount = cb->getTownsInfo().size();

	if(townCount + 2 > heroes.size())
	if(!objs.empty() && ai->canRecruitAnyHero()) //probably no point to recruit hero if we see no objects to capture
		ret.push_back(sptr(Goals::RecruitHero()));

	ret.push_back(sptr(Goals::Explore())); //we need to find an enemy

	return ret;
}

namespace Goals {
	class BuildingInfo {
	public:
		BuildingID id;
		TResources buildCost;
		int creatureScore;
		int creatureGrows;
		TResources creatureCost;
		TResources dailyIncome;
		CBuilding::TRequired requirements;
		bool exists = false;
		bool canBuild = false;

		BuildingInfo() {
			id = BuildingID::NONE;
			creatureGrows = 0;
			creatureScore = 0;
		}

		BuildingInfo(const CBuilding* building, const CCreature* creature) {
			id = building->bid;
			buildCost = building->resources;
			dailyIncome = building->produce;
			requirements = building->requirements;
			exists = false;

			if (creature) {
				bool isShooting = creature->isShooting();
				int demage = (creature->getMinDamage(isShooting) + creature->getMinDamage(isShooting)) / 2;

				creatureGrows = creature->growth;
				creatureScore = demage;
				creatureCost = creature->cost;
			}
			else {
				creatureScore = 0;
				creatureGrows = 0;
				creatureCost = TResources();
			}
		}

		std::string toString() {
			return std::to_string(id) + ", cost: " + buildCost.toString()
				+ ", score: " + std::to_string(creatureGrows) + " x " + std::to_string(creatureScore)
				+ " x " + creatureCost.toString()
				+ ", daily: " + dailyIncome.toString();
		}

		double costPerScore() {
			if (creatureScore == 0) return 0;

			double gold = creatureCost[Res::ERes::GOLD];

			return (double)creatureScore / (double)gold;
		}
	};

	class TownDevelopmentInfo {
	public:
		const CGTownInstance* town;
		BuildingID nextDwelling;
		BuildingID nextGoldSource;
		int armyScore;
		int economicsScore;

		TownDevelopmentInfo(const CGTownInstance* town) {
			this->town = town;
			this->armyScore = 0;
			this->economicsScore = 0;
		}
	};

	BuildingInfo Build::getBuildingOrPrerequisite(
		const CGTownInstance* town,
		BuildingID toBuild,
		TResources &requiredResourcesAccumulator,
		TResources &totalDevelopmentCostAccumulator,
		bool excludeDwellingDependencies)
	{
		BuildingID building = toBuild;
		auto townInfo = town->town;

		const CBuilding * buildPtr = townInfo->buildings.at(building);
		const CCreature* creature = NULL;

		if (BuildingID::DWELL_FIRST <= toBuild && toBuild <= BuildingID::DWELL_UP_LAST) {
			int level = toBuild - BuildingID::DWELL_FIRST;
			auto creatureID = townInfo->creatures.at(level % GameConstants::CREATURES_PER_TOWN).at(level / GameConstants::CREATURES_PER_TOWN);
			creature = creatureID.toCreature();
		}

		while (buildPtr) {
			auto info = BuildingInfo(buildPtr, creature);

			logAi->trace("checking %s", buildPtr->Name());
			logAi->trace("buildInfo %s", info.toString());

			buildPtr = NULL;

			if (!town->hasBuilt(building)) {
				auto canBuild = cb->canBuildStructure(town, building);

				totalDevelopmentCostAccumulator += info.buildCost;

				if (canBuild == EBuildingState::ALLOWED) {
					info.canBuild = true;

					return info;
				}
				else if (canBuild == EBuildingState::NO_RESOURCES) {
					logAi->trace("cant build. Not enough resources. Need %s", info.buildCost.toString());
					requiredResourcesAccumulator += info.buildCost;
				}
				else if (canBuild == EBuildingState::PREREQUIRES) {
					auto buildExpression = town->genBuildingRequirements(building, false);
					auto alreadyBuiltOrOtherDwelling = [&](const BuildingID & id) -> bool {
						return BuildingID::DWELL_FIRST <= id && id <= BuildingID::DWELL_LAST || town->hasBuilt(id);
					};

					auto missingBuildings = buildExpression.getFulfillmentCandidates(alreadyBuiltOrOtherDwelling);

					if (missingBuildings.empty()) {
						logAi->trace("cant build. Need other dwelling");
					}
					else {
						building = missingBuildings[0];
						buildPtr = townInfo->buildings.at(building);
						creature = NULL;

						logAi->trace("cant build. Need %s", buildPtr->Name());
					}
				}
			}
			else {
				logAi->trace("exists");
				info.exists = true;

				return info;
			}
		}

		return BuildingInfo();
	}

	Tasks::TaskList Build::getTasks()
	{
		Tasks::TaskList tasks = Tasks::TaskList();
		TResources availableResources = cb->getResourceAmount();

		logAi->trace("Searching tasks for BUILD");
		logAi->trace("Resources amount: %s", availableResources.toString());

		auto objects = cb->getMyObjects();
		TResources dailyIncome = TResources();

		for (const CGObjectInstance* obj : objects) {
			const CGMine* mine = dynamic_cast<const CGMine*>(obj);

			if (mine) {
				dailyIncome[mine->producedResource] += mine->producedQuantity;
			}
		}

		auto towns = cb->getTownsInfo();
		TResources requiredResources = TResources();
		TResources totalDevelopmentCost = TResources();
		auto developmentInfos = std::vector<TownDevelopmentInfo>();

		TResources armyCost = TResources();

		for (const CGTownInstance* town : towns) {
			auto townInfo = town->town;
			auto creatures = townInfo->creatures;
			auto buildings = townInfo->getAllBuildings();

			auto developmentInfo = TownDevelopmentInfo(town);

			dailyIncome += town->dailyIncome();;

			std::map<BuildingID, BuildingID> parentMap;

			for (auto &pair : townInfo->buildings) {
				if (pair.second->upgrade != -1) {
					parentMap[pair.second->upgrade] = pair.first;
				}
			}

			std::vector<BuildingInfo> existingDwellings;
			std::vector<BuildingInfo> toBuild;
			TResources townDevelopmentCost;

			for (int level = 0; level < GameConstants::CREATURES_PER_TOWN; level++)
			{
				BuildingID building = BuildingID(BuildingID::DWELL_FIRST + level);

				if (!vstd::contains(buildings, building))
					continue; // no such building in town

				BuildingInfo info = getBuildingOrPrerequisite(town, building, requiredResources, townDevelopmentCost);

				if (info.exists) {
					existingDwellings.push_back(info);
					armyCost += info.creatureCost * info.creatureGrows;
				}
				else if (info.canBuild) {
					toBuild.push_back(info);
				}
			}
			
			std::sort(toBuild.begin(), toBuild.end(), [](BuildingInfo& b1, BuildingInfo& b2) -> bool {
				return b1.costPerScore() > b2.costPerScore();
			});

			if (!town->hasBuilt(BuildingID::TOWN_HALL) && cb->canBuildStructure(town, BuildingID::TOWN_HALL) == EBuildingState::ALLOWED) {
				auto buildingInfo = getBuildingOrPrerequisite(town, BuildingID::TOWN_HALL, requiredResources, TResources());

				if (buildingInfo.canBuild) {
					tasks.push_back(Tasks::sptr(Tasks::BuildStructure(BuildingID::TOWN_HALL, town)));
					continue;
				}
			}

			if (toBuild.size() > 0) {
				developmentInfo.nextDwelling = toBuild[0].id;
			}

			if (existingDwellings.size() >= 2 && cb->getDate(Date::DAY_OF_WEEK) > boost::date_time::Friday) {
				if (!town->hasBuilt(BuildingID::CITADEL)) {
					if (cb->canBuildStructure(town, BuildingID::CITADEL) == EBuildingState::ALLOWED) {
						developmentInfo.nextDwelling = BuildingID::CITADEL;
						logAi->trace("Building preferences Citadel");
					}
				}

				else if (existingDwellings.size() >= 3 && !town->hasBuilt(BuildingID::CASTLE)) {
					if (cb->canBuildStructure(town, BuildingID::CASTLE) == EBuildingState::ALLOWED) {
						developmentInfo.nextDwelling = BuildingID::CASTLE;
						logAi->trace("Building preferences CASTLE");
					}
				}
			}

			for (auto bi : toBuild) {
				logAi->trace("Building preferences %s", bi.toString());
			}

			if (!town->hasBuilt(BuildingID::CITY_HALL)) {
				auto buildingInfo = getBuildingOrPrerequisite(town, BuildingID::CITY_HALL, requiredResources, TResources());

				developmentInfo.nextGoldSource = buildingInfo.id;
			}
			else {
				if (cb->canBuildStructure(town, BuildingID::CAPITOL) == EBuildingState::ALLOWED) {
					developmentInfo.nextGoldSource = BuildingID::CAPITOL;
				}
				// todo resource silo and treasury
			}

			if (developmentInfo.nextDwelling == BuildingID::NONE) {
				if (developmentInfo.nextGoldSource == BuildingID::NONE) {
					continue;
				}

				developmentInfo.armyScore = 1000; // nothing to build for army here
			}
			else {
				const int resourceMultiplier = 200;
				developmentInfo.armyScore = townDevelopmentCost[Res::GOLD]
					+ townDevelopmentCost[Res::WOOD] * resourceMultiplier
					+ townDevelopmentCost[Res::ORE] * resourceMultiplier
					+ townDevelopmentCost[Res::GEMS] * 2 * resourceMultiplier
					+ townDevelopmentCost[Res::SULFUR] * 2 * resourceMultiplier
					+ townDevelopmentCost[Res::CRYSTAL] * 2 * resourceMultiplier
					+ townDevelopmentCost[Res::MERCURY] * 2 * resourceMultiplier;

				totalDevelopmentCost += townDevelopmentCost;
			}

			developmentInfos.push_back(developmentInfo);
		}

		logAi->trace("daily income: %s", dailyIncome.toString());
		logAi->trace("resources required to develop towns now: %s, total: %s",
			requiredResources.toString(),
			totalDevelopmentCost.toString());

		//TODO: we need to try enforce capturing resources of particular type if we need them.

		TResources weeklyIncome = dailyIncome * 7;
		std::vector<BuildingID> result;

		armyCost *= 3; // multiply a few times to have gold for town development

		std::sort(developmentInfos.begin(), developmentInfos.end(), [&](TownDevelopmentInfo &b1, TownDevelopmentInfo &b2) -> bool {
			return b1.armyScore < b2.armyScore;
		});

		int i = 0;

		if (weeklyIncome.canAfford(armyCost)) {
			for (; i < (developmentInfos.size() + 1) / 2; ++i) {
				auto toBuild = developmentInfos[i].nextDwelling;

				if (toBuild != BuildingID::NONE) {
					tasks.push_back(Tasks::sptr(Tasks::BuildStructure(toBuild, developmentInfos[i].town)));
				}
			}
		}

		for (; i < developmentInfos.size(); ++i) {
			auto toBuild = developmentInfos[i].nextGoldSource;

			if (toBuild != BuildingID::NONE) {
				tasks.push_back(Tasks::sptr(Tasks::BuildStructure(toBuild, developmentInfos[i].town)));
			}
		}

		return tasks;
	}
}

TSubgoal Build::whatToDoToAchieve()
{
	return iAmElementar();
}

TSubgoal Invalid::whatToDoToAchieve()
{
	return iAmElementar();
}

std::string GatherArmy::completeMessage() const
{
	return "Hero " + hero.get()->name + " gathered army of value " + boost::lexical_cast<std::string>(value);
}

TSubgoal GatherArmy::whatToDoToAchieve()
{
	//TODO: find hero if none set
	assert(hero.h);

	return fh->chooseSolution(getAllPossibleSubgoals()); //find dwelling. use current hero to prevent him from doing nothing.
}

Tasks::TaskList GatherArmy::getTasks() {
	Tasks::TaskList tasks;

	auto heroes = cb->getHeroesInfo();
	auto towns = cb->getTownsInfo();

	if (!this->hero) {
		if (heroes.empty()) {
			return tasks;
		}

		for (auto town : towns) {
			if (!town->getArmyStrength()) {
				continue;
			}

			int3 pos = town->visitablePos();
			std::vector<const CGHeroInstance*> copy = heroes;

			vstd::erase_if(copy, [town](const CGHeroInstance* h) -> bool {
				return howManyReinforcementsCanGet(h, town) < h->getArmyStrength() / 10;
			});

			if (copy.empty()) {
				continue;
			}
			
			auto carrier = nearestHero(copy, pos);
			
			addTask(tasks, Tasks::VisitTile(pos, carrier), 1);
		}

		return tasks;
	}


	auto targetHeroPosition = this->hero->visitablePos();

	for (HeroPtr hero : heroes) {
		auto isStronger = hero->getFightingStrength() > this->hero->getFightingStrength();
		auto isAccessible = ai->isAccessibleForHero(targetHeroPosition, hero, true);

		if (hero.h == this->hero.h
			|| isStronger
			|| !isAccessible
			|| !ai->canGetArmy(this->hero.get(), hero.get())) {
			continue;
		}

		addTask(tasks, Tasks::VisitTile(targetHeroPosition, hero), 1);

		break;
	}
	//TODO take town if it is closer than hero
	boost::sort(towns, CDistanceSorter(this->hero.get()));

	for (auto town : towns) {
		auto pos = town->visitablePos();
		auto pathInfo = cb->getPathsInfo(this->hero.get())->getPathInfo(pos);

		if (pathInfo->reachable()) {
			ai->buildArmyIn(town);
		}

		if (howManyReinforcementsCanGet(hero, town))
		{
			// TODO: invoke army pickup logic from above
			break;
		}
	}
	
	return tasks;
}

static const BuildingID unitsSource[] = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3,
	BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7};

TGoalVec GatherArmy::getAllPossibleSubgoals()
{
	//get all possible towns, heroes and dwellings we may use
	TGoalVec ret;

	//TODO: include evaluation of monsters gather in calculation
	for(auto t : cb->getTownsInfo())
	{
		logAi->trace("hero %s checking town %s for army", hero->name, t->name);

		auto pos = t->visitablePos();
		if(ai->isAccessibleForHero(pos, hero))
		{
			if(!t->visitingHero)
			{
				auto score = howManyReinforcementsCanGet(hero, t);

				if (score) {
					logAi->trace("hero %s can get %i reinforsments from town %s. Moving there", hero->name, score, t->name);
					//if(!vstd::contains(ai->townVisitsThisWeek[hero], t))
						ret.push_back(sptr(Goals::VisitTile(pos).sethero(hero)));
				}
			}
			/*auto bid = ai->canBuildAnyStructure(t, std::vector<BuildingID>(unitsSource, unitsSource + ARRAY_COUNT(unitsSource)), 8 - cb->getDate(Date::DAY_OF_WEEK));
			if(bid != BuildingID::NONE)
				ret.push_back(sptr(BuildThis(bid, t)));*/
		}
	}

	if (value > 1) {
		auto otherHeroes = cb->getHeroesInfo();
		auto heroDummy = hero;
		vstd::erase_if(otherHeroes, [heroDummy](const CGHeroInstance * h)
		{
			if (h == heroDummy.h)
				return true;
			else if (!ai->isAccessibleForHero(heroDummy->visitablePos(), h, true))
				return true;
			else if (!ai->canGetArmy(heroDummy.h, h))
				return true;
			else if (ai->getGoal(h)->goalType == Goals::GATHER_ARMY)
				return true;
			else
				return false;
		});
		for (auto h : otherHeroes)
		{
			// Go to the other hero if we are faster
			ret.push_back(sptr(Goals::VisitHero(h->id.getNum()).setisAbstract(true).sethero(hero)));
			// Let the other hero come to us
			ret.push_back(sptr(Goals::VisitHero(hero->id.getNum()).setisAbstract(true).sethero(h)));
		}
	}

	std::vector<const CGObjectInstance *> objs;
	for(auto obj : ai->visitableObjs)
	{
		if(obj->ID == Obj::CREATURE_GENERATOR1)
		{
			auto relationToOwner = cb->getPlayerRelations(obj->getOwner(), ai->playerID);

			//Use flagged dwellings only when there are available creatures that we can afford
			if(relationToOwner == PlayerRelations::SAME_PLAYER)
			{
				auto dwelling = dynamic_cast<const CGDwelling *>(obj);
				for(auto & creLevel : dwelling->creatures)
				{
					if(creLevel.first)
					{
						for(auto & creatureID : creLevel.second)
						{
							auto creature = VLC->creh->creatures[creatureID];
							if(ai->freeResources().canAfford(creature->cost))
								objs.push_back(obj);
						}
					}
				}
			}
		}
	}
	for(auto h : cb->getHeroesInfo())
	{
		auto sm = ai->getCachedSectorMap(h);
		for(auto obj : objs)
		{
			//find safe dwelling
			auto pos = obj->visitablePos();
			if(ai->isGoodForVisit(obj, h, *sm))
				ret.push_back(sptr(Goals::VisitTile(pos).sethero(h)));
		}
	}

	if(ai->canRecruitAnyHero() && ai->freeResources()[Res::GOLD] > GameConstants::HERO_GOLD_COST * 3) //this is not stupid in early phase of game
	{
		if(auto t = ai->findTownWithTavern())
		{
			for(auto h : cb->getAvailableHeroes(t)) //we assume that all towns have same set of heroes
			{
				if(h && h->getTotalStrength() > 500) //do not buy heroes with single creatures for GatherArmy
				{
					ret.push_back(sptr(Goals::RecruitHero()));
					break;
				}
			}
		}
	}

	if(ret.empty())
	{
		if(hero == ai->primaryHero() || value >= 1.1f) // FIXME: check PR388
			ret.push_back(sptr(Goals::Explore()));
		else //workaround to break loop - seemingly there are no ways to explore left
			throw goalFulfilledException(sptr(Goals::GatherArmy(0).sethero(hero)));
	}

	return ret;
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

void AbstractGoal::accept(VCAI * ai)
{
	ai->tryRealize(*this);
}

void AbstractGoal::addTasks(Tasks::TaskList &target, TSubgoal subgoal, double priority) {
	auto tasks = subgoal->getTasks();

	for (Tasks::TaskPtr t : tasks) {
		t->addAncestorPriority(priority);
		target.push_back(t);
	}
}

void AbstractGoal::addTask(Tasks::TaskList &target, Tasks::CTask &task, double priority) {
	if (task.canExecute()) {
		task.addAncestorPriority(priority);
		target.push_back(Tasks::sptr(task));
	}
}

void AbstractGoal::sortByPriority(Tasks::TaskList &tasks) {
	std::sort(tasks.begin(), tasks.end(), [](Tasks::TaskPtr t1, Tasks::TaskPtr t2) -> bool {
		return t1->getPriority() > t2->getPriority();
	});
}

template<typename T>
void CGoal<T>::accept(VCAI * ai)
{
	ai->tryRealize(static_cast<T &>(*this)); //casting enforces template instantiation
}

float AbstractGoal::accept(FuzzyHelper * f)
{
	return f->evaluate(*this);
}

template<typename T>
float CGoal<T>::accept(FuzzyHelper * f)
{
	return f->evaluate(static_cast<T &>(*this)); //casting enforces template instantiation
}

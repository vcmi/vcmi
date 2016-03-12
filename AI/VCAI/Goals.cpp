#include "StdInc.h"
#include "Goals.h"
#include "VCAI.h"
#include "Fuzzy.h"
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"

/*
 * Goals.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

std::string Goals::AbstractGoal::name() const //TODO: virtualize
{
	std::string desc;
	switch (goalType)
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
		case COLLECT_RES:
			desc = "COLLECT RESOURCE";
			break;
		case GATHER_TROOPS:
			desc = "GATHER TROOPS";
			break;
		case GET_OBJ:
		{
			auto obj = cb->getObjInstance(ObjectInstanceID(objid));
			if (obj)
				desc = "GET OBJ " + obj->getObjectName();
		}
		case FIND_OBJ:
			desc = "FIND OBJ " + boost::lexical_cast<std::string>(objid);
			break;
		case VISIT_HERO:
		{
			auto obj = cb->getObjInstance(ObjectInstanceID(objid));
			if (obj)
				desc = "VISIT HERO " + obj->getObjectName();
		}
			break;
		case GET_ART_TYPE:
			desc = "GET ARTIFACT OF TYPE " + VLC->arth->artifacts[aid]->Name();
			break;
		case ISSUE_COMMAND:
			return "ISSUE COMMAND (unsupported)";
		case VISIT_TILE:
			desc = "VISIT TILE " + tile();
			break;
		case CLEAR_WAY_TO:
			desc = "CLEAR WAY TO " + tile();
			break;
		case DIG_AT_TILE:
			desc = "DIG AT TILE " + tile();
			break;
		default:
			return boost::lexical_cast<std::string>(goalType);
	}
	if (hero.get(true)) //FIXME: used to crash when we lost hero and failed goal
		desc += " (" + hero->name + ")";
	return desc;
}

//TODO: virtualize if code gets complex?
bool Goals::AbstractGoal::operator== (AbstractGoal &g)
{
	if (g.goalType != goalType)
		return false;
	if (g.isElementar != isElementar) //elementar goals fulfill long term non-elementar goals (VisitTile)
		return false;

	switch (goalType)
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
		case COLLECT_RES:
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
	template <>
	void CGoal<Win>::accept (VCAI * ai)
	{
		ai->tryRealize(static_cast<Win&>(*this));
	}

	template <>
	void CGoal<Build>::accept (VCAI * ai)
	{
		ai->tryRealize(static_cast<Build&>(*this));
	}
	template <>
	float CGoal<Win>::accept (FuzzyHelper * f)
	{
		return f->evaluate(static_cast<Win&>(*this));
	}

	template <>
	float CGoal<Build>::accept (FuzzyHelper * f)
	{
		return f->evaluate(static_cast<Build&>(*this));
	}
}

//TSubgoal AbstractGoal::whatToDoToAchieve()
//{
//	logAi->debugStream() << boost::format("Decomposing goal of type %s") % name();
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

	for (const TriggeredEvent & event : cb->getMapHeader()->triggeredEvents)
	{
		//TODO: try to eliminate human player(s) using loss conditions that have isHuman element

		if (event.effect.type == EventEffect::VICTORY)
		{
			boost::range::copy(event.trigger.getFulfillmentCandidates(toBool), std::back_inserter(goals));
		}
	}

	//TODO: instead of returning first encountered goal AI should generate list of possible subgoals
	for (const EventCondition & goal : goals)
	{
		switch(goal.condition)
		{
		case EventCondition::HAVE_ARTIFACT:
			return sptr (Goals::GetArtOfType(goal.objectType));
		case EventCondition::DESTROY:
			{
				if (goal.object)
				{
					auto obj = cb->getObj (goal.object->id);
					if (obj)
						if (obj->getOwner() == ai->playerID) //we can't capture our own object
							return sptr(Goals::Conquer());


					return sptr (Goals::GetObj(goal.object->id.getNum()));
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

				if (goal.objectType == BuildingID::GRAIL)
				{
					if(auto h = ai->getHeroWithGrail())
					{
						//hero is in a town that can host Grail
						if(h->visitedTown && !vstd::contains(h->visitedTown->forbiddenBuildings, BuildingID::GRAIL))
						{
							const CGTownInstance *t = h->visitedTown;
							return sptr (Goals::BuildThis(BuildingID::GRAIL, t));
						}
						else
						{
							auto towns = cb->getTownsInfo();
							towns.erase(boost::remove_if(towns,
												[](const CGTownInstance *t) -> bool
												{
													return vstd::contains(t->forbiddenBuildings, BuildingID::GRAIL);
												}),
										towns.end());
							boost::sort(towns, CDistanceSorter(h.get()));
							if(towns.size())
							{
								return sptr (Goals::VisitTile(towns.front()->visitablePos()).sethero(h));
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
						return sptr (Goals::DigAtTile(grailPos));
					} //TODO: use FIND_OBJ
					else if(const CGObjectInstance * obj = ai->getUnvisitedObj(objWithID<Obj::OBELISK>)) //there are unvisited Obelisks
						return sptr (Goals::GetObj(obj->id.getNum()));
					else
						return sptr (Goals::Explore());
				}
				break;
			}
		case EventCondition::CONTROL:
			{
				if (goal.object)
				{
					return sptr (Goals::GetObj(goal.object->id.getNum()));
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
			return sptr (Goals::CollectRes(static_cast<Res::ERes>(goal.objectType), goal.value));
		case EventCondition::HAVE_CREATURES:
			return sptr (Goals::GatherTroops(goal.objectType, goal.value));
		case EventCondition::TRANSPORT:
			{
				//TODO. merge with bring Grail to town? So AI will first dig grail, then transport it using this goal and builds it
				// Represents "transport artifact" condition:
				// goal.objectType = type of artifact
				// goal.object = destination-town where artifact should be transported
				break;
			}
		case EventCondition::STANDARD_WIN:
			return sptr (Goals::Conquer());

		// Conditions that likely don't need any implementation
		case EventCondition::DAYS_PASSED:
			break; // goal.value = number of days for condition to trigger
		case EventCondition::DAYS_WITHOUT_TOWN:
			break; // goal.value = number of days to trigger this
		case EventCondition::IS_HUMAN:
			break; // Should be only used in calculation of candidates (see toBool lambda)
		case EventCondition::CONST_VALUE:
			break;
		default:
			assert(0);
		}
	}
	return sptr (Goals::Invalid());
}

TSubgoal FindObj::whatToDoToAchieve()
{
	const CGObjectInstance * o = nullptr;
	if (resID > -1) //specified
	{
		for(const CGObjectInstance *obj : ai->visitableObjs)
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
		for(const CGObjectInstance *obj : ai->visitableObjs)
		{
			if(obj->ID == objid)
			{
				o = obj;
				break; //TODO: consider multiple objects and choose best
			}
		}
	}
	if (o && ai->isAccessible(o->pos)) //we don't use isAccessibleForHero as we don't know which hero it is
		return sptr (Goals::GetObj(o->id.getNum()));
	else
		return sptr (Goals::Explore());
}

std::string GetObj::completeMessage() const
{
	return "hero " + hero.get()->name + " captured Object ID = " + boost::lexical_cast<std::string>(objid);
}

TSubgoal GetObj::whatToDoToAchieve()
{
	const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(objid));
	if(!obj)
		return sptr (Goals::Explore());
	if (obj->tempOwner == ai->playerID) //we can't capture our own object -> move to Win codition
		throw cannotFulfillGoalException("Cannot capture my own object " + obj->getObjectName());

	int3 pos = obj->visitablePos();
	if (hero)
	{
		if (ai->isAccessibleForHero(pos, hero))
			return sptr (Goals::VisitTile(pos).sethero(hero));
	}
	else
	{
		for (auto h : cb->getHeroesInfo())
		{
			if (ai->isAccessibleForHero(pos, h))
				return sptr(Goals::VisitTile(pos).sethero(h)); //we must visit object with same hero, if any
		}
	}
	return sptr (Goals::ClearWayTo(pos).sethero(hero));
}

bool GetObj::fulfillsMe (TSubgoal goal)
{
	if (goal->goalType == Goals::VISIT_TILE)
	{
		auto obj = cb->getObj(ObjectInstanceID(objid));
		if (obj && obj->visitablePos() == goal->tile) //object could be removed
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
		return sptr (Goals::Explore());
	int3 pos = obj->visitablePos();

	if (hero && ai->isAccessibleForHero(pos, hero, true) && isSafeToVisit(hero, pos)) //enemy heroes can get reinforcements
	{
		if (hero->pos == pos)
			logAi->errorStream() << "Hero " << hero.name << " tries to visit himself.";
		else
		{
			//can't use VISIT_TILE here as tile appears blocked by target hero
			//FIXME: elementar goal should not be abstract
			return sptr (Goals::VisitHero(objid).sethero(hero).settile(pos).setisElementar(true));
		}
	}
	return sptr (Goals::Invalid());
}

bool VisitHero::fulfillsMe (TSubgoal goal)
{
	if (goal->goalType != Goals::VISIT_TILE)
	{
		return false;
	}
	auto obj = cb->getObj(ObjectInstanceID(objid));
	if (!obj)
	{
		logAi->errorStream() << boost::format("Hero %s: VisitHero::fulfillsMe at %s: object %d not found")
			% hero.name % goal->tile % objid;
		return false;
	}
	return obj->visitablePos() == goal->tile;
}

TSubgoal GetArtOfType::whatToDoToAchieve()
{
	TSubgoal alternativeWay = CGoal::lookForArtSmart(aid); //TODO: use
	if(alternativeWay->invalid())
		return sptr (Goals::FindObj(Obj::ARTIFACT, aid));
	return sptr (Goals::Invalid());
}

TSubgoal ClearWayTo::whatToDoToAchieve()
{
	assert(cb->isInTheMap(tile)); //set tile
	if(!cb->isVisible(tile))
	{
		logAi->errorStream() << "Clear way should be used with visible tiles!";
		return sptr (Goals::Explore());
	}

	return (fh->chooseSolution(getAllPossibleSubgoals()));
}

TGoalVec ClearWayTo::getAllPossibleSubgoals()
{
	TGoalVec ret;

	std::vector<const CGHeroInstance *> heroes;

	if (hero)
		heroes.push_back(hero.h);
	else
	{
		heroes = cb->getHeroesInfo();
	}

	for (auto h : heroes)
	{
		//TODO: handle clearing way to allied heroes that are blocked
		//if ((hero && hero->visitablePos() == tile && hero == *h) || //we can't free the way ourselves
		//	h->visitablePos() == tile) //we are already on that tile! what does it mean?
		//	continue;

		//if our hero is trapped, make sure we request clearing the way from OUR perspective

		auto sm = ai->getCachedSectorMap(h);

		int3 tileToHit = sm->firstTileToGet(h, tile);
		if (!tileToHit.valid())
			continue;

		if (isBlockedBorderGate(tileToHit))
		{	//FIXME: this way we'll not visit gate and activate quest :?
			ret.push_back  (sptr (Goals::FindObj (Obj::KEYMASTER, cb->getTile(tileToHit)->visitableObjects.back()->subID)));
		}

		auto topObj = cb->getTopObj(tileToHit);
		if (topObj)
		{
			if (vstd::contains(ai->reservedObjs, topObj) && !vstd::contains(ai->reservedHeroesMap[h], topObj))
			{
				throw goalFulfilledException (sptr(Goals::ClearWayTo(tile, h)));
				continue; //do not capure object reserved by other hero
			}

			if (topObj->ID == Obj::HERO && cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
				if (topObj != hero.get(true)) //the hero we want to free
					logAi->errorStream() << boost::format("%s stands in the way of %s") % topObj->getObjectName()  % h->getObjectName();
			if (topObj->ID == Obj::QUEST_GUARD || topObj->ID == Obj::BORDERGUARD)
			{
				if (shouldVisit(h, topObj))
				{
					//do NOT use VISIT_TILE, as tile with quets guard can't be visited
					ret.push_back (sptr (Goals::GetObj(topObj->id.getNum()).sethero(h)));
					continue; //do not try to visit tile or gather army
				}
				else
				{
					//TODO: we should be able to return apriopriate quest here (VCAI::striveToQuest)
					logAi->debugStream() << "Quest guard blocks the way to " + tile();
					continue; //do not access quets guard if we can't complete the quest
				}
			}
		}
		if (isSafeToVisit(h, tileToHit)) //this makes sense only if tile is guarded, but there i no quest object
		{
			ret.push_back (sptr (Goals::VisitTile(tileToHit).sethero(h)));
		}
		else
		{
			ret.push_back (sptr (Goals::GatherArmy(evaluateDanger(tileToHit, h)*SAFE_ATTACK_CONSTANT).
				sethero(h).setisAbstract(true)));
		}
	}
	if (ai->canRecruitAnyHero())
		ret.push_back (sptr (Goals::RecruitHero()));

	if (ret.empty())
	{
		logAi->warnStream() << "There is no known way to clear the way to tile " + tile();
		throw goalFulfilledException (sptr(Goals::ClearWayTo(tile))); //make sure asigned hero gets unlocked
	}

	return ret;
}

std::string Explore::completeMessage() const
{
	return "Hero " + hero.get()->name + " completed exploration";
}

TSubgoal Explore::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());
	if (hero) //use best step for this hero
		return ret;
	else
	{
		if (ret->hero.get(true))
			return sptr (sethero(ret->hero.h).setisAbstract(true)); //choose this hero and then continue with him
		else
			return ret; //other solutions, like buying hero from tavern
	}
}

TGoalVec Explore::getAllPossibleSubgoals()
{
	TGoalVec ret;
	std::vector<const CGHeroInstance *> heroes;

	if (hero)
		heroes.push_back(hero.h);
	else
	{
		//heroes = ai->getUnblockedHeroes();
		heroes = cb->getHeroesInfo();
		vstd::erase_if(heroes, [](const HeroPtr h)
		{
			if (ai->getGoal(h)->goalType == Goals::EXPLORE) //do not reassign hero who is already explorer
				return true;

			if (!ai->isAbleToExplore(h))
				return true;

			return !h->movement; //saves time, immobile heroes are useless anyway
		});
	}

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
				objs.push_back (obj);
				break;
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				assert(ai->knownTeleportChannels.find(tObj->channel) != ai->knownTeleportChannels.end());
				if(TeleportChannel::IMPASSABLE != ai->knownTeleportChannels[tObj->channel]->passability)
					objs.push_back (obj);
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
				if(TeleportChannel::IMPASSABLE == ai->knownTeleportChannels[tObj->channel]->passability)
					break;
				for(auto exit : ai->knownTeleportChannels[tObj->channel]->exits)
				{
					if(!cb->getObj(exit))
					{ // Always attempt to visit two-way teleports if one of channel exits is not visible
						objs.push_back(obj);
						break;
					}
				}
				break;
			}
		}
	}

	for (auto h : heroes)
	{
		auto sm = ai->getCachedSectorMap(h);

		for (auto obj : objs) //double loop, performance risk?
		{
			auto t = sm->firstTileToGet(h, obj->visitablePos()); //we assume that no more than one tile on the way is guarded
			if (ai->isTileNotReserved(h, t))
				ret.push_back (sptr(Goals::ClearWayTo(obj->visitablePos(), h).setisAbstract(true)));
		}

		int3 t = whereToExplore(h);
		if (t.valid())
		{
			ret.push_back (sptr (Goals::VisitTile(t).sethero(h)));
		}
		else
		{
			ai->markHeroUnableToExplore (h); //there is no freely accessible tile, do not poll this hero anymore
			//possible issues when gathering army to break

			if (hero.h == h || (!hero && h == ai->primaryHero().h)) //check this only ONCE, high cost
			{
				t = ai->explorationDesperate(h);
				if (t.valid()) //don't waste time if we are completely blocked
					ret.push_back (sptr(Goals::ClearWayTo(t, h).setisAbstract(true)));
			}
		}
	}
	//we either don't have hero yet or none of heroes can explore
	if ((!hero || ret.empty()) && ai->canRecruitAnyHero())
		ret.push_back (sptr(Goals::RecruitHero()));

	if (ret.empty())
	{
		throw goalFulfilledException (sptr(Goals::Explore().sethero(hero)));
	}
	//throw cannotFulfillGoalException("Cannot explore - no possible ways found!");

	return ret;
}

bool Explore::fulfillsMe (TSubgoal goal)
{
	if (goal->goalType == Goals::EXPLORE)
	{
		if (goal->hero)
			return hero == goal->hero;
		else
			return true; //cancel ALL exploration
	}
	return false;
}


TSubgoal RecruitHero::whatToDoToAchieve()
{
	const CGTownInstance *t = ai->findTownWithTavern();
	if(!t)
		return sptr (Goals::BuildThis(BuildingID::TAVERN));

	if(cb->getResourceAmount(Res::GOLD) < GameConstants::HERO_GOLD_COST)
		return sptr (Goals::CollectRes(Res::GOLD, GameConstants::HERO_GOLD_COST));

	return iAmElementar();
}

std::string VisitTile::completeMessage() const
{
	return "Hero " + hero.get()->name + " visited tile " + tile();
}

TSubgoal VisitTile::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());

	if (ret->hero)
	{
		if (isSafeToVisit(ret->hero, tile) && ai->isAccessibleForHero(tile, ret->hero))
		{
			ret->setisElementar(true);
			return ret;
		}
		else
		{
			return sptr (Goals::GatherArmy(evaluateDanger(tile, *ret->hero) * SAFE_ATTACK_CONSTANT)
						.sethero(ret->hero).setisAbstract(true));
		}
	}
	return ret;
}

TGoalVec VisitTile::getAllPossibleSubgoals()
{
	assert(cb->isInTheMap(tile));

	TGoalVec ret;
	if (!cb->isVisible(tile))
		ret.push_back (sptr(Goals::Explore())); //what sense does it make?
	else
	{
		std::vector<const CGHeroInstance *> heroes;
		if (hero)
			heroes.push_back(hero.h); //use assigned hero if any
		else
			heroes = cb->getHeroesInfo(); //use most convenient hero

		for (auto h : heroes)
		{
			if (ai->isAccessibleForHero(tile, h))
				ret.push_back (sptr(Goals::VisitTile(tile).sethero(h)));
		}
		if (ai->canRecruitAnyHero())
			ret.push_back (sptr(Goals::RecruitHero()));
	}
	if(ret.empty())
	{
		auto obj = vstd::frontOrNull(cb->getVisitableObjs(tile));
		if(obj && obj->ID == Obj::HERO && obj->tempOwner == ai->playerID) //our own hero stands on that tile
		{
			if (hero.get(true) && hero->id == obj->id) //if it's assigned hero, visit tile. If it's different hero, we can't visit tile now
				ret.push_back(sptr(Goals::VisitTile(tile).sethero(dynamic_cast<const CGHeroInstance *>(obj)).setisElementar(true)));
			else
				throw cannotFulfillGoalException("Tile is already occupied by another hero "); //FIXME: we should give up this tile earlier
		}
		else
			ret.push_back (sptr(Goals::ClearWayTo(tile)));
	}

	//important - at least one sub-goal must handle case which is impossible to fulfill (unreachable tile)
	return ret;
}

TSubgoal DigAtTile::whatToDoToAchieve()
{
	const CGObjectInstance *firstObj = vstd::frontOrNull(cb->getVisitableObjs(tile));
	if(firstObj && firstObj->ID == Obj::HERO && firstObj->tempOwner == ai->playerID) //we have hero at dest
	{
		const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(firstObj);
		sethero(h).setisElementar(true);
		return sptr (*this);
	}

	return sptr (Goals::VisitTile(tile));
}

TSubgoal BuildThis::whatToDoToAchieve()
{
	//TODO check res
	//look for town
	//prerequisites?
	return iAmElementar();
}

TSubgoal CollectRes::whatToDoToAchieve()
{
	std::vector<const IMarket*> markets;

	std::vector<const CGObjectInstance*> visObjs;
	ai->retreiveVisitableObjs(visObjs, true);
	for(const CGObjectInstance *obj : visObjs)
	{
		if(const IMarket *m = IMarket::castFrom(obj, false))
		{
			if(obj->ID == Obj::TOWN && obj->tempOwner == ai->playerID && m->allowsTrade(EMarketMode::RESOURCE_RESOURCE))
				markets.push_back(m);
			else if(obj->ID == Obj::TRADING_POST) //TODO a moze po prostu test na pozwalanie handlu?
				markets.push_back(m);
		}
	}

	boost::sort(markets, [](const IMarket *m1, const IMarket *m2) -> bool
	{
		return m1->getMarketEfficiency() < m2->getMarketEfficiency();
	});

	markets.erase(boost::remove_if(markets, [](const IMarket *market) -> bool
	{
		return !(market->o->ID == Obj::TOWN && market->o->tempOwner == ai->playerID)
			&& !ai->isAccessible(market->o->visitablePos());
	}),markets.end());

	if(!markets.size())
	{
		for(const CGTownInstance *t : cb->getTownsInfo())
		{
			if(cb->canBuildStructure(t, BuildingID::MARKETPLACE) == EBuildingState::ALLOWED)
				return sptr (Goals::BuildThis(BuildingID::MARKETPLACE, t));
		}
	}
	else
	{
		const IMarket *m = markets.back();
		//attempt trade at back (best prices)
		int howManyCanWeBuy = 0;
		for(Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, 1))
		{
			if(i == resID) continue;
			int toGive = -1, toReceive = -1;
			m->getOffer(i, resID, toGive, toReceive, EMarketMode::RESOURCE_RESOURCE);
			assert(toGive > 0 && toReceive > 0);
			howManyCanWeBuy += toReceive * (cb->getResourceAmount(i) / toGive);
		}

		if(howManyCanWeBuy + cb->getResourceAmount(static_cast<Res::ERes>(resID)) >= value)
		{
			auto backObj = cb->getTopObj(m->o->visitablePos()); //it'll be a hero if we have one there; otherwise marketplace
			assert(backObj);
			if (backObj->tempOwner != ai->playerID)
			{
				return sptr (Goals::GetObj(m->o->id.getNum()));
			}
			else
			{
				return sptr (Goals::GetObj(m->o->id.getNum()).setisElementar(true));
			}
		}
	}
	return sptr (setisElementar(true)); //all the conditions for trade are met
}

TSubgoal GatherTroops::whatToDoToAchieve()
{
	std::vector<const CGDwelling *> dwellings;
	for(const CGTownInstance *t : cb->getTownsInfo())
	{
		auto creature = VLC->creh->creatures[objid];
		if (t->subID == creature->faction) //TODO: how to force AI to build unupgraded creatures? :O
		{
			auto creatures = vstd::tryAt(t->town->creatures, creature->level - 1);
			if(!creatures)
				continue;

			int upgradeNumber = vstd::find_pos(*creatures, creature->idNumber);
			if(upgradeNumber < 0)
				continue;

			BuildingID bid(BuildingID::DWELL_FIRST + creature->level - 1 + upgradeNumber * GameConstants::CREATURES_PER_TOWN);
			if (t->hasBuilt(bid)) //this assumes only creatures with dwellings are assigned to faction
			{
				dwellings.push_back(t);
			}
			else
			{
				return sptr (Goals::BuildThis(bid, t));
			}
		}
	}
	for (auto obj : ai->visitableObjs)
	{
		if (obj->ID != Obj::CREATURE_GENERATOR1) //TODO: what with other creature generators?
			continue;

		auto d = dynamic_cast<const CGDwelling *>(obj);
		for (auto creature : d->creatures)
		{
			if (creature.first) //there are more than 0 creatures avaliabe
			{
				for (auto type : creature.second)
				{
					if (type == objid  &&  ai->freeResources().canAfford(VLC->creh->creatures[type]->cost))
						dwellings.push_back(d);
				}
			}
		}
	}
	if (dwellings.size())
	{
		typedef std::map<const CGHeroInstance *, const CGDwelling *> TDwellMap;

		// sorted helper
		auto comparator = [](const TDwellMap::value_type & a, const TDwellMap::value_type & b) -> bool
		{
			const CGPathNode *ln = ai->myCb->getPathsInfo(a.first)->getPathInfo(a.second->visitablePos()),
			                 *rn = ai->myCb->getPathsInfo(b.first)->getPathInfo(b.second->visitablePos());

			if(ln->turns != rn->turns)
				return ln->turns < rn->turns;

			return (ln->moveRemains > rn->moveRemains);
		};

		// for all owned heroes generate map <hero -> nearest dwelling>
		TDwellMap nearestDwellings;
		for (const CGHeroInstance * hero : cb->getHeroesInfo(true))
		{
			nearestDwellings[hero] = *boost::range::min_element(dwellings, CDistanceSorter(hero));
		}

		// find hero who is nearest to a dwelling
		const CGDwelling * nearest = boost::range::min_element(nearestDwellings, comparator)->second;

		return sptr (Goals::GetObj(nearest->id.getNum()));
	}
	else
		return sptr (Goals::Explore());
	//TODO: exchange troops between heroes
}

TSubgoal Conquer::whatToDoToAchieve()
{
	return fh->chooseSolution (getAllPossibleSubgoals());
}
TGoalVec Conquer::getAllPossibleSubgoals()
{
	TGoalVec ret;

	auto conquerable = [](const CGObjectInstance * obj) -> bool
	{
		if (cb->getPlayerRelations(ai->playerID, obj->tempOwner) == PlayerRelations::ENEMIES)
		{
			switch (obj->ID.num)
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
	for (auto obj : ai->visitableObjs)
	{
		if (conquerable(obj))
			objs.push_back (obj);
	}

	for (auto h : cb->getHeroesInfo())
	{
		auto sm = ai->getCachedSectorMap(h);
		std::vector<const CGObjectInstance *> ourObjs(objs); //copy common objects

		for (auto obj : ai->reservedHeroesMap[h]) //add objects reserved by this hero
		{
			if (conquerable(obj))
				ourObjs.push_back(obj);
		}
		for (auto obj : ourObjs)
		{
			int3 dest = obj->visitablePos();
			auto t = sm->firstTileToGet(h, dest); //we assume that no more than one tile on the way is guarded
			if (t.valid()) //we know any path at all
			{
				if (ai->isTileNotReserved(h, t)) //no other hero wants to conquer that tile
				{
					if (isSafeToVisit(h, dest))
					{
						if (dest != t) //there is something blocking our way
							ret.push_back(sptr(Goals::ClearWayTo(dest, h).setisAbstract(true)));
						else
						{
							if (obj->ID.num == Obj::HERO) //enemy hero may move to other position
								ret.push_back(sptr(Goals::VisitHero(obj->id.getNum()).sethero(h).setisAbstract(true)));
							else //just visit that tile
								ret.push_back(sptr(Goals::VisitTile(dest).sethero(h).setisAbstract(true)));
						}
					}
					else //we need to get army in order to conquer that place
						ret.push_back(sptr(Goals::GatherArmy(evaluateDanger(dest, h) * SAFE_ATTACK_CONSTANT).sethero(h).setisAbstract(true)));
				}
			}
		}
	}
	if (!objs.empty() && ai->canRecruitAnyHero()) //probably no point to recruit hero if we see no objects to capture
		ret.push_back (sptr(Goals::RecruitHero()));

	if (ret.empty())
		ret.push_back (sptr(Goals::Explore())); //we need to find an enemy
	return ret;
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

	return fh->chooseSolution (getAllPossibleSubgoals()); //find dwelling. use current hero to prevent him from doing nothing.
}

static const BuildingID unitsSource[] = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3,
	BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7};

TGoalVec GatherArmy::getAllPossibleSubgoals()
{
	//get all possible towns, heroes and dwellings we may use
	TGoalVec ret;

	//TODO: include evaluation of monsters gather in calculation
	for (auto t : cb->getTownsInfo())
	{
		auto pos = t->visitablePos();
		if (ai->isAccessibleForHero(pos, hero))
		{
			if(!t->visitingHero && howManyReinforcementsCanGet(hero,t))
			{
				if (!vstd::contains (ai->townVisitsThisWeek[hero], t))
					ret.push_back (sptr (Goals::VisitTile(pos).sethero(hero)));
			}
			auto bid = ai->canBuildAnyStructure(t, std::vector<BuildingID>
							(unitsSource, unitsSource + ARRAY_COUNT(unitsSource)), 8 - cb->getDate(Date::DAY_OF_WEEK));
			if (bid != BuildingID::NONE)
				ret.push_back (sptr(BuildThis(bid, t)));
		}
	}

	auto otherHeroes = cb->getHeroesInfo();
	auto heroDummy = hero;
	vstd::erase_if(otherHeroes, [heroDummy](const CGHeroInstance * h)
	{
		return (h == heroDummy.h || !ai->isAccessibleForHero(heroDummy->visitablePos(), h, true)
			|| !ai->canGetArmy(heroDummy.h, h) || ai->getGoal(h)->goalType == Goals::GATHER_ARMY);
	});
	for (auto h : otherHeroes)
	{
		ret.push_back (sptr (Goals::VisitHero(h->id.getNum()).setisAbstract(true).sethero(hero)));
				//go to the other hero if we are faster
		ret.push_back (sptr (Goals::VisitHero(hero->id.getNum()).setisAbstract(true).sethero(h)));
				//let the other hero come to us
	}

	std::vector <const CGObjectInstance *> objs;
	for (auto obj : ai->visitableObjs)
	{
		if(obj->ID == Obj::CREATURE_GENERATOR1)
		{
			auto relationToOwner = cb->getPlayerRelations(obj->getOwner(), ai->playerID);

			//Use flagged dwellings only when there are available creatures that we can afford
			if(relationToOwner == PlayerRelations::SAME_PLAYER)
			{
				auto dwelling = dynamic_cast<const CGDwelling*>(obj);
				for(auto & creLevel : dwelling->creatures)
				{
					if(creLevel.first)
					{
						for(auto & creatureID : creLevel.second)
						{
							auto creature = VLC->creh->creatures[creatureID];
							if (ai->freeResources().canAfford(creature->cost))
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
		for (auto obj : objs)
		{ //find safe dwelling
			auto pos = obj->visitablePos();
			if (ai->isGoodForVisit(obj, h, *sm))
				ret.push_back (sptr (Goals::VisitTile(pos).sethero(h)));
		}
	}
	if (ai->canRecruitAnyHero()) //this is not stupid in early phase of game
		ret.push_back (sptr(Goals::RecruitHero()));

	if (ret.empty())
	{
		if (hero == ai->primaryHero() || value >= 1.1f)
			ret.push_back (sptr(Goals::Explore()));
		else //workaround to break loop - seemingly there are no ways to explore left
			throw goalFulfilledException (sptr(Goals::GatherArmy(0).sethero(hero)));
	}

	return ret;
}

//TSubgoal AbstractGoal::whatToDoToAchieve()
//{
//	logAi->debugStream() << boost::format("Decomposing goal of type %s") % name();
//	return sptr (Goals::Explore());
//}

TSubgoal AbstractGoal::goVisitOrLookFor(const CGObjectInstance *obj)
{
	if(obj)
		return sptr (Goals::GetObj(obj->id.getNum()));
	else
		return sptr (Goals::Explore());
}

TSubgoal AbstractGoal::lookForArtSmart(int aid)
{
	return sptr (Goals::Invalid());
}

bool AbstractGoal::invalid() const
{
	return goalType == INVALID;
}

void AbstractGoal::accept (VCAI * ai)
{
	ai->tryRealize(*this);
}


template<typename T>
void CGoal<T>::accept (VCAI * ai)
{
	ai->tryRealize(static_cast<T&>(*this)); //casting enforces template instantiation
}

float AbstractGoal::accept (FuzzyHelper * f)
{
	return f->evaluate(*this);
}

template<typename T>
float  CGoal<T>::accept (FuzzyHelper * f)
{
	return f->evaluate(static_cast<T&>(*this)); //casting enforces template instantiation
}

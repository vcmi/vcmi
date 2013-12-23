#include "StdInc.h"
#include "Goals.h"
#include "VCAI.h"
#include "Fuzzy.h"

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

using namespace vstd;
using namespace Goals;

TSubgoal Goals::sptr(const AbstractGoal & tmp)
{
	shared_ptr<AbstractGoal> ptr;
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
			desc = "GET OBJ " + cb->getObjInstance(ObjectInstanceID(objid))->getHoverText();
			break;
		case FIND_OBJ:
			desc = "FIND OBJ " + boost::lexical_cast<std::string>(objid);
			break;
		case VISIT_HERO:
			desc = "VISIT HERO " + cb->getObjInstance(ObjectInstanceID(objid))->getHoverText();
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
//    logAi->debugStream() << boost::format("Decomposing goal of type %s") % name();
//        return sptr (Goals::Explore());
//}

TSubgoal Win::whatToDoToAchieve()
{
	const VictoryCondition &vc = cb->getMapHeader()->victoryCondition;
	EVictoryConditionType::EVictoryConditionType cond = vc.condition;

	if(!vc.appliesToAI)
	{
		//TODO deduce victory from human loss condition
		cond = EVictoryConditionType::WINSTANDARD;
	}

	switch(cond)
	{
	case EVictoryConditionType::ARTIFACT:
		return sptr (Goals::GetArtOfType(vc.objectId));
	case EVictoryConditionType::BEATHERO:
		return sptr (Goals::GetObj(vc.obj->id.getNum()));
	case EVictoryConditionType::BEATMONSTER:
		return sptr (Goals::GetObj(vc.obj->id.getNum()));
	case EVictoryConditionType::BUILDCITY:
		//TODO build castle/capitol
		break;
	case EVictoryConditionType::BUILDGRAIL:
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
					boost::sort(towns, isCloser);
					if(towns.size())
					{
						return sptr (Goals::VisitTile(towns.front()->visitablePos()).sethero(h));
					}
				}
			}
			double ratio = 0;
			int3 grailPos = cb->getGrailPos(ratio);
			if(ratio > 0.99)
			{
				return sptr (Goals::DigAtTile(grailPos));
			} //TODO: use FIND_OBJ
			else if(const CGObjectInstance * obj = ai->getUnvisitedObj(objWithID<Obj::OBELISK>)) //there are unvisited Obelisks
			{
				return sptr (Goals::GetObj(obj->id.getNum()));
			}
			else
				return sptr (Goals::Explore());
		}
		break;
	case EVictoryConditionType::CAPTURECITY:
		return sptr (Goals::GetObj(vc.obj->id.getNum()));
	case EVictoryConditionType::GATHERRESOURCE:
        return sptr (Goals::CollectRes(static_cast<Res::ERes>(vc.objectId), vc.count));
		//TODO mines? piles? marketplace?
		//save?
		break;
	case EVictoryConditionType::GATHERTROOP:
		return sptr (Goals::GatherTroops(vc.objectId, vc.count));
		break;
	case EVictoryConditionType::TAKEDWELLINGS:
		break;
	case EVictoryConditionType::TAKEMINES:
		break;
	case EVictoryConditionType::TRANSPORTITEM:
		break;
	case EVictoryConditionType::WINSTANDARD:
		return sptr (Goals::Conquer());
	default:
		assert(0);
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
	if (o && isReachable(o)) //we don't use isAccessibleForHero as we don't know which hero it is
		return sptr (Goals::GetObj(o->id.getNum()));
	else
		return sptr (Goals::Explore());
}
float FindObj::importanceWhenLocked() const
{
	return 1; //we will probably fins it anyway, someday
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
	int3 pos = obj->visitablePos();
	return sptr (Goals::VisitTile(pos).sethero(hero)); //we must visit object with same hero, if any
}

float GetObj::importanceWhenLocked() const
{
	return 3;
}


bool GetObj::fulfillsMe (shared_ptr<VisitTile> goal)
{
	if (cb->getObj(ObjectInstanceID(objid))->visitablePos() == goal->tile)
		return true;
	else
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
			settile(pos).setisElementar(true);
			return sptr (*this);
		}
	}
	return sptr (Goals::Invalid());
}

float VisitHero::importanceWhenLocked() const
{
	return 4;
}

bool VisitHero::fulfillsMe (shared_ptr<VisitTile> goal)
{
	if (cb->getObj(ObjectInstanceID(objid))->visitablePos() == goal->tile)
		return true;
	else
		return false;
}

TSubgoal GetArtOfType::whatToDoToAchieve()
{
	TSubgoal alternativeWay = CGoal::lookForArtSmart(aid); //TODO: use
	if(alternativeWay->invalid())
		return sptr (Goals::FindObj(Obj::ARTIFACT, aid));
	return sptr (Goals::Invalid());
}

float GetArtOfType::importanceWhenLocked() const
{
	return 2;
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
	for (auto h : cb->getHeroesInfo())
	{
		cb->setSelection(h);

		SectorMap sm;

		int3 tileToHit = sm.firstTileToGet(hero ? hero : h, tile);
		//if our hero is trapped, make sure we request clearing the way from OUR perspective

		if (isBlockedBorderGate(tileToHit))
		{	//FIXME: this way we'll not visit gate and activate quest :?
			ret.push_back  (sptr (Goals::FindObj (Obj::KEYMASTER, cb->getTile(tileToHit)->visitableObjects.back()->subID)));
		}

		////FIXME: this code shouldn't be necessary
		//if(tileToHit == tile)
		//{
		//	logAi->errorStream() << boost::format("Very strange, tile to hit is %s and tile is also %s, while hero %s is at %s\n")
		//		% tileToHit % tile % h->name % h->visitablePos();
		//	throw cannotFulfillGoalException("Retrieving first tile to hit failed (probably)!");
		//}

		auto topObj = backOrNull(cb->getVisitableObjs(tileToHit));
		if(topObj && topObj->ID == Obj::HERO && cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
		{
			logAi->errorStream() << boost::format("%s stands in the way of %s.\n") % topObj->getHoverText()  % h->getHoverText();
		}
		else
			ret.push_back (sptr (Goals::VisitTile(tileToHit).sethero(h)));
	}
	if (ai->canRecruitAnyHero())
		ret.push_back (sptr (Goals::RecruitHero()));

	if (ret.empty())
		throw cannotFulfillGoalException("There is no known way to clear the way to tile " + tile());

	return ret;
}

float ClearWayTo::importanceWhenLocked() const
{
	return 5;
}

std::string Explore::completeMessage() const
{
	return "Hero " + hero.get()->name + " completed exploration";
};

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
};

TGoalVec Explore::getAllPossibleSubgoals()
{
	TGoalVec ret;
	std::vector<const CGHeroInstance *> heroes;
	//std::vector<HeroPtr> heroes;
	if (hero)
		//heroes.push_back(hero);
		heroes.push_back(hero.h);
	else
	{
		//heroes = ai->getUnblockedHeroes();
		heroes = cb->getHeroesInfo();
		erase_if (heroes, [](const HeroPtr h)
		{
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
				case Obj::SUBTERRANEAN_GATE: //TODO: check ai->knownSubterraneanGates
					objs.push_back (obj);
			}
		}
	}
	for (auto h : heroes)
	{
		for (auto obj : objs) //double loop, performance risk?
		{
			if (ai->isAccessibleForHero(obj->visitablePos(), h) && isSafeToVisit(h, obj->visitablePos()))
			{
				ret.push_back (sptr (Goals::VisitTile(obj->visitablePos()).sethero(h)));
			}
		}

		int3 t = whereToExplore(h);
		if (cb->isInTheMap(t)) //valid tile was found - could be invalid (none)
			ret.push_back (sptr (Goals::VisitTile(t).sethero(h)));
	}
	//we either don't have hero yet or none of heroes can explore
	if ((!hero || ret.empty()) && ai->canRecruitAnyHero())
		ret.push_back (sptr(Goals::RecruitHero()));

	if (ret.empty())
	{
		auto h = ai->primaryHero(); //we may need to gather big army to break!
		if (h.h)
		{
			int3 t = ai->explorationNewPoint(h->getSightRadious(), h, true);
			if (cb->isInTheMap(t))
				ret.push_back (sptr(ClearWayTo(t).setisAbstract(true).sethero(ai->primaryHero())));
		}
	}
	if (ret.empty())
		throw cannotFulfillGoalException("Cannot explore - no possible ways found!");

	return ret;
};

float Explore::importanceWhenLocked() const
{
	return 1; //exploration is natural and lowpriority process
}

TSubgoal RecruitHero::whatToDoToAchieve()
{
	const CGTownInstance *t = ai->findTownWithTavern();
	if(!t)
		return sptr (Goals::BuildThis(BuildingID::TAVERN));

	if(cb->getResourceAmount(Res::GOLD) < HERO_GOLD_COST)
		return sptr (Goals::CollectRes(Res::GOLD, HERO_GOLD_COST));

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
		if (isSafeToVisit(ret->hero, tile))
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

float VisitTile::importanceWhenLocked() const
{
	return 5; //depends on a distance, but we should really reach the tile once it was selected
}

TGoalVec VisitTile::getAllPossibleSubgoals()
{
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
	if (ret.empty())
		ret.push_back (sptr(Goals::ClearWayTo(tile)));

	//important - at least one sub-goal must handle case which is impossible to fulfill (unreachable tile)
	return ret;
}

TSubgoal DigAtTile::whatToDoToAchieve()
{
	const CGObjectInstance *firstObj = frontOrNull(cb->getVisitableObjs(tile));
	if(firstObj && firstObj->ID == Obj::HERO && firstObj->tempOwner == ai->playerID) //we have hero at dest
	{
		const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(firstObj);
		sethero(h).setisElementar(true);
		return sptr (*this);
	}

	return sptr (Goals::VisitTile(tile));
}

float DigAtTile::importanceWhenLocked() const
{
	return 20; //do not! interrupt tile digging
}

TSubgoal BuildThis::whatToDoToAchieve()
{
	//TODO check res
	//look for town
	//prerequisites?
	return iAmElementar();
}

float BuildThis::importanceWhenLocked() const
{
	return 5;
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
			auto backObj = backOrNull(cb->getVisitableObjs(m->o->visitablePos())); //it'll be a hero if we have one there; otherwise marketplace
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
	return sptr (Goals::Invalid()); //FIXME: unused?
}

float CollectRes::importanceWhenLocked() const
{
	return 2;
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
		boost::sort(dwellings, isCloser);
		return sptr (Goals::GetObj(dwellings.front()->id.getNum()));
	}
	else
		return sptr (Goals::Explore());
	//TODO: exchange troops between heroes
}

float GatherTroops::importanceWhenLocked() const
{
	return 2;
}

TSubgoal Conquer::whatToDoToAchieve()
{
	return fh->chooseSolution (getAllPossibleSubgoals());
}
TGoalVec Conquer::getAllPossibleSubgoals()
{
	TGoalVec ret;

	std::vector<const CGObjectInstance *> objs; //here we'll gather enemy towns and heroes
	ai->retreiveVisitableObjs(objs);
	erase_if(objs, [&](const CGObjectInstance *obj)
	{
		return (obj->ID != Obj::TOWN && obj->ID != Obj::HERO && //not town/hero
			obj->ID != Obj::CREATURE_GENERATOR1 && obj->ID != Obj::MINE) //not dwelling or mine
			|| cb->getPlayerRelations(ai->playerID, obj->tempOwner) != PlayerRelations::ENEMIES; //only enemy objects are interesting
	});
	erase_if(objs,  [&](const CGObjectInstance *obj)
	{
		return vstd::contains (ai->reservedObjs, obj);
		//no need to capture same object twice
	});

	for (auto h : cb->getHeroesInfo())
	{
		for (auto obj : objs) //double loop, performance risk?
		{
			if (ai->isAccessibleForHero(obj->visitablePos(), h) && isSafeToVisit(h, obj->visitablePos()))
			{
				if (obj->ID == Obj::HERO)
					ret.push_back (sptr (Goals::VisitHero(obj->id.getNum()).sethero(h).setisAbstract(true)));
					//track enemy hero
				else
					ret.push_back (sptr (Goals::VisitTile(obj->visitablePos()).sethero(h)));
			}
		}
	}
	if (!objs.empty() && ai->canRecruitAnyHero()) //probably no point to recruit hero if we see no objects to capture
		ret.push_back (sptr(Goals::RecruitHero()));

	if (ret.empty())
		ret.push_back (sptr(Goals::Explore())); //we need to find an enemy
	return ret;
}

float Conquer::importanceWhenLocked() const
{
	return 10; //defeating opponent is hig priority, always
}

TSubgoal Build::whatToDoToAchieve()
{
	return iAmElementar();
}

float Build::importanceWhenLocked() const
{
	return 1;
}

TSubgoal Invalid::whatToDoToAchieve()
{
	return iAmElementar();
}

std::string GatherArmy::completeMessage() const
{
	return "Hero " + hero.get()->name + " gathered army of value " + boost::lexical_cast<std::string>(value);
};

TSubgoal GatherArmy::whatToDoToAchieve()
{
	//TODO: find hero if none set
	assert(hero.h);

	return fh->chooseSolution (getAllPossibleSubgoals()); //find dwelling. use current hero to prevent him from doing nothing.
}
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
	erase_if(otherHeroes, [heroDummy](const CGHeroInstance * h)
	{
		return (h == heroDummy.h || !ai->isAccessibleForHero(heroDummy->visitablePos(), h, true) || !ai->canGetArmy(heroDummy.h, h));
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
		for (auto obj : objs)
		{ //find safe dwelling
			auto pos = obj->visitablePos();
			if (shouldVisit (h, obj) && isSafeToVisit(h, pos) && ai->isAccessibleForHero(pos, h))
				ret.push_back (sptr (Goals::VisitTile(pos).sethero(h)));
		}
	}
	if (ret.empty())
		ret.push_back (sptr(Goals::Explore()));

	return ret;
}

float GatherArmy::importanceWhenLocked() const
{
	return 2.5;
}

//TSubgoal AbstractGoal::whatToDoToAchieve()
//{
//    logAi->debugStream() << boost::format("Decomposing goal of type %s") % name();
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



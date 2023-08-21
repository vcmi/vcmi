/*
 * VCAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "VCAI.h"
#include "FuzzyHelper.h"
#include "ResourceManager.h"
#include "BuildingManager.h"
#include "Goals/Goals.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/NetPacks.h"
#include "../../lib/bonuses/CBonusSystemNode.h"
#include "../../lib/bonuses/Limiters.h"
#include "../../lib/bonuses/Updaters.h"
#include "../../lib/bonuses/Propagators.h"
#include "../../lib/serializer/CTypeList.h"
#include "../../lib/serializer/BinarySerializer.h"
#include "../../lib/serializer/BinaryDeserializer.h"

#include "AIhelper.h"

extern FuzzyHelper * fh;

const double SAFE_ATTACK_CONSTANT = 1.5;

//one thread may be turn of AI and another will be handling a side effect for AI2
boost::thread_specific_ptr<CCallback> cb;
boost::thread_specific_ptr<VCAI> ai;

//std::map<int, std::map<int, int> > HeroView::infosCount;

//helper RAII to manage global ai/cb ptrs
struct SetGlobalState
{
	SetGlobalState(VCAI * AI)
	{
		assert(!ai.get());
		assert(!cb.get());

		ai.reset(AI);
		cb.reset(AI->myCb.get());
	}
	~SetGlobalState()
	{
		//TODO: how to handle rm? shouldn't be called after ai is destroyed, hopefully
		//TODO: to ensure that, make rm unique_ptr
		ai.release();
		cb.release();
	}
};


#define SET_GLOBAL_STATE(ai) SetGlobalState _hlpSetState(ai);

#define NET_EVENT_HANDLER SET_GLOBAL_STATE(this)
#define MAKING_TURN SET_GLOBAL_STATE(this)

VCAI::VCAI()
{
	LOG_TRACE(logAi);
	makingTurn = nullptr;
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);

	ah = new AIhelper();
	ah->setAI(this);
}

VCAI::~VCAI()
{
	delete ah;
	LOG_TRACE(logAi);
	finish();
}

void VCAI::availableCreaturesChanged(const CGDwelling * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroMoved(const TryMoveHero & details, bool verbose)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	//enemy hero may have left visible area
	validateObject(details.id);
	auto hero = cb->getHero(details.id);

	const int3 from = hero ? hero->convertToVisitablePos(details.start) : (details.start - int3(0,1,0));;
	const int3 to   = hero ? hero->convertToVisitablePos(details.end)   : (details.end   - int3(0,1,0));

	const CGObjectInstance * o1 = vstd::frontOrNull(cb->getVisitableObjs(from, verbose));
	const CGObjectInstance * o2 = vstd::frontOrNull(cb->getVisitableObjs(to, verbose));

	if(details.result == TryMoveHero::TELEPORTATION)
	{
		auto t1 = dynamic_cast<const CGTeleport *>(o1);
		auto t2 = dynamic_cast<const CGTeleport *>(o2);
		if(t1 && t2)
		{
			if(cb->isTeleportChannelBidirectional(t1->channel))
			{
				if(o1->ID == Obj::SUBTERRANEAN_GATE && o1->ID == o2->ID) // We need to only add subterranean gates in knownSubterraneanGates. Used for features not yet ported to use teleport channels
				{
					knownSubterraneanGates[o1] = o2;
					knownSubterraneanGates[o2] = o1;
					logAi->debug("Found a pair of subterranean gates between %s and %s!", from.toString(), to.toString());
				}
			}
		}
		//FIXME: teleports are not correctly visited
		unreserveObject(hero, t1);
		unreserveObject(hero, t2);
	}
	else if(details.result == TryMoveHero::EMBARK && hero)
	{
		//make sure AI not attempt to visit used boat
		validateObject(hero->boat);
	}
	else if(details.result == TryMoveHero::DISEMBARK && o1)
	{
		auto boat = dynamic_cast<const CGBoat *>(o1);
		if(boat)
			addVisitableObj(boat);
	}
}

void VCAI::heroInGarrisonChange(const CGTownInstance * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::centerView(int3 pos, int focusTime)
{
	LOG_TRACE_PARAMS(logAi, "focusTime '%i'", focusTime);
	NET_EVENT_HANDLER;
}

void VCAI::artifactMoved(const ArtifactLocation & src, const ArtifactLocation & dst)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::artifactAssembled(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showTavernWindow(const CGObjectInstance * townOrTavern)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showThievesGuildWindow(const CGObjectInstance * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::playerBlocked(int reason, bool start)
{
	LOG_TRACE_PARAMS(logAi, "reason '%i', start '%i'", reason % start);
	NET_EVENT_HANDLER;
	if(start && reason == PlayerBlocked::UPCOMING_BATTLE)
		status.setBattle(UPCOMING_BATTLE);

	if(reason == PlayerBlocked::ONGOING_MOVEMENT)
		status.setMove(start);
}

void VCAI::showPuzzleMap()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showShipyardDialog(const IShipyard * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult)
{
	LOG_TRACE_PARAMS(logAi, "victoryLossCheckResult '%s'", victoryLossCheckResult.messageToSelf.toString());
	NET_EVENT_HANDLER;
	logAi->debug("Player %d (%s): I heard that player %d (%s) %s.", playerID, playerID.getStr(), player, player.getStr(), (victoryLossCheckResult.victory() ? "won" : "lost"));
	if(player == playerID)
	{
		if(victoryLossCheckResult.victory())
		{
			logAi->debug("VCAI: I won! Incredible!");
			logAi->debug("Turn nr %d", myCb->getDate());
		}
		else
		{
			logAi->debug("VCAI: Player %d (%s) lost. It's me. What a disappointment! :(", player, player.getStr());
		}

		finish();
	}
}

void VCAI::artifactPut(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::artifactRemoved(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::artifactDisassembled(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start)
{
	LOG_TRACE_PARAMS(logAi, "start '%i'; obj '%s'", start % (visitedObj ? visitedObj->getObjectName() : std::string("n/a")));
	NET_EVENT_HANDLER;

	if(start && visitedObj) //we can end visit with null object, anyway
	{
		markObjectVisited(visitedObj);
		unreserveObject(visitor, visitedObj);
		completeGoal(sptr(Goals::VisitObj(visitedObj->id.getNum()).sethero(visitor))); //we don't need to visit it anymore
		//TODO: what if we visited one-time visitable object that was reserved by another hero (shouldn't, but..)
		if (visitedObj->ID == Obj::HERO)
		{
			visitedHeroes[visitor].insert(HeroPtr(dynamic_cast<const CGHeroInstance *>(visitedObj)));
		}
	}

	status.heroVisit(visitedObj, start);
}

void VCAI::availableArtifactsChanged(const CGBlackMarket * bm)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroVisitsTown(const CGHeroInstance * hero, const CGTownInstance * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	//buildArmyIn(town);
	//moveCreaturesToHero(town);
}

void VCAI::tileHidden(const std::unordered_set<int3> & pos)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	validateVisitableObjs();
	clearPathsInfo();
}

void VCAI::tileRevealed(const std::unordered_set<int3> & pos)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	for(int3 tile : pos)
	{
		for(const CGObjectInstance * obj : myCb->getVisitableObjs(tile))
			addVisitableObj(obj);
	}

	clearPathsInfo();
}

void VCAI::heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	auto firstHero = cb->getHero(hero1);
	auto secondHero = cb->getHero(hero2);

	status.addQuery(query, boost::str(boost::format("Exchange between heroes %s (%d) and %s (%d)") % firstHero->getNameTranslated() % firstHero->tempOwner % secondHero->getNameTranslated() % secondHero->tempOwner));

	requestActionASAP([=]()
	{
		float goalpriority1 = 0, goalpriority2 = 0;

		auto firstGoal = getGoal(firstHero);
		if(firstGoal->goalType == Goals::GATHER_ARMY)
			goalpriority1 = firstGoal->priority;
		auto secondGoal = getGoal(secondHero);
		if(secondGoal->goalType == Goals::GATHER_ARMY)
			goalpriority2 = secondGoal->priority;

		auto transferFrom2to1 = [this](const CGHeroInstance * h1, const CGHeroInstance * h2) -> void
		{
			this->pickBestCreatures(h1, h2);
			this->pickBestArtifacts(h1, h2);
		};

		//Do not attempt army or artifacts exchange if we visited ally player
		//Visits can still be useful if hero have skills like Scholar
		if(firstHero->tempOwner != secondHero->tempOwner)
		{
			logAi->debug("Heroes owned by different players. Do not exchange army or artifacts.");
		}
		else if(goalpriority1 > goalpriority2)
		{
			transferFrom2to1(firstHero, secondHero);
		}
		else if(goalpriority1 < goalpriority2)
		{
			transferFrom2to1(secondHero, firstHero);
		}
		else //regular criteria
		{
			if(firstHero->getFightingStrength() > secondHero->getFightingStrength() && ah->canGetArmy(firstHero, secondHero))
				transferFrom2to1(firstHero, secondHero);
			else if(ah->canGetArmy(secondHero, firstHero))
				transferFrom2to1(secondHero, firstHero);
		}

		completeGoal(sptr(Goals::VisitHero(firstHero->id.getNum()))); //TODO: what if we were visited by other hero in the meantime?
		completeGoal(sptr(Goals::VisitHero(secondHero->id.getNum())));

		answerQuery(query, 0);
	});
}

void VCAI::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val)
{
	LOG_TRACE_PARAMS(logAi, "which '%i', val '%i'", which % val);
	NET_EVENT_HANDLER;
}

void VCAI::showRecruitmentDialog(const CGDwelling * dwelling, const CArmedInstance * dst, int level)
{
	LOG_TRACE_PARAMS(logAi, "level '%i'", level);
	NET_EVENT_HANDLER;
}

void VCAI::heroMovePointsChanged(const CGHeroInstance * hero)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::newObject(const CGObjectInstance * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(obj->isVisitable())
		addVisitableObj(obj);
}

//to prevent AI from accessing objects that got deleted while they became invisible (Cover of Darkness, enemy hero moved etc.) below code allows AI to know deletion of objects out of sight
//see: RemoveObject::applyFirstCl, to keep AI "not cheating" do not use advantage of this and use this function just to prevent crashes
void VCAI::objectRemoved(const CGObjectInstance * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	vstd::erase_if_present(visitableObjs, obj);
	vstd::erase_if_present(alreadyVisited, obj);

	for(auto h : cb->getHeroesInfo())
		unreserveObject(h, obj);

	std::function<bool(const Goals::TSubgoal &)> checkRemovalValidity = [&](const Goals::TSubgoal & x) -> bool
	{
		if((x->goalType == Goals::VISIT_OBJ) && (x->objid == obj->id.getNum()))
			return true;
		else if(x->parent && checkRemovalValidity(x->parent)) //repeat this lambda check recursively on parent goal
			return true;
		else
			return false;
	};

	//clear VCAI / main loop caches
	vstd::erase_if(lockedHeroes, [&](const std::pair<HeroPtr, Goals::TSubgoal> & x) -> bool
	{
		return checkRemovalValidity(x.second);
	});

	vstd::erase_if(ultimateGoalsFromBasic, [&](const std::pair<Goals::TSubgoal, Goals::TGoalVec> & x) -> bool
	{
		return checkRemovalValidity(x.first);
	});

	vstd::erase_if(basicGoals, checkRemovalValidity);
	vstd::erase_if(goalsToAdd, checkRemovalValidity);
	vstd::erase_if(goalsToRemove, checkRemovalValidity);

	for(auto goal : ultimateGoalsFromBasic)
		vstd::erase_if(goal.second, checkRemovalValidity);

	//clear resource manager goal cache
	ah->removeOutdatedObjectives(checkRemovalValidity);

	//TODO: Find better way to handle hero boat removal
	if(auto hero = dynamic_cast<const CGHeroInstance *>(obj))
	{
		if(hero->boat)
		{
			vstd::erase_if_present(visitableObjs, hero->boat);
			vstd::erase_if_present(alreadyVisited, hero->boat);

			for(auto h : cb->getHeroesInfo())
				unreserveObject(h, hero->boat);
		}
	}

	//TODO
	//there are other places where CGObjectinstance ptrs are stored...
	//

	if(obj->ID == Obj::HERO && obj->tempOwner == playerID)
	{
		lostHero(cb->getHero(obj->id)); //we can promote, since objectRemoved is called just before actual deletion
	}
}

void VCAI::showHillFortWindow(const CGObjectInstance * object, const CGHeroInstance * visitor)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	requestActionASAP([=]()
	{
		makePossibleUpgrades(visitor);
	});
}

void VCAI::playerBonusChanged(const Bonus & bonus, bool gain)
{
	LOG_TRACE_PARAMS(logAi, "gain '%i'", gain);
	NET_EVENT_HANDLER;
}

void VCAI::heroCreated(const CGHeroInstance * h)
{
	LOG_TRACE(logAi);
	if(h->visitedTown)
		townVisitsThisWeek[HeroPtr(h)].insert(h->visitedTown);
	NET_EVENT_HANDLER;
}

void VCAI::advmapSpellCast(const CGHeroInstance * caster, int spellID)
{
	LOG_TRACE_PARAMS(logAi, "spellID '%i", spellID);
	NET_EVENT_HANDLER;
}

void VCAI::showInfoDialog(EInfoWindowMode type, const std::string & text, const std::vector<Component> & components, int soundID)
{
	LOG_TRACE_PARAMS(logAi, "soundID '%i'", soundID);
	NET_EVENT_HANDLER;
}

void VCAI::requestRealized(PackageApplied * pa)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(status.haveTurn())
	{
		if(pa->packType == typeList.getTypeID<EndTurn>())
		{
			if(pa->result)
				status.madeTurn();
		}
	}

	if(pa->packType == typeList.getTypeID<QueryReply>())
	{
		status.receivedAnswerConfirmation(pa->requestID, pa->result);
	}
}

void VCAI::receivedResource()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showUniversityWindow(const IMarket * market, const CGHeroInstance * visitor)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroManaPointsChanged(const CGHeroInstance * hero)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	LOG_TRACE_PARAMS(logAi, "which '%d', val '%d'", which % val);
	NET_EVENT_HANDLER;
}

void VCAI::battleResultsApplied()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	assert(status.getBattle() == ENDING_BATTLE);
	status.setBattle(NO_BATTLE);
}

void VCAI::beforeObjectPropertyChanged(const SetObjectProperty * sop)
{

}

void VCAI::objectPropertyChanged(const SetObjectProperty * sop)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(sop->what == ObjProperty::OWNER)
	{
		if(myCb->getPlayerRelations(playerID, (PlayerColor)sop->val) == PlayerRelations::ENEMIES)
		{
			//we want to visit objects owned by oppponents
			auto obj = myCb->getObj(sop->id, false);
			if(obj)
			{
				addVisitableObj(obj); // TODO: Remove once save compatability broken. In past owned objects were removed from this set
				vstd::erase_if_present(alreadyVisited, obj);
			}
		}
	}
}

void VCAI::buildChanged(const CGTownInstance * town, BuildingID buildingID, int what)
{
	LOG_TRACE_PARAMS(logAi, "what '%i'", what);
	NET_EVENT_HANDLER;

	if(town->getOwner() == playerID && what == 1) //built
		completeGoal(sptr(Goals::BuildThis(buildingID, town)));
}

void VCAI::heroBonusChanged(const CGHeroInstance * hero, const Bonus & bonus, bool gain)
{
	LOG_TRACE_PARAMS(logAi, "gain '%i'", gain);
	NET_EVENT_HANDLER;
}

void VCAI::showMarketWindow(const IMarket * market, const CGHeroInstance * visitor)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain)
{
	//TODO: AI support for ViewXXX spell
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB)
{
	LOG_TRACE(logAi);
	env = ENV;
	myCb = CB;
	cbc = CB;

	ah->init(CB.get());

	NET_EVENT_HANDLER; //sets ah->rm->cb
	playerID = *myCb->getMyColor();
	myCb->waitTillRealize = true;
	myCb->unlockGsWhenWaiting = true;

	if(!fh)
		fh = new FuzzyHelper();

	retrieveVisitableObjs();
}

void VCAI::yourTurn()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	status.startedTurn();
	makingTurn = std::make_unique<boost::thread>(&VCAI::makeTurn, this);
}

void VCAI::heroGotLevel(const CGHeroInstance * hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> & skills, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;
	status.addQuery(queryID, boost::str(boost::format("Hero %s got level %d") % hero->getNameTranslated() % hero->level));
	requestActionASAP([=](){ answerQuery(queryID, 0); });
}

void VCAI::commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;
	status.addQuery(queryID, boost::str(boost::format("Commander %s of %s got level %d") % commander->name % commander->armyObj->nodeName() % (int)commander->level));
	requestActionASAP([=](){ answerQuery(queryID, 0); });
}

void VCAI::showBlockingDialog(const std::string & text, const std::vector<Component> & components, QueryID askID, const int soundID, bool selection, bool cancel)
{
	LOG_TRACE_PARAMS(logAi, "text '%s', askID '%i', soundID '%i', selection '%i', cancel '%i'", text % askID % soundID % selection % cancel);
	NET_EVENT_HANDLER;
	int sel = 0;
	status.addQuery(askID, boost::str(boost::format("Blocking dialog query with %d components - %s")
									  % components.size() % text));

	if(selection) //select from multiple components -> take the last one (they're indexed [1-size])
		sel = static_cast<int>(components.size());

	if(!selection && cancel) //yes&no -> always answer yes, we are a brave AI :)
		sel = 1;

	requestActionASAP([=]()
	{
		answerQuery(askID, sel);
	});
}

void VCAI::showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
//	LOG_TRACE_PARAMS(logAi, "askID '%i', exits '%s'", askID % exits);
	NET_EVENT_HANDLER;
	status.addQuery(askID, boost::str(boost::format("Teleport dialog query with %d exits")
																			% exits.size()));

	int choosenExit = -1;
	if(impassable)
	{
		knownTeleportChannels[channel]->passability = TeleportChannel::IMPASSABLE;
	}
	else if(destinationTeleport != ObjectInstanceID() && destinationTeleportPos.valid())
	{
		auto neededExit = std::make_pair(destinationTeleport, destinationTeleportPos);
		if(destinationTeleport != ObjectInstanceID() && vstd::contains(exits, neededExit))
			choosenExit = vstd::find_pos(exits, neededExit);
	}

	for(auto exit : exits)
	{
		if(status.channelProbing() && exit.first == destinationTeleport)
		{
			choosenExit = vstd::find_pos(exits, exit);
			break;
		}
		else
		{
			// TODO: Implement checking if visiting that teleport will uncovert any FoW
			// So far this is the best option to handle decision about probing
			auto obj = cb->getObj(exit.first, false);
			if(obj == nullptr && !vstd::contains(teleportChannelProbingList, exit.first))
			{
				if(exit.first != destinationTeleport)
					teleportChannelProbingList.push_back(exit.first);
			}
		}
	}

	requestActionASAP([=]()
	{
		answerQuery(askID, choosenExit);
	});
}

void VCAI::showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "removableUnits '%i', queryID '%i'", removableUnits % queryID);
	NET_EVENT_HANDLER;

	std::string s1 = up ? up->nodeName() : "NONE";
	std::string s2 = down ? down->nodeName() : "NONE";

	status.addQuery(queryID, boost::str(boost::format("Garrison dialog with %s and %s") % s1 % s2));

	//you can't request action from action-response thread
	requestActionASAP([=]()
	{
		if(removableUnits)
			pickBestCreatures(down, up);

		answerQuery(queryID, 0);
	});
}

void VCAI::showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects)
{
	NET_EVENT_HANDLER;
	status.addQuery(askID, "Map object select query");
	requestActionASAP([=](){ answerQuery(askID, selectedObject.getNum()); });
}

void VCAI::saveGame(BinarySerializer & h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	NET_EVENT_HANDLER;
	validateVisitableObjs();

	#if 0
	//disabled due to issue 2890
	registerGoals(h);
	#endif // 0
	CAdventureAI::saveGame(h, version);
	serializeInternal(h, version);
}

void VCAI::loadGame(BinaryDeserializer & h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	//NET_EVENT_HANDLER;

	#if 0
	//disabled due to issue 2890
	registerGoals(h);
	#endif // 0
	CAdventureAI::loadGame(h, version);
	serializeInternal(h, version);
}

void makePossibleUpgrades(const CArmedInstance * obj)
{
	if(!obj)
		return;

	for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
	{
		if(const CStackInstance * s = obj->getStackPtr(SlotID(i)))
		{
			UpgradeInfo ui;
			cb->fillUpgradeInfo(obj, SlotID(i), ui);
			if(ui.oldID >= 0 && cb->getResourceAmount().canAfford(ui.cost[0] * s->count))
			{
				cb->upgradeCreature(obj, SlotID(i), ui.newID[0]);
			}
		}
	}
}

void VCAI::makeTurn()
{
	MAKING_TURN;

	auto day = cb->getDate(Date::EDateType::DAY);
	logAi->info("Player %d (%s) starting turn, day %d", playerID, playerID.getStr(), day);

	boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);
	setThreadName("VCAI::makeTurn");

	switch(cb->getDate(Date::DAY_OF_WEEK))
	{
	case 1:
	{
		townVisitsThisWeek.clear();
		std::vector<const CGObjectInstance *> objs;
		retrieveVisitableObjs(objs, true);
		for(const CGObjectInstance * obj : objs)
		{
			if(isWeeklyRevisitable(obj))
			{
				addVisitableObj(obj);
				vstd::erase_if_present(alreadyVisited, obj);
			}
		}
		break;
	}
	}
	markHeroAbleToExplore(primaryHero());
	visitedHeroes.clear();

	try
	{
		//it looks messy here, but it's better to have armed heroes before attempting realizing goals
		for (const CGTownInstance * t : cb->getTownsInfo())
			moveCreaturesToHero(t);

		mainLoop();

		/*Below function is also responsible for hero movement via internal wander function. By design it is separate logic for heroes that have nothing to do.
		Heroes that were not picked by striveToGoal(sptr(Goals::Win())); recently (so they do not have new goals and cannot continue/reevaluate previously locked goals) will do logic in wander().*/
		performTypicalActions();

		//for debug purpose
		for (auto h : cb->getHeroesInfo())
		{
			if (h->movementPointsRemaining())
				logAi->info("Hero %s has %d MP left", h->getNameTranslated(), h->movementPointsRemaining());
		}
	}
	catch (boost::thread_interrupted & e)
	{
	(void)e;
		logAi->debug("Making turn thread has been interrupted. We'll end without calling endTurn.");
		return;
	}
	catch (std::exception & e)
	{
		logAi->debug("Making turn thread has caught an exception: %s", e.what());
	}

	endTurn();
}

std::vector<HeroPtr> VCAI::getMyHeroes() const
{
	std::vector<HeroPtr> ret;

	for(auto h : cb->getHeroesInfo())
	{
		ret.push_back(h);
	}

	return ret;
}

void VCAI::mainLoop()
{
	std::vector<Goals::TSubgoal> elementarGoals; //no duplicates allowed (operator ==)
	basicGoals.clear();

	validateVisitableObjs();

	//get all potential and saved goals
	//TODO: not lose
	basicGoals.push_back(sptr(Goals::Win()));
	for (auto goalPair : lockedHeroes)
	{
		fh->setPriority(goalPair.second);  //re-evaluate, as heroes moved in the meantime
		basicGoals.push_back(goalPair.second);
	}
	if (ah->hasTasksLeft())
		basicGoals.push_back(ah->whatToDo());
	for (auto quest : myCb->getMyQuests())
	{
		basicGoals.push_back(sptr(Goals::CompleteQuest(quest)));
	}
	basicGoals.push_back(sptr(Goals::Build()));

	invalidPathHeroes.clear();

	for (int pass = 0; pass< 30 && basicGoals.size(); pass++)
	{
		vstd::removeDuplicates(basicGoals); //TODO: container which does this automagically without has would be nice
		goalsToAdd.clear();
		goalsToRemove.clear();
		elementarGoals.clear();
		ultimateGoalsFromBasic.clear();

		ah->updatePaths(getMyHeroes());

		logAi->debug("Main loop: decomposing %i basic goals", basicGoals.size());

		for (auto basicGoal : basicGoals)
		{
			logAi->debug("Main loop: decomposing basic goal %s", basicGoal->name());

			auto goalToDecompose = basicGoal;
			Goals::TSubgoal elementarGoal = sptr(Goals::Invalid());
			int maxAbstractGoals = 10;
			while (!elementarGoal->isElementar && maxAbstractGoals)
			{
				try
				{
					elementarGoal = decomposeGoal(goalToDecompose);
				}
				catch (goalFulfilledException & e)
				{
					//it is impossible to continue some goals (like exploration, for example)
					//complete abstract goal for now, but maybe main goal finds another path
					logAi->debug("Goal %s decomposition failed: goal was completed as much as possible", e.goal->name());
					completeGoal(e.goal); //put in goalsToRemove
					break;
				}
				catch(cannotFulfillGoalException & e)
				{
					//it is impossible to continue some goals (like exploration, for example)
					//complete abstract goal for now, but maybe main goal finds another path
					goalsToRemove.push_back(basicGoal);
					logAi->debug("Goal %s decomposition failed: %s", goalToDecompose->name(), e.what());
					break;
				}
				catch (std::exception & e) //decomposition failed, which means we can't decompose entire tree
				{
					goalsToRemove.push_back(basicGoal);
					logAi->debug("Goal %s decomposition failed: %s", basicGoal->name(), e.what());
					break;
				}
				if (elementarGoal->isAbstract) //we can decompose it further
				{
					goalsToAdd.push_back(elementarGoal);
					//decompose further now - this is necesssary if we can't add over 10 goals in the pool
					goalToDecompose = elementarGoal;
					//there is a risk of infinite abstract goal loop, though it indicates failed logic
					maxAbstractGoals--;
				}
				else if (elementarGoal->isElementar) //should be
				{
					logAi->debug("Found elementar goal %s", elementarGoal->name());
					elementarGoals.push_back(elementarGoal);
					ultimateGoalsFromBasic[elementarGoal].push_back(goalToDecompose); //TODO: how about indirect basicGoal?
					break;
				}
				else //should never be here
					throw cannotFulfillGoalException("Goal %s is neither abstract nor elementar!" + basicGoal->name());
			}
		}

		logAi->trace("Main loop: selecting best elementar goal");

		//now choose one elementar goal to realize
		Goals::TGoalVec possibleGoals(elementarGoals.begin(), elementarGoals.end()); //copy to vector
		Goals::TSubgoal goalToRealize = sptr(Goals::Invalid());
		while (possibleGoals.size())
		{
			//allow assign goals to heroes with 0 movement, but don't realize them
			//maybe there are beter ones left

			auto bestGoal = fh->chooseSolution(possibleGoals);
			if (bestGoal->hero) //lock this hero to fulfill goal
			{
				setGoal(bestGoal->hero, bestGoal);
				if (!bestGoal->hero->movementPointsRemaining() || vstd::contains(invalidPathHeroes, bestGoal->hero))
				{
					if (!vstd::erase_if_present(possibleGoals, bestGoal))
					{
						logAi->error("erase_if_preset failed? Something very wrong!");
						break;
					}
					continue; //chose next from the list
				}
			}
			goalToRealize = bestGoal; //we found our goal to execute
			break;
		}

		//realize best goal
		if (!goalToRealize->invalid())
		{
			logAi->debug("Trying to realize %s (value %2.3f)", goalToRealize->name(), goalToRealize->priority);

			try
			{
				boost::this_thread::interruption_point();
				goalToRealize->accept(this); //visitor pattern
				boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted & e)
			{
				(void)e;
				logAi->debug("Player %d: Making turn thread received an interruption!", playerID);
				throw; //rethrow, we want to truly end this thread
			}
			catch (goalFulfilledException & e)
			{
				//the sub-goal was completed successfully
				completeGoal(e.goal);
				//local goal was also completed?
				completeGoal(goalToRealize);

				// remove abstract visit tile if we completed the elementar one
				vstd::erase_if_present(goalsToAdd, goalToRealize);
			}
			catch (std::exception & e)
			{
				logAi->debug("Failed to realize subgoal of type %s, I will stop.", goalToRealize->name());
				logAi->debug("The error message was: %s", e.what());

				//erase base goal if we failed to execute decomposed goal
				for (auto basicGoal : ultimateGoalsFromBasic[goalToRealize])
					goalsToRemove.push_back(basicGoal);

				// sometimes resource manager contains an elementar goal which is not able to execute anymore and just fails each turn.
				ai->ah->notifyGoalCompleted(goalToRealize);

				//we failed to realize best goal, but maybe others are still possible?
			}

			//remove goals we couldn't decompose
			for (auto goal : goalsToRemove)
				vstd::erase_if_present(basicGoals, goal);

			//add abstract goals
			boost::sort(goalsToAdd, [](const Goals::TSubgoal & lhs, const Goals::TSubgoal & rhs) -> bool
			{
				return lhs->priority > rhs->priority; //highest priority at the beginning
			});

			//max number of goals = 10
			int i = 0;
			while (basicGoals.size() < 10 && goalsToAdd.size() > i)
			{
				if (!vstd::contains(basicGoals, goalsToAdd[i])) //don't add duplicates
					basicGoals.push_back(goalsToAdd[i]);
				i++;
			}
		}
		else //no elementar goals possible
		{
			logAi->debug("Goal decomposition exhausted");
			break;
		}
	}
}

void VCAI::performObjectInteraction(const CGObjectInstance * obj, HeroPtr h)
{
	LOG_TRACE_PARAMS(logAi, "Hero %s and object %s at %s", h->getNameTranslated() % obj->getObjectName() % obj->pos.toString());
	switch(obj->ID)
	{
	case Obj::CREATURE_GENERATOR1:
		recruitCreatures(dynamic_cast<const CGDwelling *>(obj), h.get());
		checkHeroArmy(h);
		break;
	case Obj::TOWN:
		moveCreaturesToHero(dynamic_cast<const CGTownInstance *>(obj));
		if(h->visitedTown) //we are inside, not just attacking
		{
			townVisitsThisWeek[h].insert(h->visitedTown);
			if(!h->hasSpellbook() && ah->freeGold() >= GameConstants::SPELLBOOK_GOLD_COST)
			{
				if(h->visitedTown->hasBuilt(BuildingID::MAGES_GUILD_1))
					cb->buyArtifact(h.get(), ArtifactID::SPELLBOOK);
			}
		}
		break;
	}
	completeGoal(sptr(Goals::VisitObj(obj->id.getNum()).sethero(h)));
}

void VCAI::moveCreaturesToHero(const CGTownInstance * t)
{
	if(t->visitingHero && t->armedGarrison() && t->visitingHero->tempOwner == t->tempOwner)
	{
		pickBestCreatures(t->visitingHero, t);
	}
}

void VCAI::pickBestCreatures(const CArmedInstance * destinationArmy, const CArmedInstance * source)
{
	const CArmedInstance * armies[] = {destinationArmy, source};

	auto bestArmy = ah->getSortedSlots(destinationArmy, source);

	//foreach best type -> iterate over slots in both armies and if it's the appropriate type, send it to the slot where it belongs
	for(SlotID i = SlotID(0); i.getNum() < bestArmy.size() && i.validSlot(); i.advance(1)) //i-th strongest creature type will go to i-th slot
	{
		const CCreature * targetCreature = bestArmy[i.getNum()].creature;

		for(auto armyPtr : armies)
		{
			for(SlotID j = SlotID(0); j.validSlot(); j.advance(1))
			{
				if(armyPtr->getCreature(j) == targetCreature && (i != j || armyPtr != destinationArmy)) //it's a searched creature not in dst SLOT
				{
					//can't take away last creature without split. generate a new stack with 1 creature which is weak but fast
					if(armyPtr == source
						&& source->needsLastStack()
						&& source->stacksCount() == 1
						&& (!destinationArmy->hasStackAtSlot(i) || destinationArmy->getCreature(i) == targetCreature))
					{
						auto weakest = ah->getWeakestCreature(bestArmy);

						if(weakest->creature == targetCreature)
						{
							if(1 == source->getStackCount(j))
								break;

							// move all except 1 of weakest creature from source to destination
							cb->splitStack(
								source,
								destinationArmy,
								j,
								destinationArmy->getSlotFor(targetCreature),
								destinationArmy->getStackCount(i) + source->getStackCount(j) - 1);

							break;
						}
						else
						{
							// Source last stack is not weakest. Move 1 of weakest creature from destination to source
							cb->splitStack(
								destinationArmy,
								source,
								destinationArmy->getSlotFor(weakest->creature),
								source->getFreeSlot(),
								1);
						}
					}

					cb->mergeOrSwapStacks(armyPtr, destinationArmy, j, i);
				}
			}
		}
	}

	//TODO - having now strongest possible army, we may want to think about arranging stacks

	auto hero = dynamic_cast<const CGHeroInstance *>(destinationArmy);
	if(hero)
		checkHeroArmy(hero);
}

void VCAI::pickBestArtifacts(const CGHeroInstance * h, const CGHeroInstance * other)
{
	auto equipBest = [](const CGHeroInstance * h, const CGHeroInstance * otherh, bool giveStuffToFirstHero) -> void
	{
		bool changeMade = false;
		do
		{
			changeMade = false;

			//we collect gear always in same order
			std::vector<ArtifactLocation> allArtifacts;
			if(giveStuffToFirstHero)
			{
				for(auto p : h->artifactsWorn)
				{
					if(p.second.artifact)
						allArtifacts.push_back(ArtifactLocation(h, p.first));
				}
			}
			for(auto slot : h->artifactsInBackpack)
				allArtifacts.push_back(ArtifactLocation(h, h->getArtPos(slot.artifact)));

			if(otherh)
			{
				for(auto p : otherh->artifactsWorn)
				{
					if(p.second.artifact)
						allArtifacts.push_back(ArtifactLocation(otherh, p.first));
				}
				for(auto slot : otherh->artifactsInBackpack)
					allArtifacts.push_back(ArtifactLocation(otherh, otherh->getArtPos(slot.artifact)));
			}
			//we give stuff to one hero or another, depending on giveStuffToFirstHero

			const CGHeroInstance * target = nullptr;
			if(giveStuffToFirstHero || !otherh)
				target = h;
			else
				target = otherh;

			for(auto location : allArtifacts)
			{
				if(location.slot == ArtifactPosition::MACH4 || location.slot == ArtifactPosition::SPELLBOOK)
					continue; // don't attempt to move catapult and spellbook

				if(location.relatedObj() == target && location.slot < ArtifactPosition::AFTER_LAST)
					continue; //don't reequip artifact we already wear

				auto s = location.getSlot();
				if(!s || s->locked) //we can't move locks
					continue;
				auto artifact = s->artifact;
				if(!artifact)
					continue;
				//FIXME: why are the above possible to be null?

				bool emptySlotFound = false;
				for(auto slot : artifact->artType->getPossibleSlots().at(target->bearerType()))
				{
					ArtifactLocation destLocation(target, slot);
					if(target->isPositionFree(slot) && artifact->canBePutAt(destLocation, true)) //combined artifacts are not always allowed to move
					{
						cb->swapArtifacts(location, destLocation); //just put into empty slot
						emptySlotFound = true;
						changeMade = true;
						break;
					}
				}
				if(!emptySlotFound) //try to put that atifact in already occupied slot
				{
					for(auto slot : artifact->artType->getPossibleSlots().at(target->bearerType()))
					{
						auto otherSlot = target->getSlot(slot);
						if(otherSlot && otherSlot->artifact) //we need to exchange artifact for better one
						{
							ArtifactLocation destLocation(target, slot);
							//if that artifact is better than what we have, pick it
							if(compareArtifacts(artifact, otherSlot->artifact) && artifact->canBePutAt(destLocation, true)) //combined artifacts are not always allowed to move
							{
								cb->swapArtifacts(location, ArtifactLocation(target, target->getArtPos(otherSlot->artifact)));
								changeMade = true;
								break;
							}
						}
					}
				}
				if(changeMade)
					break; //start evaluating artifacts from scratch
			}
		}
		while(changeMade);
	};

	equipBest(h, other, true);

	if(other)
		equipBest(h, other, false);
}

void VCAI::recruitCreatures(const CGDwelling * d, const CArmedInstance * recruiter)
{
	//now used only for visited dwellings / towns, not BuyArmy goal
	for(int i = 0; i < d->creatures.size(); i++)
	{
		if(!d->creatures[i].second.size())
			continue;

		int count = d->creatures[i].first;
		CreatureID creID = d->creatures[i].second.back();

		vstd::amin(count, ah->freeResources() / VLC->creatures()->getById(creID)->getFullRecruitCost());
		if(count > 0)
			cb->recruitCreatures(d, recruiter, creID, count, i);
	}
}

bool VCAI::isGoodForVisit(const CGObjectInstance * obj, HeroPtr h, std::optional<float> movementCostLimit)
{
	int3 op = obj->visitablePos();
	auto paths = ah->getPathsToTile(h, op);

	for(const auto & path : paths)
	{
		if(movementCostLimit && movementCostLimit.value() < path.movementCost())
			return false;

		if(isGoodForVisit(obj, h, path))
			return true;
	}

	return false;
}

bool VCAI::isGoodForVisit(const CGObjectInstance * obj, HeroPtr h, const AIPath & path) const
{
	const int3 pos = obj->visitablePos();
	const int3 targetPos = path.firstTileToGet();
	if (!targetPos.valid())
		return false;
	if (!isTileNotReserved(h.get(), targetPos))
		return false;
	if (obj->wasVisited(playerID))
		return false;
	if (cb->getPlayerRelations(playerID, obj->tempOwner) != PlayerRelations::ENEMIES && !isWeeklyRevisitable(obj))
		return false; // Otherwise we flag or get weekly resources / creatures
	if (!isSafeToVisit(h, pos))
		return false;
	if (!shouldVisit(h, obj))
		return false;
	if (vstd::contains(alreadyVisited, obj))
		return false;
	if (vstd::contains(reservedObjs, obj))
		return false;

	// TODO: looks extra if we already have AIPath
	//if (!isAccessibleForHero(targetPos, h))
	//	return false;

	const CGObjectInstance * topObj = cb->getVisitableObjs(obj->visitablePos()).back(); //it may be hero visiting this obj
																						//we don't try visiting object on which allied or owned hero stands
																						// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
	return !(topObj->ID == Obj::HERO && cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES); //all of the following is met
}

bool VCAI::isTileNotReserved(const CGHeroInstance * h, int3 t) const
{
	if(t.valid())
	{
		auto obj = cb->getTopObj(t);
		if(obj && vstd::contains(ai->reservedObjs, obj)
			&& vstd::contains(reservedHeroesMap, h)
			&& !vstd::contains(reservedHeroesMap.at(h), obj))
			return false; //do not capture object reserved by another hero
		else
			return true;
	}
	else
	{
		return false;
	}
}

bool VCAI::canRecruitAnyHero(const CGTownInstance * t) const
{
	//TODO: make gathering gold, building tavern or conquering town (?) possible subgoals
	if(!t)
		t = findTownWithTavern();
	if(!t)
		return false;
	if(cb->getResourceAmount(EGameResID::GOLD) < GameConstants::HERO_GOLD_COST) //TODO: use ResourceManager
		return false;
	if(cb->getHeroesInfo().size() >= ALLOWED_ROAMING_HEROES)
		return false;
	if(cb->getHeroesInfo().size() >= VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP))
		return false;
	if(!cb->getAvailableHeroes(t).size())
		return false;

	return true;
}

void VCAI::wander(HeroPtr h)
{
	auto visitTownIfAny = [this](HeroPtr h) -> bool
	{
		if (h->visitedTown)
		{
			townVisitsThisWeek[h].insert(h->visitedTown);
			buildArmyIn(h->visitedTown);
			return true;
		}
		return false;
	};

	//unclaim objects that are now dangerous for us
	auto reservedObjsSetCopy = reservedHeroesMap[h];
	for(auto obj : reservedObjsSetCopy)
	{
		if(!isSafeToVisit(h, obj->visitablePos()))
			unreserveObject(h, obj);
	}

	TimeCheck tc("looking for wander destination");

	for(int k = 0; k < 10 && h->movementPointsRemaining(); k++)
	{
		validateVisitableObjs();
		ah->updatePaths(getMyHeroes());

		std::vector<ObjectIdRef> dests;

		//also visit our reserved objects - but they are not prioritized to avoid running back and forth
		vstd::copy_if(reservedHeroesMap[h], std::back_inserter(dests), [&](ObjectIdRef obj) -> bool
		{
			return ah->isTileAccessible(h, obj->visitablePos());
		});

		int pass = 0;
		std::vector<std::optional<float>> distanceLimits = {1.0, 2.0, std::nullopt};

		while(!dests.size() && pass < distanceLimits.size())
		{
			auto & distanceLimit = distanceLimits[pass];

			logAi->debug("Looking for wander destination pass=%i, cost limit=%f", pass, distanceLimit.value_or(-1.0));

			vstd::copy_if(visitableObjs, std::back_inserter(dests), [&](ObjectIdRef obj) -> bool
			{
				return isGoodForVisit(obj, h, distanceLimit);
			});

			pass++;
		}

		if(!dests.size())
		{
			logAi->debug("Looking for town destination");

			if(cb->getVisitableObjs(h->visitablePos()).size() > 1)
				moveHeroToTile(h->visitablePos(), h); //just in case we're standing on blocked subterranean gate

			auto compareReinforcements = [&](const CGTownInstance * lhs, const CGTownInstance * rhs) -> bool
			{
				const CGHeroInstance * hptr = h.get();
				auto r1 = ah->howManyReinforcementsCanGet(hptr, lhs),
					r2 = ah->howManyReinforcementsCanGet(hptr, rhs);
				if (r1 != r2)
					return r1 < r2;
				else
					return ah->howManyReinforcementsCanBuy(hptr, lhs) < ah->howManyReinforcementsCanBuy(hptr, rhs);
			};

			std::vector<const CGTownInstance *> townsReachable;
			std::vector<const CGTownInstance *> townsNotReachable;
			for(const CGTownInstance * t : cb->getTownsInfo())
			{
				if(!t->visitingHero && !vstd::contains(townVisitsThisWeek[h], t))
				{
					if(isAccessibleForHero(t->visitablePos(), h))
						townsReachable.push_back(t);
					else
						townsNotReachable.push_back(t);
				}
			}
			if(townsReachable.size()) //travel to town with largest garrison, or empty - better than nothing
			{
				dests.push_back(*boost::max_element(townsReachable, compareReinforcements));
			}
			else if(townsNotReachable.size())
			{
				//TODO pick the truly best
				const CGTownInstance * t = *boost::max_element(townsNotReachable, compareReinforcements);
				logAi->debug("%s can't reach any town, we'll try to make our way to %s at %s", h->getNameTranslated(), t->getNameTranslated(), t->visitablePos().toString());
				int3 pos1 = h->pos;
				striveToGoal(sptr(Goals::ClearWayTo(t->visitablePos()).sethero(h))); //TODO: drop "strive", add to mainLoop
				//if out hero is stuck, we may need to request another hero to clear the way we see

				if(pos1 == h->pos && h == primaryHero()) //hero can't move
				{
					if(canRecruitAnyHero(t))
						recruitHero(t);
				}
				break;
			}
			else if(cb->getResourceAmount(EGameResID::GOLD) >= GameConstants::HERO_GOLD_COST)
			{
				std::vector<const CGTownInstance *> towns = cb->getTownsInfo();
				vstd::erase_if(towns, [&](const CGTownInstance * t) -> bool
				{
					for(const CGHeroInstance * h : cb->getHeroesInfo())
					{
						if(!t->getArmyStrength() || ah->howManyReinforcementsCanGet(h, t))
							return true;
					}
					return false;
				});
				if (towns.size())
				{
					recruitHero(*boost::max_element(towns, compareArmyStrength));
				}
				break;
			}
			else
			{
				logAi->debug("Nowhere more to go...");
				break;
			}
		}
		//end of objs empty

		if(dests.size()) //performance improvement
		{
			Goals::TGoalVec targetObjectGoals;
			for(auto destination : dests)
			{
				vstd::concatenate(targetObjectGoals, ah->howToVisitObj(h, destination, false));
			}

			if(targetObjectGoals.size())
			{
				auto bestObjectGoal = fh->chooseSolution(targetObjectGoals);

				//wander should not cause heroes to be reserved - they are always considered free
				if(bestObjectGoal->goalType == Goals::VISIT_OBJ)
				{
					auto chosenObject = cb->getObjInstance(ObjectInstanceID(bestObjectGoal->objid));
					if(chosenObject != nullptr)
						logAi->debug("Of all %d destinations, object %s at pos=%s seems nice", dests.size(), chosenObject->getObjectName(), chosenObject->pos.toString());
				}
				else
					logAi->debug("Trying to realize goal of type %s as part of wandering.", bestObjectGoal->name());

				try
				{
					decomposeGoal(bestObjectGoal)->accept(this);
				}
				catch(const goalFulfilledException & e)
				{
					if(e.goal->goalType == Goals::EGoals::VISIT_TILE || e.goal->goalType == Goals::EGoals::VISIT_OBJ)
						continue;

					throw e;
				}
			}
			else
			{
				logAi->debug("Nowhere more to go...");
				break;
			}

			visitTownIfAny(h);
		}
	}

	visitTownIfAny(h); //in case hero is just sitting in town
}

void VCAI::setGoal(HeroPtr h, Goals::TSubgoal goal)
{
	if(goal->invalid())
	{
		vstd::erase_if_present(lockedHeroes, h);
	}
	else
	{
		lockedHeroes[h] = goal;
		goal->setisElementar(false); //Force always evaluate goals before realizing
	}
}
void VCAI::evaluateGoal(HeroPtr h)
{
	if(vstd::contains(lockedHeroes, h))
		fh->setPriority(lockedHeroes[h]);
}

void VCAI::completeGoal(Goals::TSubgoal goal)
{
	if (goal->goalType == Goals::WIN) //we can never complete this goal - unless we already won
		return;

	logAi->debug("Completing goal: %s", goal->name());

	//notify Managers
	ah->notifyGoalCompleted(goal);
	//notify mainLoop()
	goalsToRemove.push_back(goal); //will be removed from mainLoop() goals
	for (auto basicGoal : basicGoals) //we could luckily fulfill any of our goals
	{
		if (basicGoal->fulfillsMe(goal))
			goalsToRemove.push_back(basicGoal);
	}

	//unreserve heroes
	if(const CGHeroInstance * h = goal->hero.get(true))
	{
		auto it = lockedHeroes.find(h);
		if(it != lockedHeroes.end())
		{
			if(it->second == goal || it->second->fulfillsMe(goal)) //FIXME this is overspecified, fulfillsMe shoudl be complete
			{
				logAi->debug(goal->completeMessage());
				lockedHeroes.erase(it); //goal fulfilled, free hero
			}
		}
	}
	else //complete goal for all heroes maybe?
	{
		vstd::erase_if(lockedHeroes, [goal](std::pair<HeroPtr, Goals::TSubgoal> p)
		{
			if(p.second == goal || p.second->fulfillsMe(goal)) //we could have fulfilled goals of other heroes by chance
			{
				logAi->debug(p.second->completeMessage());
				return true;
			}
			return false;
		});
	}

}

void VCAI::battleStart(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side, bool replayAllowed)
{
	NET_EVENT_HANDLER;
	assert(playerID > PlayerColor::PLAYER_LIMIT || status.getBattle() == UPCOMING_BATTLE);
	status.setBattle(ONGOING_BATTLE);
	const CGObjectInstance * presumedEnemy = vstd::backOrNull(cb->getVisitableObjs(tile)); //may be nullptr in some very are cases -> eg. visited monolith and fighting with an enemy at the FoW covered exit
	battlename = boost::str(boost::format("Starting battle of %s attacking %s at %s") % (hero1 ? hero1->getNameTranslated() : "a army") % (presumedEnemy ? presumedEnemy->getObjectName() : "unknown enemy") % tile.toString());
	CAdventureAI::battleStart(army1, army2, tile, hero1, hero2, side, replayAllowed);
}

void VCAI::battleEnd(const BattleResult * br, QueryID queryID)
{
	NET_EVENT_HANDLER;
	assert(status.getBattle() == ONGOING_BATTLE);
	status.setBattle(ENDING_BATTLE);
	bool won = br->winner == myCb->battleGetMySide();
	logAi->debug("Player %d (%s): I %s the %s!", playerID, playerID.getStr(), (won ? "won" : "lost"), battlename);
	battlename.clear();

	if (queryID != -1)
	{
		status.addQuery(queryID, "Combat result dialog");
		const int confirmAction = 0;
		requestActionASAP([=]()
		{
			answerQuery(queryID, confirmAction);
		});
	}
	CAdventureAI::battleEnd(br, queryID);
}

void VCAI::waitTillFree()
{
	auto unlock = vstd::makeUnlockSharedGuard(CGameState::mutex);
	status.waitTillFree();
}

void VCAI::markObjectVisited(const CGObjectInstance * obj)
{
	if(!obj)
		return;

	if(const auto * rewardable = dynamic_cast<const CRewardableObject *>(obj)) //we may want to visit it with another hero
	{
		if (rewardable->configuration.getVisitMode() == Rewardable::VISIT_HERO) //we may want to visit it with another hero
			return;

		if (rewardable->configuration.getVisitMode() == Rewardable::VISIT_BONUS) //or another time
			return;
	}

	if(obj->ID == Obj::MONSTER)
		return;

	alreadyVisited.insert(obj);
}

void VCAI::reserveObject(HeroPtr h, const CGObjectInstance * obj)
{
	reservedObjs.insert(obj);
	reservedHeroesMap[h].insert(obj);
	logAi->debug("reserved object id=%d; address=%p; name=%s", obj->id, obj, obj->getObjectName());
}

void VCAI::unreserveObject(HeroPtr h, const CGObjectInstance * obj)
{
	vstd::erase_if_present(reservedObjs, obj); //unreserve objects
	vstd::erase_if_present(reservedHeroesMap[h], obj);
}

void VCAI::markHeroUnableToExplore(HeroPtr h)
{
	heroesUnableToExplore.insert(h);
}
void VCAI::markHeroAbleToExplore(HeroPtr h)
{
	vstd::erase_if_present(heroesUnableToExplore, h);
}
bool VCAI::isAbleToExplore(HeroPtr h)
{
	return !vstd::contains(heroesUnableToExplore, h);
}
void VCAI::clearPathsInfo()
{
	heroesUnableToExplore.clear();
}

void VCAI::validateVisitableObjs()
{
	std::string errorMsg;
	auto shouldBeErased = [&](const CGObjectInstance * obj) -> bool
	{
		if(obj)
			return !cb->getObj(obj->id, false); // no verbose output needed as we check object visibility
		else
			return true;
	};

	//errorMsg is captured by ref so lambda will take the new text
	errorMsg = " shouldn't be on the visitable objects list!";
	vstd::erase_if(visitableObjs, shouldBeErased);

	//FIXME: how comes our own heroes become inaccessible?
	vstd::erase_if(reservedHeroesMap, [](std::pair<HeroPtr, std::set<const CGObjectInstance *>> hp) -> bool
	{
		return !hp.first.get(true);
	});
	for(auto & p : reservedHeroesMap)
	{
		errorMsg = " shouldn't be on list for hero " + p.first->getNameTranslated() + "!";
		vstd::erase_if(p.second, shouldBeErased);
	}

	errorMsg = " shouldn't be on the reserved objs list!";
	vstd::erase_if(reservedObjs, shouldBeErased);

	//TODO overkill, hidden object should not be removed. However, we can't know if hidden object is erased from game.
	errorMsg = " shouldn't be on the already visited objs list!";
	vstd::erase_if(alreadyVisited, shouldBeErased);
}

void VCAI::retrieveVisitableObjs(std::vector<const CGObjectInstance *> & out, bool includeOwned) const
{
	foreach_tile_pos([&](const int3 & pos)
	{
		for(const CGObjectInstance * obj : myCb->getVisitableObjs(pos, false))
		{
			if(includeOwned || obj->tempOwner != playerID)
				out.push_back(obj);
		}
	});
}

void VCAI::retrieveVisitableObjs()
{
	foreach_tile_pos([&](const int3 & pos)
	{
		for(const CGObjectInstance * obj : myCb->getVisitableObjs(pos, false))
		{
			if(obj->tempOwner != playerID)
				addVisitableObj(obj);
		}
	});
}

std::vector<const CGObjectInstance *> VCAI::getFlaggedObjects() const
{
	std::vector<const CGObjectInstance *> ret;
	for(const CGObjectInstance * obj : visitableObjs)
	{
		if(obj->tempOwner == playerID)
			ret.push_back(obj);
	}
	return ret;
}

void VCAI::addVisitableObj(const CGObjectInstance * obj)
{
	if(obj->ID == Obj::EVENT)
		return;

	visitableObjs.insert(obj);

	// All teleport objects seen automatically assigned to appropriate channels
	auto teleportObj = dynamic_cast<const CGTeleport *>(obj);
	if(teleportObj)
		CGTeleport::addToChannel(knownTeleportChannels, teleportObj);
}

const CGObjectInstance * VCAI::lookForArt(int aid) const
{
	for(const CGObjectInstance * obj : ai->visitableObjs)
	{
		if(obj->ID == Obj::ARTIFACT && obj->subID == aid)
			return obj;
	}

	return nullptr;

	//TODO what if more than one artifact is available? return them all or some slection criteria
}

bool VCAI::isAccessible(const int3 & pos) const
{
	//TODO precalculate for speed

	for(const CGHeroInstance * h : cb->getHeroesInfo())
	{
		if(isAccessibleForHero(pos, h))
			return true;
	}

	return false;
}

HeroPtr VCAI::getHeroWithGrail() const
{
	for(const CGHeroInstance * h : cb->getHeroesInfo())
	{
		if(h->hasArt(ArtifactID::GRAIL))
			return h;
	}
	return nullptr;
}

const CGObjectInstance * VCAI::getUnvisitedObj(const std::function<bool(const CGObjectInstance *)> & predicate)
{
	//TODO smarter definition of unvisited
	for(const CGObjectInstance * obj : visitableObjs)
	{
		if(predicate(obj) && !vstd::contains(alreadyVisited, obj))
			return obj;
	}
	return nullptr;
}

bool VCAI::isAccessibleForHero(const int3 & pos, HeroPtr h, bool includeAllies) const
{
	// Don't visit tile occupied by allied hero
	if(!includeAllies)
	{
		for(auto obj : cb->getVisitableObjs(pos))
		{
			if(obj->ID == Obj::HERO && cb->getPlayerRelations(ai->playerID, obj->tempOwner) != PlayerRelations::ENEMIES)
			{
				if(obj != h.get())
					return false;
			}
		}
	}
	return cb->getPathsInfo(h.get())->getPathInfo(pos)->reachable();
}

bool VCAI::moveHeroToTile(int3 dst, HeroPtr h)
{
	//TODO: consider if blockVisit objects change something in our checks: AIUtility::isBlockVisitObj()

	auto afterMovementCheck = [&]() -> void
	{
		waitTillFree(); //movement may cause battle or blocking dialog
		if(!h)
		{
			lostHero(h);
			teleportChannelProbingList.clear();
			if(status.channelProbing()) // if hero lost during channel probing we need to switch this mode off
				status.setChannelProbing(false);
			throw cannotFulfillGoalException("Hero was lost!");
		}
	};

	logAi->debug("Moving hero %s to tile %s", h->getNameTranslated(), dst.toString());
	int3 startHpos = h->visitablePos();
	bool ret = false;
	if(startHpos == dst)
	{
		//FIXME: this assertion fails also if AI moves onto defeated guarded object
		assert(cb->getVisitableObjs(dst).size() > 1); //there's no point in revisiting tile where there is no visitable object
		cb->moveHero(*h, h->convertFromVisitablePos(dst));
		afterMovementCheck(); // TODO: is it feasible to hero get killed there if game work properly?
		// If revisiting, teleport probing is never done, and so the entries into the list would remain unused and uncleared
		teleportChannelProbingList.clear();
		// not sure if AI can currently reconsider to attack bank while staying on it. Check issue 2084 on mantis for more information.
		ret = true;
	}
	else
	{
		CGPath path;
		cb->getPathsInfo(h.get())->getPath(path, dst);
		if(path.nodes.empty())
		{
			logAi->error("Hero %s cannot reach %s.", h->getNameTranslated(), dst.toString());
			throw goalFulfilledException(sptr(Goals::VisitTile(dst).sethero(h)));
		}
		int i = (int)path.nodes.size() - 1;

		auto getObj = [&](int3 coord, bool ignoreHero)
		{
			auto tile = cb->getTile(coord, false);
			assert(tile);
			return tile->topVisitableObj(ignoreHero);
			//return cb->getTile(coord,false)->topVisitableObj(ignoreHero);
		};

		auto isTeleportAction = [&](EPathNodeAction action) -> bool
		{
			if(action != EPathNodeAction::TELEPORT_NORMAL && action != EPathNodeAction::TELEPORT_BLOCKING_VISIT)
			{
				if(action != EPathNodeAction::TELEPORT_BATTLE)
				{
					return false;
				}
			}

			return true;
		};

		auto getDestTeleportObj = [&](const CGObjectInstance * currentObject, const CGObjectInstance * nextObjectTop, const CGObjectInstance * nextObject) -> const CGObjectInstance *
		{
			if(CGTeleport::isConnected(currentObject, nextObjectTop))
				return nextObjectTop;
			if(nextObjectTop && nextObjectTop->ID == Obj::HERO)
			{
				if(CGTeleport::isConnected(currentObject, nextObject))
					return nextObject;
			}

			return nullptr;
		};

		auto doMovement = [&](int3 dst, bool transit)
		{
			cb->moveHero(*h, h->convertFromVisitablePos(dst), transit);
		};

		auto doTeleportMovement = [&](ObjectInstanceID exitId, int3 exitPos)
		{
			destinationTeleport = exitId;
			if(exitPos.valid())
				destinationTeleportPos = h->convertFromVisitablePos(exitPos);
			cb->moveHero(*h, h->pos);
			destinationTeleport = ObjectInstanceID();
			destinationTeleportPos = int3(-1);
			afterMovementCheck();
		};

		auto doChannelProbing = [&]() -> void
		{
			auto currentPos = h->visitablePos();
			auto currentExit = getObj(currentPos, true)->id;

			status.setChannelProbing(true);
			for(auto exit : teleportChannelProbingList)
				doTeleportMovement(exit, int3(-1));
			teleportChannelProbingList.clear();
			status.setChannelProbing(false);

			doTeleportMovement(currentExit, currentPos);
		};

		for(; i > 0; i--)
		{
			int3 currentCoord = path.nodes[i].coord;
			int3 nextCoord = path.nodes[i - 1].coord;

			auto currentObject = getObj(currentCoord, currentCoord == h->visitablePos());
			auto nextObjectTop = getObj(nextCoord, false);
			auto nextObject = getObj(nextCoord, true);
			auto destTeleportObj = getDestTeleportObj(currentObject, nextObjectTop, nextObject);
			if(isTeleportAction(path.nodes[i - 1].action) && destTeleportObj != nullptr)
			{
				//we use special login if hero standing on teleporter it's mean we need
				doTeleportMovement(destTeleportObj->id, nextCoord);
				if(teleportChannelProbingList.size())
					doChannelProbing();
				markObjectVisited(destTeleportObj); //FIXME: Monoliths are not correctly visited

				continue;
			}

			//stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
			if(path.nodes[i - 1].turns)
			{
				//blockedHeroes.insert(h); //to avoid attempts of moving heroes with very little MPs
				break;
			}

			int3 endpos = path.nodes[i - 1].coord;
			if(endpos == h->visitablePos())
				continue;

			bool isConnected = false;
			bool isNextObjectTeleport = false;
			// Check there is node after next one; otherwise transit is pointless
			if(i - 2 >= 0)
			{
				isConnected = CGTeleport::isConnected(nextObjectTop, getObj(path.nodes[i - 2].coord, false));
				isNextObjectTeleport = CGTeleport::isTeleport(nextObjectTop);
			}
			if(isConnected || isNextObjectTeleport)
			{
				// Hero should be able to go through object if it's allow transit
				doMovement(endpos, true);
			}
			else if(path.nodes[i - 1].layer == EPathfindingLayer::AIR)
			{
				doMovement(endpos, true);
			}
			else
			{
				doMovement(endpos, false);
			}

			afterMovementCheck();

			if(teleportChannelProbingList.size())
				doChannelProbing();
		}

		if(path.nodes[0].action == EPathNodeAction::BLOCKING_VISIT)
		{
			ret = h && i == 0; // when we take resource we do not reach its position. We even might not move
		}
	}
	if(h)
	{
		if(auto visitedObject = vstd::frontOrNull(cb->getVisitableObjs(h->visitablePos()))) //we stand on something interesting
		{
			if(visitedObject != *h)
				performObjectInteraction(visitedObject, h);
		}
	}
	if(h) //we could have lost hero after last move
	{
		completeGoal(sptr(Goals::VisitTile(dst).sethero(h))); //we stepped on some tile, anyway
		completeGoal(sptr(Goals::ClearWayTo(dst).sethero(h)));

		ret = ret || (dst == h->visitablePos());

		if(!ret) //reserve object we are heading towards
		{
			auto obj = vstd::frontOrNull(cb->getVisitableObjs(dst));
			if(obj && obj != *h)
				reserveObject(h, obj);
		}

		if(startHpos == h->visitablePos() && !ret) //we didn't move and didn't reach the target
		{
			vstd::erase_if_present(lockedHeroes, h); //hero seemingly is confused or has only 95mp which is not enough to move
			invalidPathHeroes.insert(h);
			throw cannotFulfillGoalException("Invalid path found!");
		}
		evaluateGoal(h); //new hero position means new game situation
		logAi->debug("Hero %s moved from %s to %s. Returning %d.", h->getNameTranslated(), startHpos.toString(), h->visitablePos().toString(), ret);
	}
	return ret;
}

void VCAI::buildStructure(const CGTownInstance * t, BuildingID building)
{
	auto name = t->town->buildings.at(building)->getNameTranslated();
	logAi->debug("Player %d will build %s in town of %s at %s", ai->playerID, name, t->getNameTranslated(), t->pos.toString());
	cb->buildBuilding(t, building); //just do this;
}

void VCAI::tryRealize(Goals::Explore & g)
{
	throw cannotFulfillGoalException("EXPLORE is not an elementar goal!");
}

void VCAI::tryRealize(Goals::RecruitHero & g)
{
	if(const CGTownInstance * t = findTownWithTavern())
	{
		recruitHero(t, true);
		//TODO try to free way to blocked town
		//TODO: adventure map tavern or prison?
	}
	else
	{
		throw cannotFulfillGoalException("No town to recruit hero!");
	}
}

void VCAI::tryRealize(Goals::VisitTile & g)
{
	if(!g.hero->movementPointsRemaining())
		throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
	if(g.tile == g.hero->visitablePos() && cb->getVisitableObjs(g.hero->visitablePos()).size() < 2)
	{
		logAi->warn("Why do I want to move hero %s to tile %s? Already standing on that tile! ", g.hero->getNameTranslated(), g.tile.toString());
		throw goalFulfilledException(sptr(g));
	}
	if(ai->moveHeroToTile(g.tile, g.hero.get()))
	{
		throw goalFulfilledException(sptr(g));
	}
}

void VCAI::tryRealize(Goals::VisitObj & g)
{
	auto position = g.tile;
	if(!g.hero->movementPointsRemaining())
		throw cannotFulfillGoalException("Cannot visit object: hero is out of MPs!");
	if(position == g.hero->visitablePos() && cb->getVisitableObjs(g.hero->visitablePos()).size() < 2)
	{
		logAi->warn("Why do I want to move hero %s to tile %s? Already standing on that tile! ", g.hero->getNameTranslated(), g.tile.toString());
		throw goalFulfilledException(sptr(g));
	}
	if(ai->moveHeroToTile(position, g.hero.get()))
	{
		throw goalFulfilledException(sptr(g));
	}
}

void VCAI::tryRealize(Goals::VisitHero & g)
{
	if(!g.hero->movementPointsRemaining())
		throw cannotFulfillGoalException("Cannot visit target hero: hero is out of MPs!");

	const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(g.objid));
	if(obj)
	{
		if(ai->moveHeroToTile(obj->visitablePos(), g.hero.get()))
		{
			throw goalFulfilledException(sptr(g));
		}
	}
	else
	{
		throw cannotFulfillGoalException("Cannot visit hero: object not found!");
	}
}

void VCAI::tryRealize(Goals::BuildThis & g)
{
	auto b = BuildingID(g.bid);
	auto t = g.town;

	if (t)
	{
		if (cb->canBuildStructure(t, b) == EBuildingState::ALLOWED)
		{
			logAi->debug("Player %d will build %s in town of %s at %s",
				playerID, t->town->buildings.at(b)->getNameTranslated(), t->getNameTranslated(), t->pos.toString());
			cb->buildBuilding(t, b);
			throw goalFulfilledException(sptr(g));
		}
	}
	throw cannotFulfillGoalException("Cannot build a given structure!");
}

void VCAI::tryRealize(Goals::DigAtTile & g)
{
	assert(g.hero->visitablePos() == g.tile); //surely we want to crash here?
	if(g.hero->diggingStatus() == EDiggingStatus::CAN_DIG)
	{
		cb->dig(g.hero.get());
		completeGoal(sptr(g)); // finished digging
	}
	else
	{
		ai->lockedHeroes[g.hero] = sptr(g); //hero who tries to dig shouldn't do anything else
		throw cannotFulfillGoalException("A hero can't dig!\n");
	}
}

void VCAI::tryRealize(Goals::Trade & g) //trade
{
	if(ah->freeResources()[g.resID] >= g.value) //goal is already fulfilled. Why we need this check, anyway?
		throw goalFulfilledException(sptr(g));

	int accquiredResources = 0;
	if(const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(g.objid), false))
	{
		if(const IMarket * m = IMarket::castFrom(obj, false))
		{
			auto freeRes = ah->freeResources(); //trade only resources which are not reserved
			for(auto it = ResourceSet::nziterator(freeRes); it.valid(); it++)
			{
				auto res = it->resType;
				if(res.getNum() == g.resID) //sell any other resource
					continue;

				int toGive, toGet;
				m->getOffer(res, g.resID, toGive, toGet, EMarketMode::RESOURCE_RESOURCE);
				toGive = static_cast<int>(toGive * (it->resVal / toGive)); //round down
				//TODO trade only as much as needed
				if (toGive) //don't try to sell 0 resources
				{
					cb->trade(m, EMarketMode::RESOURCE_RESOURCE, res, g.resID, toGive);
					accquiredResources = static_cast<int>(toGet * (it->resVal / toGive));
					logAi->debug("Traded %d of %s for %d of %s at %s", toGive, res, accquiredResources, g.resID, obj->getObjectName());
				}
				if (ah->freeResources()[g.resID] >= g.value)
					throw goalFulfilledException(sptr(g)); //we traded all we needed
			}

			throw cannotFulfillGoalException("I cannot get needed resources by trade!");
		}
		else
		{
			throw cannotFulfillGoalException("I don't know how to use this object to raise resources!");
		}
	}
	else
	{
		throw cannotFulfillGoalException("No object that could be used to raise resources!");
	}
}

void VCAI::tryRealize(Goals::BuyArmy & g)
{
	auto t = g.town;

	ui64 valueBought = 0;
	//buy the stacks with largest AI value

	makePossibleUpgrades(t);

	while (valueBought < g.value)
	{
		auto res = ah->allResources();
		std::vector<creInfo> creaturesInDwellings;

		for (int i = 0; i < t->creatures.size(); i++)
		{
			auto ci = infoFromDC(t->creatures[i]);

			if(!ci.count
				|| ci.creID == -1
				|| (g.objid != -1 && ci.creID != g.objid)
				|| t->getUpperArmy()->getSlotFor(ci.creID) == SlotID())
				continue;

			vstd::amin(ci.count, res / ci.cre->getFullRecruitCost()); //max count we can afford

			if(!ci.count)
				continue;

			ci.level = i; //this is important for Dungeon Summoning Portal
			creaturesInDwellings.push_back(ci);
		}

		if (creaturesInDwellings.empty())
			throw cannotFulfillGoalException("Can't buy any more creatures!");

		creInfo ci =
			*boost::max_element(creaturesInDwellings, [](const creInfo & lhs, const creInfo & rhs)
		{
			//max value of creatures we can buy with our res
			int value1 = lhs.cre->getAIValue() * lhs.count,
				value2 = rhs.cre->getAIValue() * rhs.count;

			return value1 < value2;
		});


		cb->recruitCreatures(t, t->getUpperArmy(), ci.creID, ci.count, ci.level);
		valueBought += ci.count * ci.cre->getAIValue();
	}

	throw goalFulfilledException(sptr(g)); //we bought as many creatures as we wanted
}

void VCAI::tryRealize(Goals::Invalid & g)
{
	throw cannotFulfillGoalException("I don't know how to fulfill this!");
}

void VCAI::tryRealize(Goals::AbstractGoal & g)
{
	logAi->debug("Attempting realizing goal with code %s", g.name());
	throw cannotFulfillGoalException("Unknown type of goal !");
}

const CGTownInstance * VCAI::findTownWithTavern() const
{
	for(const CGTownInstance * t : cb->getTownsInfo())
		if(t->hasBuilt(BuildingID::TAVERN) && !t->visitingHero)
			return t;

	return nullptr;
}

Goals::TSubgoal VCAI::getGoal(HeroPtr h) const
{
	auto it = lockedHeroes.find(h);
	if(it != lockedHeroes.end())
		return it->second;
	else
		return sptr(Goals::Invalid());
}


std::vector<HeroPtr> VCAI::getUnblockedHeroes() const
{
	std::vector<HeroPtr> ret;
	for(auto h : cb->getHeroesInfo())
	{
		//&& !vstd::contains(lockedHeroes, h)
		//at this point we assume heroes exhausted their locked goals
		if(canAct(h))
			ret.push_back(h);
	}
	return ret;
}

bool VCAI::canAct(HeroPtr h) const
{
	auto mission = lockedHeroes.find(h);
	if(mission != lockedHeroes.end())
	{
		//FIXME: I'm afraid there can be other conditions when heroes can act but not move :?
		if(mission->second->goalType == Goals::DIG_AT_TILE && !mission->second->isElementar)
			return false;
	}

	return h->movementPointsRemaining();
}

HeroPtr VCAI::primaryHero() const
{
	auto hs = cb->getHeroesInfo();
	if (hs.empty())
		return nullptr;
	else
		return *boost::max_element(hs, compareHeroStrength);
}

void VCAI::endTurn()
{
	logAi->info("Player %d (%s) ends turn", playerID, playerID.getStr());
	if(!status.haveTurn())
	{
		logAi->error("Not having turn at the end of turn???");
	}
	logAi->debug("Resources at the end of turn: %s", cb->getResourceAmount().toString());
	do
	{
		cb->endTurn();
	}
	while(status.haveTurn()); //for some reasons, our request may fail -> stop requesting end of turn only after we've received a confirmation that it's over

	logGlobal->info("Player %d (%s) ended turn", playerID, playerID.getStr());
}

void VCAI::striveToGoal(Goals::TSubgoal basicGoal)
{
	//TODO: this function is deprecated and should be dropped altogether

	auto goalToDecompose = basicGoal;
	Goals::TSubgoal elementarGoal = sptr(Goals::Invalid());
	int maxAbstractGoals = 10;
	while (!elementarGoal->isElementar && maxAbstractGoals)
	{
		try
		{
			elementarGoal = decomposeGoal(goalToDecompose);
		}
		catch (goalFulfilledException & e)
		{
			//it is impossible to continue some goals (like exploration, for example)
			completeGoal(e.goal); //put in goalsToRemove
			logAi->debug("Goal %s decomposition failed: goal was completed as much as possible", e.goal->name());
			return;
		}
		catch (std::exception & e)
		{
			goalsToRemove.push_back(basicGoal);
			logAi->debug("Goal %s decomposition failed: %s", basicGoal->name(), e.what());
			return;
		}
		if (elementarGoal->isAbstract) //we can decompose it further
		{
			goalsToAdd.push_back(elementarGoal);
			//decompose further now - this is necesssary if we can't add over 10 goals in the pool
			goalToDecompose = elementarGoal;
			//there is a risk of infinite abstract goal loop, though it indicates failed logic
			maxAbstractGoals--;
		}
		else if (elementarGoal->isElementar) //should be
		{
			logAi->debug("Found elementar goal %s", elementarGoal->name());
			ultimateGoalsFromBasic[elementarGoal].push_back(goalToDecompose); //TODO: how about indirect basicGoal?
			break;
		}
		else //should never be here
			throw cannotFulfillGoalException("Goal %s is neither abstract nor elementar!" + basicGoal->name());
	}

	//realize best goal
	if (!elementarGoal->invalid())
	{
		logAi->debug("Trying to realize %s (value %2.3f)", elementarGoal->name(), elementarGoal->priority);

		try
		{
			boost::this_thread::interruption_point();
			elementarGoal->accept(this); //visitor pattern
			boost::this_thread::interruption_point();
		}
		catch (boost::thread_interrupted & e)
		{
			(void)e;
			logAi->debug("Player %d: Making turn thread received an interruption!", playerID);
			throw; //rethrow, we want to truly end this thread
		}
		catch (goalFulfilledException & e)
		{
			//the sub-goal was completed successfully
			completeGoal(e.goal);
			//local goal was also completed
			completeGoal(elementarGoal);
		}
		catch (std::exception & e)
		{
			logAi->debug("Failed to realize subgoal of type %s, I will stop.", elementarGoal->name());
			logAi->debug("The error message was: %s", e.what());

			//erase base goal if we failed to execute decomposed goal
			for (auto basicGoalToRemove : ultimateGoalsFromBasic[elementarGoal])
				goalsToRemove.push_back(basicGoalToRemove);
		}
	}
}

Goals::TSubgoal VCAI::decomposeGoal(Goals::TSubgoal ultimateGoal)
{
	if(ultimateGoal->isElementar)
	{
		logAi->warn("Trying to decompose elementar goal %s", ultimateGoal->name());

		return ultimateGoal;
	}

	const int searchDepth = 30;

	Goals::TSubgoal goal = ultimateGoal;
	logAi->debug("Decomposing goal %s", ultimateGoal->name());
	int maxGoals = searchDepth; //preventing deadlock for mutually dependent goals
	while (maxGoals)
	{
		boost::this_thread::interruption_point();

		goal = goal->whatToDoToAchieve(); //may throw if decomposition fails
		--maxGoals;
		if (goal == ultimateGoal) //compare objects by value
			if (goal->isElementar == ultimateGoal->isElementar)
				throw cannotFulfillGoalException((boost::format("Goal dependency loop detected for %s!")
												% ultimateGoal->name()).str());
		if (goal->isAbstract || goal->isElementar)
			return goal;
		else
			logAi->debug("Considering: %s", goal->name());
	}

	throw cannotFulfillGoalException("Too many subgoals, don't know what to do");
}

void VCAI::performTypicalActions()
{
	for(auto h : getUnblockedHeroes())
	{
		if(!h) //hero might be lost. getUnblockedHeroes() called once on start of turn
			continue;

		logAi->debug("Hero %s started wandering, MP=%d", h->getNameTranslated(), h->movementPointsRemaining());
		makePossibleUpgrades(*h);
		pickBestArtifacts(*h);
		try
		{
			wander(h);
		}
		catch(std::exception & e)
		{
			logAi->debug("Cannot use this hero anymore, received exception: %s", e.what());
			continue;
		}
	}
}

void VCAI::buildArmyIn(const CGTownInstance * t)
{
	makePossibleUpgrades(t->visitingHero);
	makePossibleUpgrades(t);
	recruitCreatures(t, t->getUpperArmy());
	moveCreaturesToHero(t);
}

void VCAI::checkHeroArmy(HeroPtr h)
{
	auto it = lockedHeroes.find(h);
	if(it != lockedHeroes.end())
	{
		if(it->second->goalType == Goals::GATHER_ARMY && it->second->value <= h->getArmyStrength())
			completeGoal(sptr(Goals::GatherArmy(it->second->value).sethero(h)));
	}
}

void VCAI::recruitHero(const CGTownInstance * t, bool throwing)
{
	logAi->debug("Trying to recruit a hero in %s at %s", t->getNameTranslated(), t->visitablePos().toString());

	auto heroes = cb->getAvailableHeroes(t);
	if(heroes.size())
	{
		auto hero = heroes[0];
		if(heroes.size() >= 2) //makes sense to recruit two heroes with starting amries in first week
		{
			if(heroes[1]->getTotalStrength() > hero->getTotalStrength())
				hero = heroes[1];
		}
		cb->recruitHero(t, hero);
		throw goalFulfilledException(sptr(Goals::RecruitHero().settown(t)));
	}
	else if(throwing)
	{
		throw cannotFulfillGoalException("No available heroes in tavern in " + t->nodeName());
	}
}

void VCAI::finish()
{
	//we want to lock to avoid multiple threads from calling makingTurn->join() at same time
	boost::lock_guard<boost::mutex> multipleCleanupGuard(turnInterruptionMutex);
	if(makingTurn)
	{
		makingTurn->interrupt();
		makingTurn->join();
		makingTurn.reset();
	}
}

void VCAI::requestActionASAP(std::function<void()> whatToDo)
{
	boost::thread newThread([this, whatToDo]()
	{
		setThreadName("VCAI::requestActionASAP::whatToDo");
		SET_GLOBAL_STATE(this);
		boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);
		whatToDo();
	});
}

void VCAI::lostHero(HeroPtr h)
{
	logAi->debug("I lost my hero %s. It's best to forget and move on.", h.name);

	vstd::erase_if_present(lockedHeroes, h);
	for(auto obj : reservedHeroesMap[h])
	{
		vstd::erase_if_present(reservedObjs, obj); //unreserve all objects for that hero
	}
	vstd::erase_if_present(reservedHeroesMap, h);
	vstd::erase_if_present(visitedHeroes, h);
	for (auto heroVec : visitedHeroes)
	{
		vstd::erase_if_present(heroVec.second, h);
	}

	//remove goals with removed hero assigned from main loop
	vstd::erase_if(ultimateGoalsFromBasic, [&](const std::pair<Goals::TSubgoal, Goals::TGoalVec> & x) -> bool
	{
		if(x.first->hero == h)
			return true;
		else
			return false;
	});

	auto removedHeroGoalPredicate = [&](const Goals::TSubgoal & x) ->bool
	{
		if(x->hero == h)
			return true;
		else
			return false;
	};

	vstd::erase_if(basicGoals, removedHeroGoalPredicate);
	vstd::erase_if(goalsToAdd, removedHeroGoalPredicate);
	vstd::erase_if(goalsToRemove, removedHeroGoalPredicate);

	for(auto goal : ultimateGoalsFromBasic)
		vstd::erase_if(goal.second, removedHeroGoalPredicate);
}

void VCAI::answerQuery(QueryID queryID, int selection)
{
	logAi->debug("I'll answer the query %d giving the choice %d", queryID, selection);
	if(queryID != QueryID(-1))
	{
		cb->selectionMade(selection, queryID);
	}
	else
	{
		logAi->debug("Since the query ID is %d, the answer won't be sent. This is not a real query!", queryID);
		//do nothing
	}
}

void VCAI::requestSent(const CPackForServer * pack, int requestID)
{
	//BNLOG("I have sent request of type %s", typeid(*pack).name());
	if(auto reply = dynamic_cast<const QueryReply *>(pack))
	{
		status.attemptedAnsweringQuery(reply->qid, requestID);
	}
}

std::string VCAI::getBattleAIName() const
{
	if(settings["server"]["enemyAI"].getType() == JsonNode::JsonType::DATA_STRING)
		return settings["server"]["enemyAI"].String();
	else
		return "BattleAI";
}

void VCAI::validateObject(const CGObjectInstance * obj)
{
	validateObject(obj->id);
}

void VCAI::validateObject(ObjectIdRef obj)
{
	auto matchesId = [&](const CGObjectInstance * hlpObj) -> bool
	{
		return hlpObj->id == obj.id;
	};
	if(!obj)
	{
		vstd::erase_if(visitableObjs, matchesId);

		for(auto & p : reservedHeroesMap)
			vstd::erase_if(p.second, matchesId);

		vstd::erase_if(reservedObjs, matchesId);
	}
}

AIStatus::AIStatus()
{
	battle = NO_BATTLE;
	havingTurn = false;
	ongoingHeroMovement = false;
	ongoingChannelProbing = false;
}

AIStatus::~AIStatus()
{

}

void AIStatus::setBattle(BattleState BS)
{
	boost::unique_lock<boost::mutex> lock(mx);
	LOG_TRACE_PARAMS(logAi, "battle state=%d", (int)BS);
	battle = BS;
	cv.notify_all();
}

BattleState AIStatus::getBattle()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return battle;
}

void AIStatus::addQuery(QueryID ID, std::string description)
{
	if(ID == QueryID(-1))
	{
		logAi->debug("The \"query\" has an id %d, it'll be ignored as non-query. Description: %s", ID, description);
		return;
	}

	assert(ID.getNum() >= 0);
	boost::unique_lock<boost::mutex> lock(mx);

	assert(!vstd::contains(remainingQueries, ID));

	remainingQueries[ID] = description;

	cv.notify_all();
	logAi->debug("Adding query %d - %s. Total queries count: %d", ID, description, remainingQueries.size());
}

void AIStatus::removeQuery(QueryID ID)
{
	boost::unique_lock<boost::mutex> lock(mx);
	assert(vstd::contains(remainingQueries, ID));

	std::string description = remainingQueries[ID];
	remainingQueries.erase(ID);

	cv.notify_all();
	logAi->debug("Removing query %d - %s. Total queries count: %d", ID, description, remainingQueries.size());
}

int AIStatus::getQueriesCount()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return static_cast<int>(remainingQueries.size());
}

void AIStatus::startedTurn()
{
	boost::unique_lock<boost::mutex> lock(mx);
	havingTurn = true;
	cv.notify_all();
}

void AIStatus::madeTurn()
{
	boost::unique_lock<boost::mutex> lock(mx);
	havingTurn = false;
	cv.notify_all();
}

void AIStatus::waitTillFree()
{
	boost::unique_lock<boost::mutex> lock(mx);
	while(battle != NO_BATTLE || !remainingQueries.empty() || !objectsBeingVisited.empty() || ongoingHeroMovement)
		cv.timed_wait(lock, boost::posix_time::milliseconds(100));
}

bool AIStatus::haveTurn()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return havingTurn;
}

void AIStatus::attemptedAnsweringQuery(QueryID queryID, int answerRequestID)
{
	boost::unique_lock<boost::mutex> lock(mx);
	assert(vstd::contains(remainingQueries, queryID));
	std::string description = remainingQueries[queryID];
	logAi->debug("Attempted answering query %d - %s. Request id=%d. Waiting for results...", queryID, description, answerRequestID);
	requestToQueryID[answerRequestID] = queryID;
}

void AIStatus::receivedAnswerConfirmation(int answerRequestID, int result)
{
	QueryID query;

	{
		boost::unique_lock<boost::mutex> lock(mx);

		assert(vstd::contains(requestToQueryID, answerRequestID));
		query = requestToQueryID[answerRequestID];
		assert(vstd::contains(remainingQueries, query));
		requestToQueryID.erase(answerRequestID);
	}

	if(result)
	{
		removeQuery(query);
	}
	else
	{
		logAi->error("Something went really wrong, failed to answer query %d : %s", query.getNum(), remainingQueries[query]);
		//TODO safely retry
	}
}

void AIStatus::heroVisit(const CGObjectInstance * obj, bool started)
{
	boost::unique_lock<boost::mutex> lock(mx);
	if(started)
	{
		objectsBeingVisited.push_back(obj);
	}
	else
	{
		// There can be more than one object visited at the time (eg. hero visits Subterranean Gate
		// causing visit to hero on the other side.
		// However, we are guaranteed that start/end visit notification maintain stack order.
		assert(!objectsBeingVisited.empty());
		objectsBeingVisited.pop_back();
	}
	cv.notify_all();
}

void AIStatus::setMove(bool ongoing)
{
	boost::unique_lock<boost::mutex> lock(mx);
	ongoingHeroMovement = ongoing;
	cv.notify_all();
}

void AIStatus::setChannelProbing(bool ongoing)
{
	boost::unique_lock<boost::mutex> lock(mx);
	ongoingChannelProbing = ongoing;
	cv.notify_all();
}

bool AIStatus::channelProbing()
{
	return ongoingChannelProbing;
}



bool isWeeklyRevisitable(const CGObjectInstance * obj)
{
	//TODO: allow polling of remaining creatures in dwelling
	if(const auto * rewardable = dynamic_cast<const CRewardableObject *>(obj))
		return rewardable->configuration.getResetDuration() == 7;

	if(dynamic_cast<const CGDwelling *>(obj))
		return true;
	if(dynamic_cast<const CBank *>(obj)) //banks tend to respawn often in mods
		return true;

	switch(obj->ID)
	{
	case Obj::STABLES:
	case Obj::MAGIC_WELL:
	case Obj::HILL_FORT:
		return true;
	case Obj::BORDER_GATE:
	case Obj::BORDERGUARD:
		return (dynamic_cast<const CGKeys *>(obj))->wasMyColorVisited(ai->playerID); //FIXME: they could be revisited sooner than in a week
	}
	return false;
}

bool shouldVisit(HeroPtr h, const CGObjectInstance * obj)
{
	switch(obj->ID)
	{
	case Obj::TOWN:
	case Obj::HERO: //never visit our heroes at random
		return obj->tempOwner != h->tempOwner; //do not visit our towns at random
	case Obj::BORDER_GATE:
	{
		for(auto q : ai->myCb->getMyQuests())
		{
			if(q.obj == obj)
			{
				return false; // do not visit guards or gates when wandering
			}
		}
		return true; //we don't have this quest yet
	}
	case Obj::BORDERGUARD: //open borderguard if possible
		return (dynamic_cast<const CGKeys *>(obj))->wasMyColorVisited(ai->playerID);
	case Obj::SEER_HUT:
	case Obj::QUEST_GUARD:
	{
		for(auto q : ai->myCb->getMyQuests())
		{
			if(q.obj == obj)
			{
				if(q.quest->checkQuest(h.h))
					return true; //we completed the quest
				else
					return false; //we can't complete this quest
			}
		}
		return true; //we don't have this quest yet
	}
	case Obj::CREATURE_GENERATOR1:
	{
		if(obj->tempOwner != h->tempOwner)
			return true; //flag just in case
		bool canRecruitCreatures = false;
		const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);
		for(auto level : d->creatures)
		{
			for(auto c : level.second)
			{
				if(h->getSlotFor(CreatureID(c)) != SlotID())
					canRecruitCreatures = true;
			}
		}
		return canRecruitCreatures;
	}
	case Obj::HILL_FORT:
	{
		for(auto slot : h->Slots())
		{
			if(slot.second->type->hasUpgrades())
				return true; //TODO: check price?
		}
		return false;
	}
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::MONOLITH_TWO_WAY:
	case Obj::WHIRLPOOL:
		return false;
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
	{
		if (ai->ah->freeGold() < 1000)
			return false;
		break;
	}
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		if(h->level < 12)
			return false;
		break;
	case Obj::TREE_OF_KNOWLEDGE:
	{
		TResources myRes = ai->ah->freeResources();
		if(myRes[EGameResID::GOLD] < 2000 || myRes[EGameResID::GEMS] < 10)
			return false;
		break;
	}
	case Obj::MAGIC_WELL:
		return h->mana < h->manaLimit();
	case Obj::PRISON:
		return ai->myCb->getHeroesInfo().size() < VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP);
	case Obj::TAVERN:
	{
		//TODO: make AI actually recruit heroes
		//TODO: only on request
		if(ai->myCb->getHeroesInfo().size() >= VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP))
			return false;
		else if(ai->ah->freeGold() < GameConstants::HERO_GOLD_COST)
			return false;
		break;
	}
	case Obj::BOAT:
		return false;
	//Boats are handled by pathfinder
	case Obj::EYE_OF_MAGI:
		return false; //this object is useless to visit, but could be visited indefinitely
	}

	if(obj->wasVisited(*h)) //it must pointer to hero instance, heroPtr calls function wasVisited(ui8 player);
		return false;

	return true;
}

std::optional<BattleAction> VCAI::makeSurrenderRetreatDecision(const BattleStateInfoForRetreat & battleState)
{
	return std::nullopt;
}

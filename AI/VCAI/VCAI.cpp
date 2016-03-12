#include "StdInc.h"
#include "VCAI.h"
#include "Goals.h"
#include "Fuzzy.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/NetPacks.h"


/*
 * VCAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern FuzzyHelper *fh;

class CGVisitableOPW;

const double SAFE_ATTACK_CONSTANT = 1.5;
const int GOLD_RESERVE = 10000; //when buying creatures we want to keep at least this much gold (10000 so at least we'll be able to reach capitol)

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
		ai.release();
		cb.release();
	}
};


#define SET_GLOBAL_STATE(ai) SetGlobalState _hlpSetState(ai);

#define NET_EVENT_HANDLER SET_GLOBAL_STATE(this)
#define MAKING_TURN SET_GLOBAL_STATE(this)

unsigned char &retreiveTileN(std::vector< std::vector< std::vector<unsigned char> > > &vectors, const int3 &pos)
{
	return vectors[pos.x][pos.y][pos.z];
}

const unsigned char &retreiveTileN(const std::vector< std::vector< std::vector<unsigned char> > > &vectors, const int3 &pos)
{
	return vectors[pos.x][pos.y][pos.z];
}

void foreach_tile(std::vector< std::vector< std::vector<unsigned char> > > &vectors, std::function<void(unsigned char &in)> foo)
{
	for(auto & vector : vectors)
		for(auto j = vector.begin(); j != vector.end(); j++)
			for(auto & elem : *j)
				foo(elem);
}

struct ObjInfo
{
	int3 pos;
	std::string name;
	ObjInfo(){}
	ObjInfo(const CGObjectInstance *obj):
		pos(obj->pos),
		name(obj->getObjectName())
	{
	}
};

std::map<const CGObjectInstance *, ObjInfo> helperObjInfo;

VCAI::VCAI(void)
{
	LOG_TRACE(logAi);
	makingTurn = nullptr;
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);
}

VCAI::~VCAI(void)
{
	LOG_TRACE(logAi);
}

void VCAI::availableCreaturesChanged(const CGDwelling *town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroMoved(const TryMoveHero & details)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	validateObject(details.id); //enemy hero may have left visible area
	cachedSectorMaps.clear();

	if(details.result == TryMoveHero::TELEPORTATION)
	{
		const int3 from = CGHeroInstance::convertPosition(details.start, false),
			to = CGHeroInstance::convertPosition(details.end, false);
		const CGObjectInstance *o1 = vstd::frontOrNull(cb->getVisitableObjs(from)),
			*o2 = vstd::frontOrNull(cb->getVisitableObjs(to));

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
					logAi->debugStream() << boost::format("Found a pair of subterranean gates between %s and %s!") % from % to;
				}
			}
		}
	}
}

void VCAI::stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute)
{
	LOG_TRACE_PARAMS(logAi, "isAbsolute '%i'", isAbsolute);
	NET_EVENT_HANDLER;
}

void VCAI::heroInGarrisonChange(const CGTownInstance *town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::centerView(int3 pos, int focusTime)
{
	LOG_TRACE_PARAMS(logAi, "focusTime '%i'", focusTime);
	NET_EVENT_HANDLER;
}

void VCAI::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::artifactAssembled(const ArtifactLocation &al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showTavernWindow(const CGObjectInstance *townOrTavern)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showThievesGuildWindow (const CGObjectInstance * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::playerBlocked(int reason, bool start)
{
	LOG_TRACE_PARAMS(logAi, "reason '%i', start '%i'", reason % start);
	NET_EVENT_HANDLER;
	if (start && reason == PlayerBlocked::UPCOMING_BATTLE)
		status.setBattle(UPCOMING_BATTLE);

	if(reason == PlayerBlocked::ONGOING_MOVEMENT)
		status.setMove(start);
}

void VCAI::showPuzzleMap()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showShipyardDialog(const IShipyard *obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult)
{
	LOG_TRACE_PARAMS(logAi, "victoryLossCheckResult '%s'", victoryLossCheckResult.messageToSelf);
	NET_EVENT_HANDLER;
	logAi->debugStream() << boost::format("Player %d: I heard that player %d %s.") % playerID % player.getNum() % (victoryLossCheckResult.victory() ? "won" : "lost");
	if(player == playerID)
	{
		if(victoryLossCheckResult.victory())
		{
			logAi->debugStream() << "VCAI: I won! Incredible!";
			logAi->debugStream() << "Turn nr " << myCb->getDate();
		}
		else
		{
			logAi->debugStream() << "VCAI: Player " << player << " lost. It's me. What a disappointment! :(";
		}

		finish();
	}
}

void VCAI::artifactPut(const ArtifactLocation &al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::artifactRemoved(const ArtifactLocation &al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::stacksErased(const StackLocation &location)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::artifactDisassembled(const ArtifactLocation &al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}


void VCAI::heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start)
{
	LOG_TRACE_PARAMS(logAi, "start '%i'; obj '%s'", start % (visitedObj ? visitedObj->getObjectName() : std::string("n/a")));
	NET_EVENT_HANDLER;
	if(start)
	{
		markObjectVisited (visitedObj);
		unreserveObject(visitor, visitedObj);
		completeGoal (sptr(Goals::GetObj(visitedObj->id.getNum()).sethero(visitor))); //we don't need to visit it anymore
		//TODO: what if we visited one-time visitable object that was reserved by another hero (shouldn't, but..)
	}

	status.heroVisit(visitedObj, start);
}

void VCAI::availableArtifactsChanged(const CGBlackMarket *bm /*= nullptr*/)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	//buildArmyIn(town);
	//moveCreaturesToHero(town);
}

void VCAI::tileHidden(const std::unordered_set<int3, ShashInt3> &pos)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	validateVisitableObjs();
}

void VCAI::tileRevealed(const std::unordered_set<int3, ShashInt3> &pos)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	for(int3 tile : pos)
		for(const CGObjectInstance *obj : myCb->getVisitableObjs(tile))
			addVisitableObj(obj);

	clearPathsInfo();
}

void VCAI::heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	auto firstHero = cb->getHero(hero1);
	auto secondHero = cb->getHero(hero2);

	status.addQuery(query, boost::str(boost::format("Exchange between heroes %s and %s") % firstHero->name % secondHero->name));

	requestActionASAP([=]()
	{
		float goalpriority1 = 0, goalpriority2 = 0;

		auto firstGoal = getGoal(firstHero);
		if (firstGoal->goalType == Goals::GATHER_ARMY)
			goalpriority1 = firstGoal->priority;
		auto secondGoal = getGoal(secondHero);
		if (secondGoal->goalType == Goals::GATHER_ARMY)
			goalpriority2 = secondGoal->priority;

		auto transferFrom2to1 = [this](const CGHeroInstance * h1, const CGHeroInstance *h2) -> void
		{
			this->pickBestCreatures(h1, h2);
			this->pickBestArtifacts(h1, h2);
		};

		if (goalpriority1 > goalpriority2)
			transferFrom2to1 (firstHero, secondHero);
		else if (goalpriority1 < goalpriority2)
			transferFrom2to1 (secondHero, firstHero);
		else //regular criteria
		{
			if (firstHero->getFightingStrength() > secondHero->getFightingStrength() && canGetArmy(firstHero, secondHero))
				transferFrom2to1 (firstHero, secondHero);
			else if (canGetArmy(secondHero, firstHero))
				transferFrom2to1 (secondHero, firstHero);
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

void VCAI::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level)
{
	LOG_TRACE_PARAMS(logAi, "level '%i'", level);
	NET_EVENT_HANDLER;
}

void VCAI::heroMovePointsChanged(const CGHeroInstance * hero)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::stackChangedType(const StackLocation &location, const CCreature &newType)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count)
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

	cachedSectorMaps.clear();
}

void VCAI::objectRemoved(const CGObjectInstance *obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	vstd::erase_if_present(visitableObjs, obj);
	vstd::erase_if_present(alreadyVisited, obj);

	for (auto h : cb->getHeroesInfo())
		unreserveObject(h, obj);

	cachedSectorMaps.clear(); //invalidate all paths

	//TODO
	//there are other places where CGObjectinstance ptrs are stored...
	//

	if(obj->ID == Obj::HERO  &&  obj->tempOwner == playerID)
	{
		lostHero(cb->getHero(obj->id)); //we can promote, since objectRemoved is called just before actual deletion
	}
}

void VCAI::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	requestActionASAP([=]()
	{
		makePossibleUpgrades(visitor);
	});
}

void VCAI::playerBonusChanged(const Bonus &bonus, bool gain)
{
	LOG_TRACE_PARAMS(logAi, "gain '%i'", gain);
	NET_EVENT_HANDLER;
}

void VCAI::newStackInserted(const StackLocation &location, const CStackInstance &stack)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::heroCreated(const CGHeroInstance* h)
{
	LOG_TRACE(logAi);
	if (h->visitedTown)
		townVisitsThisWeek[HeroPtr(h)].insert(h->visitedTown);
	NET_EVENT_HANDLER;
}

void VCAI::advmapSpellCast(const CGHeroInstance * caster, int spellID)
{
	LOG_TRACE_PARAMS(logAi, "spellID '%i", spellID);
	NET_EVENT_HANDLER;
}

void VCAI::showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID)
{
	LOG_TRACE_PARAMS(logAi, "soundID '%i'", soundID);
	NET_EVENT_HANDLER;
}

void VCAI::requestRealized(PackageApplied *pa)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(status.haveTurn())
	{
		if(pa->packType == typeList.getTypeID<EndTurn>())
			if(pa->result)
				status.madeTurn();
	}

	if(pa->packType == typeList.getTypeID<QueryReply>())
	{
		status.receivedAnswerConfirmation(pa->requestID, pa->result);
	}
}

void VCAI::receivedResource(int type, int val)
{
	LOG_TRACE_PARAMS(logAi, "type '%i', val '%i'", type % val);
	NET_EVENT_HANDLER;
}

void VCAI::stacksSwapped(const StackLocation &loc1, const StackLocation &loc2)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor)
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

void VCAI::objectPropertyChanged(const SetObjectProperty * sop)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(sop->what == ObjProperty::OWNER)
	{
		//we don't want to visit know object twice (do we really?)
		if(sop->val == playerID.getNum())
			vstd::erase_if_present(visitableObjs, myCb->getObj(sop->id));
		else if(myCb->getPlayerRelations(playerID, (PlayerColor)sop->val) == PlayerRelations::ENEMIES)
		{
			//we want to visit objects owned by oppponents
			auto obj = myCb->getObj(sop->id, false);
			if (obj)
			{
				addVisitableObj(obj);
				vstd::erase_if_present(alreadyVisited, obj);
			}
		}
	}
}

void VCAI::buildChanged(const CGTownInstance *town, BuildingID buildingID, int what)
{
	LOG_TRACE_PARAMS(logAi, "what '%i'", what);
	NET_EVENT_HANDLER;
}

void VCAI::heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain)
{
	LOG_TRACE_PARAMS(logAi, "gain '%i'", gain);
	NET_EVENT_HANDLER;
}

void VCAI::showMarketWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions)
{
	//TODO: AI support for ViewXXX spell
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void VCAI::init(std::shared_ptr<CCallback> CB)
{
	LOG_TRACE(logAi);
	myCb = CB;
	cbc = CB;
	NET_EVENT_HANDLER;
	playerID = *myCb->getMyColor();
	myCb->waitTillRealize = true;
	myCb->unlockGsWhenWaiting = true;

	if(!fh)
		fh = new FuzzyHelper();

	retreiveVisitableObjs();
}

void VCAI::yourTurn()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	status.startedTurn();
	makingTurn = make_unique<boost::thread>(&VCAI::makeTurn, this);
}

void VCAI::heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;
	status.addQuery(queryID, boost::str(boost::format("Hero %s got level %d") % hero->name % hero->level));
	requestActionASAP([=]{ answerQuery(queryID, 0); });
}

void VCAI::commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;
	status.addQuery(queryID, boost::str(boost::format("Commander %s of %s got level %d") % commander->name % commander->armyObj->nodeName() % (int)commander->level));
	requestActionASAP([=]{ answerQuery(queryID, 0); });
}

void VCAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel)
{
	LOG_TRACE_PARAMS(logAi, "text '%s', askID '%i', soundID '%i', selection '%i', cancel '%i'", text % askID % soundID % selection % cancel);
	NET_EVENT_HANDLER;
	int sel = 0;
	status.addQuery(askID, boost::str(boost::format("Blocking dialog query with %d components - %s")
									  % components.size() % text));

	if(selection) //select from multiple components -> take the last one (they're indexed [1-size])
		sel = components.size();

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
		knownTeleportChannels[channel]->passability = TeleportChannel::IMPASSABLE;
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
			if(obj == nullptr && !vstd::contains(teleportChannelProbingList, exit.first) &&
				exit.first != destinationTeleport)
			{
				teleportChannelProbingList.push_back(exit.first);
			}
		}
	}

	requestActionASAP([=]()
	{
		answerQuery(askID, choosenExit);
	});
}

void VCAI::showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "removableUnits '%i', queryID '%i'", removableUnits % queryID);
	NET_EVENT_HANDLER;

	std::string s1 = up ? up->nodeName() : "NONE";
	std::string s2 = down ? down->nodeName() : "NONE";

	status.addQuery(queryID, boost::str(boost::format("Garrison dialog with %s and %s") % s1 % s2));

	//you can't request action from action-response thread
	requestActionASAP([=]()
	{
		pickBestCreatures (down, up);
		answerQuery(queryID, 0);
	});
}

void VCAI::saveGame(COSer & h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	NET_EVENT_HANDLER;
	validateVisitableObjs();

	registerGoals(h);
	CAdventureAI::saveGame(h, version);
	serializeInternal(h, version);
}

void VCAI::loadGame(CISer & h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	NET_EVENT_HANDLER;

	registerGoals(h);
	CAdventureAI::loadGame(h, version);
	serializeInternal(h, version);
}

void makePossibleUpgrades(const CArmedInstance *obj)
{
	if(!obj)
		return;

	for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
	{
		if(const CStackInstance *s = obj->getStackPtr(SlotID(i)))
		{
			UpgradeInfo ui;
			cb->getUpgradeInfo(obj, SlotID(i), ui);
			if(ui.oldID >= 0 && cb->getResourceAmount().canAfford(ui.cost[0] * s->count))
			{
				cb->upgradeCreature(obj, SlotID(i), ui.newID[0]);
			}
		}
	}
}

void VCAI::makeTurn()
{
	logGlobal->infoStream() << boost::format("Player %d starting turn") % static_cast<int>(playerID.getNum());

	MAKING_TURN;
	boost::shared_lock<boost::shared_mutex> gsLock(cb->getGsMutex());
	setThreadName("VCAI::makeTurn");

	switch(cb->getDate(Date::DAY_OF_WEEK))
	{
		case 1:
		{
			townVisitsThisWeek.clear();
			std::vector<const CGObjectInstance *> objs;
			retreiveVisitableObjs(objs, true);
			for(const CGObjectInstance *obj : objs)
			{
				if (isWeeklyRevisitable(obj))
				{
					addVisitableObj(obj);
					vstd::erase_if_present(alreadyVisited, obj);
				}
			}
		}
			break;
	}
	markHeroAbleToExplore (primaryHero());

	makeTurnInternal();
	makingTurn.reset();

	return;
}

void VCAI::makeTurnInternal()
{
	saving = 0;

	//it looks messy here, but it's better to have armed heroes before attempting realizing goals
	for(const CGTownInstance *t : cb->getTownsInfo())
		moveCreaturesToHero(t);

	try
	{
		//Pick objects reserved in previous turn - we expect only nerby objects there
		auto reservedHeroesCopy = reservedHeroesMap; //work on copy => the map may be changed while iterating (eg because hero died when attempting a goal)
		for (auto hero : reservedHeroesCopy)
		{
			if(reservedHeroesMap.count(hero.first))
				continue; //hero might have been removed while we were in this loop
			if(!hero.first.validAndSet())
			{
				logAi->errorStream() << "Hero " << hero.first.name << " present on reserved map. Shouldn't be. ";
				continue;
			}

			std::vector<const CGObjectInstance *> vec(hero.second.begin(), hero.second.end());
			boost::sort (vec, CDistanceSorter(hero.first.get()));
			for (auto obj : vec)
			{
				if(!obj || !cb->getObj(obj->id))
				{
					logAi->errorStream() << "Error: there is wrong object on list for hero " << hero.first->name;
					continue;
				}
				striveToGoal (sptr(Goals::VisitTile(obj->visitablePos()).sethero(hero.first)));
			}
		}

		//now try to win
		striveToGoal(sptr(Goals::Win()));

		//finally, continue our abstract long-term goals
		int oldMovement = 0;
		int newMovement = 0;
		while (true)
		{
			oldMovement = newMovement; //remember old value
			newMovement = 0;
			std::vector<std::pair<HeroPtr, Goals::TSubgoal> > safeCopy;
			for (auto mission : lockedHeroes)
			{
				fh->setPriority (mission.second); //re-evaluate
				if (canAct(mission.first))
				{
					newMovement += mission.first->movement;
					safeCopy.push_back (mission);
				}
			}
			if (newMovement == oldMovement) //means our heroes didn't move or didn't re-assign their goals
			{
				logAi->warnStream() << "Our heroes don't move anymore, exhaustive decomposition failed";
				break;
			}
			if (safeCopy.empty())
				break; //all heroes exhausted their locked goals
			else
			{
				typedef std::pair<HeroPtr, Goals::TSubgoal> TItrType;

				auto lockedHeroesSorter = [](TItrType m1, TItrType m2) -> bool
				{
					return m1.second->priority < m2.second->priority;
				};
				boost::sort(safeCopy, lockedHeroesSorter);
				striveToGoal (safeCopy.back().second);
			}
		}

		auto quests = myCb->getMyQuests();
		for (auto quest : quests)
		{
			striveToQuest (quest);
		}

		striveToGoal(sptr(Goals::Build())); //TODO: smarter building management
		performTypicalActions();

		//for debug purpose
		for (auto h : cb->getHeroesInfo())
		{
			if (h->movement)
				logAi->warnStream() << boost::format("hero %s has %d MP left") % h->name % h->movement;
		}
	}
	catch(boost::thread_interrupted &e)
	{
		logAi->debugStream() << "Making turn thread has been interrupted. We'll end without calling endTurn.";
		return;
	}
	catch(std::exception &e)
	{
		logAi->debugStream() << "Making turn thread has caught an exception: " << e.what();
	}

	endTurn();
}

bool VCAI::goVisitObj(const CGObjectInstance * obj, HeroPtr h)
{
	int3 dst = obj->visitablePos();
	auto sm = getCachedSectorMap(h);
	logAi->debugStream() << boost::format("%s will try to visit %s at (%s)") % h->name % obj->getObjectName() % strFromInt3(dst);
	int3 pos = sm->firstTileToGet(h, dst);
	if (!pos.valid()) //rare case when we are already standing on one of potential objects
		return false;
	return moveHeroToTile(pos, h);
}

void VCAI::performObjectInteraction(const CGObjectInstance * obj, HeroPtr h)
{
	LOG_TRACE_PARAMS(logAi, "Hero %s and object %s at %s", h->name % obj->getObjectName() % obj->pos);
	switch (obj->ID)
	{
		case Obj::CREATURE_GENERATOR1:
			recruitCreatures (dynamic_cast<const CGDwelling *>(obj), h.get());
			checkHeroArmy (h);
			break;
		case Obj::TOWN:
			moveCreaturesToHero (dynamic_cast<const CGTownInstance *>(obj));
			if (h->visitedTown) //we are inside, not just attacking
			{
				townVisitsThisWeek[h].insert(h->visitedTown);
				if (!h->hasSpellbook() && cb->getResourceAmount(Res::GOLD) >= GameConstants::SPELLBOOK_GOLD_COST + saving[Res::GOLD] &&
					h->visitedTown->hasBuilt (BuildingID::MAGES_GUILD_1))
					cb->buyArtifact(h.get(), ArtifactID::SPELLBOOK);
			}
			break;
	}
	completeGoal (sptr(Goals::GetObj(obj->id.getNum()).sethero(h)));
}

void VCAI::moveCreaturesToHero(const CGTownInstance * t)
{
	if(t->visitingHero && t->armedGarrison() && t->visitingHero->tempOwner == t->tempOwner)
	{
		pickBestCreatures (t->visitingHero, t);
	}
}

bool VCAI::canGetArmy (const CGHeroInstance * army, const CGHeroInstance * source)
{ //TODO: merge with pickBestCreatures
	//if (ai->primaryHero().h == source)

	if(army->tempOwner != source->tempOwner)
	{
		logAi->errorStream() << "Why are we even considering exchange between heroes from different players?";
		return false;
	}


	const CArmedInstance *armies[] = {army, source};

	//we calculate total strength for each creature type available in armies
	std::map<const CCreature*, int> creToPower;
	for(auto armyPtr : armies)
		for(auto &i : armyPtr->Slots())
		{
			//TODO: allow splitting stacks?
			creToPower[i.second->type] += i.second->getPower();
		}
	//TODO - consider more than just power (ie morale penalty, hero specialty in certain stacks, etc)
	int armySize = creToPower.size();
	armySize = std::min ((source->needsLastStack() ? armySize - 1 : armySize), GameConstants::ARMY_SIZE); //can't move away last stack
	std::vector<const CCreature *> bestArmy; //types that'll be in final dst army
	for (int i = 0; i < armySize; i++) //pick the creatures from which we can get most power, as many as dest can fit
	{
		typedef const std::pair<const CCreature*, int> &CrePowerPair;
		auto creIt = boost::max_element(creToPower, [](CrePowerPair lhs, CrePowerPair rhs)
			{
				return lhs.second < rhs.second;
			});
		bestArmy.push_back(creIt->first);
		creToPower.erase(creIt);
		if(creToPower.empty())
			break;
	}

	//foreach best type -> iterate over slots in both armies and if it's the appropriate type, send it to the slot where it belongs
	for (int i = 0; i < bestArmy.size(); i++) //i-th strongest creature type will go to i-th slot
	{
		for(auto armyPtr : armies)
			for (int j = 0; j < GameConstants::ARMY_SIZE; j++)
			{
				if(armyPtr->getCreature(SlotID(j)) == bestArmy[i]  &&  armyPtr != army) //it's a searched creature not in dst ARMY
				{
					//FIXME: line below is useless when simulating exchange between two non-singular armies
					if(!(armyPtr->needsLastStack() && armyPtr->stacksCount() == 1)) //can't take away last creature
						return true; //at least one exchange will be performed
					else
						return false; //no further exchange possible
				}
			}
	}
	return false;
}

void VCAI::pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source)
{
	//TODO - what if source is a hero (the last stack problem) -> it'd good to create a single stack of weakest cre
	const CArmedInstance *armies[] = {army, source};

	//we calculate total strength for each creature type available in armies
	std::map<const CCreature*, int> creToPower;
	for(auto armyPtr : armies)
		for(auto &i : armyPtr->Slots())
		{//TODO: allow splitting stacks?
			creToPower[i.second->type] += i.second->getPower();
		}
	//TODO - consider more than just power (ie morale penalty, hero specialty in certain stacks, etc)
	int armySize = creToPower.size();

	armySize = std::min ((source->needsLastStack() ? armySize - 1 : armySize), GameConstants::ARMY_SIZE); //can't move away last stack
	std::vector<const CCreature *> bestArmy; //types that'll be in final dst army
	for (int i = 0; i < armySize; i++) //pick the creatures from which we can get most power, as many as dest can fit
	{
		typedef const std::pair<const CCreature*, int> &CrePowerPair;
		auto creIt = boost::max_element(creToPower, [](CrePowerPair lhs, CrePowerPair rhs)
			{
				return lhs.second < rhs.second;
			});
		bestArmy.push_back(creIt->first);
		creToPower.erase(creIt);
		if(creToPower.empty())
			break;
	}

	//foreach best type -> iterate over slots in both armies and if it's the appropriate type, send it to the slot where it belongs
	for (int i = 0; i < bestArmy.size(); i++) //i-th strongest creature type will go to i-th slot
	{
		for(auto armyPtr : armies)
			for (int j = 0; j < GameConstants::ARMY_SIZE; j++)
			{
				if(armyPtr->getCreature(SlotID(j)) == bestArmy[i]  &&  (i != j || armyPtr != army)) //it's a searched creature not in dst SLOT
					if(!(armyPtr->needsLastStack() && armyPtr->stacksCount() == 1)) //can't take away last creature
						cb->mergeOrSwapStacks(armyPtr, army, SlotID(j), SlotID(i));
			}
	}

	//TODO - having now strongest possible army, we may want to think about arranging stacks

	auto hero = dynamic_cast<const CGHeroInstance *>(army);
	if (hero)
	{
		checkHeroArmy (hero);
	}
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
			if (giveStuffToFirstHero)
			{
				for (auto p : h->artifactsWorn)
				{
					if (p.second.artifact)
						allArtifacts.push_back(ArtifactLocation(h, p.first));
				}
			}
			for (auto slot : h->artifactsInBackpack)
				allArtifacts.push_back(ArtifactLocation(h, h->getArtPos(slot.artifact)));

			if (otherh)
			{
				for (auto p : otherh->artifactsWorn)
				{
					if (p.second.artifact)
						allArtifacts.push_back(ArtifactLocation(otherh, p.first));
				}
				for (auto slot : otherh->artifactsInBackpack)
					allArtifacts.push_back(ArtifactLocation(otherh, otherh->getArtPos(slot.artifact)));
			}
			//we give stuff to one hero or another, depending on giveStuffToFirstHero

			const CGHeroInstance * target = nullptr;
			if (giveStuffToFirstHero)
				target = h;
			else
				target = otherh;

			for (auto location : allArtifacts)
			{
				if (location.relatedObj() == target && location.slot < ArtifactPosition::AFTER_LAST)
					continue; //don't reequip artifact we already wear

				if(location.slot == ArtifactPosition::MACH4) // don't attempt to move catapult
					continue;

				auto s = location.getSlot();
				if (!s || s->locked) //we can't move locks
					continue;
				auto artifact = s->artifact;
				if (!artifact)
					continue;
				//FIXME: why are the above possible to be null?

				bool emptySlotFound = false;
				for (auto slot : artifact->artType->possibleSlots.at(target->bearerType()))
				{
					ArtifactLocation destLocation(target, slot);
					if (target->isPositionFree(slot) && artifact->canBePutAt(destLocation, true)) //combined artifacts are not always allowed to move
					{
						cb->swapArtifacts(location, destLocation); //just put into empty slot
						emptySlotFound = true;
						changeMade = true;
						break;
					}
				}
				if (!emptySlotFound) //try to put that atifact in already occupied slot
				{
					for (auto slot : artifact->artType->possibleSlots.at(target->bearerType()))
					{
						auto otherSlot = target->getSlot(slot);
						if (otherSlot && otherSlot->artifact) //we need to exchange artifact for better one
						{
							ArtifactLocation destLocation(target, slot);
							//if that artifact is better than what we have, pick it
							if (compareArtifacts(artifact, otherSlot->artifact) && artifact->canBePutAt(destLocation, true)) //combined artifacts are not always allowed to move
							{
								cb->swapArtifacts(location, ArtifactLocation(target, target->getArtPos(otherSlot->artifact)));
								break;
								changeMade = true;
							}
						}
					}
				}
				if (changeMade)
					break; //start evaluating artifacts from scratch
			}
		} while (changeMade);
	};

	equipBest (h, other, true);

	if (other)
	{
		equipBest(h, other, false);
	}

}

void VCAI::recruitCreatures(const CGDwelling * d, const CArmedInstance * recruiter)
{
	for(int i = 0; i < d->creatures.size(); i++)
	{
		if(!d->creatures[i].second.size())
			continue;

		int count = d->creatures[i].first;
		CreatureID creID = d->creatures[i].second.back();
//		const CCreature *c = VLC->creh->creatures[creID];
// 		if(containsSavedRes(c->cost))
// 			continue;

		vstd::amin(count, freeResources() / VLC->creh->creatures[creID]->cost);
		if(count > 0)
			cb->recruitCreatures(d, recruiter, creID, count, i);
	}
}

bool VCAI::tryBuildStructure(const CGTownInstance * t, BuildingID building, unsigned int maxDays)
{
	if (maxDays == 0)
	{
		logAi->warnStream() << "Request to build building " << building <<  " in 0 days!";
		return false;
	}

	if (!vstd::contains(t->town->buildings, building))
		return false; // no such building in town

	if (t->hasBuilt(building)) //Already built? Shouldn't happen in general
		return true;

	const CBuilding * buildPtr = t->town->buildings.at(building);

	auto toBuild = buildPtr->requirements.getFulfillmentCandidates([&](const BuildingID & buildID)
	{
		return t->hasBuilt(buildID);
	});
	toBuild.push_back(building);

	for(BuildingID buildID : toBuild)
	{
		EBuildingState::EBuildingState canBuild = cb->canBuildStructure(t, buildID);
		if (canBuild == EBuildingState::HAVE_CAPITAL
		 || canBuild == EBuildingState::FORBIDDEN
		 || canBuild == EBuildingState::NO_WATER)
			return false; //we won't be able to build this
	}

	if (maxDays && toBuild.size() > maxDays)
		return false;

	TResources currentRes = cb->getResourceAmount();
	//TODO: calculate if we have enough resources to build it in maxDays

	for(const auto & buildID : toBuild)
	{
		const CBuilding *b = t->town->buildings.at(buildID);

		EBuildingState::EBuildingState canBuild = cb->canBuildStructure(t, buildID);
		if(canBuild == EBuildingState::ALLOWED)
		{
			if(!containsSavedRes(b->resources))
			{
				logAi->debugStream() << boost::format("Player %d will build %s in town of %s at %s") % playerID % b->Name() % t->name % t->pos;
				cb->buildBuilding(t, buildID);
				return true;
			}
			continue;
		}
		else if(canBuild == EBuildingState::NO_RESOURCES)
		{
			//TResources income = estimateIncome();
			TResources cost = t->town->buildings.at(buildID)->resources;
			for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
			{
				//int diff = currentRes[i] - cost[i] + income[i];
				int diff = currentRes[i] - cost[i];
				if(diff < 0)
					saving[i] = 1;
			}
			continue;
		}
		else if (canBuild == EBuildingState::PREREQUIRES)
		{
			// can happen when dependencies have their own missing dependencies
			if (tryBuildStructure(t, buildID, maxDays - 1))
				return true;
		}
		else if (canBuild == EBuildingState::MISSING_BASE)
		{
			if (tryBuildStructure(t, b->upgrade, maxDays - 1))
				 return true;
		}
	}
	return false;
}

//bool VCAI::canBuildStructure(const CGTownInstance * t, BuildingID building, unsigned int maxDays=7)
//{
//		if (maxDays == 0)
//	{
//		logAi->warnStream() << "Request to build building " << building <<  " in 0 days!";
//		return false;
//	}
//
//	if (!vstd::contains(t->town->buildings, building))
//		return false; // no such building in town
//
//	if (t->hasBuilt(building)) //Already built? Shouldn't happen in general
//		return true;
//
//	const CBuilding * buildPtr = t->town->buildings.at(building);
//
//	auto toBuild = buildPtr->requirements.getFulfillmentCandidates([&](const BuildingID & buildID)
//	{
//		return t->hasBuilt(buildID);
//	});
//	toBuild.push_back(building);
//
//	for(BuildingID buildID : toBuild)
//	{
//		EBuildingState::EBuildingState canBuild = cb->canBuildStructure(t, buildID);
//		if (canBuild == EBuildingState::HAVE_CAPITAL
//		 || canBuild == EBuildingState::FORBIDDEN
//		 || canBuild == EBuildingState::NO_WATER)
//			return false; //we won't be able to build this
//	}
//
//	if (maxDays && toBuild.size() > maxDays)
//		return false;
//
//	TResources currentRes = cb->getResourceAmount();
//	TResources income = estimateIncome();
//	//TODO: calculate if we have enough resources to build it in maxDays
//
//	for(const auto & buildID : toBuild)
//	{
//		const CBuilding *b = t->town->buildings.at(buildID);
//
//		EBuildingState::EBuildingState canBuild = cb->canBuildStructure(t, buildID);
//		if(canBuild == EBuildingState::ALLOWED)
//		{
//			if(!containsSavedRes(b->resources))
//			{
//				logAi->debugStream() << boost::format("Player %d will build %s in town of %s at %s") % playerID % b->Name() % t->name % t->pos;
//				return true;
//			}
//			continue;
//		}
//		else if(canBuild == EBuildingState::NO_RESOURCES)
//		{
//			TResources cost = t->town->buildings.at(buildID)->resources;
//			for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
//			{
//				int diff = currentRes[i] - cost[i] + income[i];
//				if(diff < 0)
//					saving[i] = 1;
//			}
//			continue;
//		}
//		else if (canBuild == EBuildingState::PREREQUIRES)
//		{
//			// can happen when dependencies have their own missing dependencies
//			if (canBuildStructure(t, buildID, maxDays - 1))
//				return true;
//		}
//		else if (canBuild == EBuildingState::MISSING_BASE)
//		{
//			if (canBuildStructure(t, b->upgrade, maxDays - 1))
//				 return true;
//		}
//	}
//	return false;
//}

bool VCAI::tryBuildAnyStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays)
{
	for(const auto & building : buildList)
	{
		if(t->hasBuilt(building))
			continue;
		if (tryBuildStructure(t, building, maxDays))
			return true;
	}
	return false; //Can't build anything
}

BuildingID VCAI::canBuildAnyStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays)
{
	for(const auto & building : buildList)
	{
		if(t->hasBuilt(building))
			continue;
		if (cb->canBuildStructure(t, building))
			return building;
	}
	return BuildingID::NONE; //Can't build anything
}

bool VCAI::tryBuildNextStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays)
{
	for(const auto & building : buildList)
	{
		if(t->hasBuilt(building))
			continue;
		return tryBuildStructure(t, building, maxDays);
	}
	return false;//Nothing to build
}

//Set of buildings for different goals. Does not include any prerequisites.
static const BuildingID essential[] = {BuildingID::TAVERN, BuildingID::TOWN_HALL};
static const BuildingID goldSource[] = {BuildingID::TOWN_HALL, BuildingID::CITY_HALL, BuildingID::CAPITOL};
static const BuildingID unitsSource[] = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3,
	BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7};
static const BuildingID unitsUpgrade[] = { BuildingID::DWELL_LVL_1_UP, BuildingID::DWELL_LVL_2_UP, BuildingID::DWELL_LVL_3_UP,
	BuildingID::DWELL_LVL_4_UP, BuildingID::DWELL_LVL_5_UP, BuildingID::DWELL_LVL_6_UP, BuildingID::DWELL_LVL_7_UP};
static const BuildingID unitGrowth[] = { BuildingID::FORT, BuildingID::CITADEL, BuildingID::CASTLE, BuildingID::HORDE_1,
	BuildingID::HORDE_1_UPGR, BuildingID::HORDE_2, BuildingID::HORDE_2_UPGR};
static const BuildingID spells[] = {BuildingID::MAGES_GUILD_1, BuildingID::MAGES_GUILD_2, BuildingID::MAGES_GUILD_3,
	BuildingID::MAGES_GUILD_4, BuildingID::MAGES_GUILD_5};
static const BuildingID extra[] = {BuildingID::RESOURCE_SILO, BuildingID::SPECIAL_1, BuildingID::SPECIAL_2, BuildingID::SPECIAL_3,
	BuildingID::SPECIAL_4, BuildingID::SHIPYARD}; // all remaining buildings

void VCAI::buildStructure(const CGTownInstance * t)
{
	//TODO make *real* town development system
	//TODO: faction-specific development: use special buildings, build dwellings in better order, etc
	//TODO: build resource silo, defences when needed
	//Possible - allow "locking" on specific building (build prerequisites and then building itself)

	TResources currentRes = cb->getResourceAmount();
	TResources currentIncome = t->dailyIncome();
	int townIncome = currentIncome[Res::GOLD];

	if (tryBuildAnyStructure(t, std::vector<BuildingID>(essential, essential + ARRAY_COUNT(essential))))
		return;

	//we're running out of gold - try to build something gold-producing. Multiplier can be tweaked, 6 is minimum due to buildings costs
	if (currentRes[Res::GOLD] < townIncome * 6)
		if (tryBuildNextStructure(t, std::vector<BuildingID>(goldSource, goldSource + ARRAY_COUNT(goldSource))))
			return;

	if (cb->getDate(Date::DAY_OF_WEEK) > 6)// last 2 days of week - try to focus on growth
	{
		if (tryBuildNextStructure(t, std::vector<BuildingID>(unitGrowth, unitGrowth + ARRAY_COUNT(unitGrowth)), 2))
			return;
	}

	// first in-game week or second half of any week: try build dwellings
	if (cb->getDate(Date::DAY) < 7 || cb->getDate(Date::DAY_OF_WEEK) > 3)
		if (tryBuildAnyStructure(t, std::vector<BuildingID>(unitsSource, unitsSource + ARRAY_COUNT(unitsSource)), 8 - cb->getDate(Date::DAY_OF_WEEK)))
			return;

	//try to upgrade dwelling
	for(int i = 0; i < ARRAY_COUNT(unitsUpgrade); i++)
	{
		if (t->hasBuilt(unitsSource[i]) && !t->hasBuilt(unitsUpgrade[i]))
		{
			if (tryBuildStructure(t, unitsUpgrade[i]))
				return;
		}
	}

	//remaining tasks
	if (tryBuildNextStructure(t, std::vector<BuildingID>(goldSource, goldSource + ARRAY_COUNT(goldSource))))
		return;
	if (tryBuildNextStructure(t, std::vector<BuildingID>(spells, spells + ARRAY_COUNT(spells))))
		return;
	if (tryBuildAnyStructure(t, std::vector<BuildingID>(extra, extra + ARRAY_COUNT(extra))))
		return;
}

bool VCAI::isGoodForVisit(const CGObjectInstance *obj, HeroPtr h, SectorMap &sm)
{
	const int3 pos = obj->visitablePos();
	const int3 targetPos = sm.firstTileToGet(h, pos);
	if (!targetPos.valid())
		return false;
	if (isTileNotReserved(h.get(), targetPos) &&
			!obj->wasVisited(playerID) &&
			(cb->getPlayerRelations(ai->playerID, obj->tempOwner) == PlayerRelations::ENEMIES || isWeeklyRevisitable(obj)) && //flag or get weekly resources / creatures
			isSafeToVisit(h, pos) &&
			shouldVisit(h, obj) &&
			!vstd::contains(alreadyVisited, obj) &&
			!vstd::contains(reservedObjs, obj) &&
			isAccessibleForHero(targetPos, h))
	{
		const CGObjectInstance *topObj = cb->getVisitableObjs(obj->visitablePos()).back(); //it may be hero visiting this obj
		//we don't try visiting object on which allied or owned hero stands
		// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
		if (topObj->ID == Obj::HERO  &&  cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
			return false;
		else
			return true; //all of the following is met
	}

	return false;
}

std::vector<const CGObjectInstance *> VCAI::getPossibleDestinations(HeroPtr h)
{
	validateVisitableObjs();
	std::vector<const CGObjectInstance *> possibleDestinations;
	auto sm = getCachedSectorMap(h);
	for(const CGObjectInstance *obj : visitableObjs)
	{
		if (isGoodForVisit(obj, h, *sm))
		{
			possibleDestinations.push_back(obj);
		}
	}

	boost::sort(possibleDestinations, CDistanceSorter(h.get()));

	return possibleDestinations;
}

bool VCAI::isTileNotReserved(const CGHeroInstance * h, int3 t)
{
	if (t.valid())
	{
		auto obj = cb->getTopObj(t);
		if (obj && vstd::contains(ai->reservedObjs, obj) && !vstd::contains(reservedHeroesMap[h], obj))
			return false; //do not capture object reserved by another hero
		else
			return true;
	}
	else
		return false;
}

bool VCAI::canRecruitAnyHero (const CGTownInstance * t) const
{
	//TODO: make gathering gold, building tavern or conquering town (?) possible subgoals
	if (!t)
		t = findTownWithTavern();
	if (t)
		return cb->getResourceAmount(Res::GOLD) >= GameConstants::HERO_GOLD_COST &&
			cb->getHeroesInfo().size() < ALLOWED_ROAMING_HEROES &&
			cb->getAvailableHeroes(t).size();
	else
		return false;
}

void VCAI::wander(HeroPtr h)
{
	//unclaim objects that are now dangerous for us
	auto reservedObjsSetCopy = reservedHeroesMap[h];
	for (auto obj : reservedObjsSetCopy)
	{
		if (!isSafeToVisit(h, obj->visitablePos()))
			unreserveObject(h, obj);
	}

	TimeCheck tc("looking for wander destination");

	while (h->movement)
	{
		validateVisitableObjs();
		std::vector <ObjectIdRef> dests, tmp;

		auto sm = getCachedSectorMap(h);

		range::copy(reservedHeroesMap[h], std::back_inserter(tmp)); //also visit our reserved objects - but they are not prioritized to avoid running back and forth
		for (auto obj : tmp)
		{
			int3 pos = sm->firstTileToGet(h, obj->visitablePos());
			if (pos.valid())
				if (isAccessibleForHero (pos, h)) //even nearby objects could be blocked by other heroes :(
					dests.push_back(obj); //can't use lambda for member function :(
		}

		range::copy(getPossibleDestinations(h), std::back_inserter(dests));
		vstd::erase_if(dests, [&](ObjectIdRef obj) -> bool
		{
			return !isSafeToVisit(h, sm->firstTileToGet(h, obj->visitablePos()));
		});

		if(!dests.size())
		{
			if (cb->getVisitableObjs(h->visitablePos()).size() > 1)
				moveHeroToTile(h->visitablePos(), h); //just in case we're standing on blocked subterranean gate

			auto compareReinforcements = [h](const CGTownInstance *lhs, const CGTownInstance *rhs) -> bool
			{
				return howManyReinforcementsCanGet(h, lhs) < howManyReinforcementsCanGet(h, rhs);
			};

			std::vector<const CGTownInstance *> townsReachable;
			std::vector<const CGTownInstance *> townsNotReachable;
			for(const CGTownInstance *t : cb->getTownsInfo())
			{
				if(!t->visitingHero && howManyReinforcementsCanGet(h,t) && !vstd::contains(townVisitsThisWeek[h], t))
				{
					if (isAccessibleForHero (t->visitablePos(), h))
						townsReachable.push_back(t);
					else
						townsNotReachable.push_back(t);
				}
			}
			if(townsReachable.size())
			{
				boost::sort(townsReachable, compareReinforcements);
				dests.push_back(townsReachable.back());
			}
			else if(townsNotReachable.size())
			{
				boost::sort(townsNotReachable, compareReinforcements);
				//TODO pick the truly best
				const CGTownInstance *t = townsNotReachable.back();
				logAi->debugStream() << boost::format("%s can't reach any town, we'll try to make our way to %s at %s") % h->name % t->name % t->visitablePos();
				int3 pos1 = h->pos;
				striveToGoal(sptr(Goals::ClearWayTo(t->visitablePos()).sethero(h)));
				//if out hero is stuck, we may need to request another hero to clear the way we see

				if (pos1 == h->pos && h == primaryHero()) //hero can't move
				{
					if (canRecruitAnyHero(t))
						recruitHero(t);
				}
				break;
			}
			else if(cb->getResourceAmount(Res::GOLD) >= GameConstants::HERO_GOLD_COST)
			{
				std::vector<const CGTownInstance *> towns = cb->getTownsInfo();
				vstd::erase_if(towns, [](const CGTownInstance *t) -> bool
				{
					for(const CGHeroInstance *h : cb->getHeroesInfo())
					if(!t->getArmyStrength() || howManyReinforcementsCanGet(h, t))
						return true;
					return false;
				});
				boost::sort(towns, compareArmyStrength);
				if(towns.size())
					recruitHero(towns.back());
				break;
			}
			else
			{
				logAi->debugStream() << "Nowhere more to go...";
				break;
			}
		}
		//end of objs empty

		if (dests.size()) //performance improvement
		{
			boost::sort(dests, CDistanceSorter(h.get())); //find next closest one

			//wander should not cause heroes to be reserved - they are always considered free
			const ObjectIdRef&dest = dests.front();
			logAi->debugStream() << boost::format("Of all %d destinations, object oid=%d seems nice") % dests.size() % dest.id.getNum();
			if(!goVisitObj(dest, h))
			{
				if(!dest)
				{
					logAi->debugStream() << boost::format("Visit attempt made the object (id=%d) gone...") % dest.id.getNum();
				}
				else
				{
					logAi->debugStream() << boost::format("Hero %s apparently used all MPs (%d left)") % h->name % h->movement;
					return;
				}
			}
		}

		if (h->visitedTown)
		{
			townVisitsThisWeek[h].insert(h->visitedTown);
			buildArmyIn(h->visitedTown);
		}
	}
}

void VCAI::setGoal(HeroPtr h, Goals::TSubgoal goal)
{ //TODO: check for presence?
	if(goal->invalid())
		vstd::erase_if_present(lockedHeroes, h);
	else
	{
		lockedHeroes[h] = goal;
		goal->setisElementar(false); //always evaluate goals before realizing
	}
}

void VCAI::completeGoal (Goals::TSubgoal goal)
{
	logAi->traceStream() << boost::format("Completing goal: %s") % goal->name();
	if (const CGHeroInstance * h = goal->hero.get(true))
	{
		auto it = lockedHeroes.find(h);
		if (it != lockedHeroes.end())
			if (it->second == goal)
			{
				logAi->debugStream() << boost::format("%s") % goal->completeMessage();
				lockedHeroes.erase(it); //goal fulfilled, free hero
			}
	}
	else //complete goal for all heroes maybe?
	{
		vstd::erase_if(lockedHeroes, [goal](std::pair<HeroPtr, Goals::TSubgoal> p)
		{
			if (*(p.second) == *goal || p.second->fulfillsMe(goal)) //we could have fulfilled goals of other heroes by chance
			{
				logAi->debugStream() << boost::format("%s") % p.second->completeMessage();
				return true;
			}
			return false;
		});
	}

}

void VCAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	NET_EVENT_HANDLER;
	assert(playerID > PlayerColor::PLAYER_LIMIT || status.getBattle() == UPCOMING_BATTLE);
	status.setBattle(ONGOING_BATTLE);
	const CGObjectInstance *presumedEnemy = vstd::backOrNull(cb->getVisitableObjs(tile)); //may be nullptr in some very are cases -> eg. visited monolith and fighting with an enemy at the FoW covered exit
	battlename = boost::str(boost::format("Starting battle of %s attacking %s at %s") % (hero1 ? hero1->name : "a army") % (presumedEnemy ? presumedEnemy->getObjectName() : "unknown enemy") % tile);
	CAdventureAI::battleStart(army1, army2, tile, hero1, hero2, side);
}

void VCAI::battleEnd(const BattleResult *br)
{
	NET_EVENT_HANDLER;
	assert(status.getBattle() == ONGOING_BATTLE);
	status.setBattle(ENDING_BATTLE);
	bool won = br->winner == myCb->battleGetMySide();
	logAi->debugStream() << boost::format("Player %d: I %s the %s!") % playerID % (won  ? "won" : "lost") % battlename;
	battlename.clear();
	CAdventureAI::battleEnd(br);
}

void VCAI::waitTillFree()
{
	auto unlock = vstd::makeUnlockSharedGuard(cb->getGsMutex());
	status.waitTillFree();
}

void VCAI::markObjectVisited (const CGObjectInstance *obj)
{
	if(dynamic_cast<const CGVisitableOPH *>(obj) || //we may want to wisit it with another hero
		dynamic_cast<const CGBonusingObject *>(obj) || //or another time
		(obj->ID == Obj::MONSTER))
		return;
	alreadyVisited.insert(obj);
}

void VCAI::reserveObject(HeroPtr h, const CGObjectInstance *obj)
{
	reservedObjs.insert(obj);
	reservedHeroesMap[h].insert(obj);
	logAi->debugStream() << "reserved object id=" << obj->id << "; address=" << (intptr_t)obj << "; name=" << obj->getObjectName();
}

void VCAI::unreserveObject(HeroPtr h, const CGObjectInstance *obj)
{
	vstd::erase_if_present(reservedObjs, obj); //unreserve objects
	vstd::erase_if_present(reservedHeroesMap[h], obj);
}

void VCAI::markHeroUnableToExplore (HeroPtr h)
{
	heroesUnableToExplore.insert(h);
}
void VCAI::markHeroAbleToExplore (HeroPtr h)
{
	vstd::erase_if_present(heroesUnableToExplore, h);
}
bool VCAI::isAbleToExplore (HeroPtr h)
{
	return !vstd::contains (heroesUnableToExplore, h);
}
void VCAI::clearPathsInfo()
{
	heroesUnableToExplore.clear();
	cachedSectorMaps.clear();
}

void VCAI::validateVisitableObjs()
{
	std::vector<const CGObjectInstance *> hlp;
	retreiveVisitableObjs(hlp, true);

	std::string errorMsg;
	auto shouldBeErased = [&](const CGObjectInstance *obj) -> bool
	{
		if (obj)
			return !cb->getObj(obj->id, false); // no verbose output needed as we check object visibility
		else
			return true;

		//why would we have our local logic for object checks? use cb!

		//if(!vstd::contains(hlp, obj))
		//{
		//	logAi->errorStream() << helperObjInfo[obj].name << " at " << helperObjInfo[obj].pos << errorMsg;
		//	return true;
		//}
		//return false;
	};

	//errorMsg is captured by ref so lambda will take the new text
	errorMsg = " shouldn't be on the visitable objects list!";
	vstd::erase_if(visitableObjs, shouldBeErased);

	//FIXME: how comes our own heroes become inaccessible?
	vstd::erase_if(reservedHeroesMap, [](std::pair<HeroPtr, std::set<const CGObjectInstance *>> hp) -> bool
	{
		return !hp.first.get(true);
	});
	for(auto &p : reservedHeroesMap)
	{
		errorMsg = " shouldn't be on list for hero " + p.first->name + "!";
		vstd::erase_if(p.second, shouldBeErased);
	}

	errorMsg = " shouldn't be on the reserved objs list!";
	vstd::erase_if(reservedObjs, shouldBeErased);

	//TODO overkill, hidden object should not be removed. However, we can't know if hidden object is erased from game.
	errorMsg = " shouldn't be on the already visited objs list!";
	vstd::erase_if(alreadyVisited, shouldBeErased);
}

void VCAI::retreiveVisitableObjs(std::vector<const CGObjectInstance *> &out, bool includeOwned /*= false*/) const
{
	foreach_tile_pos([&](const int3 &pos)
	{
		for(const CGObjectInstance *obj : myCb->getVisitableObjs(pos, false))
		{
			if(includeOwned || obj->tempOwner != playerID)
				out.push_back(obj);
		}
	});
}

void VCAI::retreiveVisitableObjs()
{
	foreach_tile_pos([&](const int3 &pos)
	{
		for(const CGObjectInstance *obj : myCb->getVisitableObjs(pos, false))
		{
			if(obj->tempOwner != playerID)
				addVisitableObj(obj);
		}
	});
}

std::vector<const CGObjectInstance *> VCAI::getFlaggedObjects() const
{
	std::vector<const CGObjectInstance *> ret;
	retreiveVisitableObjs(ret, true);
	vstd::erase_if(ret, [](const CGObjectInstance *obj)
	{
		return obj->tempOwner != ai->playerID;
	});
	return ret;
}

void VCAI::addVisitableObj(const CGObjectInstance *obj)
{
	visitableObjs.insert(obj);
	helperObjInfo[obj] = ObjInfo(obj);

	// All teleport objects seen automatically assigned to appropriate channels
	auto teleportObj = dynamic_cast<const CGTeleport *>(obj);
	if(teleportObj)
		CGTeleport::addToChannel(knownTeleportChannels, teleportObj);
}

const CGObjectInstance * VCAI::lookForArt(int aid) const
{
	for(const CGObjectInstance *obj : ai->visitableObjs)
	{
		if(obj->ID == Obj::ARTIFACT && obj->subID == aid)
			return obj;
	}

	return nullptr;

	//TODO what if more than one artifact is available? return them all or some slection criteria
}

bool VCAI::isAccessible(const int3 &pos)
{
	//TODO precalculate for speed

	for(const CGHeroInstance *h : cb->getHeroesInfo())
	{
		if(isAccessibleForHero(pos, h))
			return true;
	}

	return false;
}

HeroPtr VCAI::getHeroWithGrail() const
{
	for(const CGHeroInstance *h : cb->getHeroesInfo())
		if(h->hasArt(2)) //grail
			return h;

	return nullptr;
}

const CGObjectInstance * VCAI::getUnvisitedObj(const std::function<bool(const CGObjectInstance *)> &predicate)
{
	//TODO smarter definition of unvisited
	for(const CGObjectInstance *obj : visitableObjs)
		if(predicate(obj) && !vstd::contains(alreadyVisited, obj))
			return obj;

	return nullptr;
}

bool VCAI::isAccessibleForHero(const int3 & pos, HeroPtr h, bool includeAllies /*= false*/) const
{
	if (!includeAllies)
	{ //don't visit tile occupied by allied hero
		for (auto obj : cb->getVisitableObjs(pos))
		{
			if (obj->ID == Obj::HERO &&
				cb->getPlayerRelations(ai->playerID, obj->tempOwner) != PlayerRelations::ENEMIES &&
				obj != h.get())
				return false;
		}
	}
	return cb->getPathsInfo(h.get())->getPathInfo(pos)->reachable();
}

bool VCAI::moveHeroToTile(int3 dst, HeroPtr h)
{
	auto afterMovementCheck = [&]() -> void
	{
		waitTillFree(); //movement may cause battle or blocking dialog
		if(!h)
		{
			lostHero(h);
			teleportChannelProbingList.clear();
			if (status.channelProbing()) // if hero lost during channel probing we need to switch this mode off
				status.setChannelProbing(false);
			throw cannotFulfillGoalException("Hero was lost!");
		}
	};

	logAi->debugStream() << boost::format("Moving hero %s to tile %s") % h->name % dst;
	int3 startHpos = h->visitablePos();
	bool ret = false;
	if(startHpos == dst)
	{
		//FIXME: this assertion fails also if AI moves onto defeated guarded object
		assert(cb->getVisitableObjs(dst).size() > 1); //there's no point in revisiting tile where there is no visitable object
		cb->moveHero(*h, CGHeroInstance::convertPosition(dst, true));
		afterMovementCheck();// TODO: is it feasible to hero get killed there if game work properly?
							 // not sure if AI can currently reconsider to attack bank while staying on it. Check issue 2084 on mantis for more information.
		ret = true;
	}
	else
	{
		CGPath path;
		cb->getPathsInfo(h.get())->getPath(path, dst);
		if(path.nodes.empty())
		{
			logAi->errorStream() << "Hero " << h->name << " cannot reach " << dst;
			throw goalFulfilledException (sptr(Goals::VisitTile(dst).sethero(h)));
		}
		int i = path.nodes.size()-1;

		auto getObj = [&](int3 coord, bool ignoreHero)
		{
			auto tile = cb->getTile(coord, false);
			assert(tile);
			return tile->topVisitableObj(ignoreHero);
			//return cb->getTile(coord,false)->topVisitableObj(ignoreHero);
		};

		auto isTeleportAction = [&](CGPathNode::ENodeAction action) -> bool
		{
			if(action != CGPathNode::TELEPORT_NORMAL &&
				action != CGPathNode::TELEPORT_BLOCKING_VISIT &&
				action != CGPathNode::TELEPORT_BATTLE)
			{
				return false;
			}

			return true;
		};

		auto getDestTeleportObj = [&](const CGObjectInstance * currentObject, const CGObjectInstance * nextObjectTop, const CGObjectInstance * nextObject) -> const CGObjectInstance *
		{
			if(CGTeleport::isConnected(currentObject, nextObjectTop))
				return nextObjectTop;
			if(nextObjectTop && nextObjectTop->ID == Obj::HERO &&
				CGTeleport::isConnected(currentObject, nextObject))
			{
				return nextObject;
			}

			return nullptr;
		};

		auto doMovement = [&](int3 dst, bool transit)
		{
			cb->moveHero(*h, CGHeroInstance::convertPosition(dst, true), transit);
		};

		auto doTeleportMovement = [&](ObjectInstanceID exitId, int3 exitPos)
		{
			destinationTeleport = exitId;
			if(exitPos.valid())
				destinationTeleportPos = CGHeroInstance::convertPosition(exitPos, true);
			cb->moveHero(*h, h->pos);
			destinationTeleport = ObjectInstanceID();
			destinationTeleportPos = int3(-1);
			afterMovementCheck();
		};

		auto doChannelProbing = [&]() -> void
		{
			auto currentPos = CGHeroInstance::convertPosition(h->pos,false);
			auto currentExit = getObj(currentPos, true)->id;

			status.setChannelProbing(true);
			for(auto exit : teleportChannelProbingList)
				doTeleportMovement(exit, int3(-1));
			teleportChannelProbingList.clear();
			status.setChannelProbing(false);

			doTeleportMovement(currentExit, currentPos);
		};

		for(; i>0; i--)
		{
			int3 currentCoord = path.nodes[i].coord;
			int3 nextCoord = path.nodes[i-1].coord;

			auto currentObject = getObj(currentCoord, currentCoord == CGHeroInstance::convertPosition(h->pos,false));
			auto nextObjectTop = getObj(nextCoord, false);
			auto nextObject = getObj(nextCoord, true);
			auto destTeleportObj = getDestTeleportObj(currentObject, nextObjectTop, nextObject);
			if(isTeleportAction(path.nodes[i-1].action) && destTeleportObj != nullptr)
			{ //we use special login if hero standing on teleporter it's mean we need
				doTeleportMovement(destTeleportObj->id, nextCoord);
				if(teleportChannelProbingList.size())
					doChannelProbing();

				continue;
			}

			//stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
			if(path.nodes[i-1].turns)
			{
				//blockedHeroes.insert(h); //to avoid attempts of moving heroes with very little MPs
				break;
			}

			int3 endpos = path.nodes[i-1].coord;
			if(endpos == h->visitablePos())
				continue;

			if((i-2 >= 0) // Check there is node after next one; otherwise transit is pointless
				&& (CGTeleport::isConnected(nextObjectTop, getObj(path.nodes[i-2].coord, false))
					|| CGTeleport::isTeleport(nextObjectTop)))
			{ // Hero should be able to go through object if it's allow transit
				doMovement(endpos, true);
			}
			else if(path.nodes[i-1].layer == EPathfindingLayer::AIR)
				doMovement(endpos, true);
			else
				doMovement(endpos, false);

			afterMovementCheck();

			if(teleportChannelProbingList.size())
				doChannelProbing();
		}
	}
	if (h)
	{
		if(auto visitedObject = vstd::frontOrNull(cb->getVisitableObjs(h->visitablePos()))) //we stand on something interesting
		{
			if (visitedObject != *h)
				performObjectInteraction (visitedObject, h);
		}
	}
	if(h) //we could have lost hero after last move
	{
		completeGoal (sptr(Goals::VisitTile(dst).sethero(h))); //we stepped on some tile, anyway
		completeGoal (sptr(Goals::ClearWayTo(dst).sethero(h)));

		ret = (dst == h->visitablePos());

		if(!ret) //reserve object we are heading towards
		{
			auto obj = vstd::frontOrNull(cb->getVisitableObjs(dst));
			if(obj && obj != *h)
				reserveObject(h, obj);
		}

		if(startHpos == h->visitablePos() && !ret) //we didn't move and didn't reach the target
		{
			vstd::erase_if_present(lockedHeroes, h); //hero seemingly is confused
			throw cannotFulfillGoalException("Invalid path found!"); //FIXME: should never happen
		}
		logAi->debugStream() << boost::format("Hero %s moved from %s to %s. Returning %d.") % h->name % startHpos % h->visitablePos() % ret;
	}
	return ret;
}
void VCAI::tryRealize(Goals::Explore & g)
{
	throw cannotFulfillGoalException("EXPLORE is not an elementar goal!");
}

void VCAI::tryRealize(Goals::RecruitHero & g)
{
	if(const CGTownInstance *t = findTownWithTavern())
	{
		recruitHero(t, true);
		//TODO try to free way to blocked town
		//TODO: adventure map tavern or prison?
	}
}

void VCAI::tryRealize(Goals::VisitTile & g)
{
	if(!g.hero->movement)
		throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
	if(g.tile == g.hero->visitablePos()  &&  cb->getVisitableObjs(g.hero->visitablePos()).size() < 2)
	{
		logAi->warnStream() << boost::format("Why do I want to move hero %s to tile %s? Already standing on that tile! ")
												% g.hero->name % g.tile;
		throw goalFulfilledException (sptr(g));
	}
	if (ai->moveHeroToTile(g.tile, g.hero.get()))
	{
		throw goalFulfilledException (sptr(g));
	}
}

void VCAI::tryRealize(Goals::VisitHero & g)
{
	if(!g.hero->movement)
		throw cannotFulfillGoalException("Cannot visit target hero: hero is out of MPs!");

	const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(g.objid));
	if (obj)
	{
		if (ai->moveHeroToTile(obj->visitablePos(), g.hero.get()))
		{
			throw goalFulfilledException (sptr(g));
		}
	}
	else
		throw cannotFulfillGoalException("Cannot visit hero: object not found!");
}

void VCAI::tryRealize(Goals::BuildThis & g)
{
	const CGTownInstance *t = g.town;

	if(!t && g.hero)
		t = g.hero->visitedTown;

	if(!t)
	{
		for(const CGTownInstance *t : cb->getTownsInfo())
		{
			switch(cb->canBuildStructure(t, BuildingID(g.bid)))
			{
			case EBuildingState::ALLOWED:
				cb->buildBuilding(t, BuildingID(g.bid));
				return;
			default:
				break;
			}
		}
	}
	else if(cb->canBuildStructure(t, BuildingID(g.bid)) == EBuildingState::ALLOWED)
	{
		cb->buildBuilding(t, BuildingID(g.bid));
		return;
	}
	throw cannotFulfillGoalException("Cannot build a given structure!");
}

void VCAI::tryRealize(Goals::DigAtTile & g)
{
	assert(g.hero->visitablePos() == g.tile); //surely we want to crash here?
	if (g.hero->diggingStatus() == EDiggingStatus::CAN_DIG)
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

void VCAI::tryRealize(Goals::CollectRes & g)
{
	if(cb->getResourceAmount(static_cast<Res::ERes>(g.resID)) >= g.value)
	throw cannotFulfillGoalException("Goal is already fulfilled!");

	if(const CGObjectInstance *obj = cb->getObj(ObjectInstanceID(g.objid), false))
	{
		if(const IMarket *m = IMarket::castFrom(obj, false))
		{
			for (Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, 1))
			{
				if(i == g.resID) continue;
				int toGive, toGet;
				m->getOffer(i, g.resID, toGive, toGet, EMarketMode::RESOURCE_RESOURCE);
				toGive = toGive * (cb->getResourceAmount(i) / toGive);
				//TODO trade only as much as needed
				cb->trade(obj, EMarketMode::RESOURCE_RESOURCE, i, g.resID, toGive);
				if(cb->getResourceAmount(static_cast<Res::ERes>(g.resID)) >= g.value)
					return;
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
		saving[g.resID] = 1;
		throw cannotFulfillGoalException("No object that could be used to raise resources!");
	}
}

void VCAI::tryRealize(Goals::Build & g)
{
	for(const CGTownInstance *t : cb->getTownsInfo())
	{
		logAi->debugStream() << boost::format("Looking into %s") % t->name;
		buildStructure(t);
		buildArmyIn(t);

		if(!ai->primaryHero() ||
			(t->getArmyStrength() > ai->primaryHero()->getArmyStrength() * 2 && !isAccessibleForHero(t->visitablePos(), ai->primaryHero())))
		{
			recruitHero(t);
			buildArmyIn(t);
		}
	}

	throw cannotFulfillGoalException("BUILD has been realized as much as possible.");
}
void VCAI::tryRealize(Goals::Invalid & g)
{
	throw cannotFulfillGoalException("I don't know how to fulfill this!");
}

void VCAI::tryRealize(Goals::AbstractGoal & g)
{
	logAi->debugStream() << boost::format("Attempting realizing goal with code %s") % g.name();
	throw cannotFulfillGoalException("Unknown type of goal !");
}

const CGTownInstance * VCAI::findTownWithTavern() const
{
	for(const CGTownInstance *t : cb->getTownsInfo())
		if(t->hasBuilt(BuildingID::TAVERN) && !t->visitingHero)
			return t;

	return nullptr;
}

Goals::TSubgoal VCAI::getGoal (HeroPtr h) const
{
	auto it = lockedHeroes.find(h);
	if (it != lockedHeroes.end())
		return it->second;
	else
		return sptr(Goals::Invalid());
}


std::vector<HeroPtr> VCAI::getUnblockedHeroes() const
{
	std::vector<HeroPtr> ret;
	for (auto h : cb->getHeroesInfo())
	{
		//&& !vstd::contains(lockedHeroes, h)
		//at this point we assume heroes exhausted their locked goals
		if (canAct(h))
			ret.push_back(h);
	}
	return ret;
}

bool VCAI::canAct (HeroPtr h) const
{
	auto mission = lockedHeroes.find(h);
	if (mission != lockedHeroes.end())
	{
		//FIXME: I'm afraid there can be other conditions when heroes can act but not move :?
		if (mission->second->goalType == Goals::DIG_AT_TILE && !mission->second->isElementar)
			return false;
	}

	return h->movement;
}

HeroPtr VCAI::primaryHero() const
{
	auto hs = cb->getHeroesInfo();
	boost::sort(hs, compareHeroStrength);

	if(hs.empty())
		return nullptr;

	return hs.back();
}

void VCAI::endTurn()
{
	logAi->infoStream() << "Player " << static_cast<int>(playerID.getNum()) << " ends turn";
	if(!status.haveTurn())
	{
		logAi->errorStream() << "Not having turn at the end of turn???";
	}
	logAi->debugStream() << "Resources at the end of turn: " << cb->getResourceAmount();

	do
	{
		cb->endTurn();
	} while(status.haveTurn()); //for some reasons, our request may fail -> stop requesting end of turn only after we've received a confirmation that it's over

	logGlobal->infoStream() << "Player " << static_cast<int>(playerID.getNum()) << " ended turn";
}

void VCAI::striveToGoal(Goals::TSubgoal ultimateGoal)
{
	if (ultimateGoal->invalid())
		return;

	//we are looking for abstract goals
	auto abstractGoal = striveToGoalInternal (ultimateGoal, false);

	if (abstractGoal->invalid())
		return;

	//we received abstract goal, need to find concrete goals
	striveToGoalInternal (abstractGoal, true);

	//TODO: save abstract goals not related to hero
}

Goals::TSubgoal VCAI::striveToGoalInternal(Goals::TSubgoal ultimateGoal, bool onlyAbstract)
{
	const int searchDepth = 30;
	const int searchDepth2 = searchDepth-2;
	Goals::TSubgoal abstractGoal = sptr(Goals::Invalid());

	while(1)
	{
		Goals::TSubgoal goal = ultimateGoal;
		logAi->debugStream() << boost::format("Striving to goal of type %s") % ultimateGoal->name();
		int maxGoals = searchDepth; //preventing deadlock for mutually dependent goals
		while(!goal->isElementar && maxGoals && (onlyAbstract || !goal->isAbstract))
		{
			logAi->debugStream() << boost::format("Considering goal %s") % goal->name();
			try
			{
				boost::this_thread::interruption_point();
				goal = goal->whatToDoToAchieve();
				--maxGoals;
				if (*goal == *ultimateGoal) //compare objects by value
					throw cannotFulfillGoalException("Goal dependency loop detected!");
			}
			catch(goalFulfilledException &e)
			{
				//it is impossible to continue some goals (like exploration, for example)
				completeGoal (goal);
				logAi->debugStream() << boost::format("Goal %s decomposition failed: goal was completed as much as possible") % goal->name();
				return sptr(Goals::Invalid());
			}
			catch(std::exception &e)
			{
				logAi->debugStream() << boost::format("Goal %s decomposition failed: %s") % goal->name() % e.what();
				return sptr(Goals::Invalid());
			}
		}

		try
		{
			boost::this_thread::interruption_point();

			if (!maxGoals)
			{
				std::runtime_error e("Too many subgoals, don't know what to do");
				throw (e);
			}

			if (goal->hero) //lock this hero to fulfill ultimate goal
			{
				if (maxGoals)
				{
					setGoal(goal->hero, goal);
				}
				else
				{
					vstd::erase_if_present (lockedHeroes, goal->hero); // we seemingly don't know what to do with hero
				}
			}

			if (goal->isAbstract)
			{
				abstractGoal = goal; //allow only one abstract goal per call
				logAi->debugStream() << boost::format("Choosing abstract goal %s") % goal->name();
				break;
			}
			else
			{
				logAi->debugStream() << boost::format("Trying to realize %s (value %2.3f)") % goal->name() % goal->priority;
				goal->accept(this);
			}

			boost::this_thread::interruption_point();
		}
		catch(boost::thread_interrupted &e)
		{
			logAi->debugStream() << boost::format("Player %d: Making turn thread received an interruption!") % playerID;
			throw; //rethrow, we want to truly end this thread
		}
		catch(goalFulfilledException &e)
		{
			//the goal was completed successfully
			completeGoal (goal);
			//completed goal was main goal //TODO: find better condition
			if (ultimateGoal->fulfillsMe(goal) || maxGoals > searchDepth2)
				return sptr(Goals::Invalid());
		}
		catch(std::exception &e)
		{
			logAi->debugStream() << boost::format("Failed to realize subgoal of type %s (greater goal type was %s), I will stop.") % goal->name() % ultimateGoal->name();
			logAi->debugStream() << boost::format("The error message was: %s") % e.what();
			break;
		}
	}
	return abstractGoal;
}

void VCAI::striveToQuest (const QuestInfo &q)
{
	if (q.quest->missionType && q.quest->progress != CQuest::COMPLETE)
	{
		MetaString ms;
		q.quest->getRolloverText(ms, false);
		logAi->debugStream() << boost::format("Trying to realize quest: %s") % ms.toString();
		auto heroes = cb->getHeroesInfo();

		switch (q.quest->missionType)
		{
			case CQuest::MISSION_ART:
			{
				for (auto hero : heroes) //TODO: remove duplicated code?
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum()).sethero(hero)));
						return;
					}
				}
				for (auto art : q.quest->m5arts)
				{
					striveToGoal (sptr(Goals::GetArtOfType(art))); //TODO: transport?
				}
				break;
			}
			case CQuest::MISSION_HERO:
			{
				//striveToGoal (CGoal(RECRUIT_HERO));
				for (auto hero : heroes)
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum()).sethero(hero)));
						return;
					}
				}
				striveToGoal (sptr(Goals::FindObj(Obj::PRISON))); //rule of a thumb - quest heroes usually are locked in prisons
				//BNLOG ("Don't know how to recruit hero with id %d\n", q.quest->m13489val);
				break;
			}
			case CQuest::MISSION_ARMY:
			{
				for (auto hero : heroes)
				{
					if (q.quest->checkQuest(hero)) //very bad info - stacks can be split between multiple heroes :(
					{
						striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum()).sethero(hero)));
						return;
					}
				}
				for (auto creature : q.quest->m6creatures)
				{
					striveToGoal (sptr(Goals::GatherTroops(creature.type->idNumber, creature.count)));
				}
				//TODO: exchange armies... oh my
				//BNLOG ("Don't know how to recruit %d of %s\n", (int)(creature.count) % creature.type->namePl);
				break;
			}
			case CQuest::MISSION_RESOURCES:
			{
				if (heroes.size())
				{
					if (q.quest->checkQuest(heroes.front())) //it doesn't matter which hero it is
					{
						 striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum())));
					}
					else
					{
						for (int i = 0; i < q.quest->m7resources.size(); ++i)
						{
							if (q.quest->m7resources[i])
								striveToGoal (sptr(Goals::CollectRes(i, q.quest->m7resources[i])));
						}
					}
				}
				else
					striveToGoal (sptr(Goals::RecruitHero())); //FIXME: checkQuest requires any hero belonging to player :(
				break;
			}
			case CQuest::MISSION_KILL_HERO:
			case CQuest::MISSION_KILL_CREATURE:
			{
				auto obj = cb->getObjByQuestIdentifier(q.quest->m13489val);
				if (obj)
					striveToGoal (sptr(Goals::GetObj(obj->id.getNum())));
				else
					striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum()))); //visit seer hut
				break;
			}
			case CQuest::MISSION_PRIMARY_STAT:
			{
				auto heroes = cb->getHeroesInfo();
				for (auto hero : heroes)
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum()).sethero(hero)));
						return;
					}
				}
				for (int i = 0; i < q.quest->m2stats.size(); ++i)
				{
					logAi->debugStream() << boost::format("Don't know how to increase primary stat %d") % i;
				}
				break;
			}
			case CQuest::MISSION_LEVEL:
			{
				auto heroes = cb->getHeroesInfo();
				for (auto hero : heroes)
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (sptr(Goals::GetObj(q.obj->id.getNum()).sethero(hero))); //TODO: causes infinite loop :/
						return;
					}
				}
				logAi->debugStream() << boost::format("Don't know how to reach hero level %d") % q.quest->m13489val;
				break;
			}
			case CQuest::MISSION_PLAYER:
			{
				if (playerID.getNum() != q.quest->m13489val)
					logAi->debugStream() << boost::format("Can't be player of color %d") % q.quest->m13489val;
				break;
			}
			case CQuest::MISSION_KEYMASTER:
			{
				striveToGoal (sptr(Goals::FindObj(Obj::KEYMASTER, q.obj->subID)));
				break;
			}
		}
	}
}

void VCAI::performTypicalActions()
{
	for(auto h : getUnblockedHeroes())
	{
		logAi->debugStream() << boost::format("Looking into %s, MP=%d") % h->name.c_str() % h->movement;
		makePossibleUpgrades(*h);
		pickBestArtifacts(*h);
		try
		{
			wander(h);
		}
		catch(std::exception &e)
		{
			logAi->debugStream() << boost::format("Cannot use this hero anymore, received exception: %s") % e.what();
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

int3 VCAI::explorationBestNeighbour(int3 hpos, int radius, HeroPtr h)
{
	int3 ourPos = h->convertPosition(h->pos, false);
	std::map<int3, int> dstToRevealedTiles;
	for(crint3 dir : int3::getDirs())
		if(cb->isInTheMap(hpos+dir))
			if (ourPos != dir) //don't stand in place
				if (isSafeToVisit(h, hpos + dir) && isAccessibleForHero (hpos + dir, h))
					dstToRevealedTiles[hpos + dir] = howManyTilesWillBeDiscovered(radius, hpos, dir);

	if (dstToRevealedTiles.empty()) //yes, it DID happen!
		throw cannotFulfillGoalException("No neighbour will bring new discoveries!");

	auto best = dstToRevealedTiles.begin();
	for (auto i = dstToRevealedTiles.begin(); i != dstToRevealedTiles.end(); i++)
	{
		const CGPathNode *pn = cb->getPathsInfo(h.get())->getPathInfo(i->first);
		//const TerrainTile *t = cb->getTile(i->first);
		if(best->second < i->second && pn->reachable() && pn->accessible == CGPathNode::ACCESSIBLE)
			best = i;
	}

	if(best->second)
		return best->first;

	throw cannotFulfillGoalException("No neighbour will bring new discoveries!");
}

int3 VCAI::explorationNewPoint(HeroPtr h)
{
	int radius = h->getSightRadius();
	CCallback * cbp = cb.get();
	const CGHeroInstance * hero = h.get();

	std::vector<std::vector<int3> > tiles; //tiles[distance_to_fow]
	tiles.resize(radius);

	foreach_tile_pos([&](const int3 &pos)
	{
		if(!cbp->isVisible(pos))
			tiles[0].push_back(pos);
	});

	float bestValue = 0; //discovered tile to node distance ratio
	int3 bestTile(-1,-1,-1);
	int3 ourPos = h->convertPosition(h->pos, false);

	for (int i = 1; i < radius; i++)
	{
		getVisibleNeighbours(tiles[i-1], tiles[i]);
		vstd::removeDuplicates(tiles[i]);

		for(const int3 &tile : tiles[i])
		{
			if (tile == ourPos) //shouldn't happen, but it does
				continue;
			if (!cb->getPathsInfo(hero)->getPathInfo(tile)->reachable()) //this will remove tiles that are guarded by monsters (or removable objects)
				continue;

			CGPath path;
			cb->getPathsInfo(hero)->getPath(path, tile);
			float ourValue = (float)howManyTilesWillBeDiscovered(tile, radius, cbp) / (path.nodes.size() + 1); //+1 prevents erratic jumps

			if (ourValue > bestValue) //avoid costly checks of tiles that don't reveal much
			{
				if(isSafeToVisit(h, tile) && !isBlockedBorderGate(tile))
				{
					bestTile = tile;
					bestValue = ourValue;
				}
			}
		}
	}
	return bestTile;
}

int3 VCAI::explorationDesperate(HeroPtr h)
{
	auto sm = getCachedSectorMap(h);
	int radius = h->getSightRadius();

	std::vector<std::vector<int3> > tiles; //tiles[distance_to_fow]
	tiles.resize(radius);

	CCallback * cbp = cb.get();

	foreach_tile_pos([&](const int3 &pos)
	{
		if(!cbp->isVisible(pos))
			tiles[0].push_back(pos);
	});

	ui64 lowestDanger = -1;
	int3 bestTile(-1,-1,-1);

	for(int i = 1; i < radius; i++)
	{
		getVisibleNeighbours(tiles[i-1], tiles[i]);
		vstd::removeDuplicates(tiles[i]);

		for(const int3 &tile : tiles[i])
		{
			if (cbp->getTile(tile)->blocked) //does it shorten the time?
				continue;
			if (!howManyTilesWillBeDiscovered(tile, radius, cbp)) //avoid costly checks of tiles that don't reveal much
				continue;

			auto t = sm->firstTileToGet(h, tile);
			if (t.valid())
			{
				ui64 ourDanger = evaluateDanger(t, h.h);
				if (ourDanger < lowestDanger)
				{
					if(!isBlockedBorderGate(t))
					{
						if (!ourDanger) //at least one safe place found
							return t;

						bestTile = t;
						lowestDanger = ourDanger;
					}
				}
			}
		}
	}
	return bestTile;
}

TResources VCAI::estimateIncome() const
{
	TResources ret;
	for(const CGTownInstance *t : cb->getTownsInfo())
	{
		ret += t->dailyIncome();
	}



	for(const CGObjectInstance *obj : getFlaggedObjects())
	{
		if(obj->ID == Obj::MINE)
		{
			switch(obj->subID)
			{
			case Res::WOOD:
			case Res::ORE:
				ret[obj->subID] += WOOD_ORE_MINE_PRODUCTION;
				break;
			case Res::GOLD:
			case 7: //abandoned mine -> also gold
				ret[Res::GOLD] += GOLD_MINE_PRODUCTION;
				break;
			default:
				ret[obj->subID] += RESOURCE_MINE_PRODUCTION;
				break;
			}
		}
	}

	return ret;
}

bool VCAI::containsSavedRes(const TResources &cost) const
{
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
	{
		if(saving[i] && cost[i])
			return true;
	}

	return false;
}

void VCAI::checkHeroArmy (HeroPtr h)
{
	auto it = lockedHeroes.find(h);
	if (it != lockedHeroes.end())
	{
		if (it->second->goalType == Goals::GATHER_ARMY && it->second->value <= h->getArmyStrength())
			completeGoal(sptr(Goals::GatherArmy(it->second->value).sethero(h)));
	}
}

void VCAI::recruitHero(const CGTownInstance * t, bool throwing)
{
	logAi->debugStream() << boost::format("Trying to recruit a hero in %s at %s") % t->name % t->visitablePos();

	auto heroes = cb->getAvailableHeroes(t);
	if(heroes.size())
	{
		auto hero = heroes[0];
		if (heroes.size() >= 2) //makes sense to recruit two heroes with starting amries in first week
		{
			if (heroes[1]->getTotalStrength() > hero->getTotalStrength())
				hero = heroes[1];
		}
		cb->recruitHero(t, hero);
	}
	else if(throwing)
		throw cannotFulfillGoalException("No available heroes in tavern in " + t->nodeName());
}

void VCAI::finish()
{
	if(makingTurn)
		makingTurn->interrupt();
}

void VCAI::requestActionASAP(std::function<void()> whatToDo)
{
	boost::thread newThread([this,whatToDo]()
	{
		setThreadName("VCAI::requestActionASAP::whatToDo");
		SET_GLOBAL_STATE(this);
		boost::shared_lock<boost::shared_mutex> gsLock(cb->getGsMutex());
		whatToDo();
	});
}

void VCAI::lostHero(HeroPtr h)
{
	logAi->debugStream() << boost::format("I lost my hero %s. It's best to forget and move on.") % h.name;

	vstd::erase_if_present(lockedHeroes, h);
	for(auto obj : reservedHeroesMap[h])
	{
		vstd::erase_if_present(reservedObjs, obj); //unreserve all objects for that hero
	}
	vstd::erase_if_present(reservedHeroesMap, h);
	vstd::erase_if_present(cachedSectorMaps, h);
}

void VCAI::answerQuery(QueryID queryID, int selection)
{
	logAi->debugStream() << boost::format("I'll answer the query %d giving the choice %d") % queryID % selection;
	if(queryID != QueryID(-1))
	{
		cb->selectionMade(selection, queryID);
	}
	else
	{
		logAi->debugStream() << boost::format("Since the query ID is %d, the answer won't be sent. This is not a real query!") % queryID;
		//do nothing
	}
}

void VCAI::requestSent(const CPackForServer *pack, int requestID)
{
	//BNLOG("I have sent request of type %s", typeid(*pack).name());
	if(auto reply = dynamic_cast<const QueryReply*>(pack))
	{
		status.attemptedAnsweringQuery(reply->qid, requestID);
	}
}

std::string VCAI::getBattleAIName() const
{
	if(settings["server"]["neutralAI"].getType() == JsonNode::DATA_STRING)
		return settings["server"]["neutralAI"].String();
	else
		return "StupidAI";
}

void VCAI::validateObject(const CGObjectInstance *obj)
{
	validateObject(obj->id);
}

void VCAI::validateObject(ObjectIdRef obj)
{
	auto matchesId = [&](const CGObjectInstance *hlpObj) -> bool { return hlpObj->id == obj.id; };
	if(!obj)
	{
		vstd::erase_if(visitableObjs, matchesId);

		for(auto &p : reservedHeroesMap)
			vstd::erase_if(p.second, matchesId);

		vstd::erase_if(reservedObjs, matchesId);
	}
}

TResources VCAI::freeResources() const
{
	TResources myRes = cb->getResourceAmount();
	myRes[Res::GOLD] -= GOLD_RESERVE;
	vstd::amax(myRes[Res::GOLD], 0);
	return myRes;
}

std::shared_ptr<SectorMap> VCAI::getCachedSectorMap(HeroPtr h)
{
	auto it = cachedSectorMaps.find(h);
	if (it != cachedSectorMaps.end())
		return it->second;
	else
	{
		cachedSectorMaps[h] = std::make_shared<SectorMap>(h);
		return cachedSectorMaps[h];
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
		logAi->debugStream() << boost::format("The \"query\" has an id %d, it'll be ignored as non-query. Description: %s") % ID % description;
		return;
	}

	assert(ID.getNum() >= 0);
	boost::unique_lock<boost::mutex> lock(mx);

	assert(!vstd::contains(remainingQueries, ID));

	remainingQueries[ID] = description;

	cv.notify_all();
	logAi->debugStream() << boost::format("Adding query %d - %s. Total queries count: %d") % ID % description % remainingQueries.size();
}

void AIStatus::removeQuery(QueryID ID)
{
	boost::unique_lock<boost::mutex> lock(mx);
	assert(vstd::contains(remainingQueries, ID));

	std::string description = remainingQueries[ID];
	remainingQueries.erase(ID);

	cv.notify_all();
	logAi->debugStream() << boost::format("Removing query %d - %s. Total queries count: %d") % ID % description % remainingQueries.size();
}

int AIStatus::getQueriesCount()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return remainingQueries.size();
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
	logAi->debugStream() << boost::format("Attempted answering query %d - %s. Request id=%d. Waiting for results...") % queryID % description % answerRequestID;
	requestToQueryID[answerRequestID] = queryID;
}

void AIStatus::receivedAnswerConfirmation(int answerRequestID, int result)
{
	assert(vstd::contains(requestToQueryID, answerRequestID));
	QueryID query = requestToQueryID[answerRequestID];
	assert(vstd::contains(remainingQueries, query));
	requestToQueryID.erase(answerRequestID);

	if(result)
	{
		removeQuery(query);
	}
	else
	{
		logAi->errorStream() << "Something went really wrong, failed to answer query " << query << ": " << remainingQueries[query];
		//TODO safely retry
	}
}

void AIStatus::heroVisit(const CGObjectInstance *obj, bool started)
{
	boost::unique_lock<boost::mutex> lock(mx);
	if(started)
		objectsBeingVisited.push_back(obj);
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

SectorMap::SectorMap()
{
	update();
}

SectorMap::SectorMap(HeroPtr h)
{
	update();
	makeParentBFS(h->visitablePos());
}

bool SectorMap::markIfBlocked(ui8 &sec, crint3 pos, const TerrainTile *t)
{
	if(t->blocked && !t->visitable)
	{
		sec = NOT_AVAILABLE;
		return true;
	}

	return false;
}

bool SectorMap::markIfBlocked(ui8 &sec, crint3 pos)
{
	return markIfBlocked(sec, pos, getTile(pos));
}

void SectorMap::update()
{
	visibleTiles = cb->getAllVisibleTiles();

	clear();
	int curSector = 3; //0 is invisible, 1 is not explored

	CCallback * cbp = cb.get(); //optimization
	foreach_tile_pos([&](crint3 pos)
	{
		if(retreiveTile(pos) == NOT_CHECKED)
		{
			if(!markIfBlocked(retreiveTile(pos), pos))
				exploreNewSector(pos, curSector++, cbp);
		}
	});
	valid = true;
}

void SectorMap::clear()
{
	sector = cb->getVisibilityMap();
	valid = false;
}

void SectorMap::exploreNewSector(crint3 pos, int num, CCallback * cbp)
{
	Sector &s = infoOnSectors[num];
	s.id = num;
	s.water = getTile(pos)->isWater();

	std::queue<int3> toVisit;
	toVisit.push(pos);
	while(!toVisit.empty())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		ui8 &sec = retreiveTile(curPos);
		if(sec == NOT_CHECKED)
		{
			const TerrainTile *t = getTile(curPos);
			if(!markIfBlocked(sec, curPos, t))
			{
				if(t->isWater() == s.water) //sector is only-water or only-land
				{
					sec = num;
					s.tiles.push_back(curPos);
					foreach_neighbour(cbp, curPos, [&](CCallback * cbp, crint3 neighPos)
					{
						if(retreiveTile(neighPos) == NOT_CHECKED)
						{
							toVisit.push(neighPos);
							//parent[neighPos] = curPos;
						}
						const TerrainTile *nt = getTile(neighPos);
						if(nt && nt->isWater() != s.water && canBeEmbarkmentPoint(nt, s.water))
						{
							s.embarkmentPoints.push_back(neighPos);
						}
					});

					if(t->visitable)
					{
						auto obj = t->visitableObjects.front();
						if (vstd::contains(ai->knownSubterraneanGates, obj))
						{
							s.subterraneanGates.push_back (obj);
						}
					}
				}
			}
		}
	}

	vstd::removeDuplicates(s.embarkmentPoints);
}

void SectorMap::write(crstring fname)
{
	std::ofstream out(fname);
	for(int k = 0; k < cb->getMapSize().z; k++)
	{
		for(int j = 0; j < cb->getMapSize().y; j++)
		{
			for(int i = 0; i < cb->getMapSize().x; i++)
			{
				out << (int)sector[i][j][k] << '\t';
			}
			out << std::endl;
		}
		out << std::endl;
	}
}

bool isWeeklyRevisitable (const CGObjectInstance * obj)
{ //TODO: allow polling of remaining creatures in dwelling
	if (dynamic_cast<const CGVisitableOPW *>(obj) || //ensures future compatibility, unlike IDs
		dynamic_cast<const CGDwelling *>(obj) ||
		dynamic_cast<const CBank *>(obj)) //banks tend to respawn often in mods
		return true;
	switch (obj->ID)
	{
		case Obj::STABLES:
		case Obj::MAGIC_WELL:
		case Obj::HILL_FORT:
			return true;
		case Obj::BORDER_GATE:
		case Obj::BORDERGUARD:
			return (dynamic_cast <const CGKeys *>(obj))->wasMyColorVisited (ai->playerID); //FIXME: they could be revisited sooner than in a week
	}
	return false;
}

bool shouldVisit(HeroPtr h, const CGObjectInstance * obj)
{
	switch (obj->ID)
	{
		case Obj::TOWN:
		case Obj::HERO: //never visit our heroes at random
			return obj->tempOwner != h->tempOwner; //do not visit our towns at random
			break;
		case Obj::BORDER_GATE:
		{
			for (auto q : ai->myCb->getMyQuests())
			{
				if (q.obj == obj)
				{
					return false; // do not visit guards or gates when wandering
				}
			}
			return true; //we don't have this quest yet
		}
			break;
		case Obj::BORDERGUARD: //open borderguard if possible
			return (dynamic_cast <const CGKeys *>(obj))->wasMyColorVisited(ai->playerID);
		case Obj::SEER_HUT:
		case Obj::QUEST_GUARD:
		{
			for (auto q : ai->myCb->getMyQuests())
			{
				if (q.obj == obj)
				{
					if (q.quest->checkQuest(h.h))
						return true; //we completed the quest
					else
						return false; //we can't complete this quest
				}
			}
			return true; //we don't have this quest yet
		}
			break;
		case Obj::CREATURE_GENERATOR1:
		{
			if (obj->tempOwner != h->tempOwner)
				return true; //flag just in case
			bool canRecruitCreatures = false;
			const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);
			for(auto level : d->creatures)
			{
				for(auto c : level.second)
				{
					if (h->getSlotFor(CreatureID(c)) != SlotID())
						canRecruitCreatures = true;
				}
			}
			return canRecruitCreatures;
		}
		case Obj::HILL_FORT:
		{
			for (auto slot : h->Slots())
			{
				if (slot.second->type->upgrades.size())
					return true; //TODO: check price?
			}
			return false;
		}
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::MONOLITH_TWO_WAY:
		case Obj::WHIRLPOOL:
			//TODO: mechanism for handling monoliths
			return false;
		case Obj::SCHOOL_OF_MAGIC:
		case Obj::SCHOOL_OF_WAR:
			{
				TResources myRes = ai->myCb->getResourceAmount();
				if (myRes[Res::GOLD] - GOLD_RESERVE < 1000)
					return false;
			}
			break;
		case Obj::LIBRARY_OF_ENLIGHTENMENT:
			if (h->level < 12)
				return false;
			break;
		case Obj::TREE_OF_KNOWLEDGE:
			{
				TResources myRes = ai->myCb->getResourceAmount();
				if (myRes[Res::GOLD] - GOLD_RESERVE < 2000 || myRes[Res::GEMS] < 10)
					return false;
			}
			break;
		case Obj::MAGIC_WELL:
			return h->mana < h->manaLimit();
		case Obj::PRISON:
			return ai->myCb->getHeroesInfo().size() < VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER;// GameConstants::MAX_HEROES_PER_PLAYER;

		case Obj::BOAT:
			return false;
			//Boats are handled by pathfinder
	}

	if (obj->wasVisited(*h)) //it must pointer to hero instance, heroPtr calls function wasVisited(ui8 player);
		return false;

	return true;
}

int3 SectorMap::firstTileToGet(HeroPtr h, crint3 dst)
/*
this functions returns one target tile or invalid tile. We will use it to poll possible destinations
For ship construction etc, another function (goal?) is needed
*/
{
	int3 ret(-1,-1,-1);

	int sourceSector = retreiveTile(h->visitablePos()),
		destinationSector = retreiveTile(dst);

	const Sector *src = &infoOnSectors[sourceSector],
		*dest = &infoOnSectors[destinationSector];

	if(sourceSector != destinationSector) //use ships, shipyards etc..
	{
		if (ai->isAccessibleForHero(dst, h)) //pathfinder can find a way using ships and gates if tile is not blocked by objects
			return dst;

		std::map<const Sector*, const Sector*> preds;
		std::queue<const Sector *> sectorQueue;
		sectorQueue.push(src);
		while(!sectorQueue.empty())
		{
			const Sector *s = sectorQueue.front();
			sectorQueue.pop();

			for(int3 ep : s->embarkmentPoints)
			{
				Sector *neigh = &infoOnSectors[retreiveTile(ep)];
				//preds[s].push_back(neigh);
				if(!preds[neigh])
				{
					preds[neigh] = s;
					sectorQueue.push(neigh);
				}
			}
		}

		if(!preds[dest])
		{
			//write("test.txt");

			return ret;
            //throw cannotFulfillGoalException(boost::str(boost::format("Cannot find connection between sectors %d and %d") % src->id % dst->id));
		}

		std::vector<const Sector*> toTraverse;
		toTraverse.push_back(dest);
		while(toTraverse.back() != src)
		{
			toTraverse.push_back(preds[toTraverse.back()]);
		}

		if(preds[dest])
		{
			//TODO: would be nice to find sectors in loop
			const Sector *sectorToReach  = toTraverse.at(toTraverse.size() - 2);

			if(!src->water && sectorToReach->water) //embark
			{
				//embark on ship -> look for an EP with a boat
				auto firstEP = boost::find_if(src->embarkmentPoints, [=](crint3 pos) -> bool
				{
					const TerrainTile *t = getTile(pos);
                    return t && t->visitableObjects.size() == 1 && t->topVisitableId() == Obj::BOAT
						&& retreiveTile(pos) == sectorToReach->id;
				});

				if(firstEP != src->embarkmentPoints.end())
				{
					return *firstEP;
				}
				else
				{
					//we need to find a shipyard with an access to the desired sector's EP
					//TODO what about Summon Boat spell?
					std::vector<const IShipyard *> shipyards;
					for(const CGTownInstance *t : cb->getTownsInfo())
					{
						if(t->hasBuilt(BuildingID::SHIPYARD))
							shipyards.push_back(t);
					}

					for(const CGObjectInstance *obj : ai->getFlaggedObjects())
					{
						if(obj->ID != Obj::TOWN) //towns were handled in the previous loop
							if(const IShipyard *shipyard = IShipyard::castFrom(obj))
								shipyards.push_back(shipyard);
					}

					shipyards.erase(boost::remove_if(shipyards, [=](const IShipyard *shipyard) -> bool
					{
						return shipyard->shipyardStatus() != 0 || retreiveTile(shipyard->bestLocation()) != sectorToReach->id;
					}),shipyards.end());

					if(!shipyards.size())
					{
						//TODO consider possibility of building shipyard in a town
						return ret;

						//throw cannotFulfillGoalException("There is no known shipyard!");
					}

					//we have only shipyards that possibly can build ships onto the appropriate EP
					auto ownedGoodShipyard = boost::find_if(shipyards, [](const IShipyard *s) -> bool
					{
						return s->o->tempOwner == ai->playerID;
					});

					if(ownedGoodShipyard != shipyards.end())
					{
						const IShipyard *s = *ownedGoodShipyard;
						TResources shipCost;
						s->getBoatCost(shipCost);
						if(cb->getResourceAmount().canAfford(shipCost))
						{
							int3 ret = s->bestLocation();
							cb->buildBoat(s); //TODO: move actions elsewhere
							return ret;
						}
						else
						{
							//TODO gather res
							return ret;

							//throw cannotFulfillGoalException("Not enough resources to build a boat");
						}
					}
					else
					{
						//TODO pick best shipyard to take over
						return shipyards.front()->o->visitablePos();
					}
				}
			}
			else if(src->water && !sectorToReach->water)
			{
				//TODO
				//disembark
				return ret;
			}
			else //use subterranean gates - not needed since gates are now handled via Pathfinder
			{
				return ret;
				//throw cannotFulfillGoalException("Land-land and water-water inter-sector transitions are not implemented!");
			}
		}
		else
		{
			return ret;
			//throw cannotFulfillGoalException("Inter-sector route detection failed: not connected sectors?");
		}
	}
	else
	{
		return findFirstVisitableTile(h, dst);
	}

	//FIXME: find out why this line is reached
	logAi->errorStream() << ("Impossible happened at SectorMap::firstTileToGet");
	return ret;
}

int3 SectorMap::findFirstVisitableTile (HeroPtr h, crint3 dst)
{
	int3 ret(-1,-1,-1);
	int3 curtile = dst;

	while(curtile != h->visitablePos())
	{
		auto topObj = cb->getTopObj(curtile);
		if(topObj && topObj->ID == Obj::HERO && topObj != h.h &&
			cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
		{
			logAi->warnStream() << ("Another allied hero stands in our way");
			return ret;
		}
		if(ai->myCb->getPathsInfo(h.get())->getPathInfo(curtile)->reachable())
		{
			return curtile;
		}
		else
		{
			auto i = parent.find(curtile);
			if(i != parent.end())
			{
				assert(curtile != i->second);
				curtile = i->second;
			}
			else
			{
				return ret;
				//throw cannotFulfillGoalException("Unreachable tile in sector? Should not happen!");
			}
		}
	}
	return ret;
}

void SectorMap::makeParentBFS(crint3 source)
{
	parent.clear();

	int mySector = retreiveTile(source);
	std::queue<int3> toVisit;
	toVisit.push(source);
	while(!toVisit.empty())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		ui8 &sec = retreiveTile(curPos);
		assert(sec == mySector); //consider only tiles from the same sector
		UNUSED(sec);

		foreach_neighbour(curPos, [&](crint3 neighPos)
		{
			if(retreiveTile(neighPos) == mySector && !vstd::contains(parent, neighPos))
			{
				if (cb->canMoveBetween(curPos, neighPos))
				{
					toVisit.push(neighPos);
					parent[neighPos] = curPos;
				}
			}
		});
	}
}

unsigned char & SectorMap::retreiveTile(crint3 pos)
{
	return retreiveTileN(sector, pos);
}

TerrainTile* SectorMap::getTile(crint3 pos) const
{
	//out of bounds access should be handled by boost::multi_array
	//still we cached this array to avoid any checks
	return visibleTiles->operator[](pos.x)[pos.y][pos.z];
}

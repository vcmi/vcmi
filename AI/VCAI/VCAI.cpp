#include "StdInc.h"
#include "VCAI.h"
#include "Goals.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"

/*
 * CCreatureHandler.h, part of VCMI engine
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

using namespace vstd;
//extern Goals::TSubgoal sptr(const Goals::AbstractGoal & tmp);
//#define sptr(x) Goals::sptr(x)

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
		name(obj->getHoverText())
	{
	}
};

std::map<const CGObjectInstance *, ObjInfo> helperObjInfo;

VCAI::VCAI(void)
{
	LOG_TRACE(logAi);
	makingTurn = nullptr;
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

	if(details.result == TryMoveHero::TELEPORTATION)
	{
		const int3 from = CGHeroInstance::convertPosition(details.start, false),
			to = CGHeroInstance::convertPosition(details.end, false);
		const CGObjectInstance *o1 = frontOrNull(cb->getVisitableObjs(from)),
			*o2 = frontOrNull(cb->getVisitableObjs(to));

		if(o1 && o2 && o1->ID == Obj::SUBTERRANEAN_GATE && o2->ID == Obj::SUBTERRANEAN_GATE)
		{
			knownSubterraneanGates[o1] = o2;
			knownSubterraneanGates[o2] = o1;
            logAi->debugStream() << boost::format("Found a pair of subterranean gates between %s and %s!") % from % to;
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
	LOG_TRACE_PARAMS(logAi, "victoryLossCheckResult '%s'", victoryLossCheckResult);
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

// 		//let's make Impossible difficulty finally standing to its name :>
// 		if(myCb->getStartInfo()->difficulty == 4 && !victory)
// 		{
// 			//play dirty: crash the whole engine to avoid lose
// 			//that way AI is unbeatable!
// 			*(int*)nullptr = 666;
// 		}

// 		TODO - at least write some insults on stdout

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
	LOG_TRACE_PARAMS(logAi, "start '%i'; obj '%s'", start % (visitedObj ? visitedObj->hoverName : std::string("n/a")));
	NET_EVENT_HANDLER;
	if(start)
	{
		markObjectVisited (visitedObj);
		erase_if_present(reservedObjs, visitedObj); //unreserve objects
		erase_if_present(reservedHeroesMap[visitor], visitedObj);
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
		if (firstHero->getHeroStrength() > secondHero->getHeroStrength() && canGetArmy (firstHero, secondHero))
			pickBestCreatures (firstHero, secondHero);
		else if (canGetArmy (secondHero, firstHero))
			pickBestCreatures (secondHero, firstHero);

		completeGoal(sptr(Goals::VisitHero(firstHero->id.getNum()))); //TODO: what if we were visited by other hero in the meantime?
		completeGoal(sptr(Goals::VisitHero(secondHero->id.getNum())));
		//TODO: exchange artifacts

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
}

void VCAI::objectRemoved(const CGObjectInstance *obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	erase_if_present(visitableObjs, obj);
	erase_if_present(alreadyVisited, obj);
	erase_if_present(reservedObjs, obj);


	for(auto &p : reservedHeroesMap)
		erase_if_present(p.second, obj);

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

void VCAI::heroCreated(const CGHeroInstance*)
{
	LOG_TRACE(logAi);
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
		if(sop->val == playerID.getNum())
			erase_if_present(visitableObjs, myCb->getObj(sop->id));
		//TODO restore lost obj
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

void VCAI::init(shared_ptr<CCallback> CB)
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

	retreiveVisitableObjs(visitableObjs);
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

void VCAI::saveGame(COSer<CSaveFile> &h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	NET_EVENT_HANDLER;
	validateVisitableObjs();
	CAdventureAI::saveGame(h, version);
	serializeInternal(h, version);
}

void VCAI::loadGame(CISer<CLoadFile> &h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	NET_EVENT_HANDLER;
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
	MAKING_TURN;
	boost::shared_lock<boost::shared_mutex> gsLock(cb->getGsMutex());
	setThreadName("VCAI::makeTurn");

    logAi->debugStream() << boost::format("Player %d starting turn") % static_cast<int>(playerID.getNum());

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
					if (!vstd::contains(visitableObjs, obj))
						visitableObjs.push_back(obj);
					auto o = std::find (alreadyVisited.begin(), alreadyVisited.end(), obj);
					if (o != alreadyVisited.end())
						alreadyVisited.erase(o);
				}
			}
		}
			break;
		case 7: //reconsider strategy
		{
			if(auto h = primaryHero()) //check if our primary hero can handle danger
			{
				ui64 totalDanger = 0;
				int dangerousObjects = 0;
				std::vector<const CGObjectInstance *> objs;
				retreiveVisitableObjs(objs, false);
				for (auto obj : objs)
				{
					if (evaluateDanger(obj)) //potentilaly dnagerous
					{
						totalDanger += evaluateDanger(obj->visitablePos(), *h);
						++dangerousObjects;
					}
				}
				ui64 averageDanger = totalDanger / std::max(dangerousObjects, 1);
				if (dangerousObjects && averageDanger > h->getHeroStrength())
				{
					setGoal (h, sptr(Goals::GatherArmy(averageDanger * SAFE_ATTACK_CONSTANT).sethero(h).setisAbstract(true)));
				}
			}
		}
			break;
	}
	if(cb->getSelectedHero())
		cb->recalculatePaths();

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

			cb->setSelection(hero.first.get());
			boost::sort (hero.second, isCloser);
			for (auto obj : hero.second)
			{
				if(!obj || !obj->defInfo || !cb->getObj(obj->id))
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

		//heroes tend to die in the process and loose their goals, unsafe to iterate it
		std::vector<std::pair<HeroPtr, Goals::TSubgoal> > safeCopy;
		boost::copy(lockedHeroes, std::back_inserter(safeCopy));

		typedef std::pair<HeroPtr, Goals::TSubgoal> TItrType;

		auto lockedHeroesSorter = [](TItrType h1, TItrType h2) -> bool
		{
			return compareMovement (h1.first, h2.first);
		};
		boost::sort(safeCopy, lockedHeroesSorter);

		while (safeCopy.size()) //continue our goals
		{
			auto it = safeCopy.begin();
			if (it->first && it->first->tempOwner == playerID && vstd::contains(lockedHeroes, it->first)) //make sure hero still has his goal
			{
				cb->setSelection(*it->first);
				striveToGoal (it->second);
			}
			safeCopy.erase(it);
		}

		auto quests = myCb->getMyQuests();
		for (auto quest : quests)
		{
			striveToQuest (quest);
		}

		striveToGoal(sptr(Goals::Build())); //TODO: smarter building management
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
    logAi->debugStream() << boost::format("%s will try to visit %s at (%s)") % h->name % obj->getHoverText() % strFromInt3(dst);
	return moveHeroToTile(dst, h);
}

void VCAI::performObjectInteraction(const CGObjectInstance * obj, HeroPtr h)
{
	LOG_TRACE_PARAMS(logAi, "Hero %s and object %s at %s", h->name % obj->getHoverText() % obj->pos);
	switch (obj->ID)
	{
		case Obj::CREATURE_GENERATOR1:
			recruitCreatures (dynamic_cast<const CGDwelling *>(obj));
			checkHeroArmy (h);
			break;
		case Obj::TOWN:
			moveCreaturesToHero (dynamic_cast<const CGTownInstance *>(obj));
			if (h->visitedTown) //we are inside, not just attacking
			{
				townVisitsThisWeek[h].push_back(h->visitedTown);
				if (!h->hasSpellbook() && cb->getResourceAmount(Res::GOLD) >= GameConstants::SPELLBOOK_GOLD_COST + saving[Res::GOLD] &&
					h->visitedTown->hasBuilt (BuildingID::MAGES_GUILD_1))
					cb->buyArtifact(h.get(), ArtifactID::SPELLBOOK);
			}
			break;
	}
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
	if(army->tempOwner != source->tempOwner)
	{
		logAi->errorStream() << "Why are we even considering exchange between heroes from different players?";
		return false;
	}


	const CArmedInstance *armies[] = {army, source};
	int armySize = 0; 
	//we calculate total strength for each creature type available in armies
	std::map<const CCreature*, int> creToPower;
	for(auto armyPtr : armies)
		for(auto &i : armyPtr->Slots())
		{
			++armySize;//TODO: allow splitting stacks?
			creToPower[i.second->type] += i.second->getPower();
		}
	//TODO - consider more than just power (ie morale penalty, hero specialty in certain stacks, etc)

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
				if(armyPtr->getCreature(SlotID(j)) == bestArmy[i]  &&  (i != j || armyPtr != army)) //it's a searched creature not in dst slot
					if (!(armyPtr->needsLastStack() && armyPtr->Slots().size() == 1 && armyPtr != army)) //can't take away last creature
						return true; //at least one exchange will be performed
			}
	}
	return false;
}

void VCAI::pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source)
{
	//TODO - what if source is a hero (the last stack problem) -> it'd good to create a single stack of weakest cre
	const CArmedInstance *armies[] = {army, source};
	int armySize = 0; 
	//we calculate total strength for each creature type available in armies
	std::map<const CCreature*, int> creToPower;
	for(auto armyPtr : armies)
		for(auto &i : armyPtr->Slots())
		{
			++armySize;//TODO: allow splitting stacks?
			creToPower[i.second->type] += i.second->getPower();
		}
	//TODO - consider more than just power (ie morale penalty, hero specialty in certain stacks, etc)

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
				if(armyPtr->getCreature(SlotID(j)) == bestArmy[i]  &&  (i != j || armyPtr != army)) //it's a searched creature not in dst slot
					if (!(armyPtr->needsLastStack() && armyPtr->Slots().size() == 1 && armyPtr != army))
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

void VCAI::recruitCreatures(const CGDwelling * d)
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

		amin(count, freeResources() / VLC->creh->creatures[creID]->cost);
		if(count > 0)
			cb->recruitCreatures(d, creID, count, i);
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
	TResources income = estimateIncome();
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
			TResources cost = t->town->buildings.at(buildID)->resources;
			for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
			{
				int diff = currentRes[i] - cost[i] + income[i];
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

void VCAI::buildStructure(const CGTownInstance * t)
{
	//TODO make *real* town development system
	//TODO: faction-specific development: use special buildings, build dwellings in better order, etc
	//TODO: build resource silo, defences when needed
	//Possible - allow "locking" on specific building (build prerequisites and then building itself)

	//Set of buildings for different goals. Does not include any prerequisites.
	const BuildingID essential[] = {BuildingID::TAVERN, BuildingID::TOWN_HALL};
	const BuildingID goldSource[] = {BuildingID::TOWN_HALL, BuildingID::CITY_HALL, BuildingID::CAPITOL};
	const BuildingID unitsSource[] = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3,
		BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7};
	const BuildingID unitsUpgrade[] = { BuildingID::DWELL_LVL_1_UP, BuildingID::DWELL_LVL_2_UP, BuildingID::DWELL_LVL_3_UP,
		BuildingID::DWELL_LVL_4_UP, BuildingID::DWELL_LVL_5_UP, BuildingID::DWELL_LVL_6_UP, BuildingID::DWELL_LVL_7_UP};
	const BuildingID unitGrowth[] = { BuildingID::FORT, BuildingID::CITADEL, BuildingID::CASTLE, BuildingID::HORDE_1,
		BuildingID::HORDE_1_UPGR, BuildingID::HORDE_2, BuildingID::HORDE_2_UPGR};
	const BuildingID spells[] = {BuildingID::MAGES_GUILD_1, BuildingID::MAGES_GUILD_2, BuildingID::MAGES_GUILD_3,
		BuildingID::MAGES_GUILD_4, BuildingID::MAGES_GUILD_5};
	const BuildingID extra[] = {BuildingID::RESOURCE_SILO, BuildingID::SPECIAL_1, BuildingID::SPECIAL_2, BuildingID::SPECIAL_3,
		BuildingID::SPECIAL_4, BuildingID::SHIPYARD}; // all remaining buildings

	TResources currentRes = cb->getResourceAmount();
	TResources income = estimateIncome();

	if (tryBuildAnyStructure(t, std::vector<BuildingID>(essential, essential + ARRAY_COUNT(essential))))
		return;

	//we're running out of gold - try to build something gold-producing. Multiplier can be tweaked, 6 is minimum due to buildings costs
	if (currentRes[Res::GOLD] < income[Res::GOLD] * 6)
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

bool VCAI::isGoodForVisit(const CGObjectInstance *obj, HeroPtr h)
{
	const int3 pos = obj->visitablePos();
	if (isAccessibleForHero(obj->visitablePos(), h) &&
			!obj->wasVisited(playerID) &&
			(obj->tempOwner != playerID || isWeeklyRevisitable(obj)) && //flag or get weekly resources / creatures
			isSafeToVisit(h, pos) &&
			shouldVisit(h, obj) &&
			!vstd::contains(alreadyVisited, obj) &&
			!vstd::contains(reservedObjs, obj))
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
	for(const CGObjectInstance *obj : visitableObjs)
	{
		const int3 pos = obj->visitablePos();
		if (isGoodForVisit(obj, h))
		{
			possibleDestinations.push_back(obj);
		}
	}

	boost::sort(possibleDestinations, isCloser);

	return possibleDestinations;
}

bool VCAI::canRecruitAnyHero (const CGTownInstance * t) const
{
	//TODO: make gathering gold, building tavern or conquering town (?) possible subgoals
	if (!t)
		t = findTownWithTavern();
	if (t)
		return cb->getResourceAmount(Res::GOLD) >= HERO_GOLD_COST &&
			cb->getHeroesInfo().size() < ALLOWED_ROAMING_HEROES &&
			cb->getAvailableHeroes(t).size();
	else
		return false;
}

void VCAI::wander(HeroPtr h)
{
	while(1)
	{
		validateVisitableObjs();
		std::vector <ObjectIdRef> dests, tmp;

		range::copy(reservedHeroesMap[h], std::back_inserter(tmp)); //visit our reserved objects first
		for (auto obj : tmp)
		{
			if (isAccessibleForHero (obj->visitablePos(), h)) //even nearby objects could be blocked by other heroes :(
				dests.push_back(obj); //can't use lambda for member function :(
		}

		if (!dests.size())
			range::copy(getPossibleDestinations(h), std::back_inserter(dests));

		if(!dests.size())
		{
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
				dests.emplace_back(townsReachable.back());
			}
			else if(townsNotReachable.size())
			{
				boost::sort(townsNotReachable, compareReinforcements);
	            //TODO pick the truly best
	            const CGTownInstance *t = townsNotReachable.back();
                logAi->debugStream() << boost::format("%s can't reach any town, we'll try to make our way to %s at %s") % h->name % t->name % t->visitablePos();
				int3 pos1 = h->pos;
				striveToGoal(sptr(Goals::VisitTile(t->visitablePos()).sethero(h)));
				if (pos1 == h->pos && h == primaryHero()) //hero can't move
				{
					if (canRecruitAnyHero(t))
						recruitHero(t);
				}
	            break;
			}
			else if(cb->getResourceAmount(Res::GOLD) >= HERO_GOLD_COST)
	        {
				std::vector<const CGTownInstance *> towns = cb->getTownsInfo();
				erase_if(towns, [](const CGTownInstance *t) -> bool
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
				reserveObject(h, dest); //reserve that object - we predict it will be reached soon

				//removed - do not forget abstract goal so easily
				//setGoal(h, CGoal(VISIT_TILE).sethero(h).settile(dest->visitablePos()));
			}
			break;
		}

		if(h->visitedTown)
		{
			townVisitsThisWeek[h].push_back(h->visitedTown);
			buildArmyIn(h->visitedTown);
			break;
		}
	}
}

void VCAI::setGoal(HeroPtr h, Goals::TSubgoal goal)
{ //TODO: check for presence?
	if (goal->invalid())
		erase_if_present(lockedHeroes, h);
	else
	{
		lockedHeroes[h] = goal;
		goal->setisElementar(false); //always evaluate goals before realizing
	}
}

void VCAI::completeGoal (Goals::TSubgoal goal)
{
	logAi->debugStream() << boost::format("Completing goal: %s") % goal->name();
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
		for (auto p : lockedHeroes)
		{
			if (p.second == goal || p.second->fulfillsMe(goal)) //we could have fulfilled goals of other heroes by chance
			{
				logAi->debugStream() << boost::format("%s") % p.second->completeMessage();
				lockedHeroes.erase (lockedHeroes.find(p.first)); //is it safe?
			}
		}
	}

}

void VCAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	NET_EVENT_HANDLER;
	assert(playerID > PlayerColor::PLAYER_LIMIT || status.getBattle() == UPCOMING_BATTLE);
	status.setBattle(ONGOING_BATTLE);
	const CGObjectInstance *presumedEnemy = backOrNull(cb->getVisitableObjs(tile)); //may be nullptr in some very are cases -> eg. visited monolith and fighting with an enemy at the FoW covered exit
	battlename = boost::str(boost::format("Starting battle of %s attacking %s at %s") % (hero1 ? hero1->name : "a army") % (presumedEnemy ? presumedEnemy->hoverName : "unknown enemy") % tile);
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
	alreadyVisited.push_back(obj);
}

void VCAI::reserveObject(HeroPtr h, const CGObjectInstance *obj)
{
	reservedObjs.push_back(obj);
	reservedHeroesMap[h].push_back(obj);
	logAi->debugStream() << "reserved object id=" << obj->id << "; address=" << (intptr_t)obj << "; name=" << obj->getHoverText();
}

void VCAI::validateVisitableObjs()
{
	std::vector<const CGObjectInstance *> hlp;
	retreiveVisitableObjs(hlp, true);

	std::string errorMsg;
	auto shouldBeErased = [&](const CGObjectInstance *obj) -> bool
	{
		if(!vstd::contains(hlp, obj))
		{
			logAi->errorStream() << helperObjInfo[obj].name << " at " << helperObjInfo[obj].pos << errorMsg;
			return true;
		}
		return false;
	};

	//errorMsg is captured by ref so lambda will take the new text
	errorMsg = " shouldn't be on the visitable objects list!";
	erase_if(visitableObjs, shouldBeErased);

	for(auto &p : reservedHeroesMap)
	{
		errorMsg = " shouldn't be on list for hero " + p.first->name + "!";
		erase_if(p.second, shouldBeErased);
	}

	errorMsg = " shouldn't be on the reserved objs list!";
	erase_if(reservedObjs, shouldBeErased);

	//TODO overkill, hidden object should not be removed. However, we can't know if hidden object is erased from game.
	errorMsg = " shouldn't be on the already visited objs list!";
	erase_if(alreadyVisited, shouldBeErased);
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

std::vector<const CGObjectInstance *> VCAI::getFlaggedObjects() const
{
	std::vector<const CGObjectInstance *> ret;
	retreiveVisitableObjs(ret, true);
	erase_if(ret, [](const CGObjectInstance *obj)
	{
		return obj->tempOwner != ai->playerID;
	});
	return ret;
}

void VCAI::addVisitableObj(const CGObjectInstance *obj)
{
	visitableObjs.push_back(obj);
	helperObjInfo[obj] = ObjInfo(obj);
}

const CGObjectInstance * VCAI::lookForArt(int aid) const
{
	for(const CGObjectInstance *obj : ai->visitableObjs)
	{
		if(obj->ID == 5 && obj->subID == aid)
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
	cb->setSelection(*h);
	if (!includeAllies)
	{ //don't visit tile occupied by allied hero
		for (auto obj : cb->getVisitableObjs(pos))
		{
			if (obj->ID == Obj::HERO && obj->tempOwner == h->tempOwner && obj != h.get())
				return false;
		}
	}
	return cb->getPathInfo(pos)->reachable();
}

bool VCAI::moveHeroToTile(int3 dst, HeroPtr h)
{
	cb->setSelection(h.h); //make sure we are using the RIGHT pathfinder
	logAi->debugStream() << boost::format("Moving hero %s to tile %s") % h->name % dst;
	int3 startHpos = h->visitablePos();
	bool ret = false;
	if(startHpos == dst)
	{
		//FIXME: this assertion fails also if AI moves onto defeated guarded object
		assert(cb->getVisitableObjs(dst).size() > 1); //there's no point in revisiting tile where there is no visitable object
		cb->moveHero(*h, CGHeroInstance::convertPosition(dst, true));
		waitTillFree(); //movement may cause battle or blocking dialog
		ret = true;
	}
	else
	{
		CGPath path;
		cb->getPath2(dst, path);
		if(path.nodes.empty())
		{
            logAi->errorStream() << "Hero " << h->name << " cannot reach " << dst;
			//setGoal(h, INVALID);
			completeGoal (sptr(Goals::VisitTile(dst).sethero(h))); //TODO: better mechanism to determine goal
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
			//if (endpos == h->pos)
				continue;
// 			if(i > 1)
// 			{
// 				int3 afterEndPos = path.nodes[i-2].coord;
// 				if(afterEndPos.z != endpos.z)
//
// 			}
            //logAi->debugStream() << "Moving " << h->name << " from " << h->getPosition() << " to " << endpos;
			cb->moveHero(*h, CGHeroInstance::convertPosition(endpos, true));
			waitTillFree(); //movement may cause battle or blocking dialog
			boost::this_thread::interruption_point();
			if(!h) //we lost hero - remove all tasks assigned to him/her
			{
				lostHero(h);
				//we need to throw, otherwise hero will be assigned to sth again
				throw std::runtime_error("Hero was lost!");
			}

		}
		ret = !i;
	}
	if (auto visitedObject = frontOrNull(cb->getVisitableObjs(h->visitablePos()))) //we stand on something interesting
	{
		performObjectInteraction (visitedObject, h);
		//BNLOG("Hero %s moved from %s to %s at %s", h->name % startHpos % visitedObject->hoverName % h->visitablePos());
		//throw goalFulfilledException (CGoal(GET_OBJ).setobjid(visitedObject->id));
	}

	if(h) //we could have lost hero after last move
	{
		cb->recalculatePaths();
		if (startHpos == h->visitablePos() && !ret) //we didn't move and didn't reach the target
		{
			throw cannotFulfillGoalException("Invalid path found!");
		}
	}
    logAi->debugStream() << boost::format("Hero %s moved from %s to %s. Returning %d.") % h->name % startHpos % h->visitablePos() % ret;
	return ret;
}
void VCAI::tryRealize(Goals::Explore & g)
{
	throw cannotFulfillGoalException("EXPLORE is not a elementar goal!");
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
	//cb->recalculatePaths();
	if(!g.hero->movement)
		throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
	if(g.tile == g.hero->visitablePos()  &&  cb->getVisitableObjs(g.hero->visitablePos()).size() < 2)
	{
		logAi->warnStream() << boost::format("Why do I want to move hero %s to tile %s? Already standing on that tile! ")
												% g.hero->name % g.tile;
		throw goalFulfilledException (sptr(g));
	}
	//if(!g.isBlockedBorderGate(g.tile))
	//{
		if (ai->moveHeroToTile(g.tile, g.hero.get()))
		{
			throw goalFulfilledException (sptr(g));
		}
	//}
	//else
	//	throw cannotFulfillGoalException("There's a blocked gate!, we should never be here"); //CLEAR_WAY_TO should get keymaster tent
}

void VCAI::tryRealize(Goals::VisitHero & g)
{
	if(!g.hero->movement)
		throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
	if (ai->moveHeroToTile(g.tile, g.hero.get()))
	{
		throw goalFulfilledException (sptr(g));
	}
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
	if (g.hero->diggingStatus() == CGHeroInstance::CAN_DIG)
	{
		cb->dig(g.hero.get());
		setGoal(g.hero, sptr(Goals::Invalid())); // finished digging
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
	performTypicalActions(); //TODO: separate build and wander
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

std::vector<HeroPtr> VCAI::getUnblockedHeroes() const
{
	std::vector<HeroPtr> ret;
	boost::copy(cb->getHeroesInfo(), std::back_inserter(ret));

	for(auto h : lockedHeroes)
	{
		//if (!h.second.invalid()) //we can use heroes without valid goal
		if (h.second->goalType == Goals::DIG_AT_TILE || !h.first->movement) //experiment: use all heroes that have movement left, TODO: unlock heroes that couldn't realize their goals 
			erase_if_present(ret, h.first);
	}
	return ret;
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
    logAi->debugStream() << "Player " << static_cast<int>(playerID.getNum()) << " ends turn";
	if(!status.haveTurn())
	{
        logAi->errorStream() << "Not having turn at the end of turn???";
	}

	do
	{
		cb->endTurn();
	} while(status.haveTurn()); //for some reasons, our request may fail -> stop requesting end of turn only after we've received a confirmation that it's over

    logAi->debugStream() << "Player " << static_cast<int>(playerID.getNum()) << " ended turn";
}

void VCAI::striveToGoal(Goals::TSubgoal ultimateGoal)
{
	if (ultimateGoal->invalid())
		return;

	//we are looking for abstract goals
	auto abstractGoal = striveToGoalInternal (ultimateGoal, false);

	if (abstractGoal->invalid())
		return;

	//we received abstratc goal, need to find concrete goals
	striveToGoalInternal (abstractGoal, true);

	//TODO: save abstract goals not related to hero
}

Goals::TSubgoal VCAI::striveToGoalInternal(Goals::TSubgoal ultimateGoal, bool onlyAbstract)
{
	Goals::TSubgoal abstractGoal = sptr(Goals::Invalid());

	while(1)
	{
		Goals::TSubgoal goal = ultimateGoal;
        logAi->debugStream() << boost::format("Striving to goal of type %s") % ultimateGoal->name();
		int maxGoals = 100; //preventing deadlock for mutually dependent goals
		//FIXME: do not try to realize goal when loop didn't suceed
		while(!goal->isElementar && maxGoals && (onlyAbstract || !goal->isAbstract))
		{
            logAi->debugStream() << boost::format("Considering goal %s") % goal->name();
			try
			{
				boost::this_thread::interruption_point();
				goal = goal->whatToDoToAchieve();
				--maxGoals;
			}
			catch(std::exception &e)
			{
                logAi->debugStream() << boost::format("Goal %s decomposition failed: %s") % goal->name() % e.what();
				//setGoal (goal.hero, INVALID); //test: if we don't know how to realize goal, we should abandon it for now
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
					setGoal(goal->hero, sptr(Goals::Invalid())); // we seemingly don't know what to do with hero
				}
			}

			if (goal->isAbstract)
			{
				abstractGoal = goal; //allow only one abstract goal per call
                logAi->debugStream() << boost::format("Choosing abstract goal %s") % goal->name();
				break;
			}
			else
				goal->accept(this);

			boost::this_thread::interruption_point();
		}
		catch(boost::thread_interrupted &e)
		{
            logAi->debugStream() << boost::format("Player %d: Making turn thread received an interruption!") % playerID;
			throw; //rethrow, we want to truly end this thread
		}
		catch(goalFulfilledException &e)
		{
			completeGoal (goal);
			if (ultimateGoal->fulfillsMe(goal) || maxGoals > 98) //completed goal was main goal //TODO: find better condition
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
	if (q.quest->missionType && q.quest->progress != CQuest::COMPLETE) //FIXME: quests are never synchronized. Pointer handling needed
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
					if (q.quest->checkQuest(hero)) //veyr bad info - stacks can be split between multiple heroes :(
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
						 striveToGoal (sptr(Goals::VisitTile(q.tile)));
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
					striveToGoal (sptr(Goals::VisitTile(q.tile))); //visit seer hut
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
						striveToGoal (sptr(Goals::VisitTile(q.tile).sethero(hero))); //TODO: causes infinite loop :/
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

	for(auto h : getUnblockedHeroes())
	{
        logAi->debugStream() << boost::format("Looking into %s, MP=%d") % h->name.c_str() % h->movement;
		makePossibleUpgrades(*h);
		cb->setSelection(*h);
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
	recruitCreatures(t);
	moveCreaturesToHero(t);
}

int3 VCAI::explorationBestNeighbour(int3 hpos, int radius, HeroPtr h)
{
	std::map<int3, int> dstToRevealedTiles;
	for(crint3 dir : dirs)
		if(cb->isInTheMap(hpos+dir))
			if (isSafeToVisit(h, hpos + dir) && isAccessibleForHero (hpos + dir, h))
				dstToRevealedTiles[hpos + dir] = howManyTilesWillBeDiscovered(radius, hpos, dir);

	auto best = dstToRevealedTiles.begin();
	for (auto i = dstToRevealedTiles.begin(); i != dstToRevealedTiles.end(); i++)
	{
		const CGPathNode *pn = cb->getPathInfo(i->first);
		//const TerrainTile *t = cb->getTile(i->first);
		if(best->second < i->second  && i->second && pn->reachable() && pn->accessible == CGPathNode::ACCESSIBLE)
			best = i;
	}

	if(best->second)
		return best->first;

	throw cannotFulfillGoalException("No neighbour will bring new discoveries!");
}

int3 VCAI::explorationNewPoint(int radius, HeroPtr h, std::vector<std::vector<int3> > &tiles)
{
    logAi->debugStream() << "Looking for an another place for exploration...";
	tiles.resize(radius);

	foreach_tile_pos([&](const int3 &pos)
	{
		if(!cb->isVisible(pos))
			tiles[0].push_back(pos);
	});

	for (int i = 1; i < radius; i++)
	{
		getVisibleNeighbours(tiles[i-1], tiles[i]);
		removeDuplicates(tiles[i]);

		for(const int3 &tile : tiles[i])
		{
			if (cb->getTile(tile)->blocked) //does it shorten the time?
				continue;
			if(cb->getPathInfo(tile)->reachable() && howManyTilesWillBeDiscovered(tile, radius) &&
				isSafeToVisit(h, tile) && !isBlockedBorderGate(tile))
			{
				return tile; //return first tile that will discover anything
			}
		}
	}
	return int3 (-1,-1,-1);
	//throw cannotFulfillGoalException("No accessible tile will bring discoveries!");
}

TResources VCAI::estimateIncome() const
{
	TResources ret;
	for(const CGTownInstance *t : cb->getTownsInfo())
	{
		ret[Res::GOLD] += t->dailyIncome();

		//TODO duplikuje newturn
		if(t->hasBuilt(BuildingID::RESOURCE_SILO)) //there is resource silo
		{
			if(t->town->primaryRes == Res::WOOD_AND_ORE) //we'll give wood and ore
			{
				ret[Res::WOOD] ++;
				ret[Res::ORE] ++;
			}
			else
			{
				ret[t->town->primaryRes] ++;
			}
		}
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

	if(auto availableHero = frontOrNull(cb->getAvailableHeroes(t)))
		cb->recruitHero(t, availableHero);
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
// 	static boost::mutex m;
// 	boost::unique_lock<boost::mutex> mylock(m);

	boost::barrier b(2);
	boost::thread newThread([&b,this,whatToDo]()
	{
		setThreadName("VCAI::requestActionASAP::helper");
		SET_GLOBAL_STATE(this);
		boost::shared_lock<boost::shared_mutex> gsLock(cb->getGsMutex());
		b.wait();
		whatToDo();
	});
	b.wait();
}

void VCAI::lostHero(HeroPtr h)
{
    logAi->debugStream() << boost::format("I lost my hero %s. It's best to forget and move on.") % h.name;

	erase_if_present(lockedHeroes, h);
	for(auto obj : reservedHeroesMap[h])
	{
		erase_if_present(reservedObjs, obj); //unreserve all objects for that hero
	}
	erase_if_present(reservedHeroesMap, h);
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
	auto matchesId = [&] (const CGObjectInstance *hlpObj) -> bool { return hlpObj->id == obj.id; };
	if(!obj)
	{
		erase_if(visitableObjs, matchesId);

		for(auto &p : reservedHeroesMap)
			erase_if(p.second, matchesId);

		erase_if(reservedObjs, matchesId);
	}
}

TResources VCAI::freeResources() const
{
	TResources myRes = cb->getResourceAmount();
	myRes[Res::GOLD] -= GOLD_RESERVE;
	vstd::amax(myRes[Res::GOLD], 0);
	return myRes;
}

AIStatus::AIStatus()
{
	battle = NO_BATTLE;
	havingTurn = false;
	ongoingHeroMovement = false;
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
	boost::unique_lock<boost::mutex> lock(mx);
	if(ID == QueryID(-1))
	{
        logAi->debugStream() << boost::format("The \"query\" has an id %d, it'll be ignored as non-query. Description: %s") % ID % description;
		return;
	}

	assert(!vstd::contains(remainingQueries, ID));
	assert(ID.getNum() >= 0);

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

SectorMap::SectorMap()
{
// 	int3 sizes = cb->getMapSize();
// 	sector.resize(sizes.x);
// 	for(auto &i : sector)
// 		i.resize(sizes.y);
//
// 	for(auto &i : sector)
// 		for(auto &j : i)
// 			j.resize(sizes.z, 0);
	update();
}

bool markIfBlocked(ui8 &sec, crint3 pos, const TerrainTile *t)
{
	if(t->blocked && !t->visitable)
	{
		sec = NOT_AVAILABLE;
		return true;
	}

	return false;
}

bool markIfBlocked(ui8 &sec, crint3 pos)
{
	return markIfBlocked(sec, pos, cb->getTile(pos));
}

void SectorMap::update()
{
	clear();
	int curSector = 3; //0 is invisible, 1 is not explored
	foreach_tile_pos([&](crint3 pos)
	{
		if(retreiveTile(pos) == NOT_CHECKED)
		{
			if(!markIfBlocked(retreiveTile(pos), pos))
				exploreNewSector(pos, curSector++);
		}
	});
	valid = true;
}

void SectorMap::clear()
{
	sector = cb->getVisibilityMap();
	valid = false;
}

void SectorMap::exploreNewSector(crint3 pos, int num)
{
	Sector &s = infoOnSectors[num];
	s.id = num;
	s.water = cb->getTile(pos)->isWater();

	std::queue<int3> toVisit;
	toVisit.push(pos);
	while(!toVisit.empty())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		ui8 &sec = retreiveTile(curPos);
		if(sec == NOT_CHECKED)
		{
			const TerrainTile *t = cb->getTile(curPos);
			if(!markIfBlocked(sec, curPos, t))
			{
				if(t->isWater() == s.water) //sector is only-water or only-land
				{
					sec = num;
					s.tiles.push_back(curPos);
					foreach_neighbour(curPos, [&](crint3 neighPos)
					{
						if(retreiveTile(neighPos) == NOT_CHECKED)
						{
							toVisit.push(neighPos);
							//parent[neighPos] = curPos;
						}
						const TerrainTile *nt = cb->getTile(neighPos, false);
						if(nt && nt->isWater() != s.water && canBeEmbarkmentPoint(nt))
						{
							s.embarkmentPoints.push_back(neighPos);
						}
					});
					if(t->visitable && vstd::contains(ai->knownSubterraneanGates, t->visitableObjects.front()))
						toVisit.push(ai->knownSubterraneanGates[t->visitableObjects.front()]->visitablePos());
				}
			}
		}
	}

	removeDuplicates(s.embarkmentPoints);
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
		case Obj::BORDERGUARD:
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
		case Obj::SEER_HUT:
		case Obj::QUEST_GUARD:
		{
			for (auto q : ai->myCb->getMyQuests())
			{
				if (q.obj == obj)
				{
					if (q.quest->checkQuest(*h))
						return true; //we completed the quest
					else
						return false; //we can't complete this quest
				}
			}
			return true; //we don't have this quest yet
		}
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
		case Obj::MONOLITH1:
		case Obj::MONOLITH2:
		case Obj::MONOLITH3:
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
			return ai->myCb->getHeroesInfo().size() < GameConstants::MAX_HEROES_PER_PLAYER;

		case Obj::BOAT:
			return false;
			//Boats are handled by pathfinder
	}

	if (obj->wasVisited(*h)) //it must pointer to hero instance, heroPtr calls function wasVisited(ui8 player);
		return false;

	return true;
}

int3 SectorMap::firstTileToGet(HeroPtr h, crint3 dst)
{
	int sourceSector = retreiveTile(h->visitablePos()),
		destinationSector = retreiveTile(dst);

	if(sourceSector != destinationSector)
	{
		const Sector *src = &infoOnSectors[sourceSector],
			*dst = &infoOnSectors[destinationSector];

		std::map<const Sector*, const Sector*> preds;
		std::queue<const Sector *> sq;
		sq.push(src);
		while(!sq.empty())
		{
			const Sector *s = sq.front();
			sq.pop();

			for(int3 ep : s->embarkmentPoints)
			{
				Sector *neigh = &infoOnSectors[retreiveTile(ep)];
				//preds[s].push_back(neigh);
				if(!preds[neigh])
				{
					preds[neigh] = s;
					sq.push(neigh);
				}
			}

			//TODO consider other types of connections between sectors?
		}

		if(!preds[dst])
		{
			write("test.txt");
			ai->completeGoal (sptr(Goals::Explore(h))); //if we can't find the way, seemingly all tiles were explored
			//TODO: more organized way?
            throw cannotFulfillGoalException(boost::str(boost::format("Cannot find connection between sectors %d and %d") % src->id % dst->id));
		}

		std::vector<const Sector*> toTraverse;
		toTraverse.push_back(dst);
		while(toTraverse.back() != src)
		{
			toTraverse.push_back(preds[toTraverse.back()]);
		}

		if(preds[dst])
		{
			const Sector *sectorToReach  = toTraverse.at(toTraverse.size() - 2);
			if(!src->water && sectorToReach->water) //embark
			{
				//embark on ship -> look for an EP with a boat
				auto firstEP = boost::find_if(src->embarkmentPoints, [=](crint3 pos) -> bool
				{
					const TerrainTile *t = cb->getTile(pos);
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

					std::vector<const CGObjectInstance*> visObjs;
					ai->retreiveVisitableObjs(visObjs, true);
					for(const CGObjectInstance *obj : visObjs)
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
						throw cannotFulfillGoalException("There is no known shipyard!");
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
							cb->buildBoat(s);
							return ret;
						}
						else
						{
							//TODO gather res
							throw cannotFulfillGoalException("Not enough resources to build a boat");
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
			}
			else
			{
				//TODO
				//transition between two land/water sectors. Monolith? Whirlpool? ...
				throw cannotFulfillGoalException("Land-land and water-water inter-sector transitions are not implemented!");
			}
		}
		else
		{
			throw cannotFulfillGoalException("Inter-sector route detection failed: not connected sectors?");
		}
	}
	else
	{
		makeParentBFS(h->visitablePos());
		int3 curtile = dst;
		while(curtile != h->visitablePos())
		{
			if(cb->getPathInfo(curtile)->reachable())
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
					throw cannotFulfillGoalException("Unreachable tile in sector? Should not happen!");
			}
		}
	}

	throw cannotFulfillGoalException("Impossible happened.");
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

		//const TerrainTile *t = cb->getTile(curPos);
		foreach_neighbour(curPos, [&](crint3 neighPos)
		{
			if(retreiveTile(neighPos) == mySector && !vstd::contains(parent, neighPos))
			{
				toVisit.push(neighPos);
				parent[neighPos] = curPos;
			}
		});
	}
}

unsigned char & SectorMap::retreiveTile(crint3 pos)
{
	return retreiveTileN(sector, pos);
}

const CGObjectInstance * ObjectIdRef::operator->() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::operator const CGObjectInstance*() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::ObjectIdRef(ObjectInstanceID _id) : id(_id)
{

}

ObjectIdRef::ObjectIdRef(const CGObjectInstance *obj) : id(obj->id)
{

}

bool ObjectIdRef::operator<(const ObjectIdRef &rhs) const
{
	return id < rhs.id;
}

HeroPtr::HeroPtr(const CGHeroInstance *H)
{
	if(!H)
	{
		//init from nullptr should equal to default init
		*this = HeroPtr();
		return;
	}

	h = H;
	name = h->name;

	hid = H->id;
//	infosCount[ai->playerID][hid]++;
}

HeroPtr::HeroPtr()
{
	h = nullptr;
	hid = ObjectInstanceID();
}

HeroPtr::~HeroPtr()
{
// 	if(hid >= 0)
// 		infosCount[ai->playerID][hid]--;
}

bool HeroPtr::operator<(const HeroPtr &rhs) const
{
	return hid < rhs.hid;
}

const CGHeroInstance * HeroPtr::get(bool doWeExpectNull /*= false*/) const
{
	//TODO? check if these all assertions every time we get info about hero affect efficiency
	//
	//behave terribly when attempting unauthorized access to hero that is not ours (or was lost)
	assert(doWeExpectNull || h);

	if(h)
	{
		auto obj = cb->getObj(hid);
		const bool owned = obj && obj->tempOwner == ai->playerID;

		if(doWeExpectNull && !owned)
		{
			return nullptr;
		}
		else
		{
			assert(obj);
			assert(owned);
		}
	}

	return h;
}

const CGHeroInstance * HeroPtr::operator->() const
{
	return get();
}

bool HeroPtr::validAndSet() const
{
	return get(true);
}

const CGHeroInstance * HeroPtr::operator*() const
{
	return get();
}


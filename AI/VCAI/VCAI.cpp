#include "StdInc.h"
#include "VCAI.h"
#include "../../lib/UnlockGuard.h"
#include "Fuzzy.h"
#include "../../lib/CObjectHandler.h"

#define I_AM_ELEMENTAR return CGoal(*this).setisElementar(true)
CLogger &aiLogger = tlog6;

extern FuzzyHelper *fh;

class CGVisitableOPW;

const int ACTUAL_RESOURCE_COUNT = 7;

const double SAFE_ATTACK_CONSTANT = 1.5;
using namespace vstd;

//one thread may be turn of AI and another will be handling a side effect for AI2
boost::thread_specific_ptr<CCallback> cb;
boost::thread_specific_ptr<VCAI> ai;

// CCallback *cb;
// VCAI *ai;

//helper RAII to manage global ai/cb ptrs
struct SetGlobalState
{
	SetGlobalState(VCAI * AI)
	{
		assert(!ai.get());
		assert(!cb.get());

		ai.reset(AI);
		cb.reset(AI->myCb);
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

const int GOLD_RESERVE = 10000; //when buying creatures we want to keep at least this much gold (10000 so at least we'll be able to reach capitol)
const int HERO_GOLD_COST = 2500;
const int ALLOWED_ROAMING_HEROES = 8;

const int GOLD_MINE_PRODUCTION = 1000, WOOD_ORE_MINE_PRODUCTION = 2, RESOURCE_MINE_PRODUCTION = 1;

bool compareHeroStrength(const CGHeroInstance *h1, const CGHeroInstance *h2)
{
	return h1->getTotalStrength() < h2->getTotalStrength();
}
bool compareArmyStrength(const CArmedInstance *a1, const CArmedInstance *a2)
{
	return a1->getArmyStrength() < a2->getArmyStrength();
}

static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
	int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

struct AILogger
{
	AILogger()
	{
		lvl = 0;
	}

	int lvl;

	struct Tab
	{
		Tab();
		~Tab();
	};
} logger;

AILogger::Tab::Tab()
{
	logger.lvl++;
}
AILogger::Tab::~Tab()
{
	logger.lvl--;
}

struct TimeCheck
{
	CStopWatch time;
	std::string txt;
	TimeCheck(crstring TXT) : txt(TXT)
	{
	}

	~TimeCheck()
	{
		BNLOG("Time of %s was %d ms.", txt % time.getDiff());
	}
};

template<typename T>
void removeDuplicates(std::vector<T> &vec)
{
	boost::sort(vec);
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}


template<typename Range, typename Predicate>
void erase_if(Range &vec, Predicate pred)
{
	vec.erase(boost::remove_if(vec, pred),vec.end());
}

struct AtScopeExit
{
	boost::function<void()> foo;
	AtScopeExit(const boost::function<void()> &FOO) : foo(FOO)
	{}
	~AtScopeExit()
	{
		foo();
	}
};

void foreach_tile_pos(boost::function<void(const int3& pos)> foo)
{
	for(int i = 0; i < cb->getMapSize().x; i++)
		for(int j = 0; j < cb->getMapSize().y; j++)
			for(int k = 0; k < cb->getMapSize().z; k++)
				foo(int3(i,j,k));
}

void foreach_neighbour(const int3 &pos, boost::function<void(const int3& pos)> foo)
{
	BOOST_FOREACH(const int3 &dir, dirs)
	{
		const int3 n = pos + dir;
		if(cb->isInTheMap(n))
			foo(pos+dir);
	}
}

unsigned char &retreiveTileN(std::vector< std::vector< std::vector<unsigned char> > > &vectors, const int3 &pos)
{
	return vectors[pos.x][pos.y][pos.z];
}

const unsigned char &retreiveTileN(const std::vector< std::vector< std::vector<unsigned char> > > &vectors, const int3 &pos)
{
	return vectors[pos.x][pos.y][pos.z];
}

void foreach_tile(std::vector< std::vector< std::vector<unsigned char> > > &vectors, boost::function<void(unsigned char &in)> foo)
{
	for(auto i = vectors.begin(); i != vectors.end(); i++)
		for(auto j = i->begin(); j != i->end(); j++)
			for(auto z = j->begin(); z != j->end(); z++)
				foo(*z);
}

struct ObjInfo
{
	int3 pos;
	std::string name;
	ObjInfo(){}
	ObjInfo(const CGObjectInstance *obj)
	{
		pos = obj->pos;
		name = obj->getHoverText();
	}
};

std::map<const CGObjectInstance *, ObjInfo> helperObjInfo;

template <typename Container, typename Item>
bool remove_if_present(Container &c, const Item &item)
{
	auto i = std::find(c.begin(), c.end(), item);
	if(i != c.end())
	{
		c.erase(i);
		return true;
	}

	return false;
}

template <typename Container, typename Pred>
void erase(Container &c, Pred pred)
{
	c.erase(boost::remove_if(c, pred), c.end());
}

bool isReachable(const CGObjectInstance *obj)
{
	return cb->getPathInfo(obj->visitablePos())->turns < 255;
}

ui64 howManyReinforcementsCanGet(const CGHeroInstance *h, const CGTownInstance *t)
{
	ui64 ret = 0;
	int freeHeroSlots = GameConstants::ARMY_SIZE - h->stacksCount();
	std::vector<const CStackInstance *> toMove;
	BOOST_FOREACH(auto const slot, t->Slots())
	{
		//can be merged woth another stack?
		TSlot dst = h->getSlotFor(slot.second->getCreatureID());
		if(h->hasStackAtSlot(dst))
			ret += t->getPower(slot.first);
		else
			toMove.push_back(slot.second);
	}
	boost::sort(toMove, [](const CStackInstance *lhs, const CStackInstance *rhs)
	{
		return lhs->getPower() < rhs->getPower();
	});
	BOOST_REVERSE_FOREACH(const CStackInstance *stack, toMove)
	{
		if(freeHeroSlots)
		{
			ret += stack->getPower();
			freeHeroSlots--;
		}
		else
			break;
	}
	return ret;
}

std::string strFromInt3(int3 pos)
{
	std::ostringstream oss;
	oss << pos;
	return oss.str();
}

bool isCloser(const CGObjectInstance *lhs, const CGObjectInstance *rhs)
{
	const CGPathNode *ln = cb->getPathInfo(lhs->visitablePos()), *rn = cb->getPathInfo(rhs->visitablePos());
	if(ln->turns != rn->turns)
		return ln->turns < rn->turns;

	return (ln->moveRemains > rn->moveRemains);
};

ui64 evaluateDanger(const CGObjectInstance *obj);

ui64 evaluateDanger(crint3 tile)
{
	const TerrainTile *t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0, guardDanger = 0;

	if(t->visitable)
		objectDanger = evaluateDanger(t->visitableObjects.back());

	int3 guardPos = cb->guardingCreaturePosition(tile);
	if(guardPos.x >= 0 && guardPos != tile)
		guardDanger = evaluateDanger(guardPos);

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);

	return 0;
}

ui64 evaluateDanger(crint3 tile, const CGHeroInstance *visitor)
{
	const TerrainTile *t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0, guardDanger = 0;
	CArmedInstance * dangerousObject;

	if(t->visitable)
	{
		dangerousObject = dynamic_cast<CArmedInstance*>(t->visitableObjects.back());
		if (dangerousObject)
		{
			objectDanger = evaluateDanger(dangerousObject);
			objectDanger *= fh->getTacticalAdvantage (visitor, dangerousObject);
		}
		else
		{
			objectDanger = 0;
		}
	}

	int3 guardPos = cb->guardingCreaturePosition(tile);
	if(guardPos.x >= 0 && guardPos != tile)
		guardDanger = evaluateDanger(guardPos, visitor);

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);

	return 0;
}

ui64 evaluateDanger(const CGObjectInstance *obj)
{
	if(obj->tempOwner < GameConstants::PLAYER_LIMIT && cb->getPlayerRelations(obj->tempOwner, ai->playerID)) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case GameConstants::HEROI_TYPE:
		{
			InfoAboutHero iah;
			cb->getHeroInfo(obj, iah);
			return iah.army.getStrength();
		}
	case GameConstants::TOWNI_TYPE:
	case Obj::GARRISON: case Obj::GARRISON2: //garrison
		{
			InfoAboutTown iat;
			cb->getTownInfo(obj, iat);
			return iat.army.getStrength();
		}
	case GameConstants::CREI_TYPE:
		{
			//TODO!!!!!!!!
			const CGCreature *cre = dynamic_cast<const CGCreature*>(obj);
			return cre->getArmyStrength();
		}
	case Obj::CRYPT: //crypt
	case Obj::CREATURE_BANK: //crebank
	case Obj::DRAGON_UTOPIA:
	case Obj::SHIPWRECK: //shipwreck
	case Obj::DERELICT_SHIP: //derelict ship
	case Obj::PYRAMID:
		return fh->estimateBankDanger (VLC->objh->bankObjToIndex(obj));
	case Obj::WHIRLPOOL: //whirlpool
	case Obj::MONOLITH1:
	case Obj::MONOLITH2:
	case Obj::MONOLITH3:
		//TODO mechanism for handling monoliths
		return 1000000000;
	default:
		return 0;
	}
}

bool compareDanger(const CGObjectInstance *lhs, const CGObjectInstance *rhs)
{
	return evaluateDanger(lhs) < evaluateDanger(rhs);
}

VCAI::VCAI(void)
{
	LOG_ENTRY;
	myCb = NULL;
	battleAIName = "StupidAI";
	makingTurn = NULL;
}


VCAI::~VCAI(void)
{
	LOG_ENTRY;
}

void VCAI::availableCreaturesChanged(const CGDwelling *town)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroMoved(const TryMoveHero & details)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if(details.result == TryMoveHero::TELEPORTATION)
	{
		const TerrainTile *t1 = cb->getTile(CGHeroInstance::convertPosition(details.start, false), false),
			*t2 = cb->getTile(CGHeroInstance::convertPosition(details.end, false), false);
		if(!t1 || !t2) //enemy may have teleported to a tile we don't see
			return;
		if(t1->visitable && t2->visitable)
		{
			const CGObjectInstance *o1 = t1->visitableObjects.front(),
				*o2 = t2->visitableObjects.front();

			if(o1->ID == Obj::SUBTERRANEAN_GATE && o2->ID == Obj::SUBTERRANEAN_GATE)
			{
				knownSubterraneanGates[o1] = o2;
				knownSubterraneanGates[o2] = o1;
			}
		}

	}
}

void VCAI::stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroInGarrisonChange(const CGTownInstance *town)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::centerView(int3 pos, int focusTime)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::artifactAssembled(const ArtifactLocation &al)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showTavernWindow(const CGObjectInstance *townOrTavern)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showThievesGuildWindow (const CGObjectInstance * obj)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::playerBlocked(int reason)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if (reason == PlayerBlocked::UPCOMING_BATTLE)
		status.setBattle(UPCOMING_BATTLE);
}

void VCAI::showPuzzleMap()
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showShipyardDialog(const IShipyard *obj)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::gameOver(ui8 player, bool victory)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	BNLOG("Player %d: I heard that player %d %s.", playerID % (int)player % (victory ? "won" : "lost"));
	if(player == playerID)
	{
		if(victory)
		{
			tlog0 << "VCAI: I won! Incredible!\n";
			tlog0 << "Turn nr " << myCb->getDate() << std::endl;
		}
		else
		{
			tlog0 << "VCAI: Player " << (int)player << " lost. It's me. What a disappointment! :(\n";
		}

// 		//let's make Impossible difficulty finally standing to its name :>
// 		if(myCb->getStartInfo()->difficulty == 4 && !victory)
// 		{
// 			//play dirty: crash the whole engine to avoid lose
// 			//that way AI is unbeatable!
// 			*(int*)NULL = 666;
// 		}

// 		TODO - at least write some insults on stdout

		finish();
	}
}

void VCAI::artifactPut(const ArtifactLocation &al)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::artifactRemoved(const ArtifactLocation &al)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::stacksErased(const StackLocation &location)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::artifactDisassembled(const ArtifactLocation &al)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}


void VCAI::heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if (start)
	{
		visitedObject = const_cast<CGObjectInstance *>(visitedObj); // remember teh object and wait for return
		if(visitedObj->ID != Obj::MONSTER) //TODO: poll bank if it was cleared
			alreadyVisited.push_back(visitedObj);
	}
}

void VCAI::availableArtifactsChanged(const CGBlackMarket *bm /*= NULL*/)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	//buildArmyIn(town);
	//moveCreaturesToHero(town);
}

void VCAI::tileHidden(const boost::unordered_set<int3, ShashInt3> &pos)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
// 	BOOST_FOREACH(int3 tile, pos)
// 		BOOST_FOREACH(const CGObjectInstance *obj, cb->getVisitableObjs(tile))
// 			remove_if_present(visitableObjs, obj);
	visitableObjs.erase(boost::remove_if(visitableObjs, [&](const CGObjectInstance *obj){return !myCb->getObj(obj->id);}), visitableObjs.end());
}

void VCAI::tileRevealed(const boost::unordered_set<int3, ShashInt3> &pos)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	BOOST_FOREACH(int3 tile, pos)
		BOOST_FOREACH(const CGObjectInstance *obj, myCb->getVisitableObjs(tile))
			addVisitableObj(obj);
}

void VCAI::heroExchangeStarted(si32 hero1, si32 hero2)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroMovePointsChanged(const CGHeroInstance * hero)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::stackChangedType(const StackLocation &location, const CCreature &newType)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::newObject(const CGObjectInstance * obj)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if(obj->isVisitable())
		addVisitableObj(obj);
}

void VCAI::objectRemoved(const CGObjectInstance *obj)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if(remove_if_present(visitableObjs, obj))
		assert(obj->isVisitable());
}

void VCAI::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::playerBonusChanged(const Bonus &bonus, bool gain)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::newStackInserted(const StackLocation &location, const CStackInstance &stack)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroCreated(const CGHeroInstance*)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::advmapSpellCast(const CGHeroInstance * caster, int spellID)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::requestRealized(PackageApplied *pa)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if(status.haveTurn())
	{
		if(pa->packType == typeList.getTypeID<EndTurn>())
			if(pa->result)
				status.madeTurn();
	}

	if(pa->packType == typeList.getTypeID<QueryReply>())
	{
		status.removeQuery();
	}
}

void VCAI::receivedResource(int type, int val)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::stacksSwapped(const StackLocation &loc1, const StackLocation &loc2)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroManaPointsChanged(const CGHeroInstance * hero)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::battleResultsApplied()
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	assert(status.getBattle() == ENDING_BATTLE);
	status.setBattle(NO_BATTLE);
}

void VCAI::objectPropertyChanged(const SetObjectProperty * sop)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	if(sop->what == ObjProperty::OWNER)
	{
		if(sop->val == playerID)
			remove_if_present(visitableObjs, myCb->getObj(sop->id));
		//TODO restore lost obj
	}
}

void VCAI::buildChanged(const CGTownInstance *town, int buildingID, int what)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::showMarketWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::init(CCallback * CB)
{
	myCb = CB;
	cbc = CB;
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	playerID = myCb->getMyColor();
	myCb->waitTillRealize = true;
	myCb->unlockGsWhenWaiting = true;

	if(!fh)
		fh = new FuzzyHelper();

	retreiveVisitableObjs(visitableObjs);
}

void VCAI::yourTurn()
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	status.startedTurn();
	makingTurn = new boost::thread(&VCAI::makeTurn, this);
}

void VCAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	status.addQuery();
	callback(0);
}

void VCAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	int sel = 0;
	status.addQuery();

	if(selection) //select from multiple components -> take the last one (they're indexed [1-size])
		sel = components.size();

	if(!selection && cancel) //yes&no -> always answer yes, we are a brave AI :)
		sel = 1;

	cb->selectionMade(sel, askID);
}

void VCAI::showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	status.addQuery();
	pickBestCreatures (down, up);
	onEnd();
}

void VCAI::serialize(COSer<CSaveFile> &h, const int version)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void VCAI::serialize(CISer<CLoadFile> &h, const int version)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
}

void makePossibleUpgrades(const CArmedInstance *obj)
{
	if(!obj)
		return;

	for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
	{
		if(const CStackInstance *s = obj->getStackPtr(i))
		{
			UpgradeInfo ui;
			cb->getUpgradeInfo(obj, i, ui);
			if(ui.oldID >= 0 && cb->getResourceAmount().canAfford(ui.cost[0] * s->count))
			{
				cb->upgradeCreature(obj, i, ui.newID[0]);
			}
		}
	}
}

void VCAI::makeTurn()
{
	MAKING_TURN;
	boost::shared_lock<boost::shared_mutex> gsLock(cb->getGsMutex());
	setThreadName(-1, "VCAI::makeTurn");

	BNLOG("Player %d starting turn", playerID);
	INDENT;

	if(cb->getDate(1) == 1)
	{
		townVisitsThisWeek.clear();
		std::vector<const CGObjectInstance *> objs;
		retreiveVisitableObjs(objs, true);
		BOOST_FOREACH(const CGObjectInstance *obj, objs)
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
	if(cb->getSelectedHero())
		cb->recalculatePaths();

	makeTurnInternal();
	vstd::clear_pointer(makingTurn);

	return;
}

void VCAI::makeTurnInternal()
{
	blockedHeroes.clear();
	saving = 0;

	//it looks messy here, but it's better to have armed heroes before attempting realizing goals
	BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
		moveCreaturesToHero(t);

	try
	{
		striveToGoal(CGoal(WIN));
		striveToGoal(CGoal(BUILD));
		striveToGoal(CGoal(EXPLORE)); //if we have any MPs left, why not use them?
	}
	catch(boost::thread_interrupted &e)
	{
		tlog0 << "Making turn thread has been interrupted. We'll end without calling endTurn.\n";
		return;
	}
	catch(std::exception &e)
	{
		tlog0 << "Making turn thread has caught an exception: " << e.what() << "\n";
	}

	endTurn();
}

bool VCAI::goVisitObj(const CGObjectInstance * obj, const CGHeroInstance * h)
{
	int3 dst = obj->visitablePos();
	BNLOG("%s will try to visit %s at (%s)", h->name % obj->hoverName % strFromInt3(dst));
	return moveHeroToTile(dst, h);
}

void VCAI::performObjectInteraction(const CGObjectInstance * obj, const CGHeroInstance * h)
{
	switch (obj->ID)
	{
		case Obj::CREATURE_GENERATOR1:
			recruitCreatures(dynamic_cast<const CGDwelling *>(obj));
		break;
	}
}

void VCAI::moveCreaturesToHero(const CGTownInstance * t)
{
	if(t->visitingHero && t->armedGarrison())
	{
		pickBestCreatures (t->visitingHero, t);
	}
}

void VCAI::pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source)
{
	//TODO - what if source is a hero (the last stack problem) -> it'd good to create a single stack of weakest cre
	const CArmedInstance *armies[] = {army, source};
	//we calculate total strength for each creature type available in armies
	std::map<const CCreature*, int> creToPower;
	BOOST_FOREACH(auto armyPtr, armies)
		BOOST_FOREACH(auto &i, armyPtr->Slots())
			creToPower[i.second->type] += i.second->getPower(); 
	//TODO - consider more than just power (ie morale penalty, hero specialty in certain stacks, etc)

	std::vector<const CCreature *> bestArmy; //types that'll be in final dst army
	for (int i = 0; i < GameConstants::ARMY_SIZE; i++) //pick the creatures from which we can get most power, as many as dest can fit
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
		BOOST_FOREACH(auto armyPtr, armies) 
			for (int j = 0; j < GameConstants::ARMY_SIZE; j++)
				if(armyPtr->getCreature(j) == bestArmy[i]  &&  (i != j || armyPtr != army)) //it's a searched creature not in dst slot
						cb->mergeOrSwapStacks(armyPtr, army, j, i);

	//TODO - having now strongest possible army, we may want to think about arranging stacks
}

void VCAI::recruitCreatures(const CGDwelling * d)
{
	for(int i = 0; i < d->creatures.size(); i++)
	{
		if(!d->creatures[i].second.size())
			continue;

		int count = d->creatures[i].first;
		int creID = d->creatures[i].second.back();
//		const CCreature *c = VLC->creh->creatures[creID];
// 		if(containsSavedRes(c->cost))
// 			continue;

		TResources myRes = cb->getResourceAmount();
		myRes[Res::GOLD] -= GOLD_RESERVE;
		amin(count, myRes / VLC->creh->creatures[creID]->cost);
		if(count > 0)
			cb->recruitCreatures(d, creID, count, i);
	}
}

void VCAI::buildStructure(const CGTownInstance * t)
{
	//TODO make *real* town development system
	const int buildings[] = {5, 11, 14, 16, 0, 12, 7, 8, 9, 13, 30, 31, 32, 33, 34, 35, 36, 37, 38,
								39, 40, 41, 42, 43, 1, 2, 3, 4, 17, 18, 19, 21, 22, 23};
	for(int i = 0; i < ARRAY_COUNT(buildings); i++)
	{
		if(t->hasBuilt(buildings[i]))
			continue;

		const CBuilding *b = VLC->buildh->buildings[t->subID][buildings[i]];

		int canBuild = cb->canBuildStructure(t, buildings[i]);
		if(canBuild == EBuildingState::ALLOWED)
		{
			if(!containsSavedRes(b->resources))
			{
				BNLOG("Player %d will build %s in town of %s at %s", playerID % b->Name() % t->name % t->pos);
				cb->buildBuilding(t, buildings[i]);
			}
			break;
		}
		else if(canBuild == EBuildingState::NO_RESOURCES)
		{
			TResources mine = cb->getResourceAmount(), cost = VLC->buildh->buildings[t->subID][buildings[i]]->resources,
				income = estimateIncome();
			for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
			{
				int diff = mine[i] - cost[i] + income[i];
				if(diff < 0)
					saving[i] = 1;
			}

			continue;
		}
	}
}

bool isSafeToVisit(const CGHeroInstance *h, crint3 tile)
{
	const ui64 heroStrength = h->getTotalStrength(),
		dangerStrength = evaluateDanger(tile, h);
	if(dangerStrength)
	{
		if(heroStrength / SAFE_ATTACK_CONSTANT > dangerStrength)
		{
			BNLOG("It's, safe for %s to visit tile %s", h->name % tile);
			return true;
		}
		else
			return false;
	}


	return true; //there's no danger
}

std::vector<const CGObjectInstance *> VCAI::getPossibleDestinations(const CGHeroInstance *h)
{
	validateVisitableObjs();
	std::vector<const CGObjectInstance *> possibleDestinations;
	BOOST_FOREACH(const CGObjectInstance *obj, visitableObjs)
	{
		if(cb->getPathInfo(obj->visitablePos())->reachable()  &&
			(obj->tempOwner != playerID || isWeeklyRevisitable(obj))) //flag or get weekly resources / creatures
			possibleDestinations.push_back(obj);
	}

	boost::sort(possibleDestinations, isCloser);

	possibleDestinations.erase(boost::remove_if(possibleDestinations, [&](const CGObjectInstance *obj) -> bool
		{
			if(vstd::contains(alreadyVisited, obj))
				return true;

			if(!isSafeToVisit(h, obj->visitablePos()))
				return true;

			return false;
		}),possibleDestinations.end());

	return possibleDestinations;
}

void VCAI::wander(const CGHeroInstance * h)
{
	while(1)
	{
		auto dests = getPossibleDestinations(h);
		if(!dests.size())
		{
			auto compareReinforcements = [h](const CGTownInstance *lhs, const CGTownInstance *rhs) -> bool
			{
				return howManyReinforcementsCanGet(h, lhs) < howManyReinforcementsCanGet(h, rhs);
			};

			std::vector<const CGTownInstance *> townsReachable;
			std::vector<const CGTownInstance *> townsNotReachable;
			BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
			{
				if(!t->visitingHero && howManyReinforcementsCanGet(h,t) && !vstd::contains(townVisitsThisWeek[h], t))
				{
					if(isReachable(t))
						townsReachable.push_back(t);
					else
						townsNotReachable.push_back(t);
				}
			}
// 			towns.erase(boost::remove_if(towns, [=](const CGTownInstance *t) -> bool
// 			{
// 				return !!t->visitingHero || !isReachable(t) || !howManyReinforcementsCanGet(h,t) || vstd::contains(townVisitsThisWeek[h], t);
// 			}),towns.end());

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
				BNLOG("%s can't reach any town, we'll try to make our way to %s at %s", h->name % t->name % t->visitablePos());
				int3 pos1 = h->pos;
				striveToGoal(CGoal(CLEAR_WAY_TO).settile(t->visitablePos()).sethero(h));
				if(pos1 == h->pos && h == primaryHero()) //hero can't move
				{
					/*boost::sort(unreachableTowns, compareArmyStrength);*/
					//BOOST_FOREACH(const CGTownInstance *t, unreachableTowns)
						if(cb->getResourceAmount(Res::GOLD) >= HERO_GOLD_COST && cb->getHeroesInfo().size() < ALLOWED_ROAMING_HEROES && cb->getAvailableHeroes(t).size())
							recruitHero(t);

				}

				break;
			}
			else if(cb->getResourceAmount(Res::GOLD) >= HERO_GOLD_COST)
			{
				std::vector<const CGTownInstance *> towns = cb->getTownsInfo();
				erase_if(towns, [](const CGTownInstance *t) -> bool
				{
					BOOST_FOREACH(const CGHeroInstance *h, cb->getHeroesInfo())
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
				PNLOG("Nowhere more to go...\n");
				break;
			}
		}

		if(!goVisitObj(dests.front(), h))
		{
			BNLOG("Hero %s apparently used all MPs (%d left)\n", h->name % h->movement);
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

void VCAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	assert(playerID > GameConstants::PLAYER_LIMIT || status.getBattle() == UPCOMING_BATTLE);
	status.setBattle(ONGOING_BATTLE);
	const TerrainTile *t = myCb->getTile(tile); //may be NULL in some very are cases -> eg. visited monolith and fighting with an enemy at the FoW covered exit
	battlename = boost::str(boost::format("battle of %s attacking %s at %s") % (hero1 ? hero1->name : "a army") % (t ? t->visitableObjects.back()->hoverName : "unknown enemy") % tile);
	CAdventureAI::battleStart(army1, army2, tile, hero1, hero2, side);
}

void VCAI::battleEnd(const BattleResult *br)
{
	assert(status.getBattle() == ONGOING_BATTLE);
	status.setBattle(ENDING_BATTLE);
	bool won = br->winner == myCb->battleGetMySide();
	BNLOG("Player %d: I %s the %s!", playerID % (won  ? "won" : "lost") % battlename);
	battlename.clear();
	CAdventureAI::battleEnd(br);
}

void VCAI::waitTillFree()
{
	auto unlock = vstd::makeUnlockSharedGuard(cb->getGsMutex());
	status.waitTillFree();
}

void VCAI::validateVisitableObjs()
{
	std::vector<const CGObjectInstance *> hlp;
	retreiveVisitableObjs(hlp, true);

	start:
	BOOST_FOREACH(const CGObjectInstance *obj, visitableObjs)
		if(!vstd::contains(hlp, obj))
		{
			tlog1 << helperObjInfo[obj].name << " at " << helperObjInfo[obj].pos << " shouldn't be on list!\n";
			remove_if_present(visitableObjs, obj);
			goto start;
		}
}

void VCAI::retreiveVisitableObjs(std::vector<const CGObjectInstance *> &out, bool includeOwned /*= false*/) const
{
	for(int i = 0; i < cb->getMapSize().x; i++)
		for(int j = 0; j < cb->getMapSize().y; j++)
			for(int k = 0; k < cb->getMapSize().z; k++)
				if(const TerrainTile *t = cb->getTile(int3(i,j,k), false))
				{
					BOOST_FOREACH(const CGObjectInstance *obj, t->visitableObjects)
					{
						if(includeOwned || obj->tempOwner != playerID)
							out.push_back(obj);
					}
				}
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
	BOOST_FOREACH(const CGObjectInstance *obj, ai->visitableObjs)
	{
		if(obj->ID == 5 && obj->subID == aid)
			return obj;
	}

	return NULL;

	//TODO what if more than one artifact is available? return them all or some slection criteria
}

bool VCAI::isAccessible(const int3 &pos)
{
	//TODO precalculate for speed

	BOOST_FOREACH(const CGHeroInstance *h, cb->getHeroesInfo())
	{
		if(isAccessibleForHero(pos, h))
			return true;
	}

	return false;
}

const CGHeroInstance * VCAI::getHeroWithGrail() const
{
	BOOST_FOREACH(const CGHeroInstance *h, cb->getHeroesInfo())
		if(h->hasArt(2)) //grail
			return h;

	return NULL;
}

const CGObjectInstance * VCAI::getUnvisitedObj(const boost::function<bool(const CGObjectInstance *)> &predicate)
{
	//TODO smarter definition of unvisited
	BOOST_FOREACH(const CGObjectInstance *obj, visitableObjs)
		if(predicate(obj) && !vstd::contains(alreadyVisited, obj))
			return obj;

	return NULL;
}

bool VCAI::isAccessibleForHero(const int3 & pos, const CGHeroInstance * h) const
{
	cb->setSelection(h);
	return cb->getPathInfo(pos)->reachable();
}

class cannotFulfillGoalException : public std::exception
{
	std::string msg;
public:
	explicit cannotFulfillGoalException(crstring _Message) : msg(_Message)
	{
	}

	virtual ~cannotFulfillGoalException() throw ()
	{
	};

	const char *what() const throw () OVERRIDE
	{
		return msg.c_str();
	}
};

bool VCAI::moveHeroToTile(int3 dst, const CGHeroInstance * h)
{
	visitedObject = NULL;
	int3 startHpos = h->visitablePos();
	bool ret = false;
	if(startHpos == dst)
	{
		assert(cb->getTile(dst)->visitableObjects.size() > 1); //there's no point in revisiting tile where there is no visitable object
		cb->moveHero(h,CGHeroInstance::convertPosition(dst, true));
		waitTillFree(); //movement may cause battle or blocking dialog
		ret = true;
	}
	else
	{
		CGPath path;
		cb->getPath2(dst, path);
		if(path.nodes.empty())
		{
			tlog1 << "Hero " << h->name << " cannot reach " << dst << std::endl;
			throw std::runtime_error("Wrong move order!");
		}

		int i=path.nodes.size()-1;
		for(; i>0; i--)
		{
			//stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
			if(path.nodes[i-1].turns)
			{
				blockedHeroes.insert(h); //to avoid attempts of moving heroes with very little MPs
				break;
			}

			int3 endpos = path.nodes[i-1].coord;
			if(endpos == h->visitablePos())
				continue;
// 			if(i > 1)
// 			{
// 				int3 afterEndPos = path.nodes[i-2].coord;
// 				if(afterEndPos.z != endpos.z)
//
// 			}
			//tlog0 << "Moving " << h->name << " from " << h->getPosition() << " to " << endpos << std::endl;
			cb->moveHero(h,CGHeroInstance::convertPosition(endpos, true));
			waitTillFree(); //movement may cause battle or blocking dialog
			boost::this_thread::interruption_point();
			if(h->tempOwner != playerID) //we lost hero
				break;

		}
		ret = !i;
	}
	if (visitedObject) //we step into something interesting
		performObjectInteraction (visitedObject, h);

	if(h->tempOwner == playerID) //lost hero after last move
		cb->recalculatePaths();
	BNLOG("Hero %s moved from %s to %s", h->name % startHpos % h->visitablePos());
	return ret;
}

int howManyTilesWillBeDiscovered(const int3 &pos, int radious)
{
	int ret = 0;
	for(int x = pos.x - radious; x <= pos.x + radious; x++)
	{
		for(int y = pos.y - radious; y <= pos.y + radious; y++)
		{
			int3 npos = int3(x,y,pos.z);
			if(cb->isInTheMap(npos) && pos.dist2d(npos) - 0.5 < radious  && !cb->isVisible(npos))
			{
				ret++;
			}
		}
	}

	return ret;
}

int howManyTilesWillBeDiscovered(int radious, int3 pos, crint3 dir)
{
	return howManyTilesWillBeDiscovered(pos + dir, radious);
}

void getVisibleNeighbours(const std::vector<int3> &tiles, std::vector<int3> &out)
{
	BOOST_FOREACH(const int3 &tile, tiles)
	{
		foreach_neighbour(tile, [&](int3 neighbour)
		{
			if(cb->isVisible(neighbour))
				out.push_back(neighbour);
		});
	}
}

void VCAI::tryRealize(CGoal g)
{
	BNLOG("Attempting realizing goal with code %d", g.goalType);
	switch(g.goalType)
	{
	case EXPLORE:
		{
			assert(0); //this goal is not elementar!
		}
		break;
	case RECRUIT_HERO:
		{
			if(const CGTownInstance *t = findTownWithTavern())
			{
				//TODO co jesli nie ma dostepnego bohatera?
				//TODO jezeli miasto jest zablokowane, sprobowac oczyscic wejscie
				cb->recruitHero(t, cb->getAvailableHeroes(t)[0]);
			}

			//TODO karkolomna alternatywa - tawerna na mapie przygod lub wiezienie (nie wiem, czy warto?)
		}
		break;
	case VISIT_TILE:
		{
			if(!g.hero->movement)
				throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
			if(!g.isBlockedBorderGate(g.tile))
			{
				ai->moveHeroToTile(g.tile, g.hero);
			}
			else
				throw cannotFulfillGoalException("There's a blocked gate!");
		}
		break;
	case BUILD_STRUCTURE:
		{
			const CGTownInstance *t = g.town;

			if(!t && g.hero)
				t = g.hero->visitedTown;

			if(!t)
			{
				BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
				{
					switch(cb->canBuildStructure(t, g.bid))
					{
					case EBuildingState::ALLOWED:
						cb->buildBuilding(t, g.bid);
						return;
					default:
						break;
					}
				}
			}
			else if(cb->canBuildStructure(t, g.bid) == EBuildingState::ALLOWED)
			{
				cb->buildBuilding(t, g.bid);
				return;
			}
			throw cannotFulfillGoalException("Cannot build a given structure!");
		}
		break;
		case DIG_AT_TILE:
		{
			assert(g.hero->visitablePos() == g.tile);
			if(g.hero->diggingStatus() == CGHeroInstance::CAN_DIG)
				cb->dig(g.hero);
			else
			{
				ai->blockedHeroes.insert(g.hero);
				throw cannotFulfillGoalException("A hero can't dig!\n");
			}
		}
		break;

	case COLLECT_RES:
		if(const CGObjectInstance *obj = cb->getObj(g.objid, false))
		{
			if(const IMarket *m = IMarket::castFrom(obj, false))
			{
				for (int i = 0; i < ACTUAL_RESOURCE_COUNT; i++)
				{
					if(i == g.resID) continue;
					int toGive, toGet;
					m->getOffer(i, g.resID, toGive, toGet, EMarketMode::RESOURCE_RESOURCE);
					toGive = toGive * (cb->getResourceAmount(i) / toGive);
					cb->trade(obj, EMarketMode::RESOURCE_RESOURCE, i, g.resID, toGive);
					if(cb->getResourceAmount(g.resID) >= g.value)
						return;
				}
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

	case CONQUER:
		// TODO: conquer??
		throw cannotFulfillGoalException("I don't know how to fulfill this!");

	case BUILD:
		performTypicalActions();
		throw cannotFulfillGoalException("BUILD has been realized as much as possible.");

	case INVALID:
		throw cannotFulfillGoalException("I don't know how to fulfill this!");

	default:
		assert(0);
	}
}

const CGTownInstance * VCAI::findTownWithTavern() const
{
	BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
		if(vstd::contains(t->builtBuildings, EBuilding::TAVERN))
			return t;

	return NULL;
}

std::vector<const CGHeroInstance *> VCAI::getUnblockedHeroes() const
{
	std::vector<const CGHeroInstance *> ret = cb->getHeroesInfo();
	BOOST_FOREACH(const CGHeroInstance *h, blockedHeroes)
		remove_if_present(ret, h);

	return ret;
}

const CGHeroInstance * VCAI::primaryHero() const
{
	auto hs = cb->getHeroesInfo();
	boost::sort(hs, compareHeroStrength);

	if(hs.empty())
		return NULL;

	return hs.back();
}

void VCAI::endTurn()
{
	tlog4 << "Player " << playerID << " ends turn\n";
	if(!status.haveTurn())
	{
		tlog1 << "Not having turn at the end of turn???\n";
	}

	do
	{
		cb->endTurn();
	} while(status.haveTurn()); //for some reasons, our request may fail -> stop requesting end of turn only after we've received a confirmation that it's over

	tlog4 << "Player " << playerID << " ended turn\n";
}

void VCAI::striveToGoal(const CGoal &ultimateGoal)
{
	while(1)
	{
		CGoal goal = ultimateGoal;
		BNLOG("Striving to goal of type %d", ultimateGoal.goalType);
		while(!goal.isElementar)
		{
			INDENT;
			BNLOG("Considering goal %d.", goal.goalType);
			try
			{
				boost::this_thread::interruption_point();
				goal = goal.whatToDoToAchieve();
			}
			catch(std::exception &e)
			{
				BNLOG("Goal %d decomposition failed: %s", goal.goalType % e.what());
				return;
			}
		}

		try
		{
			boost::this_thread::interruption_point();
			tryRealize(goal);
			boost::this_thread::interruption_point();
		}
		catch(boost::thread_interrupted &e)
		{
			BNLOG("Player %d: Making turn thread received an interruption!", playerID);
			throw; //rethrow, we want to truly end this thread
		}
		catch(std::exception &e)
		{
			BNLOG("Failed to realize subgoal of type %d (greater goal type was %d), I will stop.", goal.goalType % ultimateGoal.goalType);
			BNLOG("The error message was: %s", e.what());
			break;
		}
	}
}

void VCAI::performTypicalActions()
{
	BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
	{
		BNLOG("Looking into %s", t->name);
		buildStructure(t);
		buildArmyIn(t);

		if(!ai->primaryHero() ||
			(t->getArmyStrength() > ai->primaryHero()->getArmyStrength() * 2 && !isAccessibleForHero(t->visitablePos(), ai->primaryHero())))
		{
			recruitHero(t);
			buildArmyIn(t);
		}
	}

	BOOST_FOREACH(const CGHeroInstance *h, getUnblockedHeroes())
	{
		BNLOG("Looking into %s, MP=%d", h->name.c_str() % h->movement);
		INDENT;
		makePossibleUpgrades(h);
		cb->setSelection(h);
		wander(h);
	}
}

void VCAI::buildArmyIn(const CGTownInstance * t)
{
	makePossibleUpgrades(t->visitingHero);
	makePossibleUpgrades(t);
	recruitCreatures(t);
	moveCreaturesToHero(t);
}

int3 VCAI::explorationBestNeighbour(int3 hpos, int radius, const CGHeroInstance * h)
{
	TimeCheck tc("looking for best exploration neighbour");
	std::map<int3, int> dstToRevealedTiles;
	BOOST_FOREACH(crint3 dir, dirs)
		if(cb->isInTheMap(hpos+dir))
			dstToRevealedTiles[hpos + dir] = howManyTilesWillBeDiscovered(radius, hpos, dir) * isSafeToVisit(h, hpos + dir);

	auto best = dstToRevealedTiles.begin();
	best->second *= cb->getPathInfo(best->first)->reachable();
	best->second *= cb->getPathInfo(best->first)->accessible == CGPathNode::ACCESSIBLE;
	for(auto i = dstToRevealedTiles.begin(); i != dstToRevealedTiles.end(); i++)
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

int3 VCAI::explorationNewPoint(int radius, const CGHeroInstance * h, std::vector<std::vector<int3> > &tiles)
{
	TimeCheck tc("looking for new exploration point");
	tlog0 << "Looking for an another place for exploration...\n";
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

		BOOST_FOREACH(const int3 &tile, tiles[i])
		{
			if(cb->getPathInfo(tile)->reachable() && isSafeToVisit(h, tile) && howManyTilesWillBeDiscovered(tile, radius))
			{
				return tile;
			}
		}
	}
	throw cannotFulfillGoalException("No accessible tile will bring discoveries!");
}

TResources VCAI::estimateIncome() const
{
	TResources ret;
	BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
	{
		ret[Res::GOLD] += t->dailyIncome();

		//TODO duplikuje newturn
		if(t->hasBuilt(EBuilding::RESOURCE_SILO)) //there is resource silo
		{
			if(t->town->primaryRes == 127) //we'll give wood and ore
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

	BOOST_FOREACH(const CGObjectInstance *obj, getFlaggedObjects())
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

void VCAI::recruitHero(const CGTownInstance * t)
{
	BNLOG("Trying to recruit a hero in %s at %s", t->name % t->visitablePos())
	cb->recruitHero(t, cb->getAvailableHeroes(t).front());
}

void VCAI::finish()
{
	if(makingTurn)
		makingTurn->interrupt();
}

AIStatus::AIStatus()
{
	battle = NO_BATTLE;
	remainingQueries = 0;
	havingTurn = false;
}

AIStatus::~AIStatus()
{

}

void AIStatus::setBattle(BattleState BS)
{
	boost::unique_lock<boost::mutex> lock(mx);
	battle = BS;
	cv.notify_all();
}

BattleState AIStatus::getBattle()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return battle;
}

void AIStatus::addQueries(int val)
{
	boost::unique_lock<boost::mutex> lock(mx);
	remainingQueries += val;
	BNLOG("Changing count of queries by %d, to a total of %d", val % remainingQueries);
	assert(remainingQueries >= 0);
	cv.notify_all();
}

void AIStatus::addQuery()
{
	addQueries(1);
}

void AIStatus::removeQuery()
{
	addQueries(-1);
}

int AIStatus::getQueriesCount()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return remainingQueries;
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
	while(battle != NO_BATTLE || remainingQueries)
		cv.wait(lock);
}

bool AIStatus::haveTurn()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return havingTurn;
}

int3 whereToExplore(const CGHeroInstance *h)
{
	//TODO it's stupid and ineffective, write sth better
	cb->setSelection(h);
	int radius = h->getSightRadious();
	int3 hpos = h->visitablePos();

	//look for nearby objs -> visit them if they're close enouh
	const int DIST_LIMIT = 3;
	std::vector<const CGObjectInstance *> nearbyVisitableObjs;
	BOOST_FOREACH(const CGObjectInstance *obj, ai->getPossibleDestinations(h))
	{
		int3 op = obj->visitablePos();
		CGPath p;
		cb->getPath2(op, p);
		if(p.nodes.size() && p.endPos() == op && p.nodes.size() <= DIST_LIMIT)
			nearbyVisitableObjs.push_back(obj);
	}
	boost::sort(nearbyVisitableObjs, isCloser);
	if(nearbyVisitableObjs.size())
		return nearbyVisitableObjs.back()->visitablePos();

	try
	{
		return ai->explorationBestNeighbour(hpos, radius, h);
	}
	catch(cannotFulfillGoalException &e)
	{
		std::vector<std::vector<int3> > tiles; //tiles[distance_to_fow], metryka taksówkowa
		try
		{
			return ai->explorationNewPoint(radius, h, tiles);
		}
		catch(cannotFulfillGoalException &e)
		{
			std::map<int, std::vector<int3> > profits;
			{
				TimeCheck tc("Evaluating exploration possibilities");
				tiles[0].clear(); //we can't reach FoW anyway
				BOOST_FOREACH(auto &vt, tiles)
					BOOST_FOREACH(auto &tile, vt)
						profits[howManyTilesWillBeDiscovered(tile, radius)].push_back(tile);
			}

			if(profits.empty())
				throw cannotFulfillGoalException("Cannot explore - no possible ways found!");

			auto bestDest = profits.end();
			bestDest--;
			return bestDest->second.front(); //TODO which is the real best tile?
		}
	}
}

TSubgoal CGoal::whatToDoToAchieve()
{
	switch(goalType)
	{
	case WIN:
		{
			const CVictoryCondition &vc = cb->getMapHeader()->victoryCondition;
			EVictoryConditionType::EVictoryConditionType cond = vc.condition;

			if(!vc.appliesToAI)
			{
				//TODO deduce victory from human loss condition
				cond = EVictoryConditionType::WINSTANDARD;
			}

			switch(cond)
			{
			case EVictoryConditionType::ARTIFACT:
				return CGoal(GET_ART_TYPE).setaid(vc.ID);
			case EVictoryConditionType::BEATHERO:
				return CGoal(GET_OBJ).setobjid(vc.ID);
			case EVictoryConditionType::BEATMONSTER:
				return CGoal(GET_OBJ).setobjid(vc.ID);
			case EVictoryConditionType::BUILDCITY:
				//TODO build castle/capitol
				break;
			case EVictoryConditionType::BUILDGRAIL:
				{
					if(const CGHeroInstance *h = ai->getHeroWithGrail())
					{
						//hero is in a town that can host Grail
						if(h->visitedTown && !vstd::contains(h->visitedTown->forbiddenBuildings, EBuilding::GRAIL))
						{
							const CGTownInstance *t = h->visitedTown;
							return CGoal(BUILD_STRUCTURE).setbid(EBuilding::GRAIL).settown(t);
						}
						else
						{
							auto towns = cb->getTownsInfo();
							towns.erase(boost::remove_if(towns,
												[](const CGTownInstance *t) -> bool
												{
													return vstd::contains(t->forbiddenBuildings, EBuilding::GRAIL);
												}),
										towns.end());
							boost::sort(towns, isCloser);
							if(towns.size())
							{
								return CGoal(VISIT_TILE).sethero(h).settile(towns.front()->visitablePos());
							}
						}
					}
					double ratio = 0;
					int3 grailPos = cb->getGrailPos(ratio);
					if(ratio > 0.99)
					{
						return CGoal(DIG_AT_TILE).settile(grailPos);
					}
					else if(const CGObjectInstance * obj = ai->getUnvisitedObj(objWithID<Obj::OBELISK>)) //there are unvisited Obelisks
					{
						return CGoal(GET_OBJ).setobjid(obj->id);
					}
					else
						return CGoal(EXPLORE);
				}
				break;
			case EVictoryConditionType::CAPTURECITY:
				return CGoal(GET_OBJ).setobjid(vc.ID);
			case EVictoryConditionType::GATHERRESOURCE:
				return CGoal(COLLECT_RES).setresID(vc.ID).setvalue(vc.count);
				//TODO mines? piles? marketplace?
				//save?
				break;
			case EVictoryConditionType::GATHERTROOP:
				break;
			case EVictoryConditionType::TAKEDWELLINGS:
				break;
			case EVictoryConditionType::TAKEMINES:
				break;
			case EVictoryConditionType::TRANSPORTITEM:
				break;
			case EVictoryConditionType::WINSTANDARD:
				return CGoal(CONQUER);
			default:
				assert(0);
			}
		}
		break;
	case GET_OBJ:
		{
			const CGObjectInstance * obj = cb->getObj(objid);
			if(!obj)
				return CGoal(EXPLORE);
			int3 pos = cb->getObj(objid)->visitablePos();
			return CGoal(VISIT_TILE).settile(pos);
		}
		break;
	case GET_ART_TYPE:
		{
			const CGObjectInstance *artInst = ai->lookForArt(aid);
			if(!artInst)
			{
				TSubgoal alternativeWay = CGoal::lookForArtSmart(aid);
				if(alternativeWay.invalid())
					return CGoal(EXPLORE);
				else
					return alternativeWay;
			}
			else
				return CGoal(GET_OBJ).setobjid(artInst->id);
		}
		break;
	case CLEAR_WAY_TO:
		{
			assert(tile.x >= 0); //set tile
			if(!cb->isVisible(tile))
			{
				tlog1 << "Clear way should be used with visible tiles!\n";
				return CGoal(EXPLORE);
			}

			const CGHeroInstance *h = hero ? hero : ai->primaryHero();
			if(!h)
				return CGoal(RECRUIT_HERO);

			cb->setSelection(h);

			SectorMap sm;
			bool dropToFile = false;
			if(dropToFile) //for debug purposes
				sm.write("test.txt");

			int3 tileToHit = sm.firstTileToGet(h, tile);
			//if(isSafeToVisit(h, tileToHit))
			if(isBlockedBorderGate(tileToHit))
				throw cannotFulfillGoalException("There's blocked border gate!");

			if(tileToHit == tile)
			{
				tlog1 << boost::format("Very strange, tile to hit is %s and tile is also %s, while hero %s is at %s\n")
					% tileToHit % tile % h->name % h->visitablePos();
				throw cannotFulfillGoalException("Retreiving first tile to hit failed (probably)!");
			}

			return CGoal(VISIT_TILE).settile(tileToHit).sethero(h);

			//TODO czy istnieje lepsza droga?

		}
		throw cannotFulfillGoalException("Cannot reach given tile!");
		//return CGoal(EXPLORE); // TODO improve
	case EXPLORE:
		{
			if(cb->getHeroesInfo().empty())
				return CGoal(RECRUIT_HERO);

			auto hs = cb->getHeroesInfo();
			assert(hs.size());
			erase(hs, [](const CGHeroInstance *h)
			{
				return !h->movement || contains(ai->blockedHeroes, h); //only hero with movement are of interest for us
			});
			if(hs.empty())
			{
				throw cannotFulfillGoalException("No heroes with remaining MPs for exploring!\n");
			}

			const CGHeroInstance *h = hs.front();

			CGoal ret(VISIT_TILE);
			ret.sethero(h);
			return ret.settile(whereToExplore(h));
		}

		I_AM_ELEMENTAR;

	case RECRUIT_HERO:
		{
			const CGTownInstance *t = ai->findTownWithTavern();
			if(!t)
				return CGoal(BUILD_STRUCTURE).setbid(EBuilding::TAVERN);

			if(cb->getResourceAmount(Res::GOLD) < HERO_GOLD_COST)
				return CGoal(COLLECT_RES).setresID(Res::GOLD).setvalue(HERO_GOLD_COST);

			I_AM_ELEMENTAR;
		}
		break;

	case VISIT_TILE:
		{
			if(!cb->isVisible(tile))
				return CGoal(EXPLORE);

			if(hero && !ai->isAccessibleForHero(tile, hero))
				hero = NULL;

			if(!hero)
			{
				if(cb->getHeroesInfo().empty())
					return CGoal(RECRUIT_HERO);

				BOOST_FOREACH(const CGHeroInstance *h, cb->getHeroesInfo())
				{
					if(ai->isAccessibleForHero(tile, h))
					{
						hero = h;
						break;
					}
				}
			}

			if(hero)
			{
				if(isSafeToVisit(hero, tile))
					return CGoal(*this).setisElementar(true);
				else
					return CGoal(INVALID); //todo can gather army?
			}
			else	//inaccessible for all heroes
				return CGoal(CLEAR_WAY_TO).settile(tile);
		}
		break;

	case DIG_AT_TILE:
		{
			auto objs = cb->getTile(tile)->visitableObjects;
			if(objs.size() && objs.front()->ID == GameConstants::HEROI_TYPE && objs.front()->tempOwner == ai->playerID) //we have hero at dest
			{
				const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(objs.front());
				return CGoal(*this).sethero(h).setisElementar(true);
			}

			return CGoal(VISIT_TILE).settile(tile);
		}
		break;

	case BUILD_STRUCTURE:
		//TODO check res
		//look for town
		//prerequisites?
		I_AM_ELEMENTAR;
	case COLLECT_RES:
		{

			std::vector<const IMarket*> markets;

			std::vector<const CGObjectInstance*> visObjs;
			ai->retreiveVisitableObjs(visObjs, true);
			BOOST_FOREACH(const CGObjectInstance *obj, visObjs)
			{
				if(const IMarket *m = IMarket::castFrom(obj, false))
				{
					if(obj->ID == GameConstants::TOWNI_TYPE && obj->tempOwner == ai->playerID && m->allowsTrade(EMarketMode::RESOURCE_RESOURCE))
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
				return !(market->o->ID == GameConstants::TOWNI_TYPE && market->o->tempOwner == ai->playerID)
					&& !ai->isAccessible(market->o->visitablePos());
			}),markets.end());

			if(!markets.size())
			{
				BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
				{
					if(cb->canBuildStructure(t, EBuilding::MARKETPLACE) == EBuildingState::ALLOWED)
						return CGoal(BUILD_STRUCTURE).settown(t).setbid(EBuilding::MARKETPLACE);
				}
			}
			else
			{
				const IMarket *m = markets.back();
				//attempt trade at back (best prices)
				int howManyCanWeBuy = 0;
				for(int i = 0; i < ACTUAL_RESOURCE_COUNT; i++)
				{
					if(i == resID) continue;
					int toGive = -1, toReceive = -1;
					m->getOffer(i, resID, toGive, toReceive, EMarketMode::RESOURCE_RESOURCE);
					assert(toGive > 0 && toReceive > 0);
					howManyCanWeBuy += toReceive * (cb->getResourceAmount(i) / toGive);
				}

				if(howManyCanWeBuy + cb->getResourceAmount(resID) >= value)
				{
					if(cb->getTile(m->o->visitablePos())->visitableObjects.back()->tempOwner != ai->playerID)
						return CGoal(GET_OBJ).setobjid(m->o->id);
					return setobjid(m->o->id).setisElementar(true);
				}
			}
		}
		return CGoal(INVALID);
	case CONQUER:
		{
			//TODO make use from many heroes
			std::vector<const CGHeroInstance *> heroes = cb->getHeroesInfo();
			erase_if(heroes, [](const CGHeroInstance *h)
			{
				return vstd::contains(ai->blockedHeroes, h) || !h->movement;
			});
			boost::sort(heroes, compareHeroStrength);

			if(heroes.empty())
				I_AM_ELEMENTAR;

			const CGHeroInstance *h = heroes.back();
			cb->setSelection(h);
			std::vector<const CGObjectInstance *> objs; //here we'll gather enemy towns and heroes
			ai->retreiveVisitableObjs(objs);
			erase_if(objs, [&](const CGObjectInstance *obj)
			{
				return (obj->ID != GameConstants::TOWNI_TYPE && obj->ID != GameConstants::HEROI_TYPE) //not town/hero
					|| cb->getPlayerRelations(ai->playerID, obj->tempOwner) != 0; //not enemy
			});

			if(objs.empty())
				return CGoal(EXPLORE); //we need to find an enemy

			erase_if(objs,  [&](const CGObjectInstance *obj)
			{
				return !isSafeToVisit(h, obj->visitablePos());
			});

			if(objs.empty())
				I_AM_ELEMENTAR;

			boost::sort(objs, isCloser);
			BOOST_FOREACH(const CGObjectInstance *obj, objs)
			{
				if(ai->isAccessibleForHero(obj->visitablePos(), h))
					return CGoal(VISIT_TILE).sethero(h).settile(obj->visitablePos());
			}

			return CGoal(EXPLORE); //enemy is inaccessible
			;
		}
		break;
	case BUILD:
		I_AM_ELEMENTAR;
	case INVALID:
		I_AM_ELEMENTAR;
	default:
		assert(0);
	}

	return CGoal(EXPLORE);
}

TSubgoal CGoal::goVisitOrLookFor(const CGObjectInstance *obj)
{
	if(obj)
		return CGoal(GET_OBJ).setobjid(obj->id);
	else
		return CGoal(EXPLORE);
}

TSubgoal CGoal::lookForArtSmart(int aid)
{
	return CGoal(INVALID);
}

bool CGoal::invalid() const
{
	return goalType == INVALID;
}

bool CGoal::isBlockedBorderGate(int3 tileToHit)
{
	return cb->getTile(tileToHit)->topVisitableID() == Obj::BORDER_GATE
		&& cb->getPathInfo(tileToHit)->accessible != CGPathNode::ACCESSIBLE;
}

SectorMap::SectorMap()
{
// 	int3 sizes = cb->getMapSize();
// 	sector.resize(sizes.x);
// 	BOOST_FOREACH(auto &i, sector)
// 		i.resize(sizes.y);
//
// 	BOOST_FOREACH(auto &i, sector)
// 		BOOST_FOREACH(auto &j, i)
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

bool canBeEmbarkmentPoint(const TerrainTile *t)
{
	//tile must be free of with unoccupied boat
	return !t->blocked
		|| (t->visitableObjects.size() == 1 && t->topVisitableID() == Obj::BOAT);
}

void SectorMap::exploreNewSector(crint3 pos, int num)
{
	Sector &s = infoOnSectors[num];
	s.id = num;
	s.water = cb->getTile(pos)->isWater();

	std::queue<int3> toVisit;
	toVisit.push(pos);
	while(toVisit.size())
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
						toVisit.push(ai->knownSubterraneanGates[t->visitableObjects.front()]->pos);
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
	if (dynamic_cast<const CGVisitableOPW *>(obj) || dynamic_cast<const CGDwelling *>(obj)) //ensures future compatibility, unlike IDs
		return true;
	switch (obj->ID)
	{
		case Obj::STABLES: //any other potential visitable objects?
			return true;
	}
	return false;
}

int3 SectorMap::firstTileToGet(const CGHeroInstance *h, crint3 dst)
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

			BOOST_FOREACH(int3 ep, s->embarkmentPoints)
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
			throw cannotFulfillGoalException(str(format("Cannot found connection between sectors %d and %d") % src->id % dst->id));
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
					return t && t->visitableObjects.size() == 1 && t->topVisitableID() == Obj::BOAT
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
					BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
					{
						if(t->hasBuilt(EBuilding::SHIPYARD))
							shipyards.push_back(t);
					}

					std::vector<const CGObjectInstance*> visObjs;
					ai->retreiveVisitableObjs(visObjs, true);
					BOOST_FOREACH(const CGObjectInstance *obj, visObjs)
					{
						if(obj->ID != GameConstants::TOWNI_TYPE) //towns were handled in the previous loop
							if(const IShipyard *shipyard = IShipyard::castFrom(obj))
								shipyards.push_back(shipyard);
					}

					shipyards.erase(boost::remove_if(shipyards, [=](const IShipyard *shipyard) -> bool
					{
						return shipyard->state() != 0 || retreiveTile(shipyard->bestLocation()) != sectorToReach->id;
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
						return shipyards.front()->o->pos;
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
	while(toVisit.size())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		ui8 &sec = retreiveTile(curPos);
		assert(sec == mySector); //consider only tiles from the same sector

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
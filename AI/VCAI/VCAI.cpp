#include "StdInc.h"
#include "VCAI.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"

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

//std::map<int, std::map<int, int> > HeroView::infosCount;

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

template <typename Container>
typename Container::value_type backOrNull(const Container &c) //returns last element of container or NULL if it is empty (to be used with containers of pointers)
{
	if(c.size())
		return c.back();
	else
		return NULL;
}

template <typename Container>
typename Container::value_type frontOrNull(const Container &c) //returns first element of container or NULL if it is empty (to be used with containers of pointers)
{
	if(c.size())
		return c.front();
	else
		return NULL;
}


#define SET_GLOBAL_STATE(ai) SetGlobalState _hlpSetState(ai);

#define NET_EVENT_HANDLER SET_GLOBAL_STATE(this)
#define MAKING_TURN SET_GLOBAL_STATE(this)

const int GOLD_RESERVE = 10000; //when buying creatures we want to keep at least this much gold (10000 so at least we'll be able to reach capitol)
const int HERO_GOLD_COST = 2500;
const int ALLOWED_ROAMING_HEROES = 8;

const int GOLD_MINE_PRODUCTION = 1000, WOOD_ORE_MINE_PRODUCTION = 2, RESOURCE_MINE_PRODUCTION = 1;

std::string CGoal::name() const
{
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
			return "EXPLORE";
		case GATHER_ARMY:
			return "GATHER ARMY";
		case BOOST_HERO:
			return "BOOST_HERO (unsupported)";
		case RECRUIT_HERO:
			return "RECRUIT HERO";
		case BUILD_STRUCTURE:
			return "BUILD STRUCTURE";
		case COLLECT_RES:
			return "COLLECT RESOURCE";
		case GATHER_TROOPS:
			return "GATHER TROOPS";
		case GET_OBJ:
			return "GET OBJECT " + boost::lexical_cast<std::string>(objid);
		case FIND_OBJ:
			return "FIND OBJECT " + boost::lexical_cast<std::string>(objid);
		case VISIT_HERO:
			return "VISIT HERO " + boost::lexical_cast<std::string>(objid);
		case GET_ART_TYPE:
			return "GET ARTIFACT OF TYPE " + VLC->arth->artifacts[aid]->Name();
		case ISSUE_COMMAND:
			return "ISSUE COMMAND (unsupported)";
		case VISIT_TILE:
			return "VISIT TILE " + tile();
		case CLEAR_WAY_TO:
			return "CLEAR WAY TO " + tile();
		case DIG_AT_TILE:
			return "DIG AT TILE " + tile();
		default:
			return boost::lexical_cast<std::string>(goalType);
	}
}

bool compareHeroStrength(HeroPtr h1, HeroPtr h2)
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
	if (i != c.end())
	{
		c.erase(i);
		return true;
	}

	return false;
}


template <typename V, typename Item, typename Item2>
bool remove_if_present(std::map<Item,V> & c, const Item2 &item)
{
	auto i = c.find(item);
	if (i != c.end())
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

ui64 howManyReinforcementsCanGet(HeroPtr h, const CGTownInstance *t)
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
}

bool compareMovement(HeroPtr lhs, HeroPtr rhs)
{
	return lhs->movement > rhs->movement;
}

ui64 evaluateDanger(const CGObjectInstance *obj);

ui64 evaluateDanger(crint3 tile)
{
	const TerrainTile *t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0, guardDanger = 0;

	auto visObjs = cb->getVisitableObjs(tile);
	if(visObjs.size())
		objectDanger = evaluateDanger(visObjs.back());

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

	auto visitableObjects = cb->getVisitableObjs(tile);
	// in some scenarios hero happens to be "under" the object (eg town). Then we consider ONLY the hero.
	if(vstd::contains_if(visitableObjects, objWithID<Obj::HERO>))
		erase_if(visitableObjects, ! boost::bind(objWithID<Obj::HERO>, _1));

	if(const CGObjectInstance * dangerousObject = backOrNull(visitableObjects))
	{
		objectDanger = evaluateDanger(dangerousObject); //unguarded objects can also be dangerous or unhandled
		if (objectDanger)
		{
			//TODO: don't downcast objects AI shouldnt know about!
			auto armedObj = dynamic_cast<const CArmedInstance*>(dangerousObject);
			if(armedObj)
				objectDanger *= fh->getTacticalAdvantage(visitor, armedObj); //this line tends to go infinite for allied towns (?)
		}
	}

	auto guards = cb->getGuardingCreatures(tile);
	BOOST_FOREACH (auto cre, guards)
	{
		amax (guardDanger, evaluateDanger(cre) * fh->getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance*>(cre))); //we are interested in strongest monster around
	}

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 evaluateDanger(const CGObjectInstance *obj)
{
	if(obj->tempOwner < GameConstants::PLAYER_LIMIT && cb->getPlayerRelations(obj->tempOwner, ai->playerID)) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case Obj::HERO:
		{
			InfoAboutHero iah;
			cb->getHeroInfo(obj, iah);
			return iah.army.getStrength();
		}
	case Obj::TOWN:
	case Obj::GARRISON: case Obj::GARRISON2: //garrison
		{
			InfoAboutTown iat;
			cb->getTownInfo(obj, iat);
			return iat.army.getStrength();
		}
	case Obj::MONSTER:
		{
			//TODO!!!!!!!!
			const CGCreature *cre = dynamic_cast<const CGCreature*>(obj);
			return cre->getArmyStrength();
		}
	case Obj::CREATURE_GENERATOR1:
		{
			const CGDwelling *d = dynamic_cast<const CGDwelling*>(obj);
			return d->getArmyStrength();
		}
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
		{
			const CArmedInstance * a = dynamic_cast<const CArmedInstance*>(obj);
			return a->getArmyStrength();
		}
	case Obj::CRYPT: //crypt
	case Obj::CREATURE_BANK: //crebank
	case Obj::DRAGON_UTOPIA:
	case Obj::SHIPWRECK: //shipwreck
	case Obj::DERELICT_SHIP: //derelict ship
//	case Obj::PYRAMID:
		return fh->estimateBankDanger (VLC->objh->bankObjToIndex(obj));
	case Obj::PYRAMID:
		{
		    if(obj->subID == 0)
				return fh->estimateBankDanger (VLC->objh->bankObjToIndex(obj));
			else
				return 0;
		}
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
		const int3 from = CGHeroInstance::convertPosition(details.start, false),
			to = CGHeroInstance::convertPosition(details.end, false);
		const CGObjectInstance *o1 = frontOrNull(cb->getVisitableObjs(from)),
			*o2 = frontOrNull(cb->getVisitableObjs(to));

		if(o1 && o2 && o1->ID == Obj::SUBTERRANEAN_GATE && o2->ID == Obj::SUBTERRANEAN_GATE)
		{
			knownSubterraneanGates[o1] = o2;
			knownSubterraneanGates[o2] = o1;
			BNLOG("Found a pair of subterranean gates between %s and %s!", from % to);
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
		visitedObject = const_cast<CGObjectInstance *>(visitedObj); // remember the object and wait for return
		markObjectVisited (visitedObj);
		remove_if_present(reservedObjs, visitedObj); //unreserve objects
		remove_if_present(reservedHeroesMap[visitor], visitedObj);
		completeGoal (CGoal(GET_OBJ).sethero(visitor)); //we don't need to visit in anymore
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

	auto firstHero = cb->getHero(hero1);
	auto secondHero = cb->getHero(hero2);

	requestActionASAP([=]()
	{
		if (firstHero->getHeroStrength() > secondHero->getHeroStrength() && canGetArmy (firstHero, secondHero))
			pickBestCreatures (firstHero, secondHero);
		else if (canGetArmy (secondHero, firstHero))
			pickBestCreatures (secondHero, firstHero);

		completeGoal(CGoal(VISIT_HERO).sethero(firstHero)); //TODO: what if we were visited by other hero in the meantime?
		completeGoal(CGoal(VISIT_HERO).sethero(secondHero));
		//TODO: exchange artifacts
	});
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

	BOOST_FOREACH(auto &p, reservedHeroesMap)
		remove_if_present(p.second, obj);

	//TODO
	//there are other places where CGObjectinstance ptrs are stored...
	//

	if(obj->ID == Obj::HERO  &&  obj->tempOwner == playerID)
	{
		lostHero(cb->getHero(obj->id)); //we can promote, since objectRemoved is killed just before actual deletion
	}
}

void VCAI::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;

	requestActionASAP([=]()
	{
		makePossibleUpgrades(visitor);
	});
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
		status.receivedAnswerConfirmation(pa->requestID, pa->result);
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

void VCAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, int queryID)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	status.addQuery(queryID, boost::str(boost::format("Hero %s got level %d") % hero->name % hero->level));
	requestActionASAP([=]{ answerQuery(queryID, 0); });
}

void VCAI::commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, int queryID)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
	status.addQuery(queryID, boost::str(boost::format("Commander %s of %s got level %d") % commander->name % commander->armyObj->nodeName() % (int)commander->level));
	requestActionASAP([=]{ answerQuery(queryID, 0); });
}

void VCAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;
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

void VCAI::showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, int queryID)
{
	NET_EVENT_HANDLER;
	LOG_ENTRY;

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
	setThreadName("VCAI::makeTurn");

	BNLOG("Player %d starting turn", playerID);
	INDENT;

	switch(cb->getDate(Date::DAY_OF_WEEK))
	{
		case 1:
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
			break;
		case 7: //reconsider strategy
		{
			if(auto h = primaryHero()) //check if our primary hero can handle danger
			{
				ui64 totalDanger = 0;
				int dangerousObjects = 0;
				std::vector<const CGObjectInstance *> objs;
				retreiveVisitableObjs(objs, false);
				BOOST_FOREACH (auto obj, objs)
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
					setGoal (h, CGoal(GATHER_ARMY).sethero(h).setvalue(averageDanger * SAFE_ATTACK_CONSTANT).setisAbstract(true));
				}
			}
		}
			break;
	}
	if(cb->getSelectedHero())
		cb->recalculatePaths();

	makeTurnInternal();
	vstd::clear_pointer(makingTurn);

	return;
}

void VCAI::makeTurnInternal()
{
	saving = 0;

	//it looks messy here, but it's better to have armed heroes before attempting realizing goals
	BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
		moveCreaturesToHero(t);

	try
	{
		//Pick objects reserved in previous turn - we expect only nerby objects there
		auto reservedHeroesCopy = reservedHeroesMap; //work on copy => the map may be changed while iterating (eg because hero died when attempting a goal)
		BOOST_FOREACH (auto hero, reservedHeroesCopy)
		{
			cb->setSelection(hero.first.get());
			boost::sort (hero.second, isCloser);
			BOOST_FOREACH (auto obj, hero.second)
			{
				striveToGoal (CGoal(VISIT_TILE).sethero(hero.first).settile(obj->visitablePos()));
			}
		}

		//now try to win
		striveToGoal(CGoal(WIN));

		//finally, continue our abstract long-term goals

		//heroes tend to die in the process and loose their goals, unsafe to iterate it
		std::vector<std::pair<HeroPtr, CGoal> > safeCopy;
		boost::copy(lockedHeroes, std::back_inserter(safeCopy));

		typedef std::pair<HeroPtr, CGoal> TItrType;

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
		BOOST_FOREACH (auto quest, quests)
		{
			striveToQuest (quest);
		}

		striveToGoal(CGoal(BUILD)); //TODO: smarter building management
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

bool VCAI::goVisitObj(const CGObjectInstance * obj, HeroPtr h)
{
	int3 dst = obj->visitablePos();
	BNLOG("%s will try to visit %s at (%s)", h->name % obj->getHoverText() % strFromInt3(dst));
	return moveHeroToTile(dst, h);
}

void VCAI::performObjectInteraction(const CGObjectInstance * obj, HeroPtr h)
{
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
					h->visitedTown->hasBuilt (EBuilding::MAGES_GUILD_1))
					cb->buyArtifact(h.get(), 0); //buy spellbook
			}
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

bool VCAI::canGetArmy (const CGHeroInstance * army, const CGHeroInstance * source)
{ //TODO: merge with pickBestCreatures
	const CArmedInstance *armies[] = {army, source};
	int armySize = 0; 
	//we calculate total strength for each creature type available in armies
	std::map<const CCreature*, int> creToPower;
	BOOST_FOREACH(auto armyPtr, armies)
		BOOST_FOREACH(auto &i, armyPtr->Slots())
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
		BOOST_FOREACH(auto armyPtr, armies)
			for (int j = 0; j < GameConstants::ARMY_SIZE; j++)
			{
				if(armyPtr->getCreature(j) == bestArmy[i]  &&  (i != j || armyPtr != army)) //it's a searched creature not in dst slot
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
	BOOST_FOREACH(auto armyPtr, armies)
		BOOST_FOREACH(auto &i, armyPtr->Slots())
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
		BOOST_FOREACH(auto armyPtr, armies)
			for (int j = 0; j < GameConstants::ARMY_SIZE; j++)
			{
				if(armyPtr->getCreature(j) == bestArmy[i]  &&  (i != j || armyPtr != army)) //it's a searched creature not in dst slot
					if (!(armyPtr->needsLastStack() && armyPtr->Slots().size() == 1 && armyPtr != army))
						cb->mergeOrSwapStacks(armyPtr, army, j, i);
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

bool VCAI::tryBuildStructure(const CGTownInstance * t, int building, unsigned int maxDays)
{
	if (!vstd::contains(t->town->buildings, building))
		return false; // no such building in town

	if (t->hasBuilt(building)) //Already built? Shouldn't happen in general
		return true;

	std::set<int> toBuild = cb->getBuildingRequiments(t, building);

	//erase all already built buildings
	for (auto buildIter = toBuild.begin(); buildIter != toBuild.end();)
	{
		if (t->hasBuilt(*buildIter))
			toBuild.erase(buildIter++);
		else
			buildIter++;
	}

	toBuild.insert(building);

	BOOST_FOREACH(int buildID, toBuild)
	{
		int canBuild = cb->canBuildStructure(t, buildID);
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

	BOOST_FOREACH(int buildID, toBuild)
	{
		const CBuilding *b = t->town->buildings[buildID];

		int canBuild = cb->canBuildStructure(t, buildID);
		if(canBuild == EBuildingState::ALLOWED)
		{
			if(!containsSavedRes(b->resources))
			{
				BNLOG("Player %d will build %s in town of %s at %s", playerID % b->Name() % t->name % t->pos);
				cb->buildBuilding(t, buildID);
				return true;
			}
			continue;
		}
		else if(canBuild == EBuildingState::NO_RESOURCES)
		{
			TResources cost = t->town->buildings[buildID]->resources;
			for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
			{
				int diff = currentRes[i] - cost[i] + income[i];
				if(diff < 0)
					saving[i] = 1;
			}
			continue;
		}
	}
	return false;
}

bool VCAI::tryBuildAnyStructure(const CGTownInstance * t, std::vector<int> buildList, unsigned int maxDays)
{
	BOOST_FOREACH(int building, buildList)
	{
		if(t->hasBuilt(building))
			continue;
		if (tryBuildStructure(t, building, maxDays))
			return true;
	}
	return false; //Can't build anything
}

bool VCAI::tryBuildNextStructure(const CGTownInstance * t, std::vector<int> buildList, unsigned int maxDays)
{
	BOOST_FOREACH(int building, buildList)
	{
		if(t->hasBuilt(building))
			continue;
		return tryBuildStructure(t, building, maxDays);
	}
	return false;//Nothing to build
}

void VCAI::buildStructure(const CGTownInstance * t)
{
	using namespace EBuilding;
	//TODO make *real* town development system
	//TODO: faction-specific development: use special buildings, build dwellings in better order, etc
	//TODO: build resource silo, defences when needed
	//Possible - allow "locking" on specific building (build prerequisites and then building itself)

	//Set of buildings for different goals. Does not include any prerequisites.
	const int essential[] = {TAVERN, TOWN_HALL};
	const int goldSource[] = {TOWN_HALL, CITY_HALL, CAPITOL};
	const int unitsSource[] = { 30, 31, 32, 33, 34, 35, 36};
	const int unitsUpgrade[] = { 37, 38, 39, 40, 41, 42, 43};
	const int unitGrowth[] = { FORT, CITADEL, CASTLE, HORDE_1, HORDE_1_UPGR, HORDE_2, HORDE_2_UPGR};
	const int spells[] = {MAGES_GUILD_1, MAGES_GUILD_2, MAGES_GUILD_3, MAGES_GUILD_4, MAGES_GUILD_5};
	const int extra[] = {RESOURCE_SILO, SPECIAL_1, SPECIAL_2, SPECIAL_3, SPECIAL_4, SHIPYARD}; // all remaining buildings

	TResources currentRes = cb->getResourceAmount();
	TResources income = estimateIncome();

	if (tryBuildAnyStructure(t, std::vector<int>(essential, essential + ARRAY_COUNT(essential))))
		return;

	//we're running out of gold - try to build something gold-producing. Multiplier can be tweaked, 6 is minimum due to buildings costs
	if (currentRes[Res::GOLD] < income[Res::GOLD] * 6)
		if (tryBuildNextStructure(t, std::vector<int>(goldSource, goldSource + ARRAY_COUNT(goldSource))))
			return;

	if (cb->getDate(Date::DAY_OF_WEEK) > 6)// last 2 days of week - try to focus on growth
	{
		if (tryBuildNextStructure(t, std::vector<int>(unitGrowth, unitGrowth + ARRAY_COUNT(unitGrowth)), 2))
			return;
	}

	// first in-game week or second half of any week: try build dwellings
	if (cb->getDate(Date::DAY) < 7 || cb->getDate(Date::DAY_OF_WEEK) > 3)
		if (tryBuildAnyStructure(t, std::vector<int>(unitsSource, unitsSource + ARRAY_COUNT(unitsSource)), 8 - cb->getDate(Date::DAY_OF_WEEK)))
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
	if (tryBuildNextStructure(t, std::vector<int>(goldSource, goldSource + ARRAY_COUNT(goldSource))))
		return;
	if (tryBuildNextStructure(t, std::vector<int>(spells, spells + ARRAY_COUNT(spells))))
		return;
	if (tryBuildAnyStructure(t, std::vector<int>(extra, extra + ARRAY_COUNT(extra))))
		return;
}

bool isSafeToVisit(HeroPtr h, crint3 tile)
{
	const ui64 heroStrength = h->getTotalStrength(),
		dangerStrength = evaluateDanger(tile, *h);
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

std::vector<const CGObjectInstance *> VCAI::getPossibleDestinations(HeroPtr h)
{
	validateVisitableObjs();
	std::vector<const CGObjectInstance *> possibleDestinations;
	BOOST_FOREACH(const CGObjectInstance *obj, visitableObjs)
	{
		if(cb->getPathInfo(obj->visitablePos())->reachable() && !obj->wasVisited(playerID) &&
			(obj->tempOwner != playerID || isWeeklyRevisitable(obj))) //flag or get weekly resources / creatures
			possibleDestinations.push_back(obj);
	}

	boost::sort(possibleDestinations, isCloser);

	possibleDestinations.erase(boost::remove_if(possibleDestinations, [&](const CGObjectInstance *obj) -> bool
		{
			const int3 pos = obj->visitablePos();
			if(vstd::contains(alreadyVisited, obj))
				return true;

			if(!isSafeToVisit(h, pos))
				return true;

			if (!shouldVisit(h, obj))
				return true;

			if (vstd::contains(reservedObjs, obj)) //does checking for our own reserved objects make sense? here?
				return true;

			const CGObjectInstance *topObj = cb->getVisitableObjs(pos).back(); //it may be hero visiting this obj
			//we don't try visiting object on which allied or owned hero stands
			// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
			if(topObj->ID == Obj::HERO  &&  cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != 0)
				return true;

			return false;
		}),possibleDestinations.end());

	return possibleDestinations;
}

void VCAI::wander(HeroPtr h)
{
	while(1)
	{
		std::vector <ObjectIdRef> dests;
		range::copy(reservedHeroesMap[h], std::back_inserter(dests));
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
	            BNLOG("%s can't reach any town, we'll try to make our way to %s at %s", h->name % t->name % t->visitablePos());
				int3 pos1 = h->pos;
				striveToGoal(CGoal(CLEAR_WAY_TO).settile(t->visitablePos()).sethero(h));
				if (pos1 == h->pos && h == primaryHero()) //hero can't move
				{
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
		const ObjectIdRef&dest = dests.front();
		if(!goVisitObj(dest, h))
		{
			if(!dest)
			{
				BNLOG("Visit attempt made the object (id=%d) gone...", dest.id);
			}
			else
			{
				BNLOG("Hero %s apparently used all MPs (%d left)\n", h->name % h->movement);
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

void VCAI::setGoal(HeroPtr h, const CGoal goal)
{ //TODO: check for presence?
	if (goal.goalType == EGoals::INVALID)
		remove_if_present(lockedHeroes, h);
	else
		lockedHeroes[h] = CGoal(goal).setisElementar(false); //always evaluate goals before realizing
}

void VCAI::setGoal(HeroPtr h, EGoals goalType)
{
	if (goalType == EGoals::INVALID)
		remove_if_present(lockedHeroes, h);
	else
		lockedHeroes[h] = CGoal(goalType).setisElementar(false); //always evaluate goals before realizing;
}

void VCAI::completeGoal (const CGoal goal)
{
	if (const CGHeroInstance * h = goal.hero.get(true))
	{
		auto it = lockedHeroes.find(h);
		if (it != lockedHeroes.end())
			if (it->second.goalType == goal.goalType)
				lockedHeroes.erase(it); //goal fulfilled, free hero
	}
}

void VCAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	NET_EVENT_HANDLER;
	assert(playerID > GameConstants::PLAYER_LIMIT || status.getBattle() == UPCOMING_BATTLE);
	status.setBattle(ONGOING_BATTLE);
	const CGObjectInstance *presumedEnemy = backOrNull(cb->getVisitableObjs(tile)); //may be NULL in some very are cases -> eg. visited monolith and fighting with an enemy at the FoW covered exit
	battlename = boost::str(boost::format("Starting battle of %s attacking %s at %s") % (hero1 ? hero1->name : "a army") % (presumedEnemy ? presumedEnemy->hoverName : "unknown enemy") % tile);
	CAdventureAI::battleStart(army1, army2, tile, hero1, hero2, side);
}

void VCAI::battleEnd(const BattleResult *br)
{
	NET_EVENT_HANDLER;
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
	foreach_tile_pos([&](const int3 &pos)
	{
		BOOST_FOREACH(const CGObjectInstance *obj, myCb->getVisitableObjs(pos, false))
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

HeroPtr VCAI::getHeroWithGrail() const
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

bool VCAI::isAccessibleForHero(const int3 & pos, HeroPtr h, bool includeAllies /*= false*/) const
{
	cb->setSelection(*h);
	if (!includeAllies)
	{ //don't visit tile occupied by allied hero
		BOOST_FOREACH (auto obj, cb->getVisitableObjs(pos))
		{
			if (obj->ID == Obj::HERO && obj->tempOwner == h->tempOwner && obj != h)
				return false;
		}
	}
	return cb->getPathInfo(pos)->reachable();
}

bool VCAI::moveHeroToTile(int3 dst, HeroPtr h)
{
	visitedObject = NULL;
	int3 startHpos = h->visitablePos();
	bool ret = false;
	if(startHpos == dst)
	{
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
			tlog1 << "Hero " << h->name << " cannot reach " << dst << std::endl;
			//setGoal(h, INVALID);
			completeGoal (CGoal(VISIT_TILE).sethero(h));
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
			//tlog0 << "Moving " << h->name << " from " << h->getPosition() << " to " << endpos << std::endl;
			cb->moveHero(*h, CGHeroInstance::convertPosition(endpos, true));
			waitTillFree(); //movement may cause battle or blocking dialog
			boost::this_thread::interruption_point();
			if(!h) //we lost hero - remove all tasks assigned to him/her
			{
				lostHero(h);
				//we need to throw, otherwise hero will be assigned to sth again
				throw std::runtime_error("Hero was lost!");
				break;
			}

		}
		ret = !i;
	}
	if (visitedObject) //we step into something interesting
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
	BNLOG("Hero %s moved from %s to %s", h->name % startHpos % h->visitablePos());
	return ret;
}

int howManyTilesWillBeDiscovered(const int3 &pos, int radious)
{ //TODO: do not explore dead-end boundaries
	int ret = 0;
	for(int x = pos.x - radious; x <= pos.x + radious; x++)
	{
		for(int y = pos.y - radious; y <= pos.y + radious; y++)
		{
			int3 npos = int3(x,y,pos.z);
			if(cb->isInTheMap(npos) && pos.dist2d(npos) - 0.5 < radious  && !cb->isVisible(npos))
			{
				if (!boundaryBetweenTwoPoints (pos, npos))
					ret++;
			}
		}
	}

	return ret;
}

bool boundaryBetweenTwoPoints (int3 pos1, int3 pos2) //determines if two points are separated by known barrier
{
	int xMin = std::min (pos1.x, pos2.x);
	int xMax = std::max (pos1.x, pos2.x);
	int yMin = std::min (pos1.y, pos2.y);
	int yMax = std::max (pos1.y, pos2.y);

	for (int x = xMin; x <= xMax; ++x)
	{
		for (int y = yMin; y <= yMax; ++y)
		{
			int3 tile = int3(x, y, pos1.z); //use only on same level, ofc
			if (abs(pos1.dist2d(tile) - pos2.dist2d(tile)) < 1.5)
			{
				if (!(cb->isVisible(tile) && cb->getTile(tile)->blocked)) //if there's invisible or unblocked tile inbetween, it's good 
					return false;
			}
		}
	}
	return true; //if all are visible and blocked, we're at dead end
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
	BNLOG("Attempting realizing goal with code %s", g.name());
	switch(g.goalType)
	{
	case EXPLORE:
		{
			throw cannotFulfillGoalException("EXPLORE is not a elementar goal!");
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
			//cb->recalculatePaths();
			if(!g.hero->movement)
				throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
			//if(!g.isBlockedBorderGate(g.tile))
			//{
				if (ai->moveHeroToTile(g.tile, g.hero.get()))
				{
					throw goalFulfilledException (g);
				}
			//}
			//else
			//	throw cannotFulfillGoalException("There's a blocked gate!, we should never be here"); //CLEAR_WAY_TO should get keymaster tent
		}
		break;
	case VISIT_HERO:
		{
			if(!g.hero->movement)
				throw cannotFulfillGoalException("Cannot visit tile: hero is out of MPs!");
			if (ai->moveHeroToTile(g.tile, g.hero.get()))
			{
				throw goalFulfilledException (g);
			}
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
			if (g.hero->diggingStatus() == CGHeroInstance::CAN_DIG)
			{
				cb->dig(g.hero.get());
				setGoal(g.hero, INVALID); // finished digging
			}
			else
			{
				ai->lockedHeroes[g.hero] = g; //hero who tries to dig shouldn't do anything else
				throw cannotFulfillGoalException("A hero can't dig!\n");
			}
		}
		break;

	case COLLECT_RES: //TODO: use piles and mines?
		if(cb->getResourceAmount(g.resID) >= g.value)
			throw cannotFulfillGoalException("Goal is already fulfilled!");

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
					//TODO trade only as much as needed
					cb->trade(obj, EMarketMode::RESOURCE_RESOURCE, i, g.resID, toGive);
					if(cb->getResourceAmount(g.resID) >= g.value)
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

	case CONQUER:
	case GATHER_ARMY:
	case BOOST_HERO:
		// TODO: conquer??
		throw cannotFulfillGoalException("I don't know how to fulfill this!");

	case BUILD:
		performTypicalActions(); //TODO: separate build and wander
		throw cannotFulfillGoalException("BUILD has been realized as much as possible.");

	case INVALID:
		throw cannotFulfillGoalException("I don't know how to fulfill this!");

	default:
		throw cannotFulfillGoalException("Unknown type of goal !");
	}
}

const CGTownInstance * VCAI::findTownWithTavern() const
{
	BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
		if(t->hasBuilt(EBuilding::TAVERN) && !t->visitingHero)
			return t;

	return NULL;
}

std::vector<HeroPtr> VCAI::getUnblockedHeroes() const
{
	std::vector<HeroPtr> ret;
	boost::copy(cb->getHeroesInfo(), std::back_inserter(ret));

	BOOST_FOREACH(auto h, lockedHeroes)
	{
		//if (!h.second.invalid()) //we can use heroes without valid goal
		if (h.second.goalType == DIG_AT_TILE || !h.first->movement) //experiment: use all heroes that have movement left, TODO: unlock heroes that couldn't realize their goals 
			remove_if_present(ret, h.first);
	}
	return ret;
}

HeroPtr VCAI::primaryHero() const
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

bool VCAI::fulfillsGoal (CGoal &goal, CGoal &mainGoal)
{
	if (mainGoal.goalType == GET_OBJ && goal.goalType == VISIT_TILE) //deduce that GET_OBJ was completed by visiting object's tile
	{ //TODO: more universal mechanism
		if (cb->getObj(mainGoal.objid)->visitablePos() == goal.tile)
			return true;
	}
	return false;
}
bool VCAI::fulfillsGoal (CGoal &goal, const CGoal &mainGoal)
{
	if (mainGoal.goalType == GET_OBJ && goal.goalType == VISIT_TILE) //deduce that GET_OBJ was completed by visiting object's tile
	{ //TODO: more universal mechanism
		if (cb->getObj(mainGoal.objid)->visitablePos() == goal.tile)
			return true;
	}
	return false;
}

void VCAI::striveToGoal(const CGoal &ultimateGoal)
{
	if (ultimateGoal.invalid())
		return;

	CGoal abstractGoal;

	while(1)
	{
		CGoal goal = ultimateGoal;
		BNLOG("Striving to goal of type %s", ultimateGoal.name());
		int maxGoals = 100; //preventing deadlock for mutually dependent goals
		while(!goal.isElementar && !goal.isAbstract && maxGoals)
		{
			INDENT;
			BNLOG("Considering goal %s", goal.name());
			try
			{
				boost::this_thread::interruption_point();
				goal = goal.whatToDoToAchieve();
				--maxGoals;
			}
			catch(std::exception &e)
			{
				BNLOG("Goal %s decomposition failed: %s", goal.name() % e.what());
				//setGoal (goal.hero, INVALID); //test: if we don't know how to realize goal, we should abandon it for now
				return;
			}
		}

		try
		{
			boost::this_thread::interruption_point();

			if (goal.hero) //lock this hero to fulfill ultimate goal
			{
				if (maxGoals)
				{
					setGoal(goal.hero, goal);
				}
				else
				{
					setGoal(goal.hero, INVALID); // we seemingly don't know what to do with hero
				}
			}

			if (goal.isAbstract)
			{
				abstractGoal = goal; //allow only one abstract goal per call
				BNLOG("Choosing abstract goal %s", goal.name());
				break;
			}
			else
				tryRealize(goal);

			boost::this_thread::interruption_point();
		}
		catch(boost::thread_interrupted &e)
		{
			BNLOG("Player %d: Making turn thread received an interruption!", playerID);
			throw; //rethrow, we want to truly end this thread
		}
		catch(goalFulfilledException &e)
		{
			completeGoal (goal);
			if (fulfillsGoal (goal, ultimateGoal) || maxGoals > 98) //completed goal was main goal //TODO: find better condition
				return; 
		}
		catch(std::exception &e)
		{
			BNLOG("Failed to realize subgoal of type %s (greater goal type was %s), I will stop.", goal.name() % ultimateGoal.name());
			BNLOG("The error message was: %s", e.what());
			break;
		}
	}

	//TODO: save abstract goals not related to hero
	if (!abstractGoal.invalid()) //try to realize our one goal
	{
		while (1)
		{
			CGoal goal = CGoal(abstractGoal).setisAbstract(false);
			int maxGoals = 50;
			while (!goal.isElementar && maxGoals) //find elementar goal and fulfill it
			{
				try
				{
					boost::this_thread::interruption_point();
					goal = goal.whatToDoToAchieve();
					--maxGoals;
				}
				catch(std::exception &e)
				{
					BNLOG("Goal %s decomposition failed: %s", goal.name() % e.what());
					//setGoal (goal.hero, INVALID);
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
			catch(goalFulfilledException &e)
			{
				completeGoal (goal); //FIXME: deduce that we have realized GET_OBJ goal
				if (fulfillsGoal (goal, abstractGoal) || maxGoals > 98) //completed goal was main goal
					return;
			}
			catch(std::exception &e)
			{
				BNLOG("Failed to realize subgoal of type %s (greater goal type was %s), I will stop.", goal.name() % ultimateGoal.name());
				BNLOG("The error message was: %s", e.what());
				break;
			}
		}
	}
}

void VCAI::striveToQuest (const QuestInfo &q)
{
	if (q.quest->missionType && q.quest->progress != CQuest::COMPLETE) //FIXME: quests are never synchronized. Pointer handling needed
	{
		MetaString ms;
		q.quest->getRolloverText(ms, false);
		BNLOG ("Trying to realize quest: %s", ms.toString());
		auto heroes = cb->getHeroesInfo();

		switch (q.quest->missionType)
		{
			case CQuest::MISSION_ART:
			{
				BOOST_FOREACH (auto hero, heroes) //TODO: remove duplicated code?
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (CGoal(GET_OBJ).setobjid(q.obj->id).sethero(hero));
						return;
					}
				}
				BOOST_FOREACH (auto art, q.quest->m5arts)
				{
					striveToGoal (CGoal(GET_ART_TYPE).setaid(art)); //TODO: transport?
				}
				break;
			}
			case CQuest::MISSION_HERO:
			{
				//striveToGoal (CGoal(RECRUIT_HERO));
				BOOST_FOREACH (auto hero, heroes)
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (CGoal(GET_OBJ).setobjid(q.obj->id).sethero(hero));
						return;
					}
				}
				striveToGoal (CGoal(FIND_OBJ).setobjid(Obj::PRISON)); //rule of a thumb - quest heroes usually are locked in prisons
				//BNLOG ("Don't know how to recruit hero with id %d\n", q.quest->m13489val);
				break;
			}
			case CQuest::MISSION_ARMY:
			{
				BOOST_FOREACH (auto hero, heroes)
				{
					if (q.quest->checkQuest(hero)) //veyr bad info - stacks can be split between multiple heroes :(
					{
						striveToGoal (CGoal(GET_OBJ).setobjid(q.obj->id).sethero(hero));
						return;
					}
				}
				BOOST_FOREACH (auto creature, q.quest->m6creatures)
				{
					striveToGoal (CGoal(GATHER_TROOPS).setobjid(creature.type->idNumber).setvalue(creature.count));
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
						 striveToGoal (CGoal(VISIT_TILE).settile(q.tile));
					}
					else
					{
						for (int i = 0; i < q.quest->m7resources.size(); ++i)
						{
							if (q.quest->m7resources[i])
								striveToGoal (CGoal(COLLECT_RES).setresID(i).setvalue(q.quest->m7resources[i]));
						}
					}
				}
				else
					striveToGoal (CGoal(RECRUIT_HERO)); //FIXME: checkQuest requires any hero belonging to player :(
				break;
			}
			case CQuest::MISSION_KILL_HERO:
			case CQuest::MISSION_KILL_CREATURE:
			{
				auto obj = cb->getObjByQuestIdentifier(q.quest->m13489val);
				if (obj)
					striveToGoal (CGoal(GET_OBJ).setobjid(obj->id));
				else
					striveToGoal (CGoal(VISIT_TILE).settile(q.tile)); //visit seer hut
				break;
			}
			case CQuest::MISSION_PRIMARY_STAT:
			{
				auto heroes = cb->getHeroesInfo();
				BOOST_FOREACH (auto hero, heroes)
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (CGoal(GET_OBJ).setobjid(q.obj->id).sethero(hero));
						return;
					}
				}
				for (int i = 0; i < q.quest->m2stats.size(); ++i)
				{
					BNLOG ("Don't know how to increase primary stat %d\n", i);
				}
				break;
			}
			case CQuest::MISSION_LEVEL:
			{
				auto heroes = cb->getHeroesInfo();
				BOOST_FOREACH (auto hero, heroes)
				{
					if (q.quest->checkQuest(hero))
					{
						striveToGoal (CGoal(VISIT_TILE).settile(q.tile).sethero(hero)); //TODO: causes infinite loop :/
						return;
					}
				}
				BNLOG ("Don't know how to reach hero level %d\n", q.quest->m13489val);
				break;
			}
			case CQuest::MISSION_PLAYER:
			{
				if (playerID != q.quest->m13489val)
					BNLOG ("Can't be player of color %d\n", q.quest->m13489val);
				break;
			}
			case CQuest::MISSION_KEYMASTER:
			{
				striveToGoal (CGoal(FIND_OBJ).setobjid(Obj::KEYMASTER).setresID(q.obj->subID));
				break;
			}
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

	BOOST_FOREACH(auto h, getUnblockedHeroes())
	{
		BNLOG("Looking into %s, MP=%d", h->name.c_str() % h->movement);
		INDENT;
		makePossibleUpgrades(*h);
		cb->setSelection(*h);
		try
		{
			wander(h);
		}
		catch(std::exception &e)
		{
			BNLOG("Cannot use this hero anymore, received exception: %s", e.what());
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

int3 VCAI::explorationNewPoint(int radius, HeroPtr h, std::vector<std::vector<int3> > &tiles)
{
	TimeCheck tc("looking for new exploration point");
	PNLOG("Looking for an another place for exploration...");
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
			if(cb->getPathInfo(tile)->reachable() && isSafeToVisit(h, tile) && howManyTilesWillBeDiscovered(tile, radius) && !isBlockedBorderGate(tile))
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

void VCAI::checkHeroArmy (HeroPtr h)
{
	auto it = lockedHeroes.find(h);
	if (it != lockedHeroes.end())
	{
		if (it->second.goalType == GATHER_ARMY && it->second.value <= h->getArmyStrength())
			completeGoal(CGoal (GATHER_ARMY).sethero(h));
	}
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

void VCAI::requestActionASAP(boost::function<void()> whatToDo)
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
	BNLOG("I lost my hero %s. It's best to forget and move on.\n", h.name);

	remove_if_present(lockedHeroes, h);
	BOOST_FOREACH(auto obj, reservedHeroesMap[h])
	{
		remove_if_present(reservedObjs, obj); //unreserve all objects for that hero
	}
	remove_if_present(reservedHeroesMap, h);
}

void VCAI::answerQuery(int queryID, int selection)
{
	BNLOG("I'll answer the query %d giving the choice %d", queryID % selection);
	if(queryID != -1)
	{
		cb->selectionMade(selection, queryID);
	}
	else
	{
		BNLOG("Since the query ID is %d, the answer won't be sent. This is not a real query!", queryID);
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

AIStatus::AIStatus()
{
	battle = NO_BATTLE;
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

void AIStatus::addQuery(int ID, std::string description)
{
	boost::unique_lock<boost::mutex> lock(mx);
	if(ID == -1)
	{
		BNLOG("The \"query\" has an id %d, it'll be ignored as non-query. Description: %s", ID % description);
		return;
	}

	assert(!vstd::contains(remainingQueries, ID));
	assert(ID >= 0);

	remainingQueries[ID] = description;
	cv.notify_all();
	BNLOG("Adding query %d - %s. Total queries count: %d", ID % description % remainingQueries.size());
}

void AIStatus::removeQuery(int ID)
{
	boost::unique_lock<boost::mutex> lock(mx);
	assert(vstd::contains(remainingQueries, ID));

	std::string description = remainingQueries[ID];
	remainingQueries.erase(ID);
	cv.notify_all();
	BNLOG("Removing query %d - %s. Total queries count: %d", ID % description % remainingQueries.size());
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
	while(battle != NO_BATTLE || remainingQueries.size())
		cv.wait(lock);
}

bool AIStatus::haveTurn()
{
	boost::unique_lock<boost::mutex> lock(mx);
	return havingTurn;
}

void AIStatus::attemptedAnsweringQuery(int queryID, int answerRequestID)
{
	boost::unique_lock<boost::mutex> lock(mx);
	assert(vstd::contains(remainingQueries, queryID));
	std::string description = remainingQueries[queryID];
	BNLOG("Attempted answering query %d - %s. Request id=%d. Waiting for results...", queryID % description % answerRequestID);
	requestToQueryID[answerRequestID] = queryID;
}

void AIStatus::receivedAnswerConfirmation(int answerRequestID, int result)
{
	assert(vstd::contains(requestToQueryID, answerRequestID));
	int query = requestToQueryID[answerRequestID];
	assert(vstd::contains(remainingQueries, query));
	requestToQueryID.erase(answerRequestID);

	if(result)
	{
		removeQuery(query);
	}
	else
	{
		tlog1 << "Something went really wrong, failed to answer query " << query << ": " << remainingQueries[query];
		//TODO safely retry
	}
}

int3 whereToExplore(HeroPtr h)
{
	//TODO it's stupid and ineffective, write sth better
	cb->setSelection(*h);
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
		std::vector<std::vector<int3> > tiles; //tiles[distance_to_fow]
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
				return int3 (-1,-1,-1);

			auto bestDest = profits.end();
			bestDest--;
			return bestDest->second.front(); //TODO which is the real best tile?
		}
	}
}

TSubgoal CGoal::whatToDoToAchieve()
{
	BNLOG("Decomposing goal of type %s", name());
	INDENT;
	switch(goalType)
	{
	case WIN:
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
                return CGoal(GET_ART_TYPE).setaid(vc.objectId);
			case EVictoryConditionType::BEATHERO:
				return CGoal(GET_OBJ).setobjid(vc.obj->id);
			case EVictoryConditionType::BEATMONSTER:
				return CGoal(GET_OBJ).setobjid(vc.obj->id);
			case EVictoryConditionType::BUILDCITY:
				//TODO build castle/capitol
				break;
			case EVictoryConditionType::BUILDGRAIL:
				{
					if(auto h = ai->getHeroWithGrail())
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
					} //TODO: use FIND_OBJ
					else if(const CGObjectInstance * obj = ai->getUnvisitedObj(objWithID<Obj::OBELISK>)) //there are unvisited Obelisks
					{
						return CGoal(GET_OBJ).setobjid(obj->id);
					}
					else
						return CGoal(EXPLORE);
				}
				break;
			case EVictoryConditionType::CAPTURECITY:
				return CGoal(GET_OBJ).setobjid(vc.obj->id);
			case EVictoryConditionType::GATHERRESOURCE:
                return CGoal(COLLECT_RES).setresID(vc.objectId).setvalue(vc.count);
				//TODO mines? piles? marketplace?
				//save?
				break;
			case EVictoryConditionType::GATHERTROOP:
                return CGoal(GATHER_TROOPS).setobjid(vc.objectId).setvalue(vc.count);
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
	case FIND_OBJ:
		{
			const CGObjectInstance * o = NULL;
			if (resID > -1) //specified
			{
				BOOST_FOREACH(const CGObjectInstance *obj, ai->visitableObjs)
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
				BOOST_FOREACH(const CGObjectInstance *obj, ai->visitableObjs)
				{
					if(obj->ID == objid)
					{
						o = obj;
						break; //TODO: consider multiple objects and choose best
					}
				}
			}
			if (o && isReachable(o))
				return CGoal(GET_OBJ).setobjid(o->id);
			else
				return CGoal(EXPLORE);
		}
		break;
	case GET_OBJ:
		{
			const CGObjectInstance * obj = cb->getObj(objid);
			if(!obj)
				return CGoal(EXPLORE);
			int3 pos = obj->visitablePos();
			return CGoal(VISIT_TILE).settile(pos);
		}
		break;
	case VISIT_HERO:
		{
			const CGObjectInstance * obj = cb->getObj(objid);
			if(!obj)
				return CGoal(EXPLORE);
			int3 pos = obj->visitablePos();

			if (hero && ai->isAccessibleForHero(pos, hero, true) && isSafeToVisit(hero, pos)) //enemy heroes can get reinforcements
				return CGoal(*this).settile(pos).setisElementar(true);
		}
		break;
	case GET_ART_TYPE:
		{
			TSubgoal alternativeWay = CGoal::lookForArtSmart(aid); //TODO: use
			if(alternativeWay.invalid())
				return CGoal(FIND_OBJ).setobjid(Obj::ARTIFACT).setresID(aid);
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

			HeroPtr h = hero ? hero : ai->primaryHero();
			if(!h)
				return CGoal(RECRUIT_HERO);

			cb->setSelection(*h);

			SectorMap sm;
			bool dropToFile = false;
			if(dropToFile) //for debug purposes
				sm.write("test.txt");

			int3 tileToHit = sm.firstTileToGet(h, tile);
			//if(isSafeToVisit(h, tileToHit))
			if(isBlockedBorderGate(tileToHit))
			{	//FIXME: this way we'll not visit gate and activate quest :?
				return CGoal(FIND_OBJ).setobjid(Obj::KEYMASTER).setresID(cb->getTile(tileToHit)->visitableObjects.back()->subID);
			}


			//FIXME: this code shouldn't be necessary
			if(tileToHit == tile)
			{
				tlog1 << boost::format("Very strange, tile to hit is %s and tile is also %s, while hero %s is at %s\n")
					% tileToHit % tile % h->name % h->visitablePos();
				throw cannotFulfillGoalException("Retrieving first tile to hit failed (probably)!");
			}

			auto topObj = backOrNull(cb->getVisitableObjs(tileToHit));
			if(topObj && topObj->ID == Obj::HERO && cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != 0)
			{
				std::string problem = boost::str(boost::format("%s stands in the way of %s.\n") % topObj->getHoverText()  % h->getHoverText());
				throw cannotFulfillGoalException(problem);
			}

			return CGoal(VISIT_TILE).settile(tileToHit).sethero(h); //FIXME:: attempts to visit completely unreachable tile with hero results in stall

			//TODO czy istnieje lepsza droga?

		}
		throw cannotFulfillGoalException("Cannot reach given tile!");
	case EXPLORE:
		{
			auto objs = ai->visitableObjs; //try to use buildings that uncover map
			erase_if(objs, [&](const CGObjectInstance *obj) -> bool
			{
				if (vstd::contains(ai->alreadyVisited, obj))
					return true;
				switch (obj->ID)
				{
					case Obj::REDWOOD_OBSERVATORY:
					case Obj::PILLAR_OF_FIRE:
					case Obj::CARTOGRAPHER:
					case Obj::SUBTERRANEAN_GATE: //TODO: check ai->knownSubterraneanGates
					//case Obj::MONOLITH1:
					//case obj::MONOLITH2:
					//case obj::MONOLITH3:
					//case Obj::WHIRLPOOL:
						return false; //do not erase
						break;
					default:
						return true;
				}
			});
			if (objs.size())
			{
				if (hero.get(true))
				{
					BOOST_FOREACH (auto obj, objs)
					{
						auto pos = obj->visitablePos();
						//FIXME: this confition fails if everything but guarded subterranen gate was explored. in this case we should gather army for hero
						if (isSafeToVisit(hero, pos) && ai->isAccessibleForHero(pos, hero))
							return CGoal(VISIT_TILE).settile(pos).sethero(hero);
					}
				}
				else
				{
					BOOST_FOREACH (auto obj, objs)
					{
						auto pos = obj->visitablePos();
						if (ai->isAccessible (pos)) //TODO: check safety?
							return CGoal(VISIT_TILE).settile(pos).sethero(hero);
					}
				}
			}

			if (hero)
			{
				int3 t = whereToExplore(hero);
				if (t.z == -1) //no safe tile to explore - we need to break!
				{
					erase_if (objs, [&](const CGObjectInstance *obj) -> bool
					{
						switch (obj->ID)
						{
							case Obj::CARTOGRAPHER:
							case Obj::SUBTERRANEAN_GATE:
							//case Obj::MONOLITH1:
							//case obj::MONOLITH2:
							//case obj::MONOLITH3:
							//case Obj::WHIRLPOOL:
								return false; //do not erase
								break;
							default:
								return true;
						}
					});
					if (objs.size())
					{
						return CGoal (VISIT_TILE).settile(objs.front()->visitablePos()).sethero(hero).setisAbstract(true);
					}
					else
						throw cannotFulfillGoalException("Cannot explore - no possible ways found!");
				}
				return CGoal(VISIT_TILE).settile(t).sethero(hero);
			}

			auto hs = cb->getHeroesInfo();
			int howManyHeroes = hs.size();

			erase(hs, [](const CGHeroInstance *h)
			{
				return contains(ai->lockedHeroes, h);
			});
			if(hs.empty()) //all heroes are busy. buy new one
			{
				if (howManyHeroes < 3  && ai->findTownWithTavern()) //we may want to recruit second hero. TODO: make it smart finally
					return CGoal(RECRUIT_HERO);
				else //find mobile hero with weakest army
				{
					hs = cb->getHeroesInfo();
					erase_if(hs, [](const CGHeroInstance *h)
					{
						return !h->movement; //only hero with movement are of interest for us
					});
					if (hs.empty())
					{
						if (howManyHeroes < GameConstants::MAX_HEROES_PER_PLAYER)
							return CGoal(RECRUIT_HERO);
						else
							throw cannotFulfillGoalException("No heroes with remaining MPs for exploring!\n");
					}
					boost::sort(hs, compareMovement); //closer to what?
				}
			}

			const CGHeroInstance *h = hs.front();

			return (*this).sethero(h).setisAbstract(true);
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
				{
					return CGoal(RECRUIT_HERO);
				}

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
				{
					return CGoal(GATHER_ARMY).sethero(hero).setvalue(evaluateDanger(tile, *hero) * SAFE_ATTACK_CONSTANT); //TODO: should it be abstract?
				}
			}
			else	//inaccessible for all heroes
			{
				return CGoal(CLEAR_WAY_TO).settile(tile);
			}
		}
		break;

	case DIG_AT_TILE:
		{
			const CGObjectInstance *firstObj = frontOrNull(cb->getVisitableObjs(tile));
			if(firstObj && firstObj->ID == Obj::HERO && firstObj->tempOwner == ai->playerID) //we have hero at dest
			{
				const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(firstObj);
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
					auto backObj = backOrNull(cb->getVisitableObjs(m->o->visitablePos())); //it'll be a hero if we have one there; otherwise marketplace
					assert(backObj);
					if(backObj->tempOwner != ai->playerID)
						return CGoal(GET_OBJ).setobjid(m->o->id);
					return setobjid(m->o->id).setisElementar(true);
				}
			}
		}
		return CGoal(INVALID);
	case GATHER_TROOPS:
		{
			std::vector<const CGDwelling *> dwellings;
			BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
			{
				auto creature = VLC->creh->creatures[objid];
				if (t->subID == creature->faction) //TODO: how to force AI to build unupgraded creatures? :O
				{
					auto creatures = t->town->creatures[creature->level];
					int upgradeNumber = std::find(creatures.begin(), creatures.end(), creature->idNumber) - creatures.begin();

					int bid = EBuilding::DWELL_FIRST + creature->level + upgradeNumber * GameConstants::CREATURES_PER_TOWN;
					if (t->hasBuilt(bid)) //this assumes only creatures with dwellings are assigned to faction
					{
						dwellings.push_back(t);
					}
					else
					{
						return CGoal (BUILD_STRUCTURE).settown(t).setbid(bid);
					}
				}
			}
			BOOST_FOREACH (auto obj, ai->visitableObjs)
			{
				if (obj->ID != Obj::CREATURE_GENERATOR1) //TODO: what with other creature generators?
					continue;

				auto d = dynamic_cast<const CGDwelling *>(obj);
				BOOST_FOREACH (auto creature, d->creatures)
				{
					if (creature.first) //there are more than 0 creatures avaliabe
					{
						BOOST_FOREACH (auto type, creature.second)
						{
							if (type == objid)
								dwellings.push_back(d);
						}
					}
				}
			}
			if (dwellings.size())
			{
				boost::sort(dwellings, isCloser);
				return CGoal(GET_OBJ).setobjid (dwellings.front()->id); //TODO: consider needed resources
			}
			else
				return CGoal(EXPLORE);
			//TODO: exchange troops between heroes
		}
		break;
	case CONQUER: //TODO: put it into a function?
		{
			auto hs = cb->getHeroesInfo();
			int howManyHeroes = hs.size();

			erase(hs, [](const CGHeroInstance *h)
			{
				return contains(ai->lockedHeroes, h);
			});
			if(hs.empty()) //all heroes are busy. buy new one
			{
				if (howManyHeroes < 3  && ai->findTownWithTavern()) //we may want to recruit second hero. TODO: make it smart finally
					return CGoal(RECRUIT_HERO);
				else //find mobile hero with weakest army
				{
					hs = cb->getHeroesInfo();
					erase_if(hs, [](const CGHeroInstance *h)
					{
						return !h->movement; //only hero with movement are of interest for us
					});
					if (hs.empty())
					{
						if (howManyHeroes < GameConstants::MAX_HEROES_PER_PLAYER)
							return CGoal(RECRUIT_HERO);
						else
							throw cannotFulfillGoalException("No heroes with remaining MPs for exploring!\n");
					}
					boost::sort(hs, compareHeroStrength);
				}
			}

			const CGHeroInstance *h = hs.back();
			cb->setSelection(h);
			std::vector<const CGObjectInstance *> objs; //here we'll gather enemy towns and heroes
			ai->retreiveVisitableObjs(objs);
			erase_if(objs, [&](const CGObjectInstance *obj)
			{
				return (obj->ID != Obj::TOWN && obj->ID != Obj::HERO) //not town/hero
					|| cb->getPlayerRelations(ai->playerID, obj->tempOwner) != 0; //not enemy
			});
			
			if (objs.empty()) //experiment - try to conquer dwellings and mines, it should pay off
			{
				ai->retreiveVisitableObjs(objs);
				erase_if(objs, [&](const CGObjectInstance *obj)
				{
					return (obj->ID != Obj::CREATURE_GENERATOR1 && obj->ID != Obj::MINE) //not dwelling or mine
						|| cb->getPlayerRelations(ai->playerID, obj->tempOwner) != 0; //not enemy
				});
			}

			if(objs.empty())
				return CGoal(EXPLORE); //we need to find an enemy

			erase_if(objs,  [&](const CGObjectInstance *obj)
			{
				return !isSafeToVisit(h, obj->visitablePos()) || vstd::contains (ai->reservedObjs, obj); //no need to capture same object twice
			});

			if(objs.empty())
				I_AM_ELEMENTAR;

			boost::sort(objs, isCloser);
			BOOST_FOREACH(const CGObjectInstance *obj, objs)
			{
				if (ai->isAccessibleForHero(obj->visitablePos(), h))
				{
					ai->reserveObject(h, obj); //no one else will capture same object until we fail

					if (obj->ID == Obj::HERO)
						return CGoal(VISIT_HERO).sethero(h).setobjid(obj->id).setisAbstract(true); //track enemy hero
					else
						return CGoal(VISIT_TILE).sethero(h).settile(obj->visitablePos());
				}
			}

			return CGoal(EXPLORE); //enemy is inaccessible
		}
		break;
	case BUILD:
		I_AM_ELEMENTAR;
	case INVALID:
		I_AM_ELEMENTAR;
	case GATHER_ARMY:
		{
			//TODO: find hero if none set
			assert(hero);

			cb->setSelection(*hero);
			auto compareReinforcements = [this](const CGTownInstance *lhs, const CGTownInstance *rhs) -> bool
			{
				return howManyReinforcementsCanGet(hero, lhs) < howManyReinforcementsCanGet(hero, rhs);
			};

			std::vector<const CGTownInstance *> townsReachable;
			BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
			{
				if(!t->visitingHero && howManyReinforcementsCanGet(hero,t))
				{
					if(isReachable(t) && !vstd::contains (ai->townVisitsThisWeek[hero], t))
						townsReachable.push_back(t);
				}
			}

			if(townsReachable.size()) //try towns first
			{
				boost::sort(townsReachable, compareReinforcements);
				return CGoal(VISIT_TILE).sethero(hero).settile(townsReachable.back()->visitablePos());
			}
			else
			{
				if (hero == ai->primaryHero()) //we can get army from other heroes
				{
					auto otherHeroes = cb->getHeroesInfo();
					auto heroDummy = hero;
					erase_if(otherHeroes, [heroDummy](const CGHeroInstance * h)
					{
						return (h == heroDummy.h || !ai->isAccessibleForHero(heroDummy->visitablePos(), h, true) || !ai->canGetArmy(heroDummy.h, h));
					});
					if (otherHeroes.size())
					{
						boost::sort(otherHeroes, compareArmyStrength); //TODO:  check if hero has at least one stack more powerful than ours? not likely to fail
						int primaryPath, secondaryPath;
						auto h = otherHeroes.back();
						cb->setSelection(hero.h);
						primaryPath = cb->getPathInfo(h->visitablePos())->turns;
						cb->setSelection(h);
						secondaryPath = cb->getPathInfo(hero->visitablePos())->turns;

						if (primaryPath < secondaryPath)
							return CGoal(VISIT_HERO).setisAbstract(true).setobjid(h->id).sethero(hero); //go to the other hero if we are faster
						else
							return CGoal(VISIT_HERO).setisAbstract(true).setobjid(hero->id).sethero(h); //let the other hero come to us
					}
				}

				std::vector<const CGObjectInstance *> objs; //here we'll gather all dwellings
				ai->retreiveVisitableObjs(objs);
				erase_if(objs, [&](const CGObjectInstance *obj)
				{
					return (obj->ID != Obj::CREATURE_GENERATOR1);
				});
				if(objs.empty()) //no possible objects, we did eveyrthing already
					return CGoal(EXPLORE).sethero(hero);
				//TODO: check if we can recruit any creatures there, evaluate army
				else
				{
					boost::sort(objs, isCloser);
					HeroPtr h = NULL;
					BOOST_FOREACH(const CGObjectInstance *obj, objs)
					{ //find safe dwelling
						auto pos = obj->visitablePos();
						if (shouldVisit (hero, obj)) //creatures fit in army
							h = hero;
						else
						{
							BOOST_FOREACH(auto ourHero, cb->getHeroesInfo()) //make use of multiple heroes
							{
								if (shouldVisit(ourHero, obj))
									h = ourHero;
							}
						}
						if (h && isSafeToVisit(h, pos) && ai->isAccessibleForHero(pos, h))
							return CGoal(VISIT_TILE).sethero(h).settile(pos);
					}
				}
			}

			return CGoal(EXPLORE).sethero(hero); //find dwelling. use current hero to prevent him from doing nothing.
		}
		break;
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

bool isBlockedBorderGate(int3 tileToHit)
{
    return cb->getTile(tileToHit)->topVisitableId() == Obj::BORDER_GATE
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
        || (t->visitableObjects.size() == 1 && t->topVisitableId() == Obj::BOAT);
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
			break;
		case Obj::BORDER_GATE:
		case Obj::BORDERGUARD:
			return (dynamic_cast <const CGKeys *>(obj))->wasMyColorVisited (ai->playerID); //FIXME: they could be revisited sooner than in a week
			break;
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
			BOOST_FOREACH (auto q, ai->myCb->getMyQuests())
			{
				if (q.obj == obj)
				{
					return false; // do not visit guards or gates when wandering
				}
			}
			return true; //we don't have this quest yet
			break;
		}
		case Obj::SEER_HUT:
		case Obj::QUEST_GUARD:
		{
			BOOST_FOREACH (auto q, ai->myCb->getMyQuests())
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
			break;
		}
		case Obj::CREATURE_GENERATOR1:
		{
			if (obj->tempOwner != h->tempOwner)
				return true; //flag just in case
			bool canRecruitCreatures = false;
			const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);
			BOOST_FOREACH(auto level, d->creatures)
			{
				BOOST_FOREACH(auto c, level.second)
				{
					if (h->getSlotFor(c) != -1)
						canRecruitCreatures = true;
				}
			}
			return canRecruitCreatures;
			break;
		}
		case Obj::HILL_FORT:
		{	
			BOOST_FOREACH (auto slot, h->Slots())
			{
				if (slot.second->type->upgrades.size())
					return true; //TODO: check price?
			}
			return false;
			break;
		}
		case Obj::MONOLITH1:
		case Obj::MONOLITH2:
		case Obj::MONOLITH3:
		case Obj::WHIRLPOOL:
			//TODO: mechanism for handling monoliths
			return false;
			break;
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
			break;
		case Obj::PRISON:
			return ai->myCb->getHeroesInfo().size() < GameConstants::MAX_HEROES_PER_PLAYER;
			break;

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
			ai->completeGoal (CGoal(EXPLORE).sethero(h)); //if we can't find the way, seemingly all tiles were explored
			//TODO: more organized way?
			throw cannotFulfillGoalException(str(format("Cannot find connection between sectors %d and %d") % src->id % dst->id));
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
					BOOST_FOREACH(const CGTownInstance *t, cb->getTownsInfo())
					{
						if(t->hasBuilt(EBuilding::SHIPYARD))
							shipyards.push_back(t);
					}

					std::vector<const CGObjectInstance*> visObjs;
					ai->retreiveVisitableObjs(visObjs, true);
					BOOST_FOREACH(const CGObjectInstance *obj, visObjs)
					{
						if(obj->ID != Obj::TOWN) //towns were handled in the previous loop
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

const CGObjectInstance * ObjectIdRef::operator->() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::operator const CGObjectInstance*() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::ObjectIdRef(int _id) : id(_id)
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
	hid = -1;
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


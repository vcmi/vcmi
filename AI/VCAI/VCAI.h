#pragma once
typedef const int3& crint3;
typedef const std::string& crstring;

//provisional class for AI to store a reference to an owned hero object
//checks if it's valid on access, should be used in place of const CGHeroInstance*
struct HeroPtr
{
	const CGHeroInstance *h;
	int hid; //hero id (object subID or type ID)

public:
	std::string name;

	
	HeroPtr();
	HeroPtr(const CGHeroInstance *H);
	~HeroPtr();

	operator bool() const
	{
		return validAndSet();
	}

	bool operator<(const HeroPtr &rhs) const;
	const CGHeroInstance *operator->() const;
	const CGHeroInstance *operator*() const; //not that consistent with -> but all interfaces use CGHeroInstance*, so it's convenient

	const CGHeroInstance *get(bool doWeExpectNull = false) const;
	bool validAndSet() const;
};

enum BattleState
{
	NO_BATTLE,
	UPCOMING_BATTLE,
	ONGOING_BATTLE,
	ENDING_BATTLE
};

class AIStatus
{
	boost::mutex mx;
	boost::condition_variable cv;

	BattleState battle;
	std::map<int, std::string> remainingQueries;
	std::map<int, int> requestToQueryID; //IDs of answer-requests sent to server => query ids (so we can match answer confirmation from server to the query)

	bool havingTurn;

public:
	AIStatus();
	~AIStatus();
	void setBattle(BattleState BS);
	BattleState getBattle();
	void addQuery(int ID, std::string description);
	void removeQuery(int ID);
	int getQueriesCount();
	void startedTurn();
	void madeTurn();
	void waitTillFree();
	bool haveTurn();
	void attemptedAnsweringQuery(int queryID, int answerRequestID);
	void receivedAnswerConfirmation(int answerRequestID, int result);
};

enum EGoals
{
	INVALID = -1,
	WIN, DO_NOT_LOSE, CONQUER, BUILD, //build needs to get a real reasoning
	EXPLORE, GATHER_ARMY, BOOST_HERO,
	RECRUIT_HERO,
	BUILD_STRUCTURE, //if hero set, then in visited town
	COLLECT_RES,

	OBJECT_GOALS_BEGIN,
	GET_OBJ, //visit or defeat or collect the object
	FIND_OBJ, //find and visit any obj with objid + resid //TODO: consider universal subid for various types (aid, bid)

	GET_ART_TYPE,

	//BUILD_STRUCTURE,
	ISSUE_COMMAND,

	//hero
	//VISIT_OBJ, //hero + tile

	VISIT_TILE, //tile, in conjunction with hero elementar; assumes tile is reachable
	CLEAR_WAY_TO,
	DIG_AT_TILE //elementar with hero on tile
};

struct CGoal;
typedef CGoal TSubgoal;

#define SETTER(type, field) CGoal &set ## field(const type &rhs) { field = rhs; return *this; }
#if 0
	#define SETTER
#endif // _DEBUG

enum {LOW_PR = -1};

struct CGoal
{
	EGoals goalType;
	bool isElementar; SETTER(bool, isElementar)
	bool isAbstract; SETTER(bool, isAbstract) //allows to remember abstract goals
	int priority; SETTER(bool, priority)

	virtual TSubgoal whatToDoToAchieve();

	bool isBlockedBorderGate(int3 tileToHit);
	CGoal(EGoals goal = INVALID) : goalType(goal)
	{
		priority = 0;
		isElementar = false;
		isAbstract = false;
		value = 0;
		objid = -1;
		aid = -1;
		resID = -1;
		tile = int3(-1, -1, -1);
		town = NULL;
	}

	bool invalid() const;

	static TSubgoal goVisitOrLookFor(const CGObjectInstance *obj); //if obj is NULL, then we'll explore
	static TSubgoal lookForArtSmart(int aid); //checks non-standard ways of obtaining art (merchants, quests, etc.)
	static TSubgoal tryRecruitHero();

	int value; SETTER(int, value)
	int resID; SETTER(int, resID)
	int objid; SETTER(int, objid)
	int aid; SETTER(int, aid)
	int3 tile; SETTER(int3, tile)
	HeroPtr hero; SETTER(HeroPtr, hero)
	const CGTownInstance *town; SETTER(CGTownInstance *, town)
	int bid; SETTER(int, bid)
};

enum {NOT_VISIBLE = 0, NOT_CHECKED = 1, NOT_AVAILABLE};

struct SectorMap
{
	//a sector is set of tiles that would be mutually reachable if all visitable objs would be passable (incl monsters)
	struct Sector
	{
		int id;
		std::vector<int3> tiles;
		std::vector<int3> embarkmentPoints; //tiles of other sectors onto which we can (dis)embark
		bool water; //all tiles of sector are land or water
		Sector()
		{
			id = -1;
		}
	};

	bool valid; //some kind of lazy eval
	std::map<int3, int3> parent;
	std::vector<std::vector<std::vector<unsigned char>>> sector;
	//std::vector<std::vector<std::vector<unsigned char>>> pathfinderSector;

	std::map<int, Sector> infoOnSectors;

	SectorMap();
	void update();
	void clear();
	void exploreNewSector(crint3 pos, int num);
	void write(crstring fname);

	unsigned char &retreiveTile(crint3 pos);

	void makeParentBFS(crint3 source);

	int3 firstTileToGet(HeroPtr h, crint3 dst); //if h wants to reach tile dst, which tile he should visit to clear the way?
};

struct CIssueCommand : CGoal
{
	boost::function<bool()> command;

	CIssueCommand(boost::function<bool()> _command): CGoal(ISSUE_COMMAND), command(_command) {}
};


// AI lives in a dangerous world. CGObjectInstances under pointer may got deleted/hidden.
// This class stores object id, so we can detect when we lose access to the underlying object.
struct ObjectIdRef
{
	int id;

	const CGObjectInstance *operator->() const;
	operator const CGObjectInstance *() const;

	ObjectIdRef(int _id);
	ObjectIdRef(const CGObjectInstance *obj);

	bool operator<(const ObjectIdRef &rhs) const;
};

class ObjsVector : public std::vector<ObjectIdRef>
{
private:
};

class VCAI : public CAdventureAI
{
	//internal methods for town development

	//try build an unbuilt structure in maxDays at most (0 = indefinite)
	bool tryBuildStructure(const CGTownInstance * t, int building, unsigned int maxDays=0);
	//try build ANY unbuilt structure
	bool tryBuildAnyStructure(const CGTownInstance * t, std::vector<int> buildList, unsigned int maxDays=0);
	//try build first unbuilt structure
	bool tryBuildNextStructure(const CGTownInstance * t, std::vector<int> buildList, unsigned int maxDays=0);

public:
	friend class FuzzyHelper;

	std::map<const CGObjectInstance *, const CGObjectInstance *> knownSubterraneanGates;
	std::vector<const CGObjectInstance *> visitedThisWeek; //only OPWs
	std::map<HeroPtr, std::vector<const CGTownInstance *> > townVisitsThisWeek;

	std::map<HeroPtr, CGoal> lockedHeroes; //TODO: allow non-elementar objectives
	std::map<HeroPtr, std::vector<const CGObjectInstance *> > reservedHeroesMap; //objects reserved by specific heroes

	std::vector<const CGObjectInstance *> visitableObjs;
	std::vector<const CGObjectInstance *> alreadyVisited;
	std::vector<const CGObjectInstance *> reservedObjs; //to be visited by specific hero

	TResources saving;

	AIStatus status;
	std::string battlename;

	CCallback *myCb;
	VCAI(void);
	~VCAI(void);

	CGObjectInstance * visitedObject; //remember currently viisted object

	boost::thread *makingTurn;

	void tryRealize(CGoal g);

	int3 explorationBestNeighbour(int3 hpos, int radius, HeroPtr h);
	int3 explorationNewPoint(int radius, HeroPtr h, std::vector<std::vector<int3> > &tiles);
	void recruitHero();

	virtual void init(CCallback * CB);
	virtual void yourTurn();

	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, int queryID) OVERRIDE; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, int queryID) OVERRIDE; //TODO
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel) OVERRIDE; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, int queryID) OVERRIDE; //all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void serialize(COSer<CSaveFile> &h, const int version) OVERRIDE; //saving
	virtual void serialize(CISer<CLoadFile> &h, const int version) OVERRIDE; //loading
	virtual void finish() OVERRIDE;

	virtual void availableCreaturesChanged(const CGDwelling *town) OVERRIDE;
	virtual void heroMoved(const TryMoveHero & details) OVERRIDE;
	virtual void stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute) OVERRIDE;
	virtual void heroInGarrisonChange(const CGTownInstance *town) OVERRIDE;
	virtual void centerView(int3 pos, int focusTime) OVERRIDE;
	virtual void tileHidden(const boost::unordered_set<int3, ShashInt3> &pos) OVERRIDE;
	virtual void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst) OVERRIDE;
	virtual void artifactAssembled(const ArtifactLocation &al) OVERRIDE;
	virtual void showTavernWindow(const CGObjectInstance *townOrTavern) OVERRIDE;
	virtual void showThievesGuildWindow (const CGObjectInstance * obj) OVERRIDE;
	virtual void playerBlocked(int reason) OVERRIDE;
	virtual void showPuzzleMap() OVERRIDE;
	virtual void showShipyardDialog(const IShipyard *obj) OVERRIDE;
	virtual void gameOver(ui8 player, bool victory) OVERRIDE;
	virtual void artifactPut(const ArtifactLocation &al) OVERRIDE;
	virtual void artifactRemoved(const ArtifactLocation &al) OVERRIDE;
	virtual void stacksErased(const StackLocation &location) OVERRIDE;
	virtual void artifactDisassembled(const ArtifactLocation &al) OVERRIDE;
	virtual void heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start) OVERRIDE;
	virtual void availableArtifactsChanged(const CGBlackMarket *bm = NULL) OVERRIDE;
	virtual void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town) OVERRIDE;
	virtual void tileRevealed(const boost::unordered_set<int3, ShashInt3> &pos) OVERRIDE;
	virtual void heroExchangeStarted(si32 hero1, si32 hero2) OVERRIDE;
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val) OVERRIDE;
	virtual void showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level) OVERRIDE;
	virtual void heroMovePointsChanged(const CGHeroInstance * hero) OVERRIDE;
	virtual void stackChangedType(const StackLocation &location, const CCreature &newType) OVERRIDE;
	virtual void stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count) OVERRIDE;
	virtual void newObject(const CGObjectInstance * obj) OVERRIDE;
	virtual void showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor) OVERRIDE;
	virtual void playerBonusChanged(const Bonus &bonus, bool gain) OVERRIDE;
	virtual void newStackInserted(const StackLocation &location, const CStackInstance &stack) OVERRIDE;
	virtual void heroCreated(const CGHeroInstance*) OVERRIDE;
	virtual void advmapSpellCast(const CGHeroInstance * caster, int spellID) OVERRIDE;
	virtual void showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID) OVERRIDE;
	virtual void requestRealized(PackageApplied *pa) OVERRIDE;
	virtual void receivedResource(int type, int val) OVERRIDE;
	virtual void stacksSwapped(const StackLocation &loc1, const StackLocation &loc2) OVERRIDE;
	virtual void objectRemoved(const CGObjectInstance *obj) OVERRIDE;
	virtual void showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor) OVERRIDE;
	virtual void heroManaPointsChanged(const CGHeroInstance * hero) OVERRIDE;
	virtual void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val) OVERRIDE;
	virtual void battleResultsApplied() OVERRIDE;
	virtual void objectPropertyChanged(const SetObjectProperty * sop) OVERRIDE;
	virtual void buildChanged(const CGTownInstance *town, int buildingID, int what) OVERRIDE;
	virtual void heroBonusChanged(const CGHeroInstance *hero, const Bonus &bonus, bool gain) OVERRIDE;
	virtual void showMarketWindow(const IMarket *market, const CGHeroInstance *visitor) OVERRIDE;

	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) OVERRIDE;
	virtual void battleEnd(const BattleResult *br) OVERRIDE;
	void makeTurn();

	void makeTurnInternal();
	void performTypicalActions();

	void buildArmyIn(const CGTownInstance * t);
	void striveToGoal(const CGoal &ultimateGoal);
	void endTurn();
	void wander(HeroPtr h);
	void setGoal(HeroPtr h, const CGoal goal);
	void setGoal(HeroPtr h, EGoals goalType = INVALID);
	void completeGoal (const CGoal goal); //safely removes goal from reserved hero
	void striveToQuest (const QuestInfo &q); 

	void recruitHero(const CGTownInstance * t);
	std::vector<const CGObjectInstance *> getPossibleDestinations(HeroPtr h);
	void buildStructure(const CGTownInstance * t);
	//void recruitCreatures(const CGTownInstance * t);
	void recruitCreatures(const CGDwelling * d);
	void pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source); //called when we can't find a slot for new stack
	void moveCreaturesToHero(const CGTownInstance * t);
	bool goVisitObj(const CGObjectInstance * obj, HeroPtr h);
	void performObjectInteraction(const CGObjectInstance * obj, HeroPtr h);

	bool moveHeroToTile(int3 dst, HeroPtr h);

	void lostHero(HeroPtr h); //should remove all references to hero (assigned tasks and so on)
	void waitTillFree();

	void addVisitableObj(const CGObjectInstance *obj);
	void markObjectVisited (const CGObjectInstance *obj);
	void reserveObject (HeroPtr h, const CGObjectInstance *obj);
	//void removeVisitableObj(const CGObjectInstance *obj);
	void validateVisitableObjs();
	void retreiveVisitableObjs(std::vector<const CGObjectInstance *> &out, bool includeOwned = false) const;
	std::vector<const CGObjectInstance *> getFlaggedObjects() const;

	const CGObjectInstance *lookForArt(int aid) const;
	bool isAccessible(const int3 &pos);
	HeroPtr getHeroWithGrail() const;

	const CGObjectInstance *getUnvisitedObj(const boost::function<bool(const CGObjectInstance *)> &predicate);
	bool isAccessibleForHero(const int3 & pos, HeroPtr h, bool includeAllies = false) const;

	const CGTownInstance *findTownWithTavern() const;

	std::vector<HeroPtr> getUnblockedHeroes() const;
	HeroPtr primaryHero() const;
	TResources estimateIncome() const;
	bool containsSavedRes(const TResources &cost) const;
	void checkHeroArmy (HeroPtr h);

	void requestSent(const CPackForServer *pack, int requestID) OVERRIDE;
	void answerQuery(int queryID, int selection);
	//special function that can be called ONLY from game events handling thread and will send request ASAP
	void requestActionASAP(boost::function<void()> whatToDo); 
};


template<int id>
bool objWithID(const CGObjectInstance *obj)
{
	return obj->ID == id;
}

bool isWeeklyRevisitable (const CGObjectInstance * obj);
bool shouldVisit (HeroPtr h, const CGObjectInstance * obj);
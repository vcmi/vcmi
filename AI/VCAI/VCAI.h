#pragma once
typedef const int3& crint3;
typedef const std::string& crstring;

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
	int remainingQueries;
	bool havingTurn;

public:
	AIStatus();
	~AIStatus();
	void setBattle(BattleState BS);
	BattleState getBattle();
	void addQueries(int val);
	void addQuery();
	void removeQuery();
	int getQueriesCount();
	void startedTurn();
	void madeTurn();
	void waitTillFree();
	bool haveTurn();
};

enum EGoals
{
	INVALID = -1,
	WIN, DO_NOT_LOSE, CONQUER, BUILD, EXPLORE, //GATHER_ARMY,// BOOST_HERO,
	RECRUIT_HERO,
	BUILD_STRUCTURE, //if hero set, then in visited town
	COLLECT_RES,

	OBJECT_GOALS_BEGIN,
	GET_OBJ, //visit or defeat or collect the object

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
	int priority; SETTER(bool, priority)

	virtual TSubgoal whatToDoToAchieve();

	bool isBlockedBorderGate(int3 tileToHit);
	CGoal(EGoals goal = INVALID) : goalType(goal)
	{
		priority = 0;
		isElementar = false;
		objid = -1;
		aid = -1;
		tile = int3(-1, -1, -1);
		hero = NULL;
		town = NULL;
	}

	bool invalid() const;

	static TSubgoal goVisitOrLookFor(const CGObjectInstance *obj); //if obj is NULL, then we'll explore
	static TSubgoal lookForArtSmart(int aid); //checks non-standard ways of obtaining art (merchants, quests, etc.)

	int value; SETTER(int, value)
	int resID; SETTER(int, resID)
	int objid; SETTER(int, objid)
	int aid; SETTER(int, aid)
	int3 tile; SETTER(int3, tile)
	const CGHeroInstance *hero; SETTER(CGHeroInstance *, hero)
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

	int3 firstTileToGet(const CGHeroInstance *h, crint3 dst); //if h wants to reach tile dst, which tile he should visit to clear the way?
};

struct CIssueCommand : CGoal
{
	boost::function<bool()> command;

	CIssueCommand(boost::function<bool()> _command): CGoal(ISSUE_COMMAND), command(_command) {}
};

class VCAI : public CAdventureAI
{
public:
	friend class FuzzyHelper;

	std::map<const CGObjectInstance *, const CGObjectInstance *> knownSubterraneanGates;
	std::vector<const CGObjectInstance *> visitedThisWeek; //only OPWs
	std::map<const CGHeroInstance *, std::vector<const CGTownInstance *> > townVisitsThisWeek;

	std::set<const CGHeroInstance *> blockedHeroes; //they won't get any new action

	std::vector<const CGObjectInstance *> visitableObjs;
	std::vector<const CGObjectInstance *> alreadyVisited;

	TResources saving;

	AIStatus status;
	std::string battlename;

	CCallback *myCb;
	VCAI(void);
	~VCAI(void);

	CGoal currentGoal;

	CGObjectInstance * visitedObject; //remember currently viisted object

	boost::thread *makingTurn;

	void tryRealize(CGoal g);

	int3 explorationBestNeighbour(int3 hpos, int radius, const CGHeroInstance * h);
	int3 explorationNewPoint(int radius, const CGHeroInstance * h, std::vector<std::vector<int3> > &tiles);
	void recruitHero();

	virtual void init(CCallback * CB);
	virtual void yourTurn();
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback) OVERRIDE; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel) OVERRIDE; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd) OVERRIDE; //all stacks operations between these objects become allowed, interface has to call onEnd when done
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
	void wander(const CGHeroInstance * h);

	void recruitHero(const CGTownInstance * t);
	std::vector<const CGObjectInstance *> getPossibleDestinations(const CGHeroInstance *h);
	void buildStructure(const CGTownInstance * t);
	//void recruitCreatures(const CGTownInstance * t);
	void recruitCreatures(const CGDwelling * d);
	void moveCreaturesToHero(const CGTownInstance * t);
	bool goVisitObj(const CGObjectInstance * obj, const CGHeroInstance * h);
	void performObjectInteraction(const CGObjectInstance * obj, const CGHeroInstance * h);

	bool moveHeroToTile(int3 dst, const CGHeroInstance * h);
	void waitTillFree();

	void addVisitableObj(const CGObjectInstance *obj);
	//void removeVisitableObj(const CGObjectInstance *obj);
	void validateVisitableObjs();
	void retreiveVisitableObjs(std::vector<const CGObjectInstance *> &out, bool includeOwned = false) const;
	std::vector<const CGObjectInstance *> getFlaggedObjects() const;

	const CGObjectInstance *lookForArt(int aid) const;
	bool isAccessible(const int3 &pos);
	const CGHeroInstance *getHeroWithGrail() const;

	const CGObjectInstance *getUnvisitedObj(const boost::function<bool(const CGObjectInstance *)> &predicate);
	bool isAccessibleForHero(const int3 & pos, const CGHeroInstance * h) const;

	const CGTownInstance *findTownWithTavern() const;

	std::vector<const CGHeroInstance *> getUnblockedHeroes() const;
	const CGHeroInstance *primaryHero() const;
	TResources estimateIncome() const;
	bool containsSavedRes(const TResources &cost) const;
};


template<int id>
bool objWithID(const CGObjectInstance *obj)
{
	return obj->ID == id;
}

bool isWeeklyRevisitable (const CGObjectInstance * obj);
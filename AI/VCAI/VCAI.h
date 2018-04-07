/*
 * VCAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "AIUtility.h"
#include "Goals.h"
#include "../../lib/AI_Base.h"
#include "../../CCallback.h"

#include "../../lib/CThreadHelper.h"

#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CondSh.h"

struct QuestInfo;

class AIStatus
{
	boost::mutex mx;
	boost::condition_variable cv;

	BattleState battle;
	std::map<QueryID, std::string> remainingQueries;
	std::map<int, QueryID> requestToQueryID; //IDs of answer-requests sent to server => query ids (so we can match answer confirmation from server to the query)
	std::vector<const CGObjectInstance *> objectsBeingVisited;
	bool ongoingHeroMovement;
	bool ongoingChannelProbing; // true if AI currently explore bidirectional teleport channel exits

	bool havingTurn;

public:
	AIStatus();
	~AIStatus();
	void setBattle(BattleState BS);
	void setMove(bool ongoing);
	void setChannelProbing(bool ongoing);
	bool channelProbing();
	BattleState getBattle();
	void addQuery(QueryID ID, std::string description);
	void removeQuery(QueryID ID);
	int getQueriesCount();
	void startedTurn();
	void madeTurn();
	void waitTillFree();
	bool haveTurn();
	void attemptedAnsweringQuery(QueryID queryID, int answerRequestID);
	void receivedAnswerConfirmation(int answerRequestID, int result);
	void heroVisit(const CGObjectInstance * obj, bool started);


	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & battle;
		h & remainingQueries;
		h & requestToQueryID;
		h & havingTurn;
	}
};

enum
{
	NOT_VISIBLE = 0,
	NOT_CHECKED = 1,
	NOT_AVAILABLE
};

struct SectorMap
{
	//a sector is set of tiles that would be mutually reachable if all visitable objs would be passable (incl monsters)
	struct Sector
	{
		int id;
		std::vector<int3> tiles;
		std::vector<int3> embarkmentPoints; //tiles of other sectors onto which we can (dis)embark
		std::vector<const CGObjectInstance *> visitableObjs;
		bool water; //all tiles of sector are land or water
		Sector()
		{
			id = -1;
			water = false;
		}
	};

	typedef unsigned short TSectorID; //smaller than int to allow -1 value. Max number of sectors 65K should be enough for any proper map.
	typedef boost::multi_array<TSectorID, 3> TSectorArray;

	bool valid; //some kind of lazy eval
	std::map<int3, int3> parent;
	TSectorArray sector;
	//std::vector<std::vector<std::vector<unsigned char>>> pathfinderSector;

	std::map<int, Sector> infoOnSectors;
	std::shared_ptr<boost::multi_array<TerrainTile *, 3>> visibleTiles;

	SectorMap();
	SectorMap(HeroPtr h);
	void update();
	void clear();
	void exploreNewSector(crint3 pos, int num, CCallback * cbp);
	void write(crstring fname);

	bool markIfBlocked(TSectorID & sec, crint3 pos, const TerrainTile * t);
	bool markIfBlocked(TSectorID & sec, crint3 pos);
	TSectorID & retrieveTile(crint3 pos);
	TSectorID & retrieveTileN(TSectorArray & vectors, const int3 & pos);
	const TSectorID & retrieveTileN(const TSectorArray & vectors, const int3 & pos);
	TerrainTile * getTile(crint3 pos) const;
	std::vector<const CGObjectInstance *> getNearbyObjs(HeroPtr h, bool sectorsAround);

	void makeParentBFS(crint3 source);

	int3 firstTileToGet(HeroPtr h, crint3 dst); //if h wants to reach tile dst, which tile he should visit to clear the way?
	int3 findFirstVisitableTile(HeroPtr h, crint3 dst);
};

class VCAI : public CAdventureAI
{
public:
	//internal methods for town development

	//try build an unbuilt structure in maxDays at most (0 = indefinite)
	/*bool canBuildStructure(const CGTownInstance * t, BuildingID building, unsigned int maxDays=7);*/
	bool tryBuildStructure(const CGTownInstance * t, BuildingID building, unsigned int maxDays = 7);
	//try build ANY unbuilt structure
	BuildingID canBuildAnyStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays = 7);
	bool tryBuildAnyStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays = 7);
	//try build first unbuilt structure
	bool tryBuildNextStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays = 7);

	friend class FuzzyHelper;

	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel>> knownTeleportChannels;
	std::map<const CGObjectInstance *, const CGObjectInstance *> knownSubterraneanGates;
	ObjectInstanceID destinationTeleport;
	int3 destinationTeleportPos;
	std::vector<ObjectInstanceID> teleportChannelProbingList; //list of teleport channel exits that not visible and need to be (re-)explored
	//std::vector<const CGObjectInstance *> visitedThisWeek; //only OPWs
	std::map<HeroPtr, std::set<const CGTownInstance *>> townVisitsThisWeek;

	std::map<HeroPtr, Goals::TSubgoal> lockedHeroes; //TODO: allow non-elementar objectives
	std::map<HeroPtr, std::set<const CGObjectInstance *>> reservedHeroesMap; //objects reserved by specific heroes
	std::set<HeroPtr> heroesUnableToExplore; //these heroes will not be polled for exploration in current state of game

	//sets are faster to search, also do not contain duplicates
	std::set<const CGObjectInstance *> visitableObjs;
	std::set<const CGObjectInstance *> alreadyVisited;
	std::set<const CGObjectInstance *> reservedObjs; //to be visited by specific hero

	std::map<HeroPtr, std::shared_ptr<SectorMap>> cachedSectorMaps; //TODO: serialize? not necessary

	TResources saving;

	AIStatus status;
	std::string battlename;

	std::shared_ptr<CCallback> myCb;

	std::unique_ptr<boost::thread> makingTurn;

	VCAI();
	virtual ~VCAI();

	//TODO: use only smart pointers?
	void tryRealize(Goals::Explore & g);
	void tryRealize(Goals::RecruitHero & g);
	void tryRealize(Goals::VisitTile & g);
	void tryRealize(Goals::VisitHero & g);
	void tryRealize(Goals::BuildThis & g);
	void tryRealize(Goals::DigAtTile & g);
	void tryRealize(Goals::CollectRes & g);
	void tryRealize(Goals::Build & g);
	void tryRealize(Goals::Invalid & g);
	void tryRealize(Goals::AbstractGoal & g);

	int3 explorationBestNeighbour(int3 hpos, int radius, HeroPtr h);
	int3 explorationNewPoint(HeroPtr h);
	int3 explorationDesperate(HeroPtr h);
	bool isTileNotReserved(const CGHeroInstance * h, int3 t); //the tile is not occupied by allied hero and the object is not reserved
	void recruitHero();

	virtual std::string getBattleAIName() const override;

	virtual void init(std::shared_ptr<CCallback> CB) override;
	virtual void yourTurn() override;

	virtual void heroGotLevel(const CGHeroInstance * hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> & skills, QueryID queryID) override; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID) override; //TODO
	virtual void showBlockingDialog(const std::string & text, const std::vector<Component> & components, QueryID askID, const int soundID, bool selection, bool cancel) override; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID) override; //all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) override;
	void showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) override;
	virtual void saveGame(BinarySerializer & h, const int version) override; //saving
	virtual void loadGame(BinaryDeserializer & h, const int version) override; //loading
	virtual void finish() override;

	virtual void availableCreaturesChanged(const CGDwelling * town) override;
	virtual void heroMoved(const TryMoveHero & details) override;
	virtual void heroInGarrisonChange(const CGTownInstance * town) override;
	virtual void centerView(int3 pos, int focusTime) override;
	virtual void tileHidden(const std::unordered_set<int3, ShashInt3> & pos) override;
	virtual void artifactMoved(const ArtifactLocation & src, const ArtifactLocation & dst) override;
	virtual void artifactAssembled(const ArtifactLocation & al) override;
	virtual void showTavernWindow(const CGObjectInstance * townOrTavern) override;
	virtual void showThievesGuildWindow(const CGObjectInstance * obj) override;
	virtual void playerBlocked(int reason, bool start) override;
	virtual void showPuzzleMap() override;
	virtual void showShipyardDialog(const IShipyard * obj) override;
	virtual void gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult) override;
	virtual void artifactPut(const ArtifactLocation & al) override;
	virtual void artifactRemoved(const ArtifactLocation & al) override;
	virtual void artifactDisassembled(const ArtifactLocation & al) override;
	virtual void heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start) override;
	virtual void availableArtifactsChanged(const CGBlackMarket * bm = nullptr) override;
	virtual void heroVisitsTown(const CGHeroInstance * hero, const CGTownInstance * town) override;
	virtual void tileRevealed(const std::unordered_set<int3, ShashInt3> & pos) override;
	virtual void heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query) override;
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val) override;
	virtual void showRecruitmentDialog(const CGDwelling * dwelling, const CArmedInstance * dst, int level) override;
	virtual void heroMovePointsChanged(const CGHeroInstance * hero) override;
	virtual void garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2) override;
	virtual void newObject(const CGObjectInstance * obj) override;
	virtual void showHillFortWindow(const CGObjectInstance * object, const CGHeroInstance * visitor) override;
	virtual void playerBonusChanged(const Bonus & bonus, bool gain) override;
	virtual void heroCreated(const CGHeroInstance *) override;
	virtual void advmapSpellCast(const CGHeroInstance * caster, int spellID) override;
	virtual void showInfoDialog(const std::string & text, const std::vector<Component> & components, int soundID) override;
	virtual void requestRealized(PackageApplied * pa) override;
	virtual void receivedResource() override;
	virtual void objectRemoved(const CGObjectInstance * obj) override;
	virtual void showUniversityWindow(const IMarket * market, const CGHeroInstance * visitor) override;
	virtual void heroManaPointsChanged(const CGHeroInstance * hero) override;
	virtual void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val) override;
	virtual void battleResultsApplied() override;
	virtual void objectPropertyChanged(const SetObjectProperty * sop) override;
	virtual void buildChanged(const CGTownInstance * town, BuildingID buildingID, int what) override;
	virtual void heroBonusChanged(const CGHeroInstance * hero, const Bonus & bonus, bool gain) override;
	virtual void showMarketWindow(const IMarket * market, const CGHeroInstance * visitor) override;
	void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions) override;

	virtual void battleStart(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side) override;
	virtual void battleEnd(const BattleResult * br) override;
	void makeTurn();

	void makeTurnInternal();
	void performTypicalActions();

	void buildArmyIn(const CGTownInstance * t);
	void striveToGoal(Goals::TSubgoal ultimateGoal);
	Goals::TSubgoal striveToGoalInternal(Goals::TSubgoal ultimateGoal, bool onlyAbstract);
	void endTurn();
	void wander(HeroPtr h);
	void setGoal(HeroPtr h, Goals::TSubgoal goal);
	void evaluateGoal(HeroPtr h); //evaluates goal assigned to hero, if any
	void completeGoal(Goals::TSubgoal goal); //safely removes goal from reserved hero
	void striveToQuest(const QuestInfo & q);

	void recruitHero(const CGTownInstance * t, bool throwing = false);
	bool isGoodForVisit(const CGObjectInstance * obj, HeroPtr h, SectorMap & sm);
	void buildStructure(const CGTownInstance * t);
	//void recruitCreatures(const CGTownInstance * t);
	void recruitCreatures(const CGDwelling * d, const CArmedInstance * recruiter);
	bool canGetArmy(const CGHeroInstance * h, const CGHeroInstance * source); //can we get any better stacks from other hero?
	void pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source); //called when we can't find a slot for new stack
	void pickBestArtifacts(const CGHeroInstance * h, const CGHeroInstance * other = nullptr);
	void moveCreaturesToHero(const CGTownInstance * t);
	bool goVisitObj(const CGObjectInstance * obj, HeroPtr h);
	void performObjectInteraction(const CGObjectInstance * obj, HeroPtr h);

	bool moveHeroToTile(int3 dst, HeroPtr h);

	void lostHero(HeroPtr h); //should remove all references to hero (assigned tasks and so on)
	void waitTillFree();

	void addVisitableObj(const CGObjectInstance * obj);
	void markObjectVisited(const CGObjectInstance * obj);
	void reserveObject(HeroPtr h, const CGObjectInstance * obj); //TODO: reserve all objects that heroes attempt to visit
	void unreserveObject(HeroPtr h, const CGObjectInstance * obj);

	void markHeroUnableToExplore(HeroPtr h);
	void markHeroAbleToExplore(HeroPtr h);
	bool isAbleToExplore(HeroPtr h);
	void clearPathsInfo();

	void validateObject(const CGObjectInstance * obj); //checks if object is still visible and if not, removes references to it
	void validateObject(ObjectIdRef obj); //checks if object is still visible and if not, removes references to it
	void validateVisitableObjs();
	void retrieveVisitableObjs(std::vector<const CGObjectInstance *> & out, bool includeOwned = false) const;
	void retrieveVisitableObjs();
	std::vector<const CGObjectInstance *> getFlaggedObjects() const;

	const CGObjectInstance * lookForArt(int aid) const;
	bool isAccessible(const int3 & pos);
	HeroPtr getHeroWithGrail() const;

	const CGObjectInstance * getUnvisitedObj(const std::function<bool(const CGObjectInstance *)> & predicate);
	bool isAccessibleForHero(const int3 & pos, HeroPtr h, bool includeAllies = false) const;
	//optimization - use one SM for every hero call
	std::shared_ptr<SectorMap> getCachedSectorMap(HeroPtr h);

	const CGTownInstance * findTownWithTavern() const;
	bool canRecruitAnyHero(const CGTownInstance * t = NULL) const;

	Goals::TSubgoal getGoal(HeroPtr h) const;
	bool canAct(HeroPtr h) const;
	std::vector<HeroPtr> getUnblockedHeroes() const;
	HeroPtr primaryHero() const;
	TResources freeResources() const; //owned resources minus gold reserve
	TResources estimateIncome() const;
	bool containsSavedRes(const TResources & cost) const;
	void checkHeroArmy(HeroPtr h);

	void requestSent(const CPackForServer * pack, int requestID) override;
	void answerQuery(QueryID queryID, int selection);
	//special function that can be called ONLY from game events handling thread and will send request ASAP
	void requestActionASAP(std::function<void()> whatToDo);

	#if 0
	//disabled due to issue 2890
	template<typename Handler> void registerGoals(Handler & h)
	{
		//h.template registerType<Goals::AbstractGoal, Goals::BoostHero>();
		h.template registerType<Goals::AbstractGoal, Goals::Build>();
		h.template registerType<Goals::AbstractGoal, Goals::BuildThis>();
		//h.template registerType<Goals::AbstractGoal, Goals::CIssueCommand>();
		h.template registerType<Goals::AbstractGoal, Goals::ClearWayTo>();
		h.template registerType<Goals::AbstractGoal, Goals::CollectRes>();
		h.template registerType<Goals::AbstractGoal, Goals::Conquer>();
		h.template registerType<Goals::AbstractGoal, Goals::DigAtTile>();
		h.template registerType<Goals::AbstractGoal, Goals::Explore>();
		h.template registerType<Goals::AbstractGoal, Goals::FindObj>();
		h.template registerType<Goals::AbstractGoal, Goals::GatherArmy>();
		h.template registerType<Goals::AbstractGoal, Goals::GatherTroops>();
		h.template registerType<Goals::AbstractGoal, Goals::GetArtOfType>();
		h.template registerType<Goals::AbstractGoal, Goals::GetObj>();
		h.template registerType<Goals::AbstractGoal, Goals::Invalid>();
		//h.template registerType<Goals::AbstractGoal, Goals::NotLose>();
		h.template registerType<Goals::AbstractGoal, Goals::RecruitHero>();
		h.template registerType<Goals::AbstractGoal, Goals::VisitHero>();
		h.template registerType<Goals::AbstractGoal, Goals::VisitTile>();
		h.template registerType<Goals::AbstractGoal, Goals::Win>();
	}
	#endif

	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & knownTeleportChannels;
		h & knownSubterraneanGates;
		h & destinationTeleport;
		h & townVisitsThisWeek;

		#if 0
		//disabled due to issue 2890
		h & lockedHeroes;
		#else
		{
			ui32 length = 0;
			h & length;
			if(!h.saving)
			{
				std::set<ui32> loadedPointers;
				lockedHeroes.clear();
				for(ui32 index = 0; index < length; index++)
				{
					HeroPtr ignored1;
					h & ignored1;

					ui8 flag = 0;
					h & flag;

					if(flag)
					{
						ui32 pid = 0xffffffff;
						h & pid;

						if(!vstd::contains(loadedPointers, pid))
						{
							loadedPointers.insert(pid);

							ui16 typeId = 0;
							//this is the problem requires such hack
							//we have to explicitly ignore invalid goal class type id
							h & typeId;
							Goals::AbstractGoal ignored2;
							ignored2.serialize(h, version);
						}
					}
				}
			}
		}
		#endif

		h & reservedHeroesMap; //FIXME: cannot instantiate abstract class
		h & visitableObjs;
		h & alreadyVisited;
		h & reservedObjs;
		h & saving;
		h & status;
		h & battlename;
		h & heroesUnableToExplore;

		//myCB is restored after load by init call
	}
};

class cannotFulfillGoalException : public std::exception
{
	std::string msg;

public:
	explicit cannotFulfillGoalException(crstring _Message)
		: msg(_Message)
	{
	}

	virtual ~cannotFulfillGoalException() throw ()
	{
	};

	const char * what() const throw () override
	{
		return msg.c_str();
	}
};

class goalFulfilledException : public std::exception
{
	std::string msg;

public:
	Goals::TSubgoal goal;

	explicit goalFulfilledException(Goals::TSubgoal Goal)
		: goal(Goal)
	{
		msg = goal->name();
	}

	virtual ~goalFulfilledException() throw ()
	{
	};

	const char * what() const throw () override
	{
		return msg.c_str();
	}
};

void makePossibleUpgrades(const CArmedInstance * obj);

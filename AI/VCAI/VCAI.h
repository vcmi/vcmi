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
#include "Goals/AbstractGoal.h"

#include "../../lib/ConditionalWait.h"
#include "../../lib/GameConstants.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/callback/CAdventureAI.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/spells/CSpellHandler.h"
#include "Pathfinding/AIPathfinder.h"

VCMI_LIB_NAMESPACE_BEGIN

struct QuestInfo;
class PathfinderCache;
class AsyncRunner;

VCMI_LIB_NAMESPACE_END

class AIhelper;

class AIStatus
{
	std::mutex mx;
	std::condition_variable cv;

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
};

class DLL_EXPORT VCAI : public CAdventureAI
{
public:

	friend class FuzzyHelper;
	friend class ResourceManager;
	friend class BuildingManager;

	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel>> knownTeleportChannels;
	std::map<const CGObjectInstance *, const CGObjectInstance *> knownSubterraneanGates;
	ObjectInstanceID destinationTeleport;
	int3 destinationTeleportPos;
	std::vector<ObjectInstanceID> teleportChannelProbingList; //list of teleport channel exits that not visible and need to be (re-)explored
	//std::vector<const CGObjectInstance *> visitedThisWeek; //only OPWs
	std::map<HeroPtr, std::set<const CGTownInstance *>> townVisitsThisWeek;
	std::unique_ptr<PathfinderCache> pathfinderCache;

	//part of mainLoop, but accessible from outside
	std::vector<Goals::TSubgoal> basicGoals;
	Goals::TGoalVec goalsToRemove;
	Goals::TGoalVec goalsToAdd;
	std::map<Goals::TSubgoal, Goals::TGoalVec> ultimateGoalsFromBasic; //theoreticlaly same goal can fulfill multiple basic goals

	std::set<HeroPtr> invalidPathHeroes; //FIXME, just a workaround
	std::map<HeroPtr, Goals::TSubgoal> lockedHeroes; //TODO: allow non-elementar objectives
	std::map<HeroPtr, std::set<const CGObjectInstance *>> reservedHeroesMap; //objects reserved by specific heroes
	std::set<HeroPtr> heroesUnableToExplore; //these heroes will not be polled for exploration in current state of game

	//sets are faster to search, also do not contain duplicates
	std::set<const CGObjectInstance *> visitableObjs;
	std::set<const CGObjectInstance *> alreadyVisited;
	std::set<const CGObjectInstance *> reservedObjs; //to be visited by specific hero
	std::map<HeroPtr, std::set<HeroPtr>> visitedHeroes; //visited this turn //FIXME: this is just bug workaround

	AIStatus status;
	std::string battlename;

	std::shared_ptr<CCallback> myCb;

	std::unique_ptr<AsyncRunner> asyncTasks;
	ThreadInterruption makingTurnInterrupption;

public:
	ObjectInstanceID selectedObject;

	AIhelper * ah;

	VCAI();
	virtual ~VCAI();

	//TODO: use only smart pointers?
	void tryRealize(Goals::Explore & g);
	void tryRealize(Goals::RecruitHero & g);
	void tryRealize(Goals::VisitTile & g);
	void tryRealize(Goals::VisitObj & g);
	void tryRealize(Goals::VisitHero & g);
	void tryRealize(Goals::BuildThis & g);
	void tryRealize(Goals::DigAtTile & g);
	void tryRealize(Goals::Trade & g);
	void tryRealize(Goals::BuyArmy & g);
	void tryRealize(Goals::Invalid & g);
	void tryRealize(Goals::AbstractGoal & g);

	bool isTileNotReserved(const CGHeroInstance * h, int3 t) const; //the tile is not occupied by allied hero and the object is not reserved

	std::string getBattleAIName() const override;

	void initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB) override;
	void yourTurn(QueryID queryID) override;

	void heroGotLevel(const CGHeroInstance * hero, PrimarySkill pskill, std::vector<SecondarySkill> & skills, QueryID queryID) override; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	void commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID) override; //TODO
	void showBlockingDialog(const std::string & text, const std::vector<Component> & components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept) override; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID) override; //all stacks operations between these objects become allowed, interface has to call onEnd when done
	void showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) override;
	void showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) override;
	void finish() override;

	void availableCreaturesChanged(const CGDwelling * town) override;
	void heroMoved(const TryMoveHero & details, bool verbose = true) override;
	void heroInGarrisonChange(const CGTownInstance * town) override;
	void centerView(int3 pos, int focusTime) override;
	void tileHidden(const std::unordered_set<int3> & pos) override;
	void artifactMoved(const ArtifactLocation & src, const ArtifactLocation & dst) override;
	void artifactAssembled(const ArtifactLocation & al) override;
	void showTavernWindow(const CGObjectInstance * object, const CGHeroInstance * visitor, QueryID queryID) override;
	void showThievesGuildWindow(const CGObjectInstance * obj) override;
	void playerBlocked(int reason, bool start) override;
	void showPuzzleMap() override;
	void showShipyardDialog(const IShipyard * obj) override;
	void gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult) override;
	void artifactPut(const ArtifactLocation & al) override;
	void artifactRemoved(const ArtifactLocation & al) override;
	void artifactDisassembled(const ArtifactLocation & al) override;
	void heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start) override;
	void availableArtifactsChanged(const CGBlackMarket * bm = nullptr) override;
	void heroVisitsTown(const CGHeroInstance * hero, const CGTownInstance * town) override;
	void tileRevealed(const std::unordered_set<int3> & pos) override;
	void heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query) override;
	void heroExperienceChanged(const CGHeroInstance * hero, si64 val) override;
	void heroPrimarySkillChanged(const CGHeroInstance * hero, PrimarySkill which, si64 val) override;
	void showRecruitmentDialog(const CGDwelling * dwelling, const CArmedInstance * dst, int level, QueryID queryID) override;
	void heroMovePointsChanged(const CGHeroInstance * hero) override;
	void garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2) override;
	void newObject(const CGObjectInstance * obj) override;
	void showHillFortWindow(const CGObjectInstance * object, const CGHeroInstance * visitor) override;
	void playerBonusChanged(const Bonus & bonus, bool gain) override;
	void heroCreated(const CGHeroInstance *) override;
	void advmapSpellCast(const CGHeroInstance * caster, SpellID spellID) override;
	void showInfoDialog(EInfoWindowMode type, const std::string & text, const std::vector<Component> & components, int soundID) override;
	void requestRealized(PackageApplied * pa) override;
	void receivedResource() override;
	void objectRemoved(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void showUniversityWindow(const IMarket * market, const CGHeroInstance * visitor, QueryID queryID) override;
	void heroManaPointsChanged(const CGHeroInstance * hero) override;
	void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val) override;
	void battleResultsApplied() override;
	void beforeObjectPropertyChanged(const SetObjectProperty * sop) override;
	void objectPropertyChanged(const SetObjectProperty * sop) override;
	void buildChanged(const CGTownInstance * town, BuildingID buildingID, int what) override;
	void heroBonusChanged(const CGHeroInstance * hero, const Bonus & bonus, bool gain) override;
	void showMarketWindow(const IMarket * market, const CGHeroInstance * visitor, QueryID queryID) override;
	void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain) override;

	void battleStart(const BattleID & battleID, const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, BattleSide side, bool replayAllowed) override;
	void battleEnd(const BattleID & battleID, const BattleResult * br, QueryID queryID) override;
	std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;

	void makeTurn();
	void mainLoop();
	void performTypicalActions();

	void buildArmyIn(const CGTownInstance * t);
	void striveToGoal(Goals::TSubgoal ultimateGoal);
	Goals::TSubgoal decomposeGoal(Goals::TSubgoal ultimateGoal);
	void endTurn();
	void wander(HeroPtr h);
	void setGoal(HeroPtr h, Goals::TSubgoal goal);
	void evaluateGoal(HeroPtr h); //evaluates goal assigned to hero, if any
	void completeGoal(Goals::TSubgoal goal); //safely removes goal from reserved hero

	void recruitHero(const CGTownInstance * t, bool throwing = false);
	bool isGoodForVisit(const CGObjectInstance * obj, HeroPtr h, std::optional<float> movementCostLimit = std::nullopt);
	bool isGoodForVisit(const CGObjectInstance * obj, HeroPtr h, const AIPath & path) const;
	//void recruitCreatures(const CGTownInstance * t);
	void recruitCreatures(const CGDwelling * d, const CArmedInstance * recruiter);
	void pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source); //called when we can't find a slot for new stack
	void pickBestArtifacts(const CGHeroInstance * h, const CGHeroInstance * other = nullptr);
	void moveCreaturesToHero(const CGTownInstance * t);
	void performObjectInteraction(const CGObjectInstance * obj, HeroPtr h);

	bool moveHeroToTile(int3 dst, HeroPtr h);
	void buildStructure(const CGTownInstance * t, BuildingID building); //TODO: move to BuildingManager

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
	virtual std::vector<const CGObjectInstance *> getFlaggedObjects() const;

	const CGObjectInstance * lookForArt(ArtifactID aid) const;
	bool isAccessible(const int3 & pos) const;
	HeroPtr getHeroWithGrail() const;

	const CGObjectInstance * getUnvisitedObj(const std::function<bool(const CGObjectInstance *)> & predicate);
	bool isAccessibleForHero(const int3 & pos, HeroPtr h, bool includeAllies = false) const;
	//optimization - use one SM for every hero call

	const CGTownInstance * findTownWithTavern() const;
	bool canRecruitAnyHero(const CGTownInstance * t = nullptr) const;

	Goals::TSubgoal getGoal(HeroPtr h) const;
	bool canAct(HeroPtr h) const;
	std::vector<HeroPtr> getUnblockedHeroes() const;
	std::vector<HeroPtr> getMyHeroes() const;
	HeroPtr primaryHero() const;
	void checkHeroArmy(HeroPtr h);
	std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h) const;
	void invalidatePaths() override;

	void requestSent(const CPackForServer * pack, int requestID) override;
	void answerQuery(QueryID queryID, int selection);
	//special function that can be called ONLY from game events handling thread and will send request ASAP
	void executeActionAsync(const std::string & description, const std::function<void()> & whatToDo);
};

class cannotFulfillGoalException : public std::exception
{
	std::string msg;

public:
	explicit cannotFulfillGoalException(crstring _Message)
		: msg(_Message)
	{
	}

	const char * what() const noexcept override
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

	const char * what() const noexcept override
	{
		return msg.c_str();
	}
};

void makePossibleUpgrades(const CArmedInstance * obj);

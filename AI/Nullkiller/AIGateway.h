/*
 * AIGateway.h, part of VCMI engine
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
#include "Pathfinding/AIPathfinder.h"
#include "Engine/Nullkiller.h"

namespace NKAI
{

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

// The gateway is responsible for AI events handling. Copied from VCAI.h and refined a bit
// Probably we can use concept of hooks to handle event in their related goals
class DLL_EXPORT AIGateway : public CAdventureAI
{
public:
	ObjectInstanceID destinationTeleport;
	int3 destinationTeleportPos;
	std::vector<ObjectInstanceID> teleportChannelProbingList; //list of teleport channel exits that not visible and need to be (re-)explored
	//std::vector<const CGObjectInstance *> visitedThisWeek; //only OPWs

	//std::set<HeroPtr> invalidPathHeroes; //FIXME, just a workaround
	//std::map<HeroPtr, Goals::TSubgoal> lockedHeroes; //TODO: allow non-elementar objectives
	//std::map<HeroPtr, std::set<const CGObjectInstance *>> reservedHeroesMap; //objects reserved by specific heroes
	//std::set<HeroPtr> heroesUnableToExplore; //these heroes will not be polled for exploration in current state of game

	//sets are faster to search, also do not contain duplicates
	//std::set<const CGObjectInstance *> reservedObjs; //to be visited by specific hero
	//std::map<HeroPtr, std::set<HeroPtr>> visitedHeroes; //visited this turn //FIXME: this is just bug workaround

	AIStatus status;
	std::string battlename;
	std::shared_ptr<CCallback> myCb;
	std::unique_ptr<boost::thread> makingTurn;
private:
	boost::mutex turnInterruptionMutex;
public:
	ObjectInstanceID selectedObject;

	std::unique_ptr<Nullkiller> nullkiller;

	AIGateway();
	virtual ~AIGateway();

	//TODO: extract to apropriate goals
	void tryRealize(Goals::DigAtTile & g);
	void tryRealize(Goals::Trade & g);

	std::string getBattleAIName() const override;

	void initGameInterface(std::shared_ptr<Environment> env, std::shared_ptr<CCallback> CB) override;
	void yourTurn(QueryID queryID) override;

	void heroGotLevel(const CGHeroInstance * hero, PrimarySkill pskill, std::vector<SecondarySkill> & skills, QueryID queryID) override; //pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	void commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID) override; //TODO
	void showBlockingDialog(const std::string & text, const std::vector<Component> & components, QueryID askID, const int soundID, bool selection, bool cancel) override; //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID) override; //all stacks operations between these objects become allowed, interface has to call onEnd when done
	void showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) override;
	void showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) override;
	void saveGame(BinarySerializer & h, const int version) override; //saving
	void loadGame(BinaryDeserializer & h, const int version) override; //loading
	void finish() override;

	void availableCreaturesChanged(const CGDwelling * town) override;
	void heroMoved(const TryMoveHero & details, bool verbose = true) override;
	void heroInGarrisonChange(const CGTownInstance * town) override;
	void centerView(int3 pos, int focusTime) override;
	void tileHidden(const std::unordered_set<int3> & pos) override;
	void artifactMoved(const ArtifactLocation & src, const ArtifactLocation & dst) override;
	void artifactAssembled(const ArtifactLocation & al) override;
	void showTavernWindow(const CGObjectInstance * townOrTavern) override;
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
	void heroPrimarySkillChanged(const CGHeroInstance * hero, PrimarySkill which, si64 val) override;
	void showRecruitmentDialog(const CGDwelling * dwelling, const CArmedInstance * dst, int level) override;
	void heroMovePointsChanged(const CGHeroInstance * hero) override;
	void garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2) override;
	void newObject(const CGObjectInstance * obj) override;
	void showHillFortWindow(const CGObjectInstance * object, const CGHeroInstance * visitor) override;
	void playerBonusChanged(const Bonus & bonus, bool gain) override;
	void heroCreated(const CGHeroInstance *) override;
	void advmapSpellCast(const CGHeroInstance * caster, int spellID) override;
	void showInfoDialog(EInfoWindowMode type, const std::string & text, const std::vector<Component> & components, int soundID) override;
	void requestRealized(PackageApplied * pa) override;
	void receivedResource() override;
	void objectRemoved(const CGObjectInstance * obj) override;
	void showUniversityWindow(const IMarket * market, const CGHeroInstance * visitor) override;
	void heroManaPointsChanged(const CGHeroInstance * hero) override;
	void heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val) override;
	void battleResultsApplied() override;
	void beforeObjectPropertyChanged(const SetObjectProperty * sop) override;
	void objectPropertyChanged(const SetObjectProperty * sop) override;
	void buildChanged(const CGTownInstance * town, BuildingID buildingID, int what) override;
	void heroBonusChanged(const CGHeroInstance * hero, const Bonus & bonus, bool gain) override;
	void showMarketWindow(const IMarket * market, const CGHeroInstance * visitor) override;
	void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain) override;
	std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleStateInfoForRetreat & battleState) override;

	void battleStart(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side, bool replayAllowed) override;
	void battleEnd(const BattleResult * br, QueryID queryID) override;

	void makeTurn();

	void buildArmyIn(const CGTownInstance * t);
	void endTurn();

	// TODO: all the routines like recruiting hero or building army should be removed from here and extracted to elementar goals or whatever
	void recruitCreatures(const CGDwelling * d, const CArmedInstance * recruiter);
	void pickBestCreatures(const CArmedInstance * army, const CArmedInstance * source); //called when we can't find a slot for new stack
	void pickBestArtifacts(const CGHeroInstance * h, const CGHeroInstance * other = nullptr);
	void moveCreaturesToHero(const CGTownInstance * t);
	void performObjectInteraction(const CGObjectInstance * obj, HeroPtr h);
	bool makePossibleUpgrades(const CArmedInstance * obj);

	bool moveHeroToTile(int3 dst, HeroPtr h);
	void buildStructure(const CGTownInstance * t, BuildingID building);

	void lostHero(HeroPtr h); //should remove all references to hero (assigned tasks and so on)
	void waitTillFree();

	void addVisitableObj(const CGObjectInstance * obj);

	void validateObject(const CGObjectInstance * obj); //checks if object is still visible and if not, removes references to it
	void validateObject(ObjectIdRef obj); //checks if object is still visible and if not, removes references to it
	void retrieveVisitableObjs();
	virtual std::vector<const CGObjectInstance *> getFlaggedObjects() const;

	void requestSent(const CPackForServer * pack, int requestID) override;
	void answerQuery(QueryID queryID, int selection);
	//special function that can be called ONLY from game events handling thread and will send request ASAP
	void requestActionASAP(std::function<void()> whatToDo);

	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & nullkiller->memory->knownTeleportChannels;
		h & nullkiller->memory->knownSubterraneanGates;
		h & destinationTeleport;
		h & nullkiller->memory->visitableObjs;
		h & nullkiller->memory->alreadyVisited;
		h & status;
		h & battlename;
	}
};

}

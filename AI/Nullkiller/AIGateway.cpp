/*
 * AIGateway.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/serializer/CTypeList.h"
#include "../../lib/serializer/BinarySerializer.h"
#include "../../lib/serializer/BinaryDeserializer.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/networkPacks/StackLocation.h"
#include "../../lib/battle/BattleStateInfoForRetreat.h"
#include "../../lib/battle/BattleInfo.h"

#include "AIGateway.h"
#include "Goals/Goals.h"

namespace NKAI
{

// our to enemy strength ratio constants
const float SAFE_ATTACK_CONSTANT = 1.1f;
const float RETREAT_THRESHOLD = 0.3f;
const double RETREAT_ABSOLUTE_THRESHOLD = 10000.;

//one thread may be turn of AI and another will be handling a side effect for AI2
thread_local CCallback * cb = nullptr;
thread_local AIGateway * ai = nullptr;

//helper RAII to manage global ai/cb ptrs
struct SetGlobalState
{
	SetGlobalState(AIGateway * AI)
	{
		assert(!ai);
		assert(!cb);

		ai = AI;
		cb = AI->myCb.get();
	}
	~SetGlobalState()
	{
		//TODO: how to handle rm? shouldn't be called after ai is destroyed, hopefully
		//TODO: to ensure that, make rm unique_ptr
		ai = nullptr;
		cb = nullptr;
	}
};


#define SET_GLOBAL_STATE(ai) SetGlobalState _hlpSetState(ai);

#define NET_EVENT_HANDLER SET_GLOBAL_STATE(this)
#define MAKING_TURN SET_GLOBAL_STATE(this)

AIGateway::AIGateway()
{
	LOG_TRACE(logAi);
	makingTurn = nullptr;
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);
	nullkiller.reset(new Nullkiller());
}

AIGateway::~AIGateway()
{
	LOG_TRACE(logAi);
	finish();
	nullkiller.reset();
}

void AIGateway::availableCreaturesChanged(const CGDwelling * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::heroMoved(const TryMoveHero & details, bool verbose)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	auto hero = cb->getHero(details.id);

	if(!hero)
		validateObject(details.id); //enemy hero may have left visible area

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
					nullkiller->memory->addSubterraneanGate(o1, o2);
				}
			}
		}
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

void AIGateway::heroInGarrisonChange(const CGTownInstance * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::centerView(int3 pos, int focusTime)
{
	LOG_TRACE_PARAMS(logAi, "focusTime '%i'", focusTime);
	NET_EVENT_HANDLER;
}

void AIGateway::artifactMoved(const ArtifactLocation & src, const ArtifactLocation & dst)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::artifactAssembled(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::showTavernWindow(const CGObjectInstance * object, const CGHeroInstance * visitor, QueryID queryID)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	status.addQuery(queryID, "TavernWindow");
	requestActionASAP([=](){ answerQuery(queryID, 0); });
}

void AIGateway::showThievesGuildWindow(const CGObjectInstance * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::playerBlocked(int reason, bool start)
{
	LOG_TRACE_PARAMS(logAi, "reason '%i', start '%i'", reason % start);
	NET_EVENT_HANDLER;
	if(start && reason == PlayerBlocked::UPCOMING_BATTLE)
		status.setBattle(UPCOMING_BATTLE);

	if(reason == PlayerBlocked::ONGOING_MOVEMENT)
		status.setMove(start);
}

void AIGateway::showPuzzleMap()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::showShipyardDialog(const IShipyard * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult)
{
	LOG_TRACE_PARAMS(logAi, "victoryLossCheckResult '%s'", victoryLossCheckResult.messageToSelf.toString());
	NET_EVENT_HANDLER;
	logAi->debug("Player %d (%s): I heard that player %d (%s) %s.", playerID, playerID.toString(), player, player.toString(), (victoryLossCheckResult.victory() ? "won" : "lost"));

	// some whitespace to flush stream
	logAi->debug(std::string(200, ' '));

	if(player == playerID)
	{
		if(victoryLossCheckResult.victory())
		{
			logAi->debug("AIGateway: Player %d (%s) won. I won! Incredible!", player, player.toString());
			logAi->debug("Turn nr %d", myCb->getDate());
		}
		else
		{
			logAi->debug("AIGateway: Player %d (%s) lost. It's me. What a disappointment! :(", player, player.toString());
		}

		// some whitespace to flush stream
		logAi->debug(std::string(200, ' '));

		finish();
	}
}

void AIGateway::artifactPut(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::artifactRemoved(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::artifactDisassembled(const ArtifactLocation & al)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start)
{
	LOG_TRACE_PARAMS(logAi, "start '%i'; obj '%s'", start % (visitedObj ? visitedObj->getObjectName() : std::string("n/a")));
	NET_EVENT_HANDLER;

	if(start && visitedObj) //we can end visit with null object, anyway
	{
		nullkiller->memory->markObjectVisited(visitedObj);
	}

	status.heroVisit(visitedObj, start);
}

void AIGateway::availableArtifactsChanged(const CGBlackMarket * bm)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::heroVisitsTown(const CGHeroInstance * hero, const CGTownInstance * town)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::tileHidden(const std::unordered_set<int3> & pos)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	nullkiller->memory->removeInvisibleObjects(myCb.get());
}

void AIGateway::tileRevealed(const std::unordered_set<int3> & pos)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	for(int3 tile : pos)
	{
		for(const CGObjectInstance * obj : myCb->getVisitableObjs(tile))
			addVisitableObj(obj);
	}
}

void AIGateway::heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	auto firstHero = cb->getHero(hero1);
	auto secondHero = cb->getHero(hero2);

	status.addQuery(query, boost::str(boost::format("Exchange between heroes %s (%d) and %s (%d)") % firstHero->getNameTranslated() % firstHero->tempOwner % secondHero->getNameTranslated() % secondHero->tempOwner));

	requestActionASAP([=]()
	{
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
		else
		{
			if(nullkiller->isActive(firstHero))
				transferFrom2to1(secondHero, firstHero);
			else
				transferFrom2to1(firstHero, secondHero);
		}

		answerQuery(query, 0);
	});
}

void AIGateway::heroPrimarySkillChanged(const CGHeroInstance * hero, PrimarySkill which, si64 val)
{
	LOG_TRACE_PARAMS(logAi, "which '%i', val '%i'", static_cast<int>(which) % val);
	NET_EVENT_HANDLER;
}

void AIGateway::showRecruitmentDialog(const CGDwelling * dwelling, const CArmedInstance * dst, int level, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "level '%i'", level);
	NET_EVENT_HANDLER;

	status.addQuery(queryID, "RecruitmentDialog");

	requestActionASAP([=](){
		recruitCreatures(dwelling, dst);
		answerQuery(queryID, 0);
	});
}

void AIGateway::heroMovePointsChanged(const CGHeroInstance * hero)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::newObject(const CGObjectInstance * obj)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(obj->isVisitable())
		addVisitableObj(obj);
}

//to prevent AI from accessing objects that got deleted while they became invisible (Cover of Darkness, enemy hero moved etc.) below code allows AI to know deletion of objects out of sight
//see: RemoveObject::applyFirstCl, to keep AI "not cheating" do not use advantage of this and use this function just to prevent crashes
void AIGateway::objectRemoved(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	if(!nullkiller) // crash protection
		return;

	nullkiller->memory->removeFromMemory(obj);

	if(obj->ID == Obj::HERO && obj->tempOwner == playerID)
	{
		lostHero(cb->getHero(obj->id)); //we can promote, since objectRemoved is called just before actual deletion
	}

	if(obj->ID == Obj::HERO && cb->getPlayerRelations(obj->tempOwner, playerID) == PlayerRelations::ENEMIES)
	{
		nullkiller->dangerHitMap->reset();
	}
}

void AIGateway::showHillFortWindow(const CGObjectInstance * object, const CGHeroInstance * visitor)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::playerBonusChanged(const Bonus & bonus, bool gain)
{
	LOG_TRACE_PARAMS(logAi, "gain '%i'", gain);
	NET_EVENT_HANDLER;
}

void AIGateway::heroCreated(const CGHeroInstance * h)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::advmapSpellCast(const CGHeroInstance * caster, SpellID spellID)
{
	LOG_TRACE_PARAMS(logAi, "spellID '%i", spellID);
	NET_EVENT_HANDLER;
}

void AIGateway::showInfoDialog(EInfoWindowMode type, const std::string & text, const std::vector<Component> & components, int soundID)
{
	LOG_TRACE_PARAMS(logAi, "soundID '%i'", soundID);
	NET_EVENT_HANDLER;
}

void AIGateway::requestRealized(PackageApplied * pa)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(status.haveTurn())
	{
		if(pa->packType == CTypeList::getInstance().getTypeID<EndTurn>(nullptr))
		{
			if(pa->result)
				status.madeTurn();
		}
	}

	if(pa->packType == CTypeList::getInstance().getTypeID<QueryReply>(nullptr))
	{
		status.receivedAnswerConfirmation(pa->requestID, pa->result);
	}
}

void AIGateway::receivedResource()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::showUniversityWindow(const IMarket * market, const CGHeroInstance * visitor, QueryID queryID)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	status.addQuery(queryID, "UniversityWindow");
	requestActionASAP([=](){ answerQuery(queryID, 0); });
}

void AIGateway::heroManaPointsChanged(const CGHeroInstance * hero)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

void AIGateway::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	LOG_TRACE_PARAMS(logAi, "which '%d', val '%d'", which % val);
	NET_EVENT_HANDLER;
}

void AIGateway::battleResultsApplied()
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	assert(status.getBattle() == ENDING_BATTLE);
	status.setBattle(NO_BATTLE);
}

void AIGateway::beforeObjectPropertyChanged(const SetObjectProperty * sop)
{

}


void AIGateway::objectPropertyChanged(const SetObjectProperty * sop)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
	if(sop->what == ObjProperty::OWNER)
	{
		auto relations = myCb->getPlayerRelations(playerID, sop->identifier.as<PlayerColor>());
		auto obj = myCb->getObj(sop->id, false);

		if(!nullkiller) // crash protection
			return;

		if(obj)
		{
			if(relations == PlayerRelations::ENEMIES)
			{
				//we want to visit objects owned by oppponents
				//addVisitableObj(obj); // TODO: Remove once save compatability broken. In past owned objects were removed from this set
				nullkiller->memory->markObjectUnvisited(obj);
			}
			else if(relations == PlayerRelations::SAME_PLAYER && obj->ID == Obj::TOWN)
			{
				// reevaluate defence for a new town
				nullkiller->dangerHitMap->reset();
			}
		}
	}
}

void AIGateway::buildChanged(const CGTownInstance * town, BuildingID buildingID, int what)
{
	LOG_TRACE_PARAMS(logAi, "what '%i'", what);
	NET_EVENT_HANDLER;
}

void AIGateway::heroBonusChanged(const CGHeroInstance * hero, const Bonus & bonus, bool gain)
{
	LOG_TRACE_PARAMS(logAi, "gain '%i'", gain);
	NET_EVENT_HANDLER;
}

void AIGateway::showMarketWindow(const IMarket * market, const CGHeroInstance * visitor, QueryID queryID)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	status.addQuery(queryID, "MarketWindow");
	requestActionASAP([=](){ answerQuery(queryID, 0); });
}

void AIGateway::showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain)
{
	//TODO: AI support for ViewXXX spell
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;
}

std::optional<BattleAction> AIGateway::makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState)
{
	LOG_TRACE(logAi);
	NET_EVENT_HANDLER;

	double ourStrength = battleState.getOurStrength();
	double fightRatio = ourStrength / (double)battleState.getEnemyStrength();

	// if we have no towns - things are already bad, so retreat is not an option.
	if(cb->getTownsInfo().size() && ourStrength < RETREAT_ABSOLUTE_THRESHOLD && fightRatio < RETREAT_THRESHOLD && battleState.canFlee)
	{
		return BattleAction::makeRetreat(battleState.ourSide);
	}

	return std::nullopt;
}


void AIGateway::initGameInterface(std::shared_ptr<Environment> env, std::shared_ptr<CCallback> CB)
{
	LOG_TRACE(logAi);
	myCb = CB;
	cbc = CB;

	NET_EVENT_HANDLER;
	playerID = *myCb->getPlayerID();
	myCb->waitTillRealize = true;
	myCb->unlockGsWhenWaiting = true;

	nullkiller->init(CB, playerID);
	
	retrieveVisitableObjs();
}

void AIGateway::yourTurn(QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;
	status.addQuery(queryID, "YourTurn");
	requestActionASAP([=](){ answerQuery(queryID, 0); });
	status.startedTurn();
	makingTurn = std::make_unique<boost::thread>(&AIGateway::makeTurn, this);
}

void AIGateway::heroGotLevel(const CGHeroInstance * hero, PrimarySkill pskill, std::vector<SecondarySkill> & skills, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;

	status.addQuery(queryID, boost::str(boost::format("Hero %s got level %d") % hero->getNameTranslated() % hero->level));
	HeroPtr hPtr = hero;

	requestActionASAP([=]()
	{ 
		if(hPtr.validAndSet())
		{
			nullkiller->heroManager->update();
			answerQuery(queryID, nullkiller->heroManager->selectBestSkill(hPtr, skills));
		}
	});
}

void AIGateway::commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "queryID '%i'", queryID);
	NET_EVENT_HANDLER;
	status.addQuery(queryID, boost::str(boost::format("Commander %s of %s got level %d") % commander->name % commander->armyObj->nodeName() % (int)commander->level));
	requestActionASAP([=](){ answerQuery(queryID, 0); });
}

void AIGateway::showBlockingDialog(const std::string & text, const std::vector<Component> & components, QueryID askID, const int soundID, bool selection, bool cancel)
{
	LOG_TRACE_PARAMS(logAi, "text '%s', askID '%i', soundID '%i', selection '%i', cancel '%i'", text % askID % soundID % selection % cancel);
	NET_EVENT_HANDLER;
	status.addQuery(askID, boost::str(boost::format("Blocking dialog query with %d components - %s")
									  % components.size() % text));

	auto hero = nullkiller->getActiveHero();
	auto target = nullkiller->getTargetTile();

	if(!selection && cancel)
	{
		requestActionASAP([=]()
		{
			//yes&no -> always answer yes, we are a brave AI :)
			bool answer = true;
			auto objects = cb->getVisitableObjs(target);

			if(hero.validAndSet() && target.valid() && objects.size())
			{
				auto topObj = objects.front()->id == hero->id ? objects.back() : objects.front();
				auto objType = topObj->ID; // top object should be our hero
				auto goalObjectID = nullkiller->getTargetObject();
				auto ratio = (float)nullkiller->dangerEvaluator->evaluateDanger(target, hero.get()) / (float)hero->getTotalStrength();

				answer = topObj->id == goalObjectID; // no if we do not aim to visit this object
				logAi->trace("Query hook: %s(%s) by %s danger ratio %f", target.toString(), topObj->getObjectName(), hero.name, ratio);

				if(cb->getObj(goalObjectID, false))
				{
					logAi->trace("AI expected %s", cb->getObj(goalObjectID, false)->getObjectName());
				}

				if(objType == Obj::BORDERGUARD || objType == Obj::QUEST_GUARD)
				{
					answer = true;
				}
				else if(objType == Obj::ARTIFACT || objType == Obj::RESOURCE)
				{
					bool dangerUnknown = ratio == 0;
					bool dangerTooHigh = ratio > (1 / SAFE_ATTACK_CONSTANT);

					answer = !dangerUnknown && !dangerTooHigh;
				}
			}

			answerQuery(askID, answer ? 1 : 0);
		});

		return;
	}

	requestActionASAP([=]()
	{
		int sel = 0;

		if(selection) //select from multiple components -> take the last one (they're indexed [1-size])
			sel = components.size();

		// TODO: Find better way to understand it is Chest of Treasures
		if(hero.validAndSet()
			&& components.size() == 2
			&& components.front().type == ComponentType::RESOURCE
			&& (nullkiller->heroManager->getHeroRole(hero) != HeroRole::MAIN
			|| nullkiller->buildAnalyzer->getGoldPreasure() > MAX_GOLD_PEASURE))
		{
			sel = 1; // for now lets pick gold from a chest.
		}

		answerQuery(askID, sel);
	});
}

void AIGateway::showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	NET_EVENT_HANDLER;
	status.addQuery(askID, boost::str(boost::format("Teleport dialog query with %d exits") % exits.size()));

	int choosenExit = -1;
	if(impassable)
	{
		nullkiller->memory->knownTeleportChannels[channel]->passability = TeleportChannel::IMPASSABLE;
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

void AIGateway::showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID)
{
	LOG_TRACE_PARAMS(logAi, "removableUnits '%i', queryID '%i'", removableUnits % queryID);
	NET_EVENT_HANDLER;

	std::string s1 = up->nodeName();
	std::string s2 = down->nodeName();

	status.addQuery(queryID, boost::str(boost::format("Garrison dialog with %s and %s") % s1 % s2));

	//you can't request action from action-response thread
	requestActionASAP([=]()
	{
		if(removableUnits && up->tempOwner == down->tempOwner)
		{
			pickBestCreatures(down, up);
		}

		answerQuery(queryID, 0);
	});
}

void AIGateway::showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects)
{
	NET_EVENT_HANDLER;
	status.addQuery(askID, "Map object select query");
	requestActionASAP([=](){ answerQuery(askID, selectedObject.getNum()); });
}

void AIGateway::saveGame(BinarySerializer & h, const int version)
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	NET_EVENT_HANDLER;
	nullkiller->memory->removeInvisibleObjects(myCb.get());

	CAdventureAI::saveGame(h, version);
	serializeInternal(h, version);
}

void AIGateway::loadGame(BinaryDeserializer & h, const int version)
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

bool AIGateway::makePossibleUpgrades(const CArmedInstance * obj)
{
	if(!obj)
		return false;

	bool upgraded = false;

	for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
	{
		if(const CStackInstance * s = obj->getStackPtr(SlotID(i)))
		{
			UpgradeInfo ui;
			myCb->fillUpgradeInfo(obj, SlotID(i), ui);
			if(ui.oldID != CreatureID::NONE && nullkiller->getFreeResources().canAfford(ui.cost[0] * s->count))
			{
				myCb->upgradeCreature(obj, SlotID(i), ui.newID[0]);
				upgraded = true;
				logAi->debug("Upgraded %d %s to %s", s->count, ui.oldID.toCreature()->getNamePluralTranslated(), ui.newID[0].toCreature()->getNamePluralTranslated());
			}
		}
	}

	return upgraded;
}

void AIGateway::makeTurn()
{
	MAKING_TURN;

	auto day = cb->getDate(Date::DAY);
	logAi->info("Player %d (%s) starting turn, day %d", playerID, playerID.toString(), day);

	boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);
	setThreadName("AIGateway::makeTurn");

	cb->sendMessage("vcmieagles");

	retrieveVisitableObjs();

	if(cb->getDate(Date::DAY_OF_WEEK) == 1)
	{
		for(const CGObjectInstance * obj : nullkiller->memory->visitableObjs)
		{
			if(isWeeklyRevisitable(obj))
			{
				nullkiller->memory->markObjectUnvisited(obj);
			}
		}
	}

#if NKAI_TRACE_LEVEL == 0
	try
	{
#endif
		nullkiller->makeTurn();

		//for debug purpose
		for (auto h : cb->getHeroesInfo())
		{
			if (h->movementPointsRemaining())
				logAi->info("Hero %s has %d MP left", h->getNameTranslated(), h->movementPointsRemaining());
		}
#if NKAI_TRACE_LEVEL == 0
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
#endif

	endTurn();
}

void AIGateway::performObjectInteraction(const CGObjectInstance * obj, HeroPtr h)
{
	LOG_TRACE_PARAMS(logAi, "Hero %s and object %s at %s", h->getNameTranslated() % obj->getObjectName() % obj->pos.toString());
	switch(obj->ID)
	{
	case Obj::TOWN:
		if(h->visitedTown) //we are inside, not just attacking
		{
			makePossibleUpgrades(h.get());

			if(!h->visitedTown->garrisonHero || !nullkiller->isHeroLocked(h->visitedTown->garrisonHero))
				moveCreaturesToHero(h->visitedTown);

			if(nullkiller->heroManager->getHeroRole(h) == HeroRole::MAIN && !h->hasSpellbook()
				&& nullkiller->getFreeGold() >= GameConstants::SPELLBOOK_GOLD_COST)
			{
				if(h->visitedTown->hasBuilt(BuildingID::MAGES_GUILD_1))
					cb->buyArtifact(h.get(), ArtifactID::SPELLBOOK);
			}
		}
		break;
	case Obj::HILL_FORT:
		makePossibleUpgrades(h.get());
		break;
	}
}

void AIGateway::moveCreaturesToHero(const CGTownInstance * t)
{
	if(t->visitingHero && t->armedGarrison() && t->visitingHero->tempOwner == t->tempOwner)
	{
		pickBestCreatures(t->visitingHero, t->getUpperArmy());
	}
}

void AIGateway::pickBestCreatures(const CArmedInstance * destinationArmy, const CArmedInstance * source)
{
	const CArmedInstance * armies[] = {destinationArmy, source};

	auto bestArmy = nullkiller->armyManager->getBestArmy(destinationArmy, destinationArmy, source);

	for(auto army : armies)
	{
		// move first stack at first slot if empty to avoid can not take away last creature
		if(!army->hasStackAtSlot(SlotID(0)) && army->stacksCount() > 0)
		{
			cb->mergeOrSwapStacks(
				army,
				army,
				SlotID(0),
				army->Slots().begin()->first);
		}
	}

	//foreach best type -> iterate over slots in both armies and if it's the appropriate type, send it to the slot where it belongs
	for(SlotID i = SlotID(0); i.validSlot(); i.advance(1)) //i-th strongest creature type will go to i-th slot
	{
		if(i.getNum() >= bestArmy.size())
		{
			if(destinationArmy->hasStackAtSlot(i))
			{
				auto creature = destinationArmy->getCreature(i);
				auto targetSlot = source->getSlotFor(creature);

				if(targetSlot.validSlot())
				{
					// remove unwanted creatures
					cb->mergeOrSwapStacks(destinationArmy, source, i, targetSlot);
				}
				else if(destinationArmy->getStack(i).getPower() < destinationArmy->getArmyStrength() / 100)
				{
					// dismiss creatures if the amount is small
					cb->dismissCreature(destinationArmy, i);
				}
			}

			continue;
		}

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
						auto weakest = nullkiller->armyManager->getWeakestCreature(bestArmy);
						
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
}

void AIGateway::pickBestArtifacts(const CGHeroInstance * h, const CGHeroInstance * other)
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
						allArtifacts.push_back(ArtifactLocation(h->id, p.first));
				}
			}
			for(auto slot : h->artifactsInBackpack)
				allArtifacts.push_back(ArtifactLocation(h->id, h->getArtPos(slot.artifact)));

			if(otherh)
			{
				for(auto p : otherh->artifactsWorn)
				{
					if(p.second.artifact)
						allArtifacts.push_back(ArtifactLocation(otherh->id, p.first));
				}
				for(auto slot : otherh->artifactsInBackpack)
					allArtifacts.push_back(ArtifactLocation(otherh->id, otherh->getArtPos(slot.artifact)));
			}
			//we give stuff to one hero or another, depending on giveStuffToFirstHero

			const CGHeroInstance * target = nullptr;
			if(giveStuffToFirstHero || !otherh)
				target = h;
			else
				target = otherh;

			for(auto location : allArtifacts)
			{
				if(location.artHolder == target->id && ArtifactUtils::isSlotEquipment(location.slot))
					continue; //don't reequip artifact we already wear

				if(location.slot == ArtifactPosition::MACH4) // don't attempt to move catapult
					continue;

				auto s = cb->getHero(location.artHolder)->getSlot(location.slot);
				if(!s || s->locked) //we can't move locks
					continue;
				auto artifact = s->artifact;
				if(!artifact)
					continue;
				//FIXME: why are the above possible to be null?

				bool emptySlotFound = false;
				for(auto slot : artifact->artType->getPossibleSlots().at(target->bearerType()))
				{
					if(target->isPositionFree(slot) && artifact->canBePutAt(target, slot, true)) //combined artifacts are not always allowed to move
					{
						ArtifactLocation destLocation(target->id, slot);
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
							//if that artifact is better than what we have, pick it
							if(compareArtifacts(artifact, otherSlot->artifact) && artifact->canBePutAt(target, slot, true)) //combined artifacts are not always allowed to move
							{
								ArtifactLocation destLocation(target->id, slot);
								cb->swapArtifacts(location, ArtifactLocation(target->id, target->getArtPos(otherSlot->artifact)));
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

void AIGateway::recruitCreatures(const CGDwelling * d, const CArmedInstance * recruiter)
{
	//now used only for visited dwellings / towns, not BuyArmy goal
	for(int i = 0; i < d->creatures.size(); i++)
	{
		if(!d->creatures[i].second.size())
			continue;

		int count = d->creatures[i].first;
		CreatureID creID = d->creatures[i].second.back();

		if(!recruiter->getSlotFor(creID).validSlot())
		{
			continue;
		}

		vstd::amin(count, cb->getResourceAmount() / creID.toCreature()->getFullRecruitCost());
		if(count > 0)
			cb->recruitCreatures(d, recruiter, creID, count, i);
	}
}

void AIGateway::battleStart(const BattleID & battleID, const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side, bool replayAllowed)
{
	NET_EVENT_HANDLER;
	assert(!playerID.isValidPlayer() || status.getBattle() == UPCOMING_BATTLE);
	status.setBattle(ONGOING_BATTLE);
	const CGObjectInstance * presumedEnemy = vstd::backOrNull(cb->getVisitableObjs(tile)); //may be nullptr in some very are cases -> eg. visited monolith and fighting with an enemy at the FoW covered exit
	battlename = boost::str(boost::format("Starting battle of %s attacking %s at %s") % (hero1 ? hero1->getNameTranslated() : "a army") % (presumedEnemy ? presumedEnemy->getObjectName() : "unknown enemy") % tile.toString());
	CAdventureAI::battleStart(battleID, army1, army2, tile, hero1, hero2, side, replayAllowed);
}

void AIGateway::battleEnd(const BattleID & battleID, const BattleResult * br, QueryID queryID)
{
	NET_EVENT_HANDLER;
	assert(status.getBattle() == ONGOING_BATTLE);
	status.setBattle(ENDING_BATTLE);
	bool won = br->winner == myCb->getBattle(battleID)->battleGetMySide();
	logAi->debug("Player %d (%s): I %s the %s!", playerID, playerID.toString(), (won ? "won" : "lost"), battlename);
	battlename.clear();

	if (queryID != QueryID::NONE)
	{
		status.addQuery(queryID, "Combat result dialog");
		const int confirmAction = 0;
		requestActionASAP([=]()
		{
			answerQuery(queryID, confirmAction);
		});
	}
	CAdventureAI::battleEnd(battleID, br, queryID);
}

void AIGateway::waitTillFree()
{
	auto unlock = vstd::makeUnlockSharedGuard(CGameState::mutex);
	status.waitTillFree();
}

void AIGateway::retrieveVisitableObjs()
{
	foreach_tile_pos([&](const int3 & pos)
	{
		for(const CGObjectInstance * obj : myCb->getVisitableObjs(pos, false))
		{
			addVisitableObj(obj);
		}
	});
}

std::vector<const CGObjectInstance *> AIGateway::getFlaggedObjects() const
{
	std::vector<const CGObjectInstance *> ret;
	for(const CGObjectInstance * obj : nullkiller->memory->visitableObjs)
	{
		if(obj->tempOwner == playerID)
			ret.push_back(obj);
	}
	return ret;
}

void AIGateway::addVisitableObj(const CGObjectInstance * obj)
{
	if(obj->ID == Obj::EVENT)
		return;

	nullkiller->memory->addVisitableObject(obj);

	if(obj->ID == Obj::HERO && cb->getPlayerRelations(obj->tempOwner, playerID) == PlayerRelations::ENEMIES)
	{
		nullkiller->dangerHitMap->reset();
	}
}

bool AIGateway::moveHeroToTile(int3 dst, HeroPtr h)
{
	if(h->inTownGarrison && h->visitedTown)
	{
		cb->swapGarrisonHero(h->visitedTown);
		moveCreaturesToHero(h->visitedTown);
	}

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
		//assert(cb->getVisitableObjs(dst).size() > 1); //there's no point in revisiting tile where there is no visitable object
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
			return true;
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
				nullkiller->memory->markObjectVisited(destTeleportObj); //FIXME: Monoliths are not correctly visited

				continue;
			}

			//stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
			if(path.nodes[i - 1].turns)
			{
				//blockedHeroes.insert(h); //to avoid attempts of moving heroes with very little MPs
				return false;
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

		if(path.nodes[0].action == EPathNodeAction::BLOCKING_VISIT || path.nodes[0].action == EPathNodeAction::BATTLE)
		{
			// when we take resource we do not reach its position. We even might not move
			// also guarded town is not get visited automatically after capturing
			ret = h && i == 0;
		}
	}
	if(h)
	{
		if(auto visitedObject = vstd::frontOrNull(cb->getVisitableObjs(h->visitablePos()))) //we stand on something interesting
		{
			if(visitedObject != *h)
			{
				performObjectInteraction(visitedObject, h);
				ret = true;
			}
		}
	}
	if(h) //we could have lost hero after last move
	{
		ret = ret || (dst == h->visitablePos());

		if(startHpos == h->visitablePos() && !ret) //we didn't move and didn't reach the target
		{
			throw cannotFulfillGoalException("Invalid path found!");
		}

		logAi->debug("Hero %s moved from %s to %s. Returning %d.", h->getNameTranslated(), startHpos.toString(), h->visitablePos().toString(), ret);
	}
	return ret;
}

void AIGateway::buildStructure(const CGTownInstance * t, BuildingID building)
{
	auto name = t->town->buildings.at(building)->getNameTranslated();
	logAi->debug("Player %d will build %s in town of %s at %s", ai->playerID, name, t->getNameTranslated(), t->pos.toString());
	cb->buildBuilding(t, building); //just do this;
}

void AIGateway::tryRealize(Goals::DigAtTile & g)
{
	assert(g.hero->visitablePos() == g.tile); //surely we want to crash here?
	if(g.hero->diggingStatus() == EDiggingStatus::CAN_DIG)
	{
		cb->dig(g.hero.get());
	}
	else
	{
		throw cannotFulfillGoalException("A hero can't dig!\n");
	}
}

void AIGateway::tryRealize(Goals::Trade & g) //trade
{
	if(cb->getResourceAmount(GameResID(g.resID)) >= g.value) //goal is already fulfilled. Why we need this check, anyway?
		throw goalFulfilledException(sptr(g));

	int accquiredResources = 0;
	if(const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(g.objid), false))
	{
		if(const IMarket * m = IMarket::castFrom(obj, false))
		{
			auto freeRes = cb->getResourceAmount(); //trade only resources which are not reserved
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
					cb->trade(m, EMarketMode::RESOURCE_RESOURCE, res, GameResID(g.resID), toGive);
					accquiredResources = static_cast<int>(toGet * (it->resVal / toGive));
					logAi->debug("Traded %d of %s for %d of %s at %s", toGive, res, accquiredResources, g.resID, obj->getObjectName());
				}
				if (cb->getResourceAmount(GameResID(g.resID)))
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

void AIGateway::endTurn()
{
	logAi->info("Player %d (%s) ends turn", playerID, playerID.toString());
	if(!status.haveTurn())
	{
		logAi->error("Not having turn at the end of turn???");
	}

	logAi->debug("Resources at the end of turn: %s", cb->getResourceAmount().toString());

	if(cb->getPlayerStatus(playerID) != EPlayerStatus::INGAME)
	{
		logAi->info("Ending turn is not needed because we already lost");
		return;
	}

	do
	{
		cb->endTurn();
	}
	while(status.haveTurn()); //for some reasons, our request may fail -> stop requesting end of turn only after we've received a confirmation that it's over

	logGlobal->info("Player %d (%s) ended turn", playerID, playerID.toString());
}

void AIGateway::buildArmyIn(const CGTownInstance * t)
{
	makePossibleUpgrades(t->visitingHero);
	makePossibleUpgrades(t);
	recruitCreatures(t, t->getUpperArmy());
	moveCreaturesToHero(t);
}

void AIGateway::finish()
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

void AIGateway::requestActionASAP(std::function<void()> whatToDo)
{
	boost::thread newThread([this, whatToDo]()
	{
		setThreadName("AIGateway::requestActionASAP::whatToDo");
		SET_GLOBAL_STATE(this);
		boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);
		whatToDo();
	});

	newThread.detach();
}

void AIGateway::lostHero(HeroPtr h)
{
	logAi->debug("I lost my hero %s. It's best to forget and move on.", h.name);
}

void AIGateway::answerQuery(QueryID queryID, int selection)
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

void AIGateway::requestSent(const CPackForServer * pack, int requestID)
{
	//BNLOG("I have sent request of type %s", typeid(*pack).name());
	if(auto reply = dynamic_cast<const QueryReply *>(pack))
	{
		status.attemptedAnsweringQuery(reply->qid, requestID);
	}
}

std::string AIGateway::getBattleAIName() const
{
	if(settings["server"]["enemyAI"].getType() == JsonNode::JsonType::DATA_STRING)
		return settings["server"]["enemyAI"].String();
	else
		return "BattleAI";
}

void AIGateway::validateObject(const CGObjectInstance * obj)
{
	validateObject(obj->id);
}

void AIGateway::validateObject(ObjectIdRef obj)
{
	if(!obj)
	{
		nullkiller->memory->removeFromMemory(obj);
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
		cv.wait_for(lock, boost::chrono::milliseconds(10));
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

}

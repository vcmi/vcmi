#include "StdInc.h"
#include "windows/CAdvmapInterface.h"
#include "battle/CBattleInterface.h"
#include "battle/CBattleInterfaceClasses.h"
#include "../CCallback.h"
#include "windows/CCastleInterface.h"
#include "gui/CCursorHandler.h"
#include "windows/CKingdomInterface.h"
#include "CGameInfo.h"
#include "windows/CHeroWindow.h"
#include "windows/CCreatureWindow.h"
#include "windows/CQuestLog.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
#include "gui/SDL_Extensions.h"
#include "widgets/CComponent.h"
#include "windows/CTradeWindow.h"
#include "../lib/CConfigHandler.h"
#include "battle/CCreatureAnimation.h"
#include "Graphics.h"
#include "windows/GUIClasses.h"
#include "../lib/CArtHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/Connection.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/mapObjects/CObjectClassesHandler.h" // For displaying correct UI when interacting with objects
#include "../lib/BattleState.h"
#include "../lib/JsonNode.h"
#include "CMusicHandler.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "../lib/mapping/CMap.h"
#include "../lib/VCMIDirs.h"
#include "mapHandler.h"
#include "../lib/CStopWatch.h"
#include "../lib/StartInfo.h"
#include "../lib/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"
#include "windows/InfoWindows.h"
#include "../lib/UnlockGuard.h"
#include <SDL.h>

/*
 * CPlayerInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


// The macro below is used to mark functions that are called by client when game state changes.
// They all assume that CPlayerInterface::pim mutex is locked.
#define EVENT_HANDLER_CALLED_BY_CLIENT

// The macro marks functions that are run on a new thread by client.
// They do not own any mutexes intiially.
#define THREAD_CREATED_BY_CLIENT

#define RETURN_IF_QUICK_COMBAT		\
	if(isAutoFightOn && !battleInt)	\
		return;

#define BATTLE_EVENT_POSSIBLE_RETURN\
	if(LOCPLINT != this)			\
		return;						\
	RETURN_IF_QUICK_COMBAT

using namespace CSDL_Ext;

void processCommand(const std::string &message, CClient *&client);

extern std::queue<SDL_Event> events;
extern boost::mutex eventsM;
boost::recursive_mutex * CPlayerInterface::pim = new boost::recursive_mutex;

CPlayerInterface * LOCPLINT;

CBattleInterface * CPlayerInterface::battleInt;

enum  EMoveState {STOP_MOVE, WAITING_MOVE, CONTINUE_MOVE, DURING_MOVE};
CondSh<EMoveState> stillMoveHero; //used during hero movement

int CPlayerInterface::howManyPeople = 0;

static bool objectBlitOrderSorter(const TerrainTileObject  & a, const TerrainTileObject & b)
{
	return CMapHandler::compareObjectBlitOrder(a.obj, b.obj);
}

CPlayerInterface::CPlayerInterface(PlayerColor Player)
{
	logGlobal->traceStream() << "\tHuman player interface for player " << Player << " being constructed";
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);
	observerInDuelMode = false;
	howManyPeople++;
	GH.defActionsDef = 0;
	LOCPLINT = this;
	curAction = nullptr;
	playerID=Player;
	human=true;
	currentSelection = nullptr;
	castleInt = nullptr;
	battleInt = nullptr;
	//pim = new boost::recursive_mutex;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	cingconsole = new CInGameConsole;
	GH.terminate_cond.set(false);
	firstCall = 1; //if loading will be overwritten in serialize
	autosaveCount = 0;
	isAutoFightOn = false;

	duringMovement = false;
	ignoreEvents = false;
}

CPlayerInterface::~CPlayerInterface()
{
	logGlobal->traceStream() << "\tHuman player interface for player " << playerID << " being destructed";
	//howManyPeople--;
	//delete pim;
	//vstd::clear_pointer(pim);
	delete showingDialog;
	delete cingconsole;
	if(LOCPLINT == this)
		LOCPLINT = nullptr;
}
void CPlayerInterface::init(std::shared_ptr<CCallback> CB)
{
	cb = CB;
	if(observerInDuelMode)
		return;

	if(!towns.size() && !wanderingHeroes.size())
		initializeHeroTownList();

	// always recreate advmap interface to avoid possible memory-corruption bugs
	if(adventureInt)
		delete adventureInt;
	adventureInt = new CAdvMapInt();
}
void CPlayerInterface::yourTurn()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	{
		boost::unique_lock<boost::mutex> lock(eventsM); //block handling events until interface is ready

		LOCPLINT = this;
		GH.curInt = this;
		adventureInt->selection = nullptr;

		std::string prefix = "";
		if(settings["testing"]["enabled"].Bool())
		{
			prefix = settings["testing"]["prefix"].String();
		}

		if(firstCall)
		{
			if(howManyPeople == 1)
				adventureInt->setPlayer(playerID);

			autosaveCount = getLastIndex(prefix + "Autosave_");

			if(firstCall > 0) //new game, not loaded
			{
				int index = getLastIndex(prefix + "Newgame_");
				index %= SAVES_COUNT;
				cb->save("Saves/" + prefix + "Newgame_Autosave_" + boost::lexical_cast<std::string>(index + 1));
			}
			firstCall = 0;
		}
		else
		{
			LOCPLINT->cb->save("Saves/" + prefix + "Autosave_" + boost::lexical_cast<std::string>(autosaveCount++ + 1));
			autosaveCount %= 5;
		}

		if(adventureInt->player != playerID)
			adventureInt->setPlayer(playerID);

		if(howManyPeople > 1) //hot seat message
		{
			adventureInt->startHotSeatWait(playerID);

			makingTurn = true;
			std::string msg = CGI->generaltexth->allTexts[13];
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<CComponent*> cmp;
			cmp.push_back(new CComponent(CComponent::flag, playerID.getNum(), 0));
			showInfoDialog(msg, cmp);
		}
		else
		{
			makingTurn = true;
			adventureInt->startTurn();
		}
	}

	acceptTurn();
}

STRONG_INLINE void subRect(const int & x, const int & y, const int & z, const SDL_Rect & r, const ObjectInstanceID & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(auto & elem : hlp.objects)
		if(elem.obj && elem.obj->id == hid)
		{
			elem.rect = r;
			return;
		}
}

STRONG_INLINE void delObjRect(const int & x, const int & y, const int & z, const ObjectInstanceID & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].obj && hlp.objects[h].obj->id == hid)
		{
			hlp.objects.erase(hlp.objects.begin()+h);
			return;
		}
}
void CPlayerInterface::heroMoved(const TryMoveHero & details)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	if(LOCPLINT != this)
		return;

	const CGHeroInstance * hero = cb->getHero(details.id); //object representing this hero
	int3 hp = details.start;

	if(!hero)
	{
		//AI hero left the visible area (we can't obtain info)
		//TODO very evil workaround -> retrieve pointer to hero so we could animate it
		// TODO -> we should not need full CGHeroInstance structure to display animation or it should not be handled by playerint (but by the client itself)
		const TerrainTile2 &tile = CGI->mh->ttiles[hp.x-1][hp.y][hp.z];
		for(auto & elem : tile.objects)
			if(elem.obj && elem.obj->id == details.id)
				hero = dynamic_cast<const CGHeroInstance *>(elem.obj);

		if(!hero) //still nothing...
			return;
	}

	bool directlyAttackingCreature =
		details.attackedFrom
		&& adventureInt->terrain.currentPath					//in case if movement has been canceled in the meantime and path was already erased
		&& adventureInt->terrain.currentPath->nodes.size() == 3;//FIXME should be 2 but works nevertheless...

	if(makingTurn  &&  hero->tempOwner == playerID) //we are moving our hero - we may need to update assigned path
	{
		//We may need to change music - select new track, music handler will change it if needed
		CCS->musich->playMusicFromSet("terrain", LOCPLINT->cb->getTile(hero->visitablePos())->terType, true);

		if(details.result == TryMoveHero::TELEPORTATION)
		{
			if(adventureInt->terrain.currentPath)
			{
				assert(adventureInt->terrain.currentPath->nodes.size() >= 2);
				std::vector<CGPathNode>::const_iterator nodesIt = adventureInt->terrain.currentPath->nodes.end() - 1;

				if((nodesIt)->coord == CGHeroInstance::convertPosition(details.start, false)
					&& (nodesIt-1)->coord == CGHeroInstance::convertPosition(details.end, false))
				{
					//path was between entrance and exit of teleport -> OK, erase node as usual
					removeLastNodeFromPath(hero);
				}
				else
				{
					//teleport was not along current path, it'll now be invalid (hero is somewhere else)
					eraseCurrentPathOf(hero);

				}
			}
			adventureInt->centerOn(hero, true); //actualizing screen pos
			adventureInt->minimap.redraw();
			adventureInt->heroList.update(hero);
			return;	//teleport - no fancy moving animation
					//TODO: smooth disappear / appear effect
		}

		if (hero->pos != details.end //hero didn't change tile but visit succeeded
			|| directlyAttackingCreature) // or creature was attacked from endangering tile.
		{
			eraseCurrentPathOf(hero, false);
		}
		else if(adventureInt->terrain.currentPath  &&  hero->pos == details.end) //&& hero is moving
		{
			if(details.start != details.end) //so we don't touch path when revisiting with spacebar
				removeLastNodeFromPath(hero);
		}
	}

	if (details.result != TryMoveHero::SUCCESS) //hero failed to move
	{
		hero->isStanding = true;
		stillMoveHero.setn(STOP_MOVE);
		GH.totalRedraw();
		adventureInt->heroList.update(hero);
		return;
	}

	ui32 speed;
	if (makingTurn) // our turn, our hero moves
		speed = settings["adventure"]["heroSpeed"].Float();
	else
		speed = settings["adventure"]["enemySpeed"].Float();

	if (speed == 0)
	{
		//FIXME: is this a proper solution?
		CGI->mh->hideObject(hero);
		CGI->mh->printObject(hero);
		return; // no animation
	}


	adventureInt->centerOn(hero); //actualizing screen pos
	adventureInt->minimap.redraw();
	adventureInt->heroList.redraw();

	initMovement(details, hero, hp);

	//first initializing done
	GH.mainFPSmng->framerateDelay(); // after first move

	//main moving
	for(int i=1; i<32; i+=2*speed)
	{
		movementPxStep(details, i, hp, hero);
		adventureInt->updateScreen = true;
		adventureInt->show(screen);
		{
			//evil returns here ...
			//todo: get rid of it
			logGlobal->traceStream() << "before [un]locks in " << __FUNCTION__;
			auto unlockPim = vstd::makeUnlockGuard(*pim); //let frame to be rendered
			GH.mainFPSmng->framerateDelay(); //for animation purposes
			logGlobal->traceStream() << "after [un]locks in " << __FUNCTION__;
		}
		//CSDL_Ext::update(screen);

	} //for(int i=1; i<32; i+=4)
	//main moving done

	//finishing move
	finishMovement(details, hp, hero);
	hero->isStanding = true;

	//move finished
	adventureInt->minimap.redraw();
	adventureInt->heroList.update(hero);

	//check if user cancelled movement
	{
		boost::unique_lock<boost::mutex> un(eventsM);
		while(!events.empty())
		{
			SDL_Event ev = events.front();
			events.pop();
			switch(ev.type)
			{
			case SDL_MOUSEBUTTONDOWN:
				stillMoveHero.setn(STOP_MOVE);
				break;
			case SDL_KEYDOWN:
				if(ev.key.keysym.sym < SDLK_F1  ||  ev.key.keysym.sym > SDLK_F15)
					stillMoveHero.setn(STOP_MOVE);
				break;
			}
		}
	}

	if(stillMoveHero.get() == WAITING_MOVE)
		stillMoveHero.setn(DURING_MOVE);

	// Hero attacked creature directly, set direction to face it.
	if (directlyAttackingCreature) {
		// Get direction to attacker.
		int3 posOffset = *details.attackedFrom - details.end + int3(2, 1, 0);
		static const ui8 dirLookup[3][3] = {
			{ 1, 2, 3 },
			{ 8, 0, 4 },
			{ 7, 6, 5 }
		};
		// FIXME: Avoid const_cast, make moveDir mutable in some other way?
		const_cast<CGHeroInstance *>(hero)->moveDir = dirLookup[posOffset.y][posOffset.x];
	}
}
void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	LOG_TRACE_PARAMS(logGlobal, "Hero %s killed handler for player %s", hero->name % playerID);

	const CArmedInstance *newSelection = nullptr;
	if (makingTurn)
	{
		//find new object for selection: either hero
		int next = adventureInt->getNextHeroIndex(vstd::find_pos(wanderingHeroes, hero));
		if (next >= 0)
			newSelection = wanderingHeroes[next];

		//or town
		if (!newSelection || newSelection == hero)
		{
			if (towns.empty())
				newSelection = nullptr;
			else
				newSelection = towns.front();
		}
	}

	wanderingHeroes -= hero;
	if(vstd::contains(paths, hero))
		paths.erase(hero);

	adventureInt->heroList.update(hero);
	if (makingTurn && newSelection)
		adventureInt->select(newSelection, true);
	else if(adventureInt->selection == hero)
		adventureInt->selection = nullptr;
}

void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	wanderingHeroes.push_back(hero);
	adventureInt->heroList.update(hero);
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	if (castleInt)
		castleInt->close();
	castleInt = new CCastleInterface(town);
	GH.pushInt(castleInt);
}

int3 CPlayerInterface::repairScreenPos(int3 pos)
{
	if(pos.x<-CGI->mh->frameW)
		pos.x = -CGI->mh->frameW;
	if(pos.y<-CGI->mh->frameH)
		pos.y = -CGI->mh->frameH;
	if(pos.x>CGI->mh->sizes.x - adventureInt->terrain.tilesw + CGI->mh->frameW)
		pos.x = CGI->mh->sizes.x - adventureInt->terrain.tilesw + CGI->mh->frameW;
	if(pos.y>CGI->mh->sizes.y - adventureInt->terrain.tilesh + CGI->mh->frameH)
		pos.y = CGI->mh->sizes.y - adventureInt->terrain.tilesh + CGI->mh->frameH;
	return pos;
}
void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(which == 4)
	{
		if(CAltarWindow *ctw = dynamic_cast<CAltarWindow *>(GH.topInt()))
			ctw->setExpToLevel();
	}
	else if(which < GameConstants::PRIMARY_SKILLS) //no need to redraw infowin if this is experience (exp is treated as prim skill with id==4)
		updateInfo(hero);
}

void CPlayerInterface::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	CUniversityWindow* cuw = dynamic_cast<CUniversityWindow*>(GH.topInt());
	if(cuw) //university window is open
	{
		GH.totalRedraw();
	}
}

void CPlayerInterface::heroManaPointsChanged(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	updateInfo(hero);
	if(makingTurn && hero->tempOwner == playerID)
		adventureInt->heroList.update(hero);
}
void CPlayerInterface::heroMovePointsChanged(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(makingTurn && hero->tempOwner == playerID)
		adventureInt->heroList.update(hero);
}
void CPlayerInterface::receivedResource(int type, int val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(CMarketplaceWindow *mw = dynamic_cast<CMarketplaceWindow *>(GH.topInt()))
		mw->resourceChanged(type, val);

	GH.totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill>& skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->soundh->playSound(soundBase::heroNewLevel);

	CLevelWindow *lw = new CLevelWindow(hero,pskill,skills,
										[=](ui32 selection){ cb->selectionMade(selection, queryID); });
	GH.pushInt(lw);
}
void CPlayerInterface::commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->soundh->playSound(soundBase::heroNewLevel);

	GH.pushInt(new CStackWindow(commander, skills, [=](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	}));
}

void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	updateInfo(town);

	if(town->garrisonHero && vstd::contains(wanderingHeroes,town->garrisonHero)) //wandering hero moved to the garrison
	{
		CGI->mh->hideObject(town->garrisonHero);
		if (town->garrisonHero->tempOwner == playerID) // our hero
			wanderingHeroes -= town->garrisonHero;
	}

	if(town->visitingHero && !vstd::contains(wanderingHeroes,town->visitingHero)) //hero leaves garrison
	{
		CGI->mh->printObject(town->visitingHero);
		if (town->visitingHero->tempOwner == playerID) // our hero
			wanderingHeroes.push_back(town->visitingHero);
	}
	adventureInt->heroList.update();
	adventureInt->updateNextHero(nullptr);

	if(CCastleInterface *c = castleInt)
	{
		c->garr->selectSlot(nullptr);
		c->garr->setArmy(town->getUpperArmy(), 0);
		c->garr->setArmy(town->visitingHero, 1);
		c->garr->recreateSlots();
		c->heroes->update();
	}
	for(IShowActivatable *isa : GH.listInt)
	{
		CKingdomInterface *ki = dynamic_cast<CKingdomInterface*>(isa);
		if (ki)
		{
			ki->townChanged(town);
			ki->updateGarrisons();
		}
	}
	GH.totalRedraw();
}
void CPlayerInterface::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(hero->tempOwner != playerID )
		return;

	waitWhileDialog();
	openTownWindow(town);
}
void CPlayerInterface::garrisonsChanged(std::vector<const CGObjectInstance *> objs)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(auto object : objs)
		updateInfo(object);

	for(auto & elem : GH.listInt)
	{
		CGarrisonHolder *cgh = dynamic_cast<CGarrisonHolder*>(elem);
		if (cgh)
			cgh->updateGarrisons();

		if(CTradeWindow *cmw = dynamic_cast<CTradeWindow*>(elem))
		{
			if(vstd::contains(objs, cmw->hero))
				cmw->garrisonChanged();
		}
	}

	GH.totalRedraw();
}

void CPlayerInterface::garrisonChanged( const CGObjectInstance * obj)
{
	garrisonsChanged(std::vector<const CGObjectInstance *>(1, obj));
}

void CPlayerInterface::buildChanged(const CGTownInstance *town, BuildingID buildingID, int what) //what: 1 - built, 2 - demolished
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	switch (buildingID)
	{
	case BuildingID::FORT: case BuildingID::CITADEL: case BuildingID::CASTLE:
	case BuildingID::VILLAGE_HALL: case BuildingID::TOWN_HALL: case BuildingID::CITY_HALL: case BuildingID::CAPITOL:
	case BuildingID::RESOURCE_SILO:
		updateInfo(town);
		break;
	}

	if(!castleInt)
		return;
	if(castleInt->town!=town)
		return;
	switch(what)
	{
	case 1:
		CCS->soundh->playSound(soundBase::newBuilding);
		castleInt->addBuilding(buildingID);
		break;
	case 2:
		castleInt->removeBuilding(buildingID);
		break;
	}
	adventureInt->townList.update(town);
	castleInt->townlist->update(town);
}

void CPlayerInterface::battleStartBefore(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	//Don't wait for dialogs when we are non-active hot-seat player
	if(LOCPLINT == this)
		waitForAllDialogs();
}

void CPlayerInterface::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(settings["adventure"]["quickCombat"].Bool())
	{
		autofightingAI = CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String());
		autofightingAI->init(cb);
		autofightingAI->battleStart(army1, army2, int3(0,0,0), hero1, hero2, side);
		isAutoFightOn = true;
		cb->registerBattleInterface(autofightingAI);
		// Player shouldn't be able to move on adventure map if quick combat is going
		adventureInt->quickCombatLock();
	}

	//Don't wait for dialogs when we are non-active hot-seat player
	if(LOCPLINT == this)
		waitForAllDialogs();

	BATTLE_EVENT_POSSIBLE_RETURN;
}


void CPlayerInterface::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	for(auto & healedStack : healedStacks)
	{
		const CStack * healed = cb->battleGetStackByID(healedStack.first);
		if(battleInt->creAnims[healed->ID]->isDead())
		{
			//stack has been resurrected
			battleInt->creAnims[healed->ID]->setType(CCreatureAnim::HOLDING);
		}
	}

	if (lifeDrain)
	{
		const CStack *attacker = cb->battleGetStackByID(healedStacks[0].first, false);
		const CStack *defender = cb->battleGetStackByID(lifeDrainFrom, false);
		int textOff = 0;

		if (attacker)
		{
			battleInt->displayEffect(52, attacker->position); //TODO: transparency
			if (attacker->count > 1)
			{
				textOff += 1;
			}
			CCS->soundh->playSound(soundBase::DRAINLIF);

			//print info about life drain
			auto txt =  boost::format (CGI->generaltexth->allTexts[361 + textOff]) %  attacker->getCreature()->nameSing % healedStacks[0].second % defender->getCreature()->namePl;
			battleInt->console->addText(boost::to_string(txt));
		}
	}
	if (tentHeal)
	{
		std::string text = CGI->generaltexth->allTexts[414];
		boost::algorithm::replace_first(text, "%s", cb->battleGetStackByID(lifeDrainFrom, false)->getCreature()->nameSing);
		boost::algorithm::replace_first(text, "%s",	cb->battleGetStackByID(healedStacks[0].first, false)->getCreature()->nameSing);
		boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(healedStacks[0].second));
		battleInt->console->addText(text);
	}
}

void CPlayerInterface::battleNewStackAppeared(const CStack * stack)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newStack(stack);
}

void CPlayerInterface::battleObstaclesRemoved(const std::set<si32> & removedObstacles)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

// 	for(std::set<si32>::const_iterator it = removedObstacles.begin(); it != removedObstacles.end(); ++it)
// 	{
// 		for(std::map< int, CDefHandler * >::iterator itBat = battleInt->idToObstacle.begin(); itBat != battleInt->idToObstacle.end(); ++itBat)
// 		{
// 			if(itBat->first == *it) //remove this obstacle
// 			{
// 				battleInt->idToObstacle.erase(itBat);
// 				break;
// 			}
// 		}
// 	}
	//update accessible hexes
	battleInt->redrawBackgroundWithHexes(battleInt->activeStack);
}

void CPlayerInterface::battleCatapultAttacked(const CatapultAttack & ca)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackIsCatapulting(ca);
}

void CPlayerInterface::battleStacksRemoved(const BattleStacksRemoved & bsr)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	for(auto & elem : bsr.stackIDs) //for each removed stack
	{
		battleInt->stackRemoved(elem);
	}
}

void CPlayerInterface::battleNewRound(int round) //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRound(round);
}

void CPlayerInterface::actionStarted(const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	curAction = new BattleAction(action);
	battleInt->startAction(curAction);
}

void CPlayerInterface::actionFinished(const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->endAction(curAction);
	delete curAction;
	curAction = nullptr;
}

BattleAction CPlayerInterface::activeStack(const CStack * stack) //called when it's turn of that stack
{
	THREAD_CREATED_BY_CLIENT;
	logGlobal->traceStream() << "Awaiting command for " << stack->nodeName();
	auto stackId = stack->ID;
	auto stackName = stack->nodeName();
	if(autofightingAI)
	{
		if(isAutoFightOn)
		{
			auto ret = autofightingAI->activeStack(stack);
			if(isAutoFightOn)
			{
				return ret;
			}
		}

		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();
	}

	CBattleInterface *b = battleInt;

	if (b->givenCommand->get())
	{
		logGlobal->errorStream() << "Command buffer must be clean! (we don't want to use old command)";
		vstd::clear_pointer(b->givenCommand->data);
	}

	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);
		b->stackActivated(stack);
		//Regeneration & mana drain go there
	}
	//wait till BattleInterface sets its command
	boost::unique_lock<boost::mutex> lock(b->givenCommand->mx);
	while(!b->givenCommand->data)
	{
		b->givenCommand->cond.wait(lock);
		if(!battleInt) //battle ended while we were waiting for movement (eg. because of spell)
			throw boost::thread_interrupted(); //will shut the thread peacefully
	}

	//tidy up
	BattleAction ret = *(b->givenCommand->data);
	vstd::clear_pointer(b->givenCommand->data);

	if(ret.actionType == Battle::CANCEL)
	{
		if(stackId != ret.stackNumber)
			logGlobal->error("Not current active stack action canceled");
		logGlobal->traceStream() << "Canceled command for " << stackName;
	}
	else
		logGlobal->traceStream() << "Giving command for " << stackName;

	return ret;
}

void CPlayerInterface::battleEnd(const BattleResult *br)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(isAutoFightOn)
	{
		isAutoFightOn = false;
		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();
		adventureInt->quickCombatUnlock();

		if(!battleInt)
		{
			SDL_Rect temp_rect = genRect(561, 470, (screen->w - 800)/2 + 165, (screen->h - 600)/2 + 19);
			auto   resWindow = new CBattleResultWindow(*br, temp_rect, *this);
			GH.pushInt(resWindow);
			// #1490 - during AI turn when quick combat is on, we need to display the message and wait for user to close it.
			// Otherwise NewTurn causes freeze.
			waitWhileDialog();
			return;
		}
	}

	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->battleFinished(*br);
}

void CPlayerInterface::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackMoved(stack, dest, distance);
}
void CPlayerInterface::battleSpellCast( const BattleSpellCast *sc )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->spellCast(sc);
}
void CPlayerInterface::battleStacksEffectsSet( const SetStackEffect & sse )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->battleStacksEffectsSet(sse);
}
void CPlayerInterface::battleTriggerEffect (const BattleTriggerEffect & bte)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//TODO why is this different (no return on LOPLINT != this) ?

	RETURN_IF_QUICK_COMBAT;
	battleInt->battleTriggerEffect(bte);
}
void CPlayerInterface::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	std::vector<StackAttackedInfo> arg;
	for(auto & elem : bsa)
	{
		const CStack *defender = cb->battleGetStackByID(elem.stackAttacked, false);
		const CStack *attacker = cb->battleGetStackByID(elem.attackerID, false);
		if(elem.isEffect())
		{
			if (defender && !elem.isSecondary())
				battleInt->displayEffect(elem.effect, defender->position);
		}
		if(elem.isSpell())
		{
			if (defender)
				battleInt->displaySpellEffect(elem.spellID, defender->position);
		}
		//FIXME: why action is deleted during enchanter cast?
		bool remoteAttack = false;

		if (LOCPLINT->curAction)
			remoteAttack |= LOCPLINT->curAction->actionType != Battle::WALK_AND_ATTACK;

		StackAttackedInfo to_put = {defender, elem.damageAmount, elem.killedAmount, attacker, remoteAttack, elem.killed(), elem.willRebirth(), elem.cloneKilled()};
		arg.push_back(to_put);
	}

	battleInt->stacksAreAttacked(arg);
}
void CPlayerInterface::battleAttack(const BattleAttack *ba)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	assert(curAction);
	if(ba->lucky()) //lucky hit
	{
		const CStack *stack = cb->battleGetStackByID(ba->stackAttacking);
		std::string hlp = CGI->generaltexth->allTexts[45];
		boost::algorithm::replace_first(hlp,"%s", (stack->count != 1) ? stack->getCreature()->namePl.c_str() : stack->getCreature()->nameSing.c_str());
		battleInt->console->addText(hlp);
		battleInt->displayEffect(18, stack->position);
		CCS->soundh->playSound(soundBase::GOODLUCK);
	}
	if(ba->unlucky()) //unlucky hit
	{
		const CStack *stack = cb->battleGetStackByID(ba->stackAttacking);
		std::string hlp = CGI->generaltexth->allTexts[44];
		boost::algorithm::replace_first(hlp,"%s", (stack->count != 1) ? stack->getCreature()->namePl.c_str() : stack->getCreature()->nameSing.c_str());
		battleInt->console->addText(hlp);
		battleInt->displayEffect(48, stack->position);
		CCS->soundh->playSound(soundBase::BADLUCK);
	}
	if (ba->deathBlow())
	{
		const CStack *stack = cb->battleGetStackByID(ba->stackAttacking);
		std::string hlp = CGI->generaltexth->allTexts[(stack->count != 1) ? 366 : 365];
		boost::algorithm::replace_first(hlp,"%s", (stack->count != 1) ? stack->getCreature()->namePl.c_str() : stack->getCreature()->nameSing.c_str());
		battleInt->console->addText(hlp);
		for (auto & elem : ba->bsa)
		{
			const CStack * attacked = cb->battleGetStackByID(elem.stackAttacked);
			battleInt->displayEffect(73, attacked->position);
		}
		CCS->soundh->playSound(soundBase::deathBlow);

	}
	battleInt->waitForAnims();

	const CStack * attacker = cb->battleGetStackByID(ba->stackAttacking);

	if(ba->shot())
	{
		for(auto & elem : ba->bsa)
		{
			if (!elem.isSecondary()) //display projectile only for primary target
			{
				const CStack * attacked = cb->battleGetStackByID(elem.stackAttacked);
				battleInt->stackAttacking(attacker, attacked->position, attacked, true);
			}
		}
	}
	else
	{
		int shift = 0;
		if(ba->counter() && BattleHex::mutualPosition(curAction->destinationTile, attacker->position) < 0)
		{
			int distp = BattleHex::getDistance(curAction->destinationTile + 1, attacker->position);
			int distm = BattleHex::getDistance(curAction->destinationTile - 1, attacker->position);

			if( distp < distm )
				shift = 1;
			else
				shift = -1;
		}
		const CStack * attacked = cb->battleGetStackByID(ba->bsa.begin()->stackAttacked);
		battleInt->stackAttacking( attacker, ba->counter() ? curAction->destinationTile + shift : curAction->additionalInfo, attacked, false);
	}

	//battleInt->waitForAnims(); //FIXME: freeze

	if(ba->spellLike())
	{
		//display hit animation
		SpellID spellID = ba->spellID;
		battleInt->displaySpellHit(spellID,curAction->destinationTile);
	}
}
void CPlayerInterface::battleObstaclePlaced(const CObstacleInstance &obstacle)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->obstaclePlaced(obstacle);
}

void CPlayerInterface::yourTacticPhase(int distance)
{
	THREAD_CREATED_BY_CLIENT;
	while(battleInt && battleInt->tacticsMode)
		boost::this_thread::sleep(boost::posix_time::millisec(1));
}

void CPlayerInterface::showComp(const Component &comp, std::string message)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog(); //Fix for mantis #98

	CCS->soundh->playSoundFromSet(CCS->soundh->pickupSounds);
	adventureInt->infoBar.showComponent(comp, message);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (settings["session"]["autoSkip"].Bool() && !LOCPLINT->shiftPressed())
	{
		return;
	}
	std::vector<CComponent*> intComps;
	for(auto & component : components)
		intComps.push_back(new CComponent(*component));
	showInfoDialog(text,intComps,soundID);

}

void CPlayerInterface::showInfoDialog(const std::string &text, CComponent * component)
{
	std::vector<CComponent*> intComps;
	intComps.push_back(component);

	showInfoDialog(text, intComps, soundBase::sound_todo, true);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<CComponent*> & components, int soundID, bool delComps)
{
	LOG_TRACE_PARAMS(logGlobal, "player=%s, text=%s, is LOCPLINT=%d", playerID % text % (this==LOCPLINT));
	waitWhileDialog();

	if (settings["session"]["autoSkip"].Bool() && !LOCPLINT->shiftPressed())
	{
		return;
	}
	CInfoWindow *temp = CInfoWindow::create(text, playerID, &components);
	temp->setDelComps(delComps);
	if(makingTurn && GH.listInt.size() && LOCPLINT == this)
	{
		CCS->soundh->playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->set(true);
		stopMovement(); // interrupt movement to show dialog
		GH.pushInt(temp);
	}
	else
	{
		dialogs.push_back(temp);
	}
}

void CPlayerInterface::showInfoDialogAndWait(std::vector<Component> & components, const MetaString & text)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	std::vector<Component*> comps;
	for(auto & elem : components)
	{
		comps.push_back(&elem);
	}
	std::string str;
	text.toString(str);

	showInfoDialog(str,comps, 0);
	waitWhileDialog();
}

void CPlayerInterface::showYesNoDialog(const std::string &text, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps, const std::vector<CComponent*> & components)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	stopMovement();
	LOCPLINT->showingDialog->setn(true);
	CInfoWindow::showYesNoDialog(text, &components, onYes, onNo, DelComps, playerID);
}

void CPlayerInterface::showOkDialog(std::vector<Component> & components, const MetaString & text, const std::function<void()> & onOk)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	std::vector<Component*> comps;
	for(auto & elem : components)
	{
		comps.push_back(&elem);
	}
	std::string str;
	text.toString(str);

	stopMovement();
	showingDialog->setn(true);

	std::vector<CComponent*> intComps;
	for(auto & component : comps)
		intComps.push_back(new CComponent(*component));
	CInfoWindow::showOkDialog(str, &intComps, onOk, true, playerID);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, QueryID askID, int soundID, bool selection, bool cancel )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	stopMovement();
	CCS->soundh->playSound(static_cast<soundBase::soundID>(soundID));

	if(!selection && cancel) //simple yes/no dialog
	{
		std::vector<CComponent*> intComps;
		for(auto & component : components)
			intComps.push_back(new CComponent(component)); //will be deleted by close in window

		showYesNoDialog(text, [=]{ cb->selectionMade(1, askID); }, [=]{ cb->selectionMade(0, askID); }, true, intComps);
	}
	else if(selection)
	{
		std::vector<CSelectableComponent*> intComps;
		for(auto & component : components)
			intComps.push_back(new CSelectableComponent(component)); //will be deleted by CSelWindow::close

		std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
		pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
		if(cancel)
		{
			pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
		}

		int charperline = 35;
		if (pom.size() > 1)
			charperline = 50;
		auto   temp = new CSelWindow(text, playerID, charperline, intComps, pom, askID);
		GH.pushInt(temp);
		intComps[0]->clickLeft(true, false);
	}

}

void CPlayerInterface::showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	int choosenExit = -1;
	auto neededExit = std::make_pair(destinationTeleport, destinationTeleportPos);
	if(destinationTeleport != ObjectInstanceID() && vstd::contains(exits, neededExit))
		choosenExit = vstd::find_pos(exits, neededExit);

	cb->selectionMade(choosenExit, askID);
}

void CPlayerInterface::tileRevealed(const std::unordered_set<int3, ShashInt3> &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//FIXME: wait for dialog? Magi hut/eye would benefit from this but may break other areas
	for(auto & po : pos)
		adventureInt->minimap.showTile(po);
	if(!pos.empty())
		GH.totalRedraw();
}

void CPlayerInterface::tileHidden(const std::unordered_set<int3, ShashInt3> &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for(auto & po : pos)
		adventureInt->minimap.hideTile(po);
	if(!pos.empty())
		GH.totalRedraw();
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	GH.pushInt(new CHeroWindow(hero));
}
/*
void CPlayerInterface::heroArtifactSetChanged(const CGHeroInstance*hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(adventureInt->heroWindow->curHero && adventureInt->heroWindow->curHero->id == hero->id) //hero window is opened
	{
		adventureInt->heroWindow->deactivate();
		adventureInt->heroWindow->setHero(hero);
		adventureInt->heroWindow->activate();
	}
	else if(CExchangeWindow* cew = dynamic_cast<CExchangeWindow*>(GH.topInt())) //exchange window is open
	{
		cew->deactivate();
		for(int g=0; g<ARRAY_COUNT(cew->heroInst); ++g)
		{
			if(cew->heroInst[g]->id == hero->id)
			{
				cew->heroInst[g] = hero;
				cew->artifs[g]->updateState = true;
				cew->artifs[g]->setHero(hero);
				cew->artifs[g]->updateState = false;
			}
		}
		cew->prepareBackground();
		cew->activate();
	}
	else if(CTradeWindow *caw = dynamic_cast<CTradeWindow*>(GH.topInt()))
	{
		if(caw->arts)
		{
			caw->deactivate();
			caw->arts->updateState = true;
			caw->arts->setHero(hero);
			caw->arts->updateState = false;
			caw->activate();
		}
	}

	updateInfo(hero);
}*/

void CPlayerInterface::availableCreaturesChanged( const CGDwelling *town )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(const CGTownInstance * townObj = dynamic_cast<const CGTownInstance*>(town))
	{
		CFortScreen *fs = dynamic_cast<CFortScreen*>(GH.topInt());
		if(fs)
			fs->creaturesChanged();

		for(IShowActivatable *isa : GH.listInt)
		{
			CKingdomInterface *ki = dynamic_cast<CKingdomInterface*>(isa);
			if (ki && townObj)
				ki->townChanged(townObj);
		}
	}
	else if(GH.listInt.size() && (town->ID == Obj::CREATURE_GENERATOR1
		||  town->ID == Obj::CREATURE_GENERATOR4  ||  town->ID == Obj::WAR_MACHINE_FACTORY))
	{
		CRecruitmentWindow *crw = dynamic_cast<CRecruitmentWindow*>(GH.topInt());
		if(crw && crw->dwelling == town)
			crw->availableCreaturesChanged();
	}
}

void CPlayerInterface::heroBonusChanged( const CGHeroInstance *hero, const Bonus &bonus, bool gain )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(bonus.type == Bonus::NONE)
		return;

	updateInfo(hero);
	if ((bonus.type == Bonus::FLYING_MOVEMENT || bonus.type == Bonus::WATER_WALKING) && !gain)
	{
		//recalculate paths because hero has lost bonus influencing pathfinding
		eraseCurrentPathOf(hero, false);
	}
}

template <typename Handler> void CPlayerInterface::serializeTempl( Handler &h, const int version )
{

	h & observerInDuelMode;

	h & wanderingHeroes & towns & sleepingHeroes;

	std::map<const CGHeroInstance *, int3> pathsMap; //hero -> dest
	if(h.saving)
	{
		for(auto &p : paths)
		{
			if(p.second.nodes.size())
				pathsMap[p.first] = p.second.endPos();
			else
				logGlobal->errorStream() << p.first->name << " has assigned an empty path! Ignoring it...";
		}
		h & pathsMap;
	}
	else
	{
		h & pathsMap;

		if(cb)
			for(auto &p : pathsMap)
			{
				CGPath path;
				cb->getPathsInfo(p.first)->getPath(path, p.second);
				paths[p.first] = path;
				logGlobal->traceStream() << boost::format("Restored path for hero %s leading to %s with %d nodes")
					% p.first->nodeName() % p.second % path.nodes.size();
			}
	}

	h & spellbookSettings;
}

void CPlayerInterface::saveGame( COSer & h, const int version )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	serializeTempl(h,version);
}

void CPlayerInterface::loadGame( CISer & h, const int version )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	serializeTempl(h,version);
	firstCall = -1;
}

void CPlayerInterface::moveHero( const CGHeroInstance *h, CGPath path )
{
	logGlobal->traceStream() << __FUNCTION__;
	if(!LOCPLINT->makingTurn)
		return;
	if (!h)
		return; //can't find hero

	//It shouldn't be possible to move hero with open dialog (or dialog waiting in bg)
	if(showingDialog->get() || !dialogs.empty())
		return;

	setMovementStatus(true);

	if (adventureInt && adventureInt->isHeroSleeping(h))
	{
		adventureInt->sleepWake->clickLeft(true, false);
		adventureInt->sleepWake->clickLeft(false, true);
		//could've just called
		//adventureInt->fsleepWake();
		//but no authentic button click/sound ;-)
	}

	boost::thread moveHeroTask(std::bind(&CPlayerInterface::doMoveHero,this,h,path));
}

bool CPlayerInterface::shiftPressed() const
{
	return isShiftKeyDown();
}

bool CPlayerInterface::altPressed() const
{
	return isAltKeyDown();
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onEnd = [=]{ cb->selectionMade(0, queryID); };

	if(stillMoveHero.get() == DURING_MOVE  && adventureInt->terrain.currentPath && adventureInt->terrain.currentPath->nodes.size() > 1) //to ignore calls on passing through garrisons
	{
		onEnd();
		return;
	}

	waitForAllDialogs();

	auto  cgw = new CGarrisonWindow(up,down,removableUnits);
	cgw->quit->addCallback(onEnd);
	GH.pushInt(cgw);
}

/**
 * Shows the dialog that appears when right-clicking an artifact that can be assembled
 * into a combinational one on an artifact screen. Does not require the combination of
 * artifacts to be legal.
 * @param artifactID ID of a constituent artifact.
 * @param assembleTo ID of artifact to assemble a constituent into, not used when assemble
 * is false.
 * @param assemble True if the artifact is to be assembled, false if it is to be disassembled.
 */
void CPlayerInterface::showArtifactAssemblyDialog (ui32 artifactID, ui32 assembleTo, bool assemble, CFunctionList<bool()> onYes, CFunctionList<bool()> onNo)
{
	const CArtifact &artifact = *CGI->arth->artifacts[artifactID];
	std::string text = artifact.Description();
	text += "\n\n";
	std::vector<CComponent*> scs;

	if (assemble) {
		const CArtifact &assembledArtifact = *CGI->arth->artifacts[assembleTo];

		// You possess all of the components to...
		text += boost::str(boost::format(CGI->generaltexth->allTexts[732]) % assembledArtifact.Name());

		// Picture of assembled artifact at bottom.
		auto   sc = new CComponent(CComponent::artifact, assembledArtifact.id, 0);
		//sc->description = assembledArtifact.Description();
		//sc->subtitle = assembledArtifact.Name();
		scs.push_back(sc);
	} else {
		// Do you wish to disassemble this artifact?
		text += CGI->generaltexth->allTexts[733];
	}

	showYesNoDialog(text, onYes, onNo, true, scs);
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(pa->packType == typeList.getTypeID<MoveHero>()  &&  stillMoveHero.get() == DURING_MOVE
	   && destinationTeleport == ObjectInstanceID())
		stillMoveHero.setn(CONTINUE_MOVE);

	if(destinationTeleport != ObjectInstanceID()
	   && pa->packType == typeList.getTypeID<QueryReply>()
	   && stillMoveHero.get() == DURING_MOVE)
	{ // After teleportation via CGTeleport object is finished
		destinationTeleport = ObjectInstanceID();
		destinationTeleportPos = int3(-1);
		stillMoveHero.setn(CONTINUE_MOVE);
	}
}

void CPlayerInterface::heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.pushInt(new CExchangeWindow(hero1, hero2, query));
}

void CPlayerInterface::objectPropertyChanged(const SetObjectProperty * sop)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//redraw minimap if owner changed
	if(sop->what == ObjProperty::OWNER)
	{
		const CGObjectInstance * obj = cb->getObj(sop->id);
		std::set<int3> pos = obj->getBlockedPos();
		for(auto & po : pos)
		{
			if(cb->isVisible(po))
				adventureInt->minimap.showTile(po);
		}

		if(obj->ID == Obj::TOWN)
		{
			if(obj->tempOwner == playerID)
				towns.push_back(static_cast<const CGTownInstance *>(obj));
			else
				towns -= obj;
			adventureInt->townList.update();
		}

		assert(cb->getTownsInfo().size() == towns.size());
	}

}

void CPlayerInterface::initializeHeroTownList()
{
	std::vector<const CGHeroInstance*> allHeroes = cb->getHeroesInfo();
	/*
	std::vector <const CGHeroInstance *> newWanderingHeroes;

	//applying current heroes order to new heroes info
	int j;
	for (int i = 0; i < wanderingHeroes.size(); i++)
		if ((j = vstd::find_pos(allHeroes, wanderingHeroes[i])) >= 0)
			if (!allHeroes[j]->inTownGarrison)
			{
				newWanderingHeroes += allHeroes[j];
				allHeroes -= allHeroes[j];
			}
	//all the rest of new heroes go the end of the list
	wanderingHeroes.clear();
	wanderingHeroes = newWanderingHeroes;
	newWanderingHeroes.clear();*/

	for (auto & allHeroe : allHeroes)
		if (!allHeroe->inTownGarrison)
			wanderingHeroes.push_back(allHeroe);

	std::vector<const CGTownInstance*> allTowns = cb->getTownsInfo();
	/*
	std::vector<const CGTownInstance*> newTowns;
	for (int i = 0; i < towns.size(); i++)
		if ((j = vstd::find_pos(allTowns, towns[i])) >= 0)
		{
			newTowns += allTowns[j];
			allTowns -= allTowns[j];
		}

	towns.clear();
	towns = newTowns;
	newTowns.clear();*/
	for(auto & allTown : allTowns)
		towns.push_back(allTown);

	if (adventureInt)
		adventureInt->updateNextHero(nullptr);
}

void CPlayerInterface::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	auto recruitCb = [=](CreatureID id, int count){ LOCPLINT->cb->recruitCreatures(dwelling, dst, id, count, -1); };
	CRecruitmentWindow *cr = new CRecruitmentWindow(dwelling, level, dst, recruitCb);
	GH.pushInt(cr);
}

void CPlayerInterface::waitWhileDialog(bool unlockPim /*= true*/)
{
	if(GH.amIGuiThread())
	{
		logGlobal->warnStream() << "Cannot wait for dialogs in gui thread (deadlock risk)!";
		return;
	}

	auto unlock = vstd::makeUnlockGuardIf(*pim, unlockPim);
	boost::unique_lock<boost::mutex> un(showingDialog->mx);
	while(showingDialog->data)
		showingDialog->cond.wait(un);
}

void CPlayerInterface::showShipyardDialog(const IShipyard *obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto state = obj->shipyardStatus();
	std::vector<si32> cost;
	obj->getBoatCost(cost);
	CShipyardWindow *csw = new CShipyardWindow(cost, state, obj->getBoatType(), [=]{ cb->buildBoat(obj); });
	GH.pushInt(csw);
}

void CPlayerInterface::newObject( const CGObjectInstance * obj )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//we might have built a boat in shipyard in opened town screen
	if(obj->ID == Obj::BOAT
		&& LOCPLINT->castleInt
		&&  obj->pos-obj->getVisitableOffset() == LOCPLINT->castleInt->town->bestLocation())
	{
		CCS->soundh->playSound(soundBase::newBuilding);
		LOCPLINT->castleInt->addBuilding(BuildingID::SHIP);
	}
}

void CPlayerInterface::centerView (int3 pos, int focusTime)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->curh->hide();
	adventureInt->centerOn (pos);
	if(focusTime)
	{
		GH.totalRedraw();
		{
			auto unlockPim = vstd::makeUnlockGuard(*pim);
			IgnoreEvents ignore(*this);
			SDL_Delay(focusTime);
		}
	}
	CCS->curh->show();
}

void CPlayerInterface::objectRemoved( const CGObjectInstance *obj )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (LOCPLINT->cb->getCurrentPlayer() == playerID) {
		std::string handlerName = VLC->objtypeh->getObjectHandlerName(obj->ID);
		if ((handlerName == "pickable") || (handlerName == "scholar") || (handlerName== "artifact") || (handlerName == "pandora")) {
			waitWhileDialog();
			CCS->soundh->playSoundFromSet(CCS->soundh->pickupSounds);
		} else if ((handlerName == "monster") || (handlerName == "hero")) {
			waitWhileDialog();
			CCS->soundh->playSound(soundBase::KillFade);
		}
	}
	if(obj->ID == Obj::HERO  &&  obj->tempOwner == playerID)
	{
		const CGHeroInstance *h = static_cast<const CGHeroInstance*>(obj);
		heroKilled(h);
	}
}

bool CPlayerInterface::ctrlPressed() const
{
	return isCtrlKeyDown();
}

const CArmedInstance * CPlayerInterface::getSelection()
{
	return currentSelection;
}

void CPlayerInterface::setSelection(const CArmedInstance * obj)
{
	currentSelection = obj;
}

void CPlayerInterface::update()
{
	// Make sure that gamestate won't change when GUI objects may obtain its parts on event processing or drawing request
	boost::shared_lock<boost::shared_mutex> gsLock(cb->getGsMutex());

	// While mutexes were locked away we may be have stopped being the active interface
	if(LOCPLINT != this)
		return;

	//if there are any waiting dialogs, show them
	if((howManyPeople <= 1 || makingTurn) && !dialogs.empty() && !showingDialog->get())
	{
		showingDialog->set(true);
		GH.pushInt(dialogs.front());
		dialogs.pop_front();
	}

	//in some conditions we may receive calls before selection is initialized - we must ignore them
	if(adventureInt && !adventureInt->selection && GH.topInt() == adventureInt)
	{
		return;
	}

	// Handles mouse and key input
	GH.updateTime();
	GH.handleEvents();

	if(adventureInt && !adventureInt->isActive() && adventureInt->scrollingDir) //player forces map scrolling though interface is disabled
		GH.totalRedraw();
	else
		GH.simpleRedraw();
}

int CPlayerInterface::getLastIndex( std::string namePrefix)
{
	using namespace boost::filesystem;
	using namespace boost::algorithm;

	path gamesDir = VCMIDirs::get().userSavePath();
	std::map<std::time_t, int> dates; //save number => datestamp

	const directory_iterator enddir;
	if(!exists(gamesDir))
		create_directory(gamesDir);
	else
	for (directory_iterator dir(gamesDir); dir != enddir; ++dir)
	{
		if(is_regular(dir->status()))
		{
			std::string name = dir->path().filename().string();
			if(starts_with(name, namePrefix) && ends_with(name, ".vcgm1"))
			{
				char nr = name[namePrefix.size()];
				if(std::isdigit(nr))
					dates[last_write_time(dir->path())] = boost::lexical_cast<int>(nr);
			}
		}
	}

	if(!dates.empty())
		return (--dates.end())->second; //return latest file number
	return 0;
}

void CPlayerInterface::initMovement( const TryMoveHero &details, const CGHeroInstance * ho, const int3 &hp )
{
	if(details.end.x+1 == details.start.x && details.end.y+1 == details.start.y) //tl
	{
		//ho->moveDir = 1;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, -31)));
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 33, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 65, -31)));

		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, 1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, 33)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x == details.start.x && details.end.y+1 == details.start.y) //t
	{
		//ho->moveDir = 2;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 0, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 32, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 64, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x-1 == details.start.x && details.end.y+1 == details.start.y) //tr
	{
		//ho->moveDir = 3;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 31, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 63, -31)));
		CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, 1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 33), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, 33)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x-1 == details.start.x && details.end.y == details.start.y) //r
	{
		//ho->moveDir = 4;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 0), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, 0)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 32), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, 32)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x-1 == details.start.x && details.end.y-1 == details.start.y) //br
	{
		//ho->moveDir = 5;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, -1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, -1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 31), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, 31)));

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 31, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 63, 63)));
		CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 95, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x == details.start.x && details.end.y-1 == details.start.y) //b
	{
		//ho->moveDir = 6;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31), ho->id);

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 0, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 32, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 64, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x+1 == details.start.x && details.end.y-1 == details.start.y) //bl
	{
		//ho->moveDir = 7;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, -1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, -1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, 31)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 31), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, 63)));
		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 33, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, 65, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), objectBlitOrderSorter);
	}
	else if(details.end.x+1 == details.start.x && details.end.y == details.start.y) //l
	{
		//ho->moveDir = 8;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, 0)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 0), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(TerrainTileObject(ho, genRect(32, 32, -31, 32)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 32), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), objectBlitOrderSorter);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), objectBlitOrderSorter);
	}
}

void CPlayerInterface::movementPxStep( const TryMoveHero &details, int i, const int3 &hp, const CGHeroInstance * ho )
{
	if(details.end.x+1 == details.start.x && details.end.y+1 == details.start.y) //tl
	{
		//setting advmap shift
		adventureInt->terrain.moveX = i-32;
		adventureInt->terrain.moveY = i-32;

		subRect(hp.x-3, hp.y-2, hp.z, genRect(32, 32, -31+i, -31+i), ho->id);
		subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 1+i, -31+i), ho->id);
		subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 33+i, -31+i), ho->id);
		subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 65+i, -31+i), ho->id);

		subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 1+i), ho->id);
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 1+i), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 1+i), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 1+i), ho->id);

		subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 33+i), ho->id);
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 33+i), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 33+i), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 33+i), ho->id);
	}
	else if(details.end.x == details.start.x && details.end.y+1 == details.start.y) //t
	{
		//setting advmap shift
		adventureInt->terrain.moveY = i-32;

		subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, 0, -31+i), ho->id);
		subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 32, -31+i), ho->id);
		subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 64, -31+i), ho->id);

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1+i), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1+i), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1+i), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33+i), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33+i), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33+i), ho->id);
	}
	else if(details.end.x-1 == details.start.x && details.end.y+1 == details.start.y) //tr
	{
		//setting advmap shift
		adventureInt->terrain.moveX = -i+32;
		adventureInt->terrain.moveY = i-32;

		subRect(hp.x-2, hp.y-2, hp.z, genRect(32, 32, -1-i, -31+i), ho->id);
		subRect(hp.x-1, hp.y-2, hp.z, genRect(32, 32, 31-i, -31+i), ho->id);
		subRect(hp.x, hp.y-2, hp.z, genRect(32, 32, 63-i, -31+i), ho->id);
		subRect(hp.x+1, hp.y-2, hp.z, genRect(32, 32, 95-i, -31+i), ho->id);

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 1+i), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 1+i), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 1+i), ho->id);
		subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 1+i), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 33+i), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 33+i), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 33+i), ho->id);
		subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 33+i), ho->id);
	}
	else if(details.end.x-1 == details.start.x && details.end.y == details.start.y) //r
	{
		//setting advmap shift
		adventureInt->terrain.moveX = -i+32;

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, 0), ho->id);
		subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, 0), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 32), ho->id);
		subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 32), ho->id);
	}
	else if(details.end.x-1 == details.start.x && details.end.y-1 == details.start.y) //br
	{

		//setting advmap shift
		adventureInt->terrain.moveX = -i+32;
		adventureInt->terrain.moveY = -i+32;

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1-i, -1-i), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31-i, -1-i), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63-i, -1-i), ho->id);
		subRect(hp.x+1, hp.y-1, hp.z, genRect(32, 32, 95-i, -1-i), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1-i, 31-i), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31-i, 31-i), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63-i, 31-i), ho->id);
		subRect(hp.x+1, hp.y, hp.z, genRect(32, 32, 95-i, 31-i), ho->id);

		subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, -1-i, 63-i), ho->id);
		subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 31-i, 63-i), ho->id);
		subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 63-i, 63-i), ho->id);
		subRect(hp.x+1, hp.y+1, hp.z, genRect(32, 32, 95-i, 63-i), ho->id);
	}
	else if(details.end.x == details.start.x && details.end.y-1 == details.start.y) //b
	{
		//setting advmap shift
		adventureInt->terrain.moveY = -i+32;

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, -1-i), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, -1-i), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, -1-i), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 31-i), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 31-i), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 31-i), ho->id);

		subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 0, 63-i), ho->id);
		subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 32, 63-i), ho->id);
		subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 64, 63-i), ho->id);
	}
	else if(details.end.x+1 == details.start.x && details.end.y-1 == details.start.y) //bl
	{
		//setting advmap shift
		adventureInt->terrain.moveX = i-32;
		adventureInt->terrain.moveY = -i+32;

		subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, -1-i), ho->id);
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, -1-i), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, -1-i), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, -1-i), ho->id);

		subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 31-i), ho->id);
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 31-i), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 31-i), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 31-i), ho->id);

		subRect(hp.x-3, hp.y+1, hp.z, genRect(32, 32, -31+i, 63-i), ho->id);
		subRect(hp.x-2, hp.y+1, hp.z, genRect(32, 32, 1+i, 63-i), ho->id);
		subRect(hp.x-1, hp.y+1, hp.z, genRect(32, 32, 33+i, 63-i), ho->id);
		subRect(hp.x, hp.y+1, hp.z, genRect(32, 32, 65+i, 63-i), ho->id);
	}
	else if(details.end.x+1 == details.start.x && details.end.y == details.start.y) //l
	{
		//setting advmap shift
		adventureInt->terrain.moveX = i-32;

		subRect(hp.x-3, hp.y-1, hp.z, genRect(32, 32, -31+i, 0), ho->id);
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1+i, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33+i, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65+i, 0), ho->id);

		subRect(hp.x-3, hp.y, hp.z, genRect(32, 32, -31+i, 32), ho->id);
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1+i, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33+i, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65+i, 32), ho->id);
	}
}

void CPlayerInterface::finishMovement( const TryMoveHero &details, const int3 &hp, const CGHeroInstance * ho )
{
	adventureInt->terrain.moveX = adventureInt->terrain.moveY = 0;

	if(details.end.x+1 == details.start.x && details.end.y+1 == details.start.y) //tl
	{
		delObjRect(hp.x, hp.y-2, hp.z, ho->id);
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
		delObjRect(hp.x-3, hp.y, hp.z, ho->id);
	}
	else if(details.end.x == details.start.x && details.end.y+1 == details.start.y) //t
	{
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.end.x-1 == details.start.x && details.end.y+1 == details.start.y) //tr
	{
		delObjRect(hp.x-2, hp.y-2, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x+1, hp.y, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.end.x-1 == details.start.x && details.end.y == details.start.y) //r
	{
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
	}
	else if(details.end.x-1 == details.start.x && details.end.y-1 == details.start.y) //br
	{
		delObjRect(hp.x-2, hp.y+1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y, hp.z, ho->id);
		delObjRect(hp.x+1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
	}
	else if(details.end.x == details.start.x && details.end.y-1 == details.start.y) //b
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
	}
	else if(details.end.x+1 == details.start.x && details.end.y-1 == details.start.y) //bl
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-1, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-2, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x-3, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
		delObjRect(hp.x, hp.y+1, hp.z, ho->id);
	}
	else if(details.end.x+1 == details.start.x && details.end.y == details.start.y) //l
	{
		delObjRect(hp.x, hp.y-1, hp.z, ho->id);
		delObjRect(hp.x, hp.y, hp.z, ho->id);
	}

	//restoring good rects
	subRect(details.end.x-2, details.end.y-1, details.end.z, genRect(32, 32, 0, 0), ho->id);
	subRect(details.end.x-1, details.end.y-1, details.end.z, genRect(32, 32, 32, 0), ho->id);
	subRect(details.end.x, details.end.y-1, details.end.z, genRect(32, 32, 64, 0), ho->id);

	subRect(details.end.x-2, details.end.y, details.end.z, genRect(32, 32, 0, 32), ho->id);
	subRect(details.end.x-1, details.end.y, details.end.z, genRect(32, 32, 32, 32), ho->id);
	subRect(details.end.x, details.end.y, details.end.z, genRect(32, 32, 64, 32), ho->id);

	//restoring good order of objects
	std::stable_sort(CGI->mh->ttiles[details.end.x-2][details.end.y-1][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-2][details.end.y-1][details.end.z].objects.end(), objectBlitOrderSorter);
	std::stable_sort(CGI->mh->ttiles[details.end.x-1][details.end.y-1][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-1][details.end.y-1][details.end.z].objects.end(), objectBlitOrderSorter);
	std::stable_sort(CGI->mh->ttiles[details.end.x][details.end.y-1][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x][details.end.y-1][details.end.z].objects.end(), objectBlitOrderSorter);

	std::stable_sort(CGI->mh->ttiles[details.end.x-2][details.end.y][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-2][details.end.y][details.end.z].objects.end(), objectBlitOrderSorter);
	std::stable_sort(CGI->mh->ttiles[details.end.x-1][details.end.y][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-1][details.end.y][details.end.z].objects.end(), objectBlitOrderSorter);
	std::stable_sort(CGI->mh->ttiles[details.end.x][details.end.y][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x][details.end.y][details.end.z].objects.end(), objectBlitOrderSorter);
}

void CPlayerInterface::gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(player == playerID)
	{
		if(victoryLossCheckResult.loss())
			showInfoDialog(CGI->generaltexth->allTexts[95]);

		if(LOCPLINT == this)
		{
			GH.curInt = this; //waiting for dialogs requires this to get events
			waitForAllDialogs(); //wait till all dialogs are displayed and closed
		}

		--howManyPeople;

		if(howManyPeople == 0) //all human players eliminated
		{
			if(adventureInt)
			{
				GH.terminate_cond.setn(true);
				adventureInt->deactivate();
				if(GH.topInt() == adventureInt)
					GH.popInt(adventureInt);
				delete adventureInt;
				adventureInt = nullptr;
			}
		}

		if(cb->getStartInfo()->mode == StartInfo::CAMPAIGN)
		{
			// if you lose the campaign go back to the main menu
			// campaign wins are handled in proposeNextMission
			if(victoryLossCheckResult.loss()) requestReturningToMainMenu();
		}
		else
		{
			if(howManyPeople == 0) //all human players eliminated
			{
				requestReturningToMainMenu();
			}
			else if(victoryLossCheckResult.victory() && LOCPLINT == this) // end game if current human player has won
			{
				requestReturningToMainMenu();
			}
		}

		if(GH.curInt == this) GH.curInt = nullptr;
	}
	else
	{
		if(victoryLossCheckResult.loss() && cb->getPlayerStatus(playerID) == EPlayerStatus::INGAME) //enemy has lost
		{
			std::string str = victoryLossCheckResult.messageToSelf;
			boost::algorithm::replace_first(str, "%s", CGI->generaltexth->capColors[player.getNum()]);
			showInfoDialog(str, std::vector<CComponent*>(1, new CComponent(CComponent::flag, player.getNum(), 0)));
		}
	}
}

void CPlayerInterface::playerBonusChanged( const Bonus &bonus, bool gain )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
}

void CPlayerInterface::showPuzzleMap()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	//TODO: interface should not know the real position of Grail...
	double ratio = 0;
	int3 grailPos = cb->getGrailPos(&ratio);

	GH.pushInt(new CPuzzleWindow(grailPos, ratio));
}

void CPlayerInterface::viewWorldMap()
{
	adventureInt->changeMode(EAdvMapMode::WORLD_VIEW);
}

void CPlayerInterface::advmapSpellCast(const CGHeroInstance * caster, int spellID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (spellID == SpellID::FLY || spellID == SpellID::WATER_WALK)
	{
		eraseCurrentPathOf(caster, false);
	}
	const CSpell * spell = CGI->spellh->objects[spellID];

	if(spellID == SpellID::VIEW_EARTH)
	{
		//TODO: implement on server side
		int level = caster->getSpellSchoolLevel(spell);
		adventureInt->worldViewOptions.showAllTerrain = (level>2);
	}

	auto castSoundPath = spell->getCastSound();
	if (!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);
}

void CPlayerInterface::eraseCurrentPathOf( const CGHeroInstance * ho, bool checkForExistanceOfPath /*= true */ )
{
	if(checkForExistanceOfPath)
	{
		assert(vstd::contains(paths, ho));
	}
	else if (!vstd::contains(paths, ho))
	{
		return;
	}
	assert(ho == adventureInt->selection);

	paths.erase(ho);
	adventureInt->terrain.currentPath = nullptr;
	adventureInt->updateMoveHero(ho, false);
}

void CPlayerInterface::removeLastNodeFromPath(const CGHeroInstance *ho)
{
	adventureInt->terrain.currentPath->nodes.erase(adventureInt->terrain.currentPath->nodes.end()-1);
	if(adventureInt->terrain.currentPath->nodes.size() < 2)  //if it was the last one, remove entire path and path with only one tile is not a real path
		eraseCurrentPathOf(ho);
}

CGPath * CPlayerInterface::getAndVerifyPath(const CGHeroInstance * h)
{
	if(vstd::contains(paths,h)) //hero has assigned path
	{
		CGPath &path = paths[h];
		if(!path.nodes.size())
		{
			logGlobal->warnStream() << "Warning: empty path found...";
			paths.erase(h);
		}
		else
		{
			assert(h->getPosition(false) == path.startPos());
			//update the hero path in case of something has changed on map
			if(LOCPLINT->cb->getPathsInfo(h)->getPath(path, path.endPos()))
				return &path;
			else
				paths.erase(h);
		}
	}

	return nullptr;
}

void CPlayerInterface::acceptTurn()
{
	bool centerView = true;
	if(settings["session"]["autoSkip"].Bool())
	{
		centerView = false;
		while(CInfoWindow *iw = dynamic_cast<CInfoWindow *>(GH.topInt()))
			iw->close();
	}
	waitWhileDialog();

	if(howManyPeople > 1)
		adventureInt->startTurn();

	adventureInt->heroList.update();
	adventureInt->townList.update();

	const CGHeroInstance * heroToSelect = nullptr;

	// find first non-sleeping hero
	for (auto hero : wanderingHeroes)
	{
		if (boost::range::find(sleepingHeroes, hero) == sleepingHeroes.end())
		{
			heroToSelect = hero;
			break;
		}
	}

	//select first hero if available.
	if(heroToSelect != nullptr)
	{
		adventureInt->select(heroToSelect, centerView);
	}
	else
		adventureInt->select(towns.front(), centerView);

	//show new day animation and sound on infobar
	adventureInt->infoBar.showDate();

	adventureInt->updateNextHero(nullptr);
	adventureInt->showAll(screen);

	if(settings["session"]["autoSkip"].Bool() && !LOCPLINT->shiftPressed())
	{
		if(CInfoWindow *iw = dynamic_cast<CInfoWindow *>(GH.topInt()))
			iw->close();

		adventureInt->fendTurn();
	}

	// warn player if he has no town
	if(cb->howManyTowns() == 0)
	{
		auto playerColor = *cb->getPlayerID();

		std::vector<Component> components;
		components.push_back(Component(Component::FLAG, playerColor.getNum(), 0, 0));
		MetaString text;

		auto daysWithoutCastle = *cb->getPlayer(playerColor)->daysWithoutCastle;
		if (daysWithoutCastle < 6)
		{
			text.addTxt(MetaString::ARRAY_TXT,128); //%s, you only have %d days left to capture a town or you will be banished from this land.
			text.addReplacement(MetaString::COLOR, playerColor.getNum());
			text.addReplacement(7 - daysWithoutCastle);
		}
		else if(daysWithoutCastle == 6)
		{
			text.addTxt(MetaString::ARRAY_TXT,129); //%s, this is your last day to capture a town or you will be banished from this land.
			text.addReplacement(MetaString::COLOR, playerColor.getNum());
		}

		showInfoDialogAndWait(components, text);
	}
}

void CPlayerInterface::tryDiggging(const CGHeroInstance *h)
{
	std::string hlp;
	CGI->mh->getTerrainDescr(h->getPosition(false), hlp, false);
	auto isDiggingPossible = h->diggingStatus();
	if(hlp.length())
		isDiggingPossible = EDiggingStatus::TILE_OCCUPIED; //TODO integrate with canDig

	int msgToShow = -1;
	switch(isDiggingPossible)
	{
	case EDiggingStatus::CAN_DIG:
		break;
	case EDiggingStatus::LACK_OF_MOVEMENT:
		msgToShow = 56; //"Digging for artifacts requires a whole day, try again tomorrow."
		break;
	case EDiggingStatus::TILE_OCCUPIED:
		msgToShow = 97; //Try searching on clear ground.
		break;
	case EDiggingStatus::WRONG_TERRAIN:
		msgToShow = 60; ////Try looking on land!
		break;
	default:
		assert(0);
	}

	if(msgToShow < 0)
		cb->dig(h);
	else
		showInfoDialog(CGI->generaltexth->allTexts[msgToShow]);
}

void CPlayerInterface::updateInfo(const CGObjectInstance * specific)
{
	adventureInt->infoBar.showSelection();
}

void CPlayerInterface::battleNewRoundFirst( int round )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRoundFirst(round);
}

void CPlayerInterface::stopMovement()
{
	if(stillMoveHero.get() == DURING_MOVE)//if we are in the middle of hero movement
		stillMoveHero.setn(STOP_MOVE); //after showing dialog movement will be stopped
}

void CPlayerInterface::showMarketWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(market->o->ID == Obj::ALTAR_OF_SACRIFICE)
	{
		//EEMarketMode mode = market->availableModes().front();
		if(market->allowsTrade(EMarketMode::ARTIFACT_EXP) && visitor->getAlignment() != EAlignment::EVIL)
			GH.pushInt(new CAltarWindow(market, visitor, EMarketMode::ARTIFACT_EXP));
		else if(market->allowsTrade(EMarketMode::CREATURE_EXP) && visitor->getAlignment() != EAlignment::GOOD)
			GH.pushInt(new CAltarWindow(market, visitor, EMarketMode::CREATURE_EXP));
	}
	else
		GH.pushInt(new CMarketplaceWindow(market, visitor, market->availableModes().front()));
}

void CPlayerInterface::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto  cuw = new CUniversityWindow(visitor, market);
	GH.pushInt(cuw);
}

void CPlayerInterface::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto  chfw = new CHillFortWindow(visitor, object);
	GH.pushInt(chfw);
}

void CPlayerInterface::availableArtifactsChanged(const CGBlackMarket *bm /*= nullptr*/)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(CMarketplaceWindow *cmw = dynamic_cast<CMarketplaceWindow*>(GH.topInt()))
		cmw->artifactsChanged(false);
}

void CPlayerInterface::showTavernWindow(const CGObjectInstance *townOrTavern)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto  tv = new CTavernWindow(townOrTavern);
	GH.pushInt(tv);
}

void CPlayerInterface::showThievesGuildWindow (const CGObjectInstance * obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto  tgw = new CThievesGuildWindow(obj);
	GH.pushInt(tgw);
}

void CPlayerInterface::showQuestLog()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	CQuestLog * ql = new CQuestLog (LOCPLINT->cb->getMyQuests());
	GH.pushInt (ql);
}

void CPlayerInterface::showShipyardDialogOrProblemPopup(const IShipyard *obj)
{
	if(obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		MetaString txt;
		obj->getProblemText(txt);
		showInfoDialog(txt.toString());
	}
	else
		showShipyardDialog(obj);
}

void CPlayerInterface::requestReturningToMainMenu()
{
	sendCustomEvent(RETURN_TO_MAIN_MENU);
	cb->unregisterAllInterfaces();
}

void CPlayerInterface::requestStoppingClient()
{
	sendCustomEvent(STOP_CLIENT);
}

void CPlayerInterface::sendCustomEvent( int code )
{
	CGuiHandler::pushSDLEvent(SDL_USEREVENT, code);
}

void CPlayerInterface::stackChagedCount(const StackLocation &location, const TQuantity &change, bool isAbsolute)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	garrisonChanged(location.army);
}

void CPlayerInterface::stackChangedType(const StackLocation &location, const CCreature &newType)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	garrisonChanged(location.army);
}

void CPlayerInterface::stacksErased(const StackLocation &location)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	garrisonChanged(location.army);
}

void CPlayerInterface::stacksSwapped(const StackLocation &loc1, const StackLocation &loc2)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	std::vector<const CGObjectInstance *> objects;
	objects.push_back(loc1.army);
	if(loc2.army != loc1.army)
		objects.push_back(loc2.army);

	garrisonsChanged(objects);
}

void CPlayerInterface::newStackInserted(const StackLocation &location, const CStackInstance &stack)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	garrisonChanged(location.army);
}

void CPlayerInterface::stacksRebalanced(const StackLocation &src, const StackLocation &dst, TQuantity count)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	std::vector<const CGObjectInstance *> objects;
	objects.push_back(src.army);
	if(src.army != dst.army)
		objects.push_back(dst.army);

	garrisonsChanged(objects);
}

void CPlayerInterface::askToAssembleArtifact(const ArtifactLocation &al)
{
	auto hero = dynamic_cast<const CGHeroInstance*>(al.relatedObj());
	if(hero)
	{
		CArtPlace::askToAssemble(hero->getArt(al.slot), al.slot, hero);
	}
}

void CPlayerInterface::artifactPut(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	askToAssembleArtifact(al);
}

void CPlayerInterface::artifactRemoved(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(IShowActivatable *isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa);
		if(artWin)
			artWin->artifactRemoved(al);
	}
}

void CPlayerInterface::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(IShowActivatable *isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa);
		if(artWin)
			artWin->artifactMoved(src, dst);
	}
	askToAssembleArtifact(dst);
}

void CPlayerInterface::artifactAssembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(IShowActivatable *isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa);
		if(artWin)
			artWin->artifactAssembled(al);
	}
}

void CPlayerInterface::artifactDisassembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(IShowActivatable *isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa);
		if(artWin)
			artWin->artifactDisassembled(al);
	}
}

void CPlayerInterface::playerStartsTurn(PlayerColor player)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	if (!vstd::contains (GH.listInt, adventureInt))
	{
		GH.popInts (GH.listInt.size()); //after map load - remove everything else
		GH.pushInt (adventureInt);
	}
	else
	{
		while (GH.listInt.front() != adventureInt && !dynamic_cast<CInfoWindow*>(GH.listInt.front())) //don't remove dialogs that expect query answer
			GH.popInts(1);
	}

	if(howManyPeople == 1)
	{
		GH.curInt = this;
		adventureInt->startTurn();
	}
	if(player != playerID && this == LOCPLINT)
	{
		waitWhileDialog();
		adventureInt->aiTurnStarted();
	}
}

void CPlayerInterface::waitForAllDialogs(bool unlockPim /*= true*/)
{
	while(!dialogs.empty())
	{
		auto unlock = vstd::makeUnlockGuardIf(*pim, unlockPim);
		SDL_Delay(5);
	}
	waitWhileDialog(unlockPim);
}

void CPlayerInterface::proposeLoadingGame()
{
	showYesNoDialog(CGI->generaltexth->allTexts[68], [this] { sendCustomEvent(RETURN_TO_MENU_LOAD); }, 0, false);
}

CPlayerInterface::SpellbookLastSetting::SpellbookLastSetting()
{
	spellbookLastPageBattle = spellbokLastPageAdvmap = 0;
	spellbookLastTabBattle = spellbookLastTabAdvmap = 4;
}

bool CPlayerInterface::capturedAllEvents()
{
	if(duringMovement)
	{
		//just inform that we are capturing events. they will be processed by heroMoved() in client thread.
		return true;
	}

	if(ignoreEvents)
	{
		boost::unique_lock<boost::mutex> un(eventsM);
		while(!events.empty())
		{
			events.pop();
		}
		return true;
	}

	return false;
}

void CPlayerInterface::setMovementStatus(bool value)
{
	duringMovement = value;
	if(value)
	{
		CCS->curh->hide();
	}
	else
	{
		CCS->curh->show();
	}
}

void CPlayerInterface::doMoveHero(const CGHeroInstance * h, CGPath path)
{
	int i = 1;
	auto getObj = [&](int3 coord, bool ignoreHero)
	{
		return cb->getTile(CGHeroInstance::convertPosition(coord,false))->topVisitableObj(ignoreHero);
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

	boost::unique_lock<boost::mutex> un(stillMoveHero.mx);
	stillMoveHero.data = CONTINUE_MOVE;
	auto doMovement = [&](int3 dst, bool transit)
	{
		stillMoveHero.data = WAITING_MOVE;
		cb->moveHero(h, dst, transit);
		while(stillMoveHero.data != STOP_MOVE && stillMoveHero.data != CONTINUE_MOVE)
			stillMoveHero.cond.wait(un);
	};

	{
		path.convert(0);
		ETerrainType currentTerrain = ETerrainType::BORDER; // not init yet
		ETerrainType newTerrain;
		int sh = -1;

		auto canStop = [&](CGPathNode * node) -> bool
		{
			if(node->layer == EPathfindingLayer::LAND || node->layer == EPathfindingLayer::SAIL)
				return true;

			if(node->accessible == CGPathNode::ACCESSIBLE)
				return true;

			return false;
		};

		for(i=path.nodes.size()-1; i>0 && (stillMoveHero.data == CONTINUE_MOVE || !canStop(&path.nodes[i])); i--)
		{
			int3 currentCoord = path.nodes[i].coord;
			int3 nextCoord = path.nodes[i-1].coord;

			auto currentObject = getObj(currentCoord, currentCoord == h->pos);
			auto nextObjectTop = getObj(nextCoord, false);
			auto nextObject = getObj(nextCoord, true);
			auto destTeleportObj = getDestTeleportObj(currentObject, nextObjectTop, nextObject);
			if(isTeleportAction(path.nodes[i-1].action) && destTeleportObj != nullptr)
			{
				CCS->soundh->stopSound(sh);
				destinationTeleport = destTeleportObj->id;
				destinationTeleportPos = nextCoord;
				doMovement(h->pos, false);
				if(path.nodes[i-1].action == CGPathNode::TELEPORT_BLOCKING_VISIT)
				{
					destinationTeleport = ObjectInstanceID();
					destinationTeleportPos = int3(-1);
				}
				sh = CCS->soundh->playSound(CCS->soundh->horseSounds[currentTerrain], -1);
				continue;
			}

			if(path.nodes[i-1].turns)
			{ //stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
				stillMoveHero.data = STOP_MOVE;
				break;
			}

			// Start a new sound for the hero movement or let the existing one carry on.
#if 0
			// TODO
			if(hero is flying && sh == -1)
				sh = CCS->soundh->playSound(soundBase::horseFlying, -1);
#endif
			{
				newTerrain = cb->getTile(CGHeroInstance::convertPosition(currentCoord, false))->terType;
				if(newTerrain != currentTerrain)
				{
					CCS->soundh->stopSound(sh);
					sh = CCS->soundh->playSound(CCS->soundh->horseSounds[newTerrain], -1);
					currentTerrain = newTerrain;
				}
			}

			assert(h->pos.z == nextCoord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all
			int3 endpos(nextCoord.x, nextCoord.y, h->pos.z);
			logGlobal->traceStream() << "Requesting hero movement to " << endpos;

			bool useTransit = false;
			if((i-2 >= 0) // Check there is node after next one; otherwise transit is pointless
				&& (CGTeleport::isConnected(nextObjectTop, getObj(path.nodes[i-2].coord, false))
					|| CGTeleport::isTeleport(nextObjectTop)))
			{ // Hero should be able to go through object if it's allow transit
				useTransit = true;
			}
			else if(path.nodes[i-1].layer == EPathfindingLayer::AIR)
				useTransit = true;

			doMovement(endpos, useTransit);

			logGlobal->traceStream() << "Resuming " << __FUNCTION__;
			bool guarded = cb->isInTheMap(cb->getGuardingCreaturePosition(endpos - int3(1, 0, 0)));
			if((!useTransit && guarded) || showingDialog->get() == true) // Abort movement if a guard was fought or there is a dialog to display (Mantis #1136)
				break;
		}

		CCS->soundh->stopSound(sh);
	}

	//Update cursor so icon can change if needed when it reappears; doesn;'t apply if a dialog box pops up at the end of the movement
	if(!showingDialog->get())
		GH.fakeMouseMove();


	//todo: this should be in main thread
	if(adventureInt)
	{
		// (i == 0) means hero went through all the path
		adventureInt->updateMoveHero(h, (i != 0));
		adventureInt->updateNextHero(h);
	}

	setMovementStatus(false);
}

void CPlayerInterface::showWorldViewEx(const std::vector<ObjectPosInfo>& objectPositions)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//TODO: showWorldViewEx

	std::copy(objectPositions.begin(), objectPositions.end(), std::back_inserter(adventureInt->worldViewOptions.iconPositions));

	viewWorldMap();
}

/*
 * CPlayerInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/Artifact.h>

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
#include "windows/CSpellWindow.h"
#include "../lib/CConfigHandler.h"
#include "battle/CCreatureAnimation.h"
#include "Graphics.h"
#include "windows/GUIClasses.h"
#include "../lib/CArtHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/serializer/CTypeList.h"
#include "../lib/serializer/BinaryDeserializer.h"
#include "../lib/serializer/BinarySerializer.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/mapObjects/CObjectClassesHandler.h" // For displaying correct UI when interacting with objects
#include "../lib/CStack.h"
#include "../lib/JsonNode.h"
#include "CMusicHandler.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacksBase.h"
#include "../lib/NetPacks.h"//todo: remove
#include "../lib/mapping/CMap.h"
#include "../lib/VCMIDirs.h"
#include "mapHandler.h"
#include "../lib/CStopWatch.h"
#include "../lib/StartInfo.h"
#include "../lib/CPlayerState.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"
#include "windows/InfoWindows.h"
#include "../lib/UnlockGuard.h"
#include "../lib/CPathfinder.h"
#include <SDL.h>
#include "CServerHandler.h"
// FIXME: only needed for CGameState::mutex
#include "../lib/CGameState.h"
#include "gui/NotificationHandler.h"


// The macro below is used to mark functions that are called by client when game state changes.
// They all assume that CPlayerInterface::pim mutex is locked.
#define EVENT_HANDLER_CALLED_BY_CLIENT

// The macro marks functions that are run on a new thread by client.
// They do not own any mutexes intiially.
#define THREAD_CREATED_BY_CLIENT

#define RETURN_IF_QUICK_COMBAT		\
	if (isAutoFightOn && !battleInt)	\
		return;

#define BATTLE_EVENT_POSSIBLE_RETURN\
	if (LOCPLINT != this)			\
		return;						\
	RETURN_IF_QUICK_COMBAT

using namespace CSDL_Ext;

extern std::queue<SDL_Event> SDLEventsQueue;
extern boost::mutex eventsM;
boost::recursive_mutex * CPlayerInterface::pim = new boost::recursive_mutex;

CPlayerInterface * LOCPLINT;

CBattleInterface * CPlayerInterface::battleInt;

enum  EMoveState {STOP_MOVE, WAITING_MOVE, CONTINUE_MOVE, DURING_MOVE};
CondSh<EMoveState> stillMoveHero(STOP_MOVE); //used during hero movement

static bool objectBlitOrderSorter(const TerrainTileObject  & a, const TerrainTileObject & b)
{
	return CMapHandler::compareObjectBlitOrder(a.obj, b.obj);
}

struct HeroObjectRetriever : boost::static_visitor<const CGHeroInstance *>
{
	const CGHeroInstance * operator()(const ConstTransitivePtr<CGHeroInstance> &h) const
	{
		return h;
	}
	const CGHeroInstance * operator()(const ConstTransitivePtr<CStackInstance> &s) const
	{
		return nullptr;
	}
};

CPlayerInterface::CPlayerInterface(PlayerColor Player)
{
	logGlobal->trace("\tHuman player interface for player %s being constructed", Player.getStr());
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);
	GH.defActionsDef = 0;
	LOCPLINT = this;
	curAction = nullptr;
	playerID=Player;
	human=true;
	currentSelection = nullptr;
	battleInt = nullptr;
	castleInt = nullptr;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	cingconsole = new CInGameConsole();
	GH.terminate_cond->set(false);
	firstCall = 1; //if loading will be overwritten in serialize
	autosaveCount = 0;
	isAutoFightOn = false;

	duringMovement = false;
	ignoreEvents = false;
}

CPlayerInterface::~CPlayerInterface()
{
	if(CCS->soundh) CCS->soundh->ambientStopAllChannels();
	logGlobal->trace("\tHuman player interface for player %s being destructed", playerID.getStr());
	delete showingDialog;
	delete cingconsole;
	if (LOCPLINT == this)
		LOCPLINT = nullptr;
}
void CPlayerInterface::initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB)
{
	cb = CB;
	env = ENV;

	CCS->soundh->loadHorseSounds();
	CCS->musich->loadTerrainMusicThemes();

	initializeHeroTownList();

	// always recreate advmap interface to avoid possible memory-corruption bugs
	adventureInt.reset(new CAdvMapInt());
}
void CPlayerInterface::yourTurn()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	{
		boost::unique_lock<boost::mutex> lock(eventsM); //block handling events until interface is ready

		LOCPLINT = this;
		GH.curInt = this;
		adventureInt->selection = nullptr;

		NotificationHandler::notify("Your turn");

		std::string prefix = settings["session"]["saveprefix"].String();
		int frequency = static_cast<int>(settings["general"]["saveFrequency"].Integer());
		if (firstCall)
		{
			if(CSH->howManyPlayerInterfaces() == 1)
				adventureInt->setPlayer(playerID);

			autosaveCount = getLastIndex(prefix + "Autosave_");

			if (firstCall > 0) //new game, not loaded
			{
				int index = getLastIndex(prefix + "Newgame_");
				index %= SAVES_COUNT;
				cb->save("Saves/" + prefix + "Newgame_Autosave_" + boost::lexical_cast<std::string>(index + 1));
			}
			firstCall = 0;
		}
		else if(frequency > 0 && cb->getDate() % frequency == 0)
		{
			LOCPLINT->cb->save("Saves/" + prefix + "Autosave_" + boost::lexical_cast<std::string>(autosaveCount++ + 1));
			autosaveCount %= 5;
		}

		if (adventureInt->player != playerID)
			adventureInt->setPlayer(playerID);

		if (CSH->howManyPlayerInterfaces() > 1) //hot seat message
		{
			adventureInt->startHotSeatWait(playerID);

			makingTurn = true;
			std::string msg = CGI->generaltexth->allTexts[13];
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(CComponent::flag, playerID.getNum(), 0));
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

void CPlayerInterface::heroMoved(const TryMoveHero & details, bool verbose)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	if(LOCPLINT != this)
		return;

	//FIXME: read once and store
	if(settings["session"]["spectate"].Bool() && settings["session"]["spectate-ignore-hero"].Bool())
		return;

	const CGHeroInstance * hero = cb->getHero(details.id); //object representing this hero
	int3 hp = details.start;

	if(!hero)
	{
		//AI hero left the visible area (we can't obtain info)
		//TODO very evil workaround -> retrieve pointer to hero so we could animate it
		// TODO -> we should not need full CGHeroInstance structure to display animation or it should not be handled by playerint (but by the client itself)
		const TerrainTile2 & tile = CGI->mh->ttiles[hp.z][hp.x - 1][hp.y];
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

	if(makingTurn && hero->tempOwner == playerID) //we are moving our hero - we may need to update assigned path
	{
		updateAmbientSounds();
		//We may need to change music - select new track, music handler will change it if needed
		CCS->musich->playMusicFromSet("terrain", LOCPLINT->cb->getTile(hero->visitablePos())->terType->name, true, false);

		if(details.result == TryMoveHero::TELEPORTATION)
		{
			if(adventureInt->terrain.currentPath)
			{
				assert(adventureInt->terrain.currentPath->nodes.size() >= 2);
				std::vector<CGPathNode>::const_iterator nodesIt = adventureInt->terrain.currentPath->nodes.end() - 1;

				if((nodesIt)->coord == CGHeroInstance::convertPosition(details.start, false)
					&& (nodesIt - 1)->coord == CGHeroInstance::convertPosition(details.end, false))
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

		if(hero->pos != details.end //hero didn't change tile but visit succeeded
			|| directlyAttackingCreature) // or creature was attacked from endangering tile.
		{
			eraseCurrentPathOf(hero, false);
		}
		else if(adventureInt->terrain.currentPath && hero->pos == details.end) //&& hero is moving
		{
			if(details.start != details.end) //so we don't touch path when revisiting with spacebar
				removeLastNodeFromPath(hero);
		}
	}

	if(details.stopMovement()) //hero failed to move
	{
		hero->isStanding = true;
		stillMoveHero.setn(STOP_MOVE);
		GH.totalRedraw();
		adventureInt->heroList.update(hero);
		return;
	}

	ui32 speed = 0;
	if(settings["session"]["spectate"].Bool())
	{
		if(!settings["session"]["spectate-hero-speed"].isNull())
			speed = static_cast<ui32>(settings["session"]["spectate-hero-speed"].Integer());
	}
	else if(makingTurn) // our turn, our hero moves
		speed = static_cast<ui32>(settings["adventure"]["heroSpeed"].Float());
	else
		speed = static_cast<ui32>(settings["adventure"]["enemySpeed"].Float());

	if(speed == 0)
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

	auto waitFrame = [&]()
	{
		int frameNumber = GH.mainFPSmng->getFrameNumber();

		auto unlockPim = vstd::makeUnlockGuard(*pim);
		while(frameNumber == GH.mainFPSmng->getFrameNumber())
			SDL_Delay(5);
	};

	//first initializing done

	//main moving
	for(int i = 1; i < 32; i += 2 * speed)
	{
		movementPxStep(details, i, hp, hero);
#ifndef VCMI_ANDROID
		// currently android doesn't seem to be able to handle all these full redraws here, so let's disable it so at least it looks less choppy;
		// most likely this is connected with the way that this manual animation+framerate handling is solved
		adventureInt->updateScreen = true;
#endif

		//evil returns here ...
		//todo: get rid of it
		waitFrame(); //for animation purposes
	}
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
		while(!SDLEventsQueue.empty())
		{
			SDL_Event ev = SDLEventsQueue.front();
			SDLEventsQueue.pop();
			switch(ev.type)
			{
			case SDL_MOUSEBUTTONDOWN:
				stillMoveHero.setn(STOP_MOVE);
				break;
			case SDL_KEYDOWN:
				if (ev.key.keysym.sym < SDLK_F1  ||  ev.key.keysym.sym > SDLK_F15)
					stillMoveHero.setn(STOP_MOVE);
				break;
			}
		}
	}

	if (stillMoveHero.get() == WAITING_MOVE)
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

	adventureInt->heroList.update(hero);
	if (makingTurn && newSelection)
		adventureInt->select(newSelection, true);
	else if (adventureInt->selection == hero)
		adventureInt->selection = nullptr;
	
	if (vstd::contains(paths, hero))
		paths.erase(hero);
}

void CPlayerInterface::heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(start && visitedObj)
	{
		if(visitedObj->getVisitSound())
			CCS->soundh->playSound(visitedObj->getVisitSound().get());
	}
}

void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	wanderingHeroes.push_back(hero);
	adventureInt->heroList.update(hero);
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	if(castleInt)
		castleInt->close();
	castleInt = nullptr;

	auto newCastleInt = std::make_shared<CCastleInterface>(town);

	GH.pushInt(newCastleInt);
}

int3 CPlayerInterface::repairScreenPos(int3 pos)
{
	if (pos.x<-CGI->mh->frameW)
		pos.x = -CGI->mh->frameW;
	if (pos.y<-CGI->mh->frameH)
		pos.y = -CGI->mh->frameH;
	if (pos.x>CGI->mh->sizes.x - adventureInt->terrain.tilesw + CGI->mh->frameW)
		pos.x = CGI->mh->sizes.x - adventureInt->terrain.tilesw + CGI->mh->frameW;
	if (pos.y>CGI->mh->sizes.y - adventureInt->terrain.tilesh + CGI->mh->frameH)
		pos.y = CGI->mh->sizes.y - adventureInt->terrain.tilesh + CGI->mh->frameH;
	return pos;
}

void CPlayerInterface::activateForSpectator()
{
	adventureInt->state = CAdvMapInt::INGAME;
	adventureInt->activate();
	adventureInt->minimap.activate();
}

void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (which == 4)
	{
		if (CAltarWindow *ctw = dynamic_cast<CAltarWindow *>(GH.topInt().get()))
			ctw->setExpToLevel();
	}
	else if (which < GameConstants::PRIMARY_SKILLS) //no need to redraw infowin if this is experience (exp is treated as prim skill with id==4)
		updateInfo(hero);
}

void CPlayerInterface::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	CUniversityWindow* cuw = dynamic_cast<CUniversityWindow*>(GH.topInt().get());
	if (cuw) //university window is open
	{
		GH.totalRedraw();
	}
}

void CPlayerInterface::heroManaPointsChanged(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	updateInfo(hero);
	if (makingTurn && hero->tempOwner == playerID)
		adventureInt->heroList.update(hero);
}
void CPlayerInterface::heroMovePointsChanged(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (makingTurn && hero->tempOwner == playerID)
		adventureInt->heroList.update(hero);
}
void CPlayerInterface::receivedResource()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (CMarketplaceWindow *mw = dynamic_cast<CMarketplaceWindow *>(GH.topInt().get()))
		mw->resourceChanged();

	GH.totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill>& skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->soundh->playSound(soundBase::heroNewLevel);
	GH.pushIntT<CLevelWindow>(hero, pskill, skills, [=](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	});
}

void CPlayerInterface::commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->soundh->playSound(soundBase::heroNewLevel);
	GH.pushIntT<CStackWindow>(commander, skills, [=](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	});
}

void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	updateInfo(town);

	if (town->garrisonHero) //wandering hero moved to the garrison
	{
		CGI->mh->hideObject(town->garrisonHero);
		if (town->garrisonHero->tempOwner == playerID && vstd::contains(wanderingHeroes,town->garrisonHero)) // our hero
			wanderingHeroes -= town->garrisonHero;
	}

	if (town->visitingHero) //hero leaves garrison
	{
		CGI->mh->printObject(town->visitingHero);
		if (town->visitingHero->tempOwner == playerID && !vstd::contains(wanderingHeroes,town->visitingHero)) // our hero
			wanderingHeroes.push_back(town->visitingHero);
	}
	adventureInt->heroList.update();
	adventureInt->updateNextHero(nullptr);

	if(castleInt)
	{
		castleInt->garr->selectSlot(nullptr);
		castleInt->garr->setArmy(town->getUpperArmy(), 0);
		castleInt->garr->setArmy(town->visitingHero, 1);
		castleInt->garr->recreateSlots();
		castleInt->heroes->update();
	}
	for (auto isa : GH.listInt)
	{
		CKingdomInterface *ki = dynamic_cast<CKingdomInterface*>(isa.get());
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
	if (hero->tempOwner != playerID )
		return;

	waitWhileDialog();
	openTownWindow(town);
}

void CPlayerInterface::garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2)
{
	std::vector<const CGObjectInstance *> instances;

	if(auto obj = cb->getObj(id1))
		instances.push_back(obj);


	if(id2 != ObjectInstanceID() && id2 != id1)
	{
		if(auto obj = cb->getObj(id2))
			instances.push_back(obj);
	}

	garrisonsChanged(instances);
}

void CPlayerInterface::garrisonsChanged(std::vector<const CGObjectInstance *> objs)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for (auto object : objs)
		updateInfo(object);

	for (auto & elem : GH.listInt)
	{
		CGarrisonHolder *cgh = dynamic_cast<CGarrisonHolder*>(elem.get());
		if (cgh)
			cgh->updateGarrisons();

		if (CTradeWindow *cmw = dynamic_cast<CTradeWindow*>(elem.get()))
		{
			if (vstd::contains(objs, cmw->hero))
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

	if (castleInt)
	{
		castleInt->townlist->update(town);

		if (castleInt->town == town)
		{
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
		}
	}
	adventureInt->townList.update(town);
}

void CPlayerInterface::battleStartBefore(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	//Don't wait for dialogs when we are non-active hot-seat player
	if (LOCPLINT == this)
		waitForAllDialogs();
}

void CPlayerInterface::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (settings["adventure"]["quickCombat"].Bool())
	{
		autofightingAI = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());
		autofightingAI->initBattleInterface(env, cb);
		autofightingAI->battleStart(army1, army2, int3(0,0,0), hero1, hero2, side);
		isAutoFightOn = true;
		cb->registerBattleInterface(autofightingAI);
		// Player shouldn't be able to move on adventure map if quick combat is going
		adventureInt->quickCombatLock();
	}

	//Don't wait for dialogs when we are non-active hot-seat player
	if (LOCPLINT == this)
		waitForAllDialogs();

	BATTLE_EVENT_POSSIBLE_RETURN;
}

void CPlayerInterface::battleUnitsChanged(const std::vector<UnitChanges> & units, const std::vector<CustomEffectInfo> & customEffects)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	for(auto & info : units)
	{
		switch(info.operation)
		{
		case UnitChanges::EOperation::RESET_STATE:
			{
				const battle::Unit * unit = cb->battleGetUnitByID(info.id);

				if(!unit)
				{
					logGlobal->error("Invalid unit ID %d", info.id);
					continue;
				}

				auto iter = battleInt->creAnims.find(info.id);

				if(iter == battleInt->creAnims.end())
				{
					logGlobal->error("Unit %d have no animation", info.id);
					continue;
				}

				auto animation = iter->second;

				if(unit->alive() && animation->isDead())
					animation->setType(CCreatureAnim::HOLDING);

				if (unit->isClone())
				{
					std::unique_ptr<ColorShifterDeepBlue> shifter(new ColorShifterDeepBlue());
					animation->shiftColor(shifter.get());
				}

				//TODO: handle more cases
			}
			break;
		case UnitChanges::EOperation::REMOVE:
			battleInt->stackRemoved(info.id);
			break;
		case UnitChanges::EOperation::ADD:
			{
				const CStack * unit = cb->battleGetStackByID(info.id);
				if(!unit)
				{
					logGlobal->error("Invalid unit ID %d", info.id);
					continue;
				}
				battleInt->unitAdded(unit);
			}
			break;
		default:
			logGlobal->error("Unknown unit operation %d", (int)info.operation);
			break;
		}
	}

	battleInt->displayCustomEffects(customEffects);
}

void CPlayerInterface::battleObstaclesChanged(const std::vector<ObstacleChanges> & obstacles)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	bool needUpdate = false;

	for(auto & change : obstacles)
	{
		if(change.operation == BattleChanges::EOperation::ADD)
		{
			auto instance = cb->battleGetObstacleByID(change.id);
			if(instance)
				battleInt->obstaclePlaced(*instance);
			else
				logNetwork->error("Invalid obstacle instance %d", change.id);
		}
		else
		{
			needUpdate = true;
		}
	}

	if(needUpdate)
		//update accessible hexes
		battleInt->redrawBackgroundWithHexes(battleInt->activeStack);
}

void CPlayerInterface::battleCatapultAttacked(const CatapultAttack & ca)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackIsCatapulting(ca);
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
	logGlobal->trace("Awaiting command for %s", stack->nodeName());
	auto stackId = stack->ID;
	auto stackName = stack->nodeName();
	if (autofightingAI)
	{
		if (isAutoFightOn)
		{
			auto ret = autofightingAI->activeStack(stack);

			if(cb->battleIsFinished())
			{
				return BattleAction::makeDefend(stack); // battle finished with spellcast
			}

			if (isAutoFightOn)
			{
				return ret;
			}
		}

		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();
	}

	CBattleInterface *b = battleInt;

	if(!b)
	{
		return BattleAction::makeDefend(stack); // probably battle is finished already
	}

	if(CBattleInterface::givenCommand.get())
	{
		logGlobal->error("Command buffer must be clean! (we don't want to use old command)");
		vstd::clear_pointer(CBattleInterface::givenCommand.data);
	}

	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);
		b->stackActivated(stack);
		//Regeneration & mana drain go there
	}
	//wait till BattleInterface sets its command
	boost::unique_lock<boost::mutex> lock(CBattleInterface::givenCommand.mx);
	while(!CBattleInterface::givenCommand.data)
	{
		CBattleInterface::givenCommand.cond.wait(lock);
		if (!battleInt) //battle ended while we were waiting for movement (eg. because of spell)
			throw boost::thread_interrupted(); //will shut the thread peacefully
	}

	//tidy up
	BattleAction ret = *(CBattleInterface::givenCommand.data);
	vstd::clear_pointer(CBattleInterface::givenCommand.data);

	if(ret.actionType == EActionType::CANCEL)
	{
		if(stackId != ret.stackNumber)
			logGlobal->error("Not current active stack action canceled");
		logGlobal->trace("Canceled command for %s", stackName);
	}
	else
		logGlobal->trace("Giving command for %s", stackName);

	return ret;
}

void CPlayerInterface::battleEnd(const BattleResult *br)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(isAutoFightOn || autofightingAI)
	{
		isAutoFightOn = false;
		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();

		if(!battleInt)
		{
			GH.pushIntT<CBattleResultWindow>(*br, *this);
			// #1490 - during AI turn when quick combat is on, we need to display the message and wait for user to close it.
			// Otherwise NewTurn causes freeze.
			waitWhileDialog();
			adventureInt->quickCombatUnlock();
			return;
		}
	}

	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->battleFinished(*br);
	adventureInt->quickCombatUnlock();
}

void CPlayerInterface::battleLogMessage(const std::vector<MetaString> & lines)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->displayBattleLog(lines);
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
		const CStack * defender = cb->battleGetStackByID(elem.stackAttacked, false);
		const CStack * attacker = cb->battleGetStackByID(elem.attackerID, false);
		if(elem.isEffect())
		{
			if(defender && !elem.isSecondary())
				battleInt->displayEffect(elem.effect, defender->getPosition());
		}
		if(elem.isSpell())
		{
			if(defender)
				battleInt->displaySpellEffect(elem.spellID, defender->getPosition());
		}
		//FIXME: why action is deleted during enchanter cast?
		bool remoteAttack = false;

		if(LOCPLINT->curAction)
			remoteAttack |= LOCPLINT->curAction->actionType != EActionType::WALK_AND_ATTACK;

		StackAttackedInfo to_put = {defender, elem.damageAmount, elem.killedAmount, attacker, remoteAttack, elem.killed(), elem.willRebirth(), elem.cloneKilled()};
		arg.push_back(to_put);
	}
	battleInt->stacksAreAttacked(arg);
}
void CPlayerInterface::battleAttack(const BattleAttack * ba)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	assert(curAction);

	const CStack * attacker = cb->battleGetStackByID(ba->stackAttacking);

	if(!attacker)
	{
		logGlobal->error("Attacking stack not found");
		return;
	}

	if(ba->lucky()) //lucky hit
	{
		battleInt->console->addText(attacker->formatGeneralMessage(-45));
		battleInt->displayEffect(18, attacker->getPosition());
		CCS->soundh->playSound(soundBase::GOODLUCK);
	}
	if(ba->unlucky()) //unlucky hit
	{
		battleInt->console->addText(attacker->formatGeneralMessage(-44));
		battleInt->displayEffect(48, attacker->getPosition());
		CCS->soundh->playSound(soundBase::BADLUCK);
	}
	if(ba->deathBlow())
	{
		battleInt->console->addText(attacker->formatGeneralMessage(365));
		for(auto & elem : ba->bsa)
		{
			const CStack * attacked = cb->battleGetStackByID(elem.stackAttacked);
			battleInt->displayEffect(73, attacked->getPosition());
		}
		CCS->soundh->playSound(soundBase::deathBlow);
	}

	battleInt->displayCustomEffects(ba->customEffects);

	battleInt->waitForAnims();

	auto actionTarget = curAction->getTarget(cb.get());

	if(actionTarget.empty() || (actionTarget.size() < 2 && !ba->shot()))
	{
		logNetwork->error("Invalid current action: no destination.");
		return;
	}

	if(ba->shot())
	{
		for(auto & elem : ba->bsa)
		{
			if(!elem.isSecondary()) //display projectile only for primary target
			{
				const CStack * attacked = cb->battleGetStackByID(elem.stackAttacked);
				battleInt->stackAttacking(attacker, attacked->getPosition(), attacked, true);
			}
		}
	}
	else
	{
		auto attackTarget = actionTarget.at(1).hexValue;

		//TODO: use information from BattleAttack but not curAction

		int shift = 0;
		if(ba->counter() && BattleHex::mutualPosition(attackTarget, attacker->getPosition()) < 0)
		{
			int distp = BattleHex::getDistance(attackTarget + 1, attacker->getPosition());
			int distm = BattleHex::getDistance(attackTarget - 1, attacker->getPosition());

			if(distp < distm)
				shift = 1;
			else
				shift = -1;
		}

		if(!ba->bsa.empty())
		{
			const CStack * attacked = cb->battleGetStackByID(ba->bsa.begin()->stackAttacked);
			battleInt->stackAttacking(attacker, ba->counter() ? BattleHex(attackTarget + shift) : attackTarget, attacked, false);
		}
	}

	//battleInt->waitForAnims(); //FIXME: freeze

	if(ba->spellLike())
	{
		//TODO: use information from BattleAttack but not curAction

		auto destination = actionTarget.at(0).hexValue;
		//display hit animation
		SpellID spellID = ba->spellID;
		battleInt->displaySpellHit(spellID, destination);
	}
}

void CPlayerInterface::battleGateStateChanged(const EGateState state)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->gateStateChanged(state);
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

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<Component> & components, int soundID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (settings["session"]["autoSkip"].Bool() && !LOCPLINT->shiftPressed())
	{
		return;
	}
	std::vector<std::shared_ptr<CComponent>> intComps;
	for (auto & component : components)
		intComps.push_back(std::make_shared<CComponent>(component));
	showInfoDialog(text,intComps,soundID);

}

void CPlayerInterface::showInfoDialog(const std::string & text, std::shared_ptr<CComponent> component)
{
	std::vector<std::shared_ptr<CComponent>> intComps;
	intComps.push_back(component);

	showInfoDialog(text, intComps, soundBase::sound_todo);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<std::shared_ptr<CComponent>> & components, int soundID)
{
	LOG_TRACE_PARAMS(logGlobal, "player=%s, text=%s, is LOCPLINT=%d", playerID % text % (this==LOCPLINT));
	waitWhileDialog();

	if (settings["session"]["autoSkip"].Bool() && !LOCPLINT->shiftPressed())
	{
		return;
	}
	std::shared_ptr<CInfoWindow> temp = CInfoWindow::create(text, playerID, components);

	if (makingTurn && GH.listInt.size() && LOCPLINT == this)
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

	std::string str;
	text.toString(str);

	showInfoDialog(str, components, 0);
	waitWhileDialog();
}

void CPlayerInterface::showYesNoDialog(const std::string &text, CFunctionList<void()> onYes, CFunctionList<void()> onNo, const std::vector<std::shared_ptr<CComponent>> & components)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	stopMovement();
	LOCPLINT->showingDialog->setn(true);
	CInfoWindow::showYesNoDialog(text, components, onYes, onNo, playerID);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	stopMovement();
	CCS->soundh->playSound(static_cast<soundBase::soundID>(soundID));

	if (!selection && cancel) //simple yes/no dialog
	{
		std::vector<std::shared_ptr<CComponent>> intComps;
		for (auto & component : components)
			intComps.push_back(std::make_shared<CComponent>(component)); //will be deleted by close in window

		showYesNoDialog(text, [=](){ cb->selectionMade(1, askID); }, [=](){ cb->selectionMade(0, askID); }, intComps);
	}
	else if (selection)
	{
		std::vector<std::shared_ptr<CSelectableComponent>> intComps;
		for (auto & component : components)
			intComps.push_back(std::make_shared<CSelectableComponent>(component)); //will be deleted by CSelWindow::close

		std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
		pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
		if (cancel)
		{
			pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
		}

		int charperline = 35;
		if (pom.size() > 1)
			charperline = 50;
		GH.pushIntT<CSelWindow>(text, playerID, charperline, intComps, pom, askID);
		intComps[0]->clickLeft(true, false);
	}
}

void CPlayerInterface::showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	int choosenExit = -1;
	auto neededExit = std::make_pair(destinationTeleport, destinationTeleportPos);
	if (destinationTeleport != ObjectInstanceID() && vstd::contains(exits, neededExit))
		choosenExit = vstd::find_pos(exits, neededExit);

	cb->selectionMade(choosenExit, askID);
}

void CPlayerInterface::showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	auto selectCallback = [=](int selection)
	{
		JsonNode reply(JsonNode::JsonType::DATA_INTEGER);
		reply.Integer() = selection;
		cb->sendQueryReply(reply, askID);
	};

	auto cancelCallback = [=]()
	{
		JsonNode reply(JsonNode::JsonType::DATA_NULL);
		cb->sendQueryReply(reply, askID);
	};

	const std::string localTitle = title.toString();
	const std::string localDescription = description.toString();

	std::vector<int> tempList;
	tempList.reserve(objects.size());

	for(auto item : objects)
		tempList.push_back(item.getNum());

	CComponent localIconC(icon);

	std::shared_ptr<CIntObject> localIcon = localIconC.image;
	localIconC.removeChild(localIcon.get(), false);

	std::shared_ptr<CObjectListWindow> wnd = std::make_shared<CObjectListWindow>(tempList, localIcon, localTitle, localDescription, selectCallback);
	wnd->onExit = cancelCallback;
	GH.pushInt(wnd);
}

void CPlayerInterface::tileRevealed(const std::unordered_set<int3, ShashInt3> &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//FIXME: wait for dialog? Magi hut/eye would benefit from this but may break other areas
	for (auto & po : pos)
		adventureInt->minimap.showTile(po);
	if (!pos.empty())
		GH.totalRedraw();
}

void CPlayerInterface::tileHidden(const std::unordered_set<int3, ShashInt3> &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto & po : pos)
		adventureInt->minimap.hideTile(po);
	if (!pos.empty())
		GH.totalRedraw();
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	GH.pushIntT<CHeroWindow>(hero);
}

void CPlayerInterface::availableCreaturesChanged( const CGDwelling *town )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (const CGTownInstance * townObj = dynamic_cast<const CGTownInstance*>(town))
	{
		CFortScreen *fs = dynamic_cast<CFortScreen*>(GH.topInt().get());
		if (fs)
			fs->creaturesChanged();

		for(auto isa : GH.listInt)
		{
			CKingdomInterface *ki = dynamic_cast<CKingdomInterface*>(isa.get());
			if (ki && townObj)
				ki->townChanged(townObj);
		}
	}
	else if(town && GH.listInt.size() && (town->ID == Obj::CREATURE_GENERATOR1
		||  town->ID == Obj::CREATURE_GENERATOR4  ||  town->ID == Obj::WAR_MACHINE_FACTORY))
	{
		CRecruitmentWindow *crw = dynamic_cast<CRecruitmentWindow*>(GH.topInt().get());
		if (crw && crw->dwelling == town)
			crw->availableCreaturesChanged();
	}
}

void CPlayerInterface::heroBonusChanged( const CGHeroInstance *hero, const Bonus &bonus, bool gain )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (bonus.type == Bonus::NONE)
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
	h & wanderingHeroes;
	h & towns;
	h & sleepingHeroes;

	std::map<const CGHeroInstance *, int3> pathsMap; //hero -> dest
	if (h.saving)
	{
		for (auto &p : paths)
		{
			if (p.second.nodes.size())
				pathsMap[p.first] = p.second.endPos();
			else
				logGlobal->debug("%s has assigned an empty path! Ignoring it...", p.first->name);
		}
		h & pathsMap;
	}
	else
	{
		h & pathsMap;

		if (cb)
			for (auto &p : pathsMap)
			{
				CGPath path;
				cb->getPathsInfo(p.first)->getPath(path, p.second);
				paths[p.first] = path;
				logGlobal->trace("Restored path for hero %s leading to %s with %d nodes", p.first->nodeName(), p.second.toString(), path.nodes.size());
			}
	}

	h & spellbookSettings;
}

void CPlayerInterface::saveGame( BinarySerializer & h, const int version )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	serializeTempl(h,version);
}

void CPlayerInterface::loadGame( BinaryDeserializer & h, const int version )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	serializeTempl(h,version);
	firstCall = -1;
}

void CPlayerInterface::moveHero( const CGHeroInstance *h, CGPath path )
{
	LOG_TRACE(logGlobal);
	if (!LOCPLINT->makingTurn)
		return;
	if (!h)
		return; //can't find hero

	//It shouldn't be possible to move hero with open dialog (or dialog waiting in bg)
	if (showingDialog->get() || !dialogs.empty())
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
	auto onEnd = [=](){ cb->selectionMade(0, queryID); };

	if (stillMoveHero.get() == DURING_MOVE  && adventureInt->terrain.currentPath && adventureInt->terrain.currentPath->nodes.size() > 1) //to ignore calls on passing through garrisons
	{
		onEnd();
		return;
	}

	waitForAllDialogs();

	auto cgw = std::make_shared<CGarrisonWindow>(up, down, removableUnits);
	cgw->quit->addCallback(onEnd);
	GH.pushInt(cgw);
}

/**
 * Shows the dialog that appears when right-clicking an artifact that can be assembled
 * into a combinational one on an artifact screen. Does not require the combination of
 * artifacts to be legal.
 */
void CPlayerInterface::showArtifactAssemblyDialog(const Artifact * artifact, const Artifact * assembledArtifact, CFunctionList<bool()> onYes)
{
	std::string text = artifact->getDescription();
	text += "\n\n";
	std::vector<std::shared_ptr<CComponent>> scs;

	if(assembledArtifact)
	{
		// You possess all of the components to...
		text += boost::str(boost::format(CGI->generaltexth->allTexts[732]) % assembledArtifact->getName());

		// Picture of assembled artifact at bottom.
		auto sc = std::make_shared<CComponent>(CComponent::artifact, assembledArtifact->getIndex(), 0);
		scs.push_back(sc);
	}
	else
	{
		// Do you wish to disassemble this artifact?
		text += CGI->generaltexth->allTexts[733];
	}

	showYesNoDialog(text, onYes, nullptr, scs);
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (pa->packType == typeList.getTypeID<MoveHero>()  &&  stillMoveHero.get() == DURING_MOVE
	   && destinationTeleport == ObjectInstanceID())
		stillMoveHero.setn(CONTINUE_MOVE);

	if (destinationTeleport != ObjectInstanceID()
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
	GH.pushIntT<CExchangeWindow>(hero1, hero2, query);
}

void CPlayerInterface::objectPropertyChanged(const SetObjectProperty * sop)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//redraw minimap if owner changed
	if (sop->what == ObjProperty::OWNER)
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
			adventureInt->minimap.update();
		}
		assert(cb->getTownsInfo().size() == towns.size());
	}
}

void CPlayerInterface::initializeHeroTownList()
{
	if(!wanderingHeroes.size())
	{
		std::vector<const CGHeroInstance*> heroes = cb->getHeroesInfo();
		for(auto & hero : heroes)
		{
			if(!hero->inTownGarrison)
				wanderingHeroes.push_back(hero);
		}
	}

	if(!towns.size())
		towns = cb->getTownsInfo();

	if(adventureInt)
		adventureInt->updateNextHero(nullptr);
}

void CPlayerInterface::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	auto recruitCb = [=](CreatureID id, int count)
	{
		LOCPLINT->cb->recruitCreatures(dwelling, dst, id, count, -1);
	};
	GH.pushIntT<CRecruitmentWindow>(dwelling, level, dst, recruitCb);
}

void CPlayerInterface::waitWhileDialog(bool unlockPim)
{
	if (GH.amIGuiThread())
	{
		logGlobal->warn("Cannot wait for dialogs in gui thread (deadlock risk)!");
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
	GH.pushIntT<CShipyardWindow>(cost, state, obj->getBoatType(), [=](){ cb->buildBoat(obj); });
}

void CPlayerInterface::newObject( const CGObjectInstance * obj )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//we might have built a boat in shipyard in opened town screen
	if (obj->ID == Obj::BOAT
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
	if (focusTime)
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

void CPlayerInterface::objectRemoved(const CGObjectInstance * obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(LOCPLINT->cb->getCurrentPlayer() == playerID && obj->getRemovalSound())
	{
		waitWhileDialog();
		CCS->soundh->playSound(obj->getRemovalSound().get());
	}
	if(obj->ID == Obj::HERO && obj->tempOwner == playerID)
	{
		const CGHeroInstance * h = static_cast<const CGHeroInstance *>(obj);
		heroKilled(h);
	}
}

void CPlayerInterface::playerBlocked(int reason, bool start)
{
	if(reason == PlayerBlocked::EReason::UPCOMING_BATTLE)
	{
		if(CSH->howManyPlayerInterfaces() > 1 && LOCPLINT != this && LOCPLINT->makingTurn == false)
		{
			//one of our players who isn't last in order got attacked not by our another player (happens for example in hotseat mode)
			boost::unique_lock<boost::mutex> lock(eventsM); //TODO: copied from yourTurn, no idea if it's needed
			LOCPLINT = this;
			GH.curInt = this;
			adventureInt->selection = nullptr;
			adventureInt->setPlayer(playerID);
			std::string msg = CGI->generaltexth->localizedTexts["adventureMap"]["playerAttacked"].String();
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(CComponent::flag, playerID.getNum(), 0));
			makingTurn = true; //workaround for stiff showInfoDialog implementation
			showInfoDialog(msg, cmp);
			makingTurn = false;
		}
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
	updateAmbientSounds(true);
}

void CPlayerInterface::update()
{
	// Make sure that gamestate won't change when GUI objects may obtain its parts on event processing or drawing request
	boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);

	// While mutexes were locked away we may be have stopped being the active interface
	if (LOCPLINT != this)
		return;

	//if there are any waiting dialogs, show them
	if ((CSH->howManyPlayerInterfaces() <= 1 || makingTurn) && !dialogs.empty() && !showingDialog->get())
	{
		showingDialog->set(true);
		GH.pushInt(dialogs.front());
		dialogs.pop_front();
	}

	//in some conditions we may receive calls before selection is initialized - we must ignore them
	if(adventureInt && GH.topInt() == adventureInt
		&& (!adventureInt->selection && !settings["session"]["spectate"].Bool()))
	{
		return;
	}

	// Handles mouse and key input
	GH.updateTime();
	GH.handleEvents();

	if (!adventureInt || adventureInt->isActive())
		GH.simpleRedraw();
	else if((adventureInt->swipeEnabled && adventureInt->swipeMovementRequested) || adventureInt->scrollingDir)
		GH.totalRedraw(); //player forces map scrolling though interface is disabled
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
	if (!exists(gamesDir))
		create_directory(gamesDir);
	else
	for (directory_iterator dir(gamesDir); dir != enddir; ++dir)
	{
		if (is_regular(dir->status()))
		{
			std::string name = dir->path().filename().string();
			if (starts_with(name, namePrefix) && ends_with(name, ".vcgm1"))
			{
				char nr = name[namePrefix.size()];
				if (std::isdigit(nr))
					dates[last_write_time(dir->path())] = boost::lexical_cast<int>(nr);
			}
		}
	}

	if (!dates.empty())
		return (--dates.end())->second; //return latest file number
	return 0;
}

void CPlayerInterface::initMovement( const TryMoveHero &details, const CGHeroInstance * ho, const int3 &hp )
{
	auto subArr = (CGI->mh->ttiles)[hp.z];

	ho->isStanding = false;

	int heroWidth  = ho->appearance->getWidth();
	int heroHeight = ho->appearance->getHeight();

	int tileMinX = std::min(details.start.x, details.end.x) - heroWidth;
	int tileMaxX = std::max(details.start.x, details.end.x);
	int tileMinY = std::min(details.start.y, details.end.y) - heroHeight;
	int tileMaxY = std::max(details.start.y, details.end.y);

	// determine tiles on which hero will be visible during movement and add hero as visible object on these tiles where necessary
	for ( int tileX = tileMinX; tileX <= tileMaxX; ++tileX)
	{
		for ( int tileY = tileMinY; tileY <= tileMaxY; ++tileY)
		{
			bool heroVisibleHere = false;
			auto & tile = subArr[tileX][tileY];

			for ( auto const & obj : tile.objects)
			{
				if (obj.obj == ho)
				{
					heroVisibleHere = true;
					break;
				}
			}

			if ( !heroVisibleHere)
			{
				tile.objects.push_back(TerrainTileObject(ho, {0,0,32,32}));
				std::stable_sort(tile.objects.begin(), tile.objects.end(), objectBlitOrderSorter);
			}
		}
	}
}

void CPlayerInterface::movementPxStep( const TryMoveHero &details, int i, const int3 &hp, const CGHeroInstance * ho )
{
	auto subArr = (CGI->mh->ttiles)[hp.z];

	int heroWidth  = ho->appearance->getWidth();
	int heroHeight = ho->appearance->getHeight();

	int tileMinX = std::min(details.start.x, details.end.x) - heroWidth;
	int tileMaxX = std::max(details.start.x, details.end.x);
	int tileMinY = std::min(details.start.y, details.end.y) - heroHeight;
	int tileMaxY = std::max(details.start.y, details.end.y);

	std::shared_ptr<CAnimation> animation = graphics->getAnimation(ho);

	assert(animation);
	assert(animation->size(0) != 0);
	auto image = animation->getImage(0,0);

	int heroImageOldX = details.start.x * 32;
	int heroImageOldY = details.start.y * 32;

	int heroImageNewX = details.end.x * 32;
	int heroImageNewY = details.end.y * 32;

	int heroImageCurrX = heroImageOldX + i*(heroImageNewX - heroImageOldX)/32;
	int heroImageCurrY = heroImageOldY + i*(heroImageNewY - heroImageOldY)/32;

	// recompute which part of hero sprite will be visible on each tile at this point of movement animation
	for ( int tileX = tileMinX; tileX <= tileMaxX; ++tileX)
	{
		for ( int tileY = tileMinY; tileY <= tileMaxY; ++tileY)
		{
			auto & tile = subArr[tileX][tileY];
			for ( auto & obj : tile.objects)
			{
				if (obj.obj == ho)
				{
					int tilePosX = tileX * 32;
					int tilePosY = tileY * 32;

					obj.rect.x = tilePosX - heroImageCurrX + image->width() - 32;
					obj.rect.y = tilePosY - heroImageCurrY + image->height() - 32;
				}
			}
		}
	}

	adventureInt->terrain.moveX = (32 - i) * (heroImageNewX - heroImageOldX) / 32;
	adventureInt->terrain.moveY = (32 - i) * (heroImageNewY - heroImageOldY) / 32;
}

void CPlayerInterface::finishMovement( const TryMoveHero &details, const int3 &hp, const CGHeroInstance * ho )
{
	auto subArr = (CGI->mh->ttiles)[hp.z];

	int heroWidth  = ho->appearance->getWidth();
	int heroHeight = ho->appearance->getHeight();

	int tileMinX = std::min(details.start.x, details.end.x) - heroWidth;
	int tileMaxX = std::max(details.start.x, details.end.x);
	int tileMinY = std::min(details.start.y, details.end.y) - heroHeight;
	int tileMaxY = std::max(details.start.y, details.end.y);

	// erase hero from all tiles on which he is currently visible
	for ( int tileX = tileMinX; tileX <= tileMaxX; ++tileX)
	{
		for ( int tileY = tileMinY; tileY <= tileMaxY; ++tileY)
		{
			auto & tile = subArr[tileX][tileY];
			for (size_t i = 0; i < tile.objects.size(); ++i)
			{
				if ( tile.objects[i].obj == ho)
				{
					tile.objects.erase(tile.objects.begin() + i);
					break;
				}
			}
		}
	}

	// re-add hero to all tiles on which he will still be visible after animation is over
	for ( int tileX = details.end.x - heroWidth + 1; tileX <= details.end.x; ++tileX)
	{
		for ( int tileY = details.end.y - heroHeight + 1; tileY <= details.end.y; ++tileY)
		{
			auto & tile = subArr[tileX][tileY];
			tile.objects.push_back(TerrainTileObject(ho, {0,0,32,32}));
		}
	}

	// update object list on all tiles that were affected during previous operations
	for ( int tileX = tileMinX; tileX <= tileMaxX; ++tileX)
	{
		for ( int tileY = tileMinY; tileY <= tileMaxY; ++tileY)
		{
			auto & tile = subArr[tileX][tileY];
			std::stable_sort(tile.objects.begin(), tile.objects.end(), objectBlitOrderSorter);
		}
	}

	//recompute hero sprite positioning using hero's final position
	movementPxStep(details, 32, hp, ho);
}

void CPlayerInterface::gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if (player == playerID)
	{
		if (victoryLossCheckResult.loss())
			showInfoDialog(CGI->generaltexth->allTexts[95]);

		//we assume GH.curInt == LOCPLINT
		auto previousInterface = LOCPLINT; //without multiple player interfaces some of lines below are useless, but for hotseat we wanna swap player interface temporarily
		LOCPLINT = this; //this is needed for dialog to show and avoid freeze, dialog showing logic should be reworked someday
		GH.curInt = this; //waiting for dialogs requires this to get events
		if(!makingTurn)
		{
			makingTurn = true; //also needed for dialog to show with current implementation
			waitForAllDialogs();
			makingTurn = false;
		}
		else
			waitForAllDialogs();

		GH.curInt = previousInterface;
		LOCPLINT = previousInterface;

		if(CSH->howManyPlayerInterfaces() == 1 && !settings["session"]["spectate"].Bool()) //all human players eliminated
		{
			if(adventureInt)
			{
				GH.terminate_cond->setn(true);
				adventureInt->deactivate();
				if (GH.topInt() == adventureInt)
					GH.popInt(adventureInt);
				adventureInt.reset();
			}
		}

		if (victoryLossCheckResult.victory() && LOCPLINT == this)
		{
			// end game if current human player has won
			CSH->sendClientDisconnecting();
			requestReturningToMainMenu(true);
		}
		else if(CSH->howManyPlayerInterfaces() == 1 && !settings["session"]["spectate"].Bool())
		{
			//all human players eliminated
			CSH->sendClientDisconnecting();
			requestReturningToMainMenu(false);
		}

		if (GH.curInt == this) GH.curInt = nullptr;
	}
	else
	{
		if (victoryLossCheckResult.loss() && cb->getPlayerStatus(playerID) == EPlayerStatus::INGAME) //enemy has lost
		{
			std::string str = victoryLossCheckResult.messageToSelf;
			boost::algorithm::replace_first(str, "%s", CGI->generaltexth->capColors[player.getNum()]);
			showInfoDialog(str, std::vector<std::shared_ptr<CComponent>>(1, std::make_shared<CComponent>(CComponent::flag, player.getNum(), 0)));
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

	GH.pushIntT<CPuzzleWindow>(grailPos, ratio);
}

void CPlayerInterface::viewWorldMap()
{
	adventureInt->changeMode(EAdvMapMode::WORLD_VIEW);
}

void CPlayerInterface::advmapSpellCast(const CGHeroInstance * caster, int spellID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(dynamic_cast<CSpellWindow *>(GH.topInt().get()))
		GH.popInts(1);

	if(spellID == SpellID::FLY || spellID == SpellID::WATER_WALK)
		eraseCurrentPathOf(caster, false);

	const spells::Spell * spell = CGI->spells()->getByIndex(spellID);

	if(spellID == SpellID::VIEW_EARTH)
	{
		//TODO: implement on server side
		const auto level = caster->getSpellSchoolLevel(spell);
		adventureInt->worldViewOptions.showAllTerrain = (level > 2);
	}

	auto castSoundPath = spell->getCastSound();
	if(!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);
}

void CPlayerInterface::eraseCurrentPathOf(const CGHeroInstance * ho, bool checkForExistanceOfPath)
{
	if (checkForExistanceOfPath)
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
	if (adventureInt->terrain.currentPath->nodes.size() < 2)  //if it was the last one, remove entire path and path with only one tile is not a real path
		eraseCurrentPathOf(ho);
}

CGPath * CPlayerInterface::getAndVerifyPath(const CGHeroInstance * h)
{
	if (vstd::contains(paths,h)) //hero has assigned path
	{
		CGPath &path = paths[h];
		if (!path.nodes.size())
		{
			logGlobal->warn("Warning: empty path found...");
			paths.erase(h);
		}
		else
		{
			assert(h->visitablePos() == path.startPos());
			//update the hero path in case of something has changed on map
			if (LOCPLINT->cb->getPathsInfo(h)->getPath(path, path.endPos()))
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
	if (settings["session"]["autoSkip"].Bool())
	{
		centerView = false;
		while(CInfoWindow *iw = dynamic_cast<CInfoWindow *>(GH.topInt().get()))
			iw->close();
	}

	if(CSH->howManyPlayerInterfaces() > 1)
	{
		waitWhileDialog(); // wait for player to accept turn in hot-seat mode

		adventureInt->startTurn();
	}

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
	if (heroToSelect != nullptr)
	{
		adventureInt->select(heroToSelect, centerView);
	}
	else if (towns.size())
		adventureInt->select(towns.front(), centerView);
	else
		adventureInt->select(wanderingHeroes.front());

	//show new day animation and sound on infobar
	adventureInt->infoBar.showDate();

	adventureInt->updateNextHero(nullptr);
	adventureInt->showAll(screen);

	if(settings["session"]["autoSkip"].Bool() && !LOCPLINT->shiftPressed())
	{
		if(CInfoWindow *iw = dynamic_cast<CInfoWindow *>(GH.topInt().get()))
			iw->close();

		adventureInt->fendTurn();
	}

	// warn player if he has no town
	if (cb->howManyTowns() == 0)
	{
		auto playerColor = *cb->getPlayerID();

		std::vector<Component> components;
		components.push_back(Component(Component::FLAG, playerColor.getNum(), 0, 0));
		MetaString text;

		const auto & optDaysWithoutCastle = cb->getPlayerState(playerColor)->daysWithoutCastle;

		if(optDaysWithoutCastle)
		{
			auto daysWithoutCastle = optDaysWithoutCastle.get();
			if (daysWithoutCastle < 6)
			{
				text.addTxt(MetaString::ARRAY_TXT,128); //%s, you only have %d days left to capture a town or you will be banished from this land.
				text.addReplacement(MetaString::COLOR, playerColor.getNum());
				text.addReplacement(7 - daysWithoutCastle);
			}
			else if (daysWithoutCastle == 6)
			{
				text.addTxt(MetaString::ARRAY_TXT,129); //%s, this is your last day to capture a town or you will be banished from this land.
				text.addReplacement(MetaString::COLOR, playerColor.getNum());
			}

			showInfoDialogAndWait(components, text);
		}
		else
			logGlobal->warn("Player has no towns, but daysWithoutCastle is not set");
	}
}

void CPlayerInterface::tryDiggging(const CGHeroInstance * h)
{
	int msgToShow = -1;
	const bool isBlocked = CGI->mh->hasObjectHole(h->visitablePos()); // Don't dig in the pit.

	const auto diggingStatus = isBlocked
		? EDiggingStatus::TILE_OCCUPIED
		: h->diggingStatus().num;

	switch(diggingStatus)
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
	if (stillMoveHero.get() == DURING_MOVE)//if we are in the middle of hero movement
		stillMoveHero.setn(STOP_MOVE); //after showing dialog movement will be stopped
}

void CPlayerInterface::showMarketWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (market->o->ID == Obj::ALTAR_OF_SACRIFICE)
	{
		//EEMarketMode mode = market->availableModes().front();
		if (market->allowsTrade(EMarketMode::ARTIFACT_EXP) && visitor->getAlignment() != EAlignment::EVIL)
			GH.pushIntT<CAltarWindow>(market, visitor, EMarketMode::ARTIFACT_EXP);
		else if (market->allowsTrade(EMarketMode::CREATURE_EXP) && visitor->getAlignment() != EAlignment::GOOD)
			GH.pushIntT<CAltarWindow>(market, visitor, EMarketMode::CREATURE_EXP);
	}
	else
	{
		GH.pushIntT<CMarketplaceWindow>(market, visitor, market->availableModes().front());
	}
}

void CPlayerInterface::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.pushIntT<CUniversityWindow>(visitor, market);
}

void CPlayerInterface::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.pushIntT<CHillFortWindow>(visitor, object);
}

void CPlayerInterface::availableArtifactsChanged(const CGBlackMarket * bm)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (CMarketplaceWindow *cmw = dynamic_cast<CMarketplaceWindow*>(GH.topInt().get()))
		cmw->artifactsChanged(false);
}

void CPlayerInterface::showTavernWindow(const CGObjectInstance *townOrTavern)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.pushIntT<CTavernWindow>(townOrTavern);
}

void CPlayerInterface::showThievesGuildWindow (const CGObjectInstance * obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.pushIntT<CThievesGuildWindow>(obj);
}

void CPlayerInterface::showQuestLog()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.pushIntT<CQuestLog>(LOCPLINT->cb->getMyQuests());
}

void CPlayerInterface::showShipyardDialogOrProblemPopup(const IShipyard *obj)
{
	if (obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		MetaString txt;
		obj->getProblemText(txt);
		showInfoDialog(txt.toString());
	}
	else
		showShipyardDialog(obj);
}

void CPlayerInterface::requestReturningToMainMenu(bool won)
{
	CCS->soundh->ambientStopAllChannels();
	if(won && cb->getStartInfo()->campState)
		CSH->startCampaignScenario(cb->getStartInfo()->campState);
	else
		sendCustomEvent(EUserEvent::RETURN_TO_MAIN_MENU);
}

void CPlayerInterface::sendCustomEvent( int code )
{
	CGuiHandler::pushSDLEvent(SDL_USEREVENT, code);
}

void CPlayerInterface::askToAssembleArtifact(const ArtifactLocation &al)
{
	auto hero = boost::apply_visitor(HeroObjectRetriever(), al.artHolder);
	if(hero)
	{
		auto art = hero->getArt(al.slot);
		if(art == nullptr)
		{
			logGlobal->error("artifact location %d points to nothing",
							 al.slot.num);
			return;
		}
		CHeroArtPlace::askToAssemble(art, al.slot, hero);
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
	for(auto isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa.get());
		if (artWin)
			artWin->artifactRemoved(al);
	}
}

void CPlayerInterface::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(auto isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa.get());
		if (artWin)
			artWin->artifactMoved(src, dst);
	}
	if(!GH.objsToBlit.empty())
		GH.objsToBlit.back()->redraw();
}

void CPlayerInterface::artifactPossibleAssembling(const ArtifactLocation & dst)
{
	askToAssembleArtifact(dst);
}

void CPlayerInterface::artifactAssembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(auto isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa.get());
		if (artWin)
			artWin->artifactAssembled(al);
	}
}

void CPlayerInterface::artifactDisassembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->infoBar.showSelection();
	for(auto isa : GH.listInt)
	{
		auto artWin = dynamic_cast<CArtifactHolder*>(isa.get());
		if (artWin)
			artWin->artifactDisassembled(al);
	}
}

void CPlayerInterface::playerStartsTurn(PlayerColor player)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (!vstd::contains (GH.listInt, adventureInt))
	{
		GH.popInts ((int)GH.listInt.size()); //after map load - remove everything else
		GH.pushInt (adventureInt);
	}
	else
	{
		adventureInt->infoBar.showSelection();
		while (GH.listInt.front() != adventureInt && !dynamic_cast<CInfoWindow*>(GH.listInt.front().get())) //don't remove dialogs that expect query answer
			GH.popInts(1);
	}

	if(CSH->howManyPlayerInterfaces() == 1)
	{
		GH.curInt = this;
		adventureInt->startTurn();
	}
	if (player != playerID && this == LOCPLINT)
	{
		waitWhileDialog();
		adventureInt->aiTurnStarted();
	}
}

void CPlayerInterface::waitForAllDialogs(bool unlockPim)
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
	showYesNoDialog(CGI->generaltexth->allTexts[68], [this](){ sendCustomEvent(EUserEvent::RETURN_TO_MENU_LOAD); }, nullptr);
}

CPlayerInterface::SpellbookLastSetting::SpellbookLastSetting()
{
	spellbookLastPageBattle = spellbokLastPageAdvmap = 0;
	spellbookLastTabBattle = spellbookLastTabAdvmap = 4;
}

bool CPlayerInterface::capturedAllEvents()
{
	if (duringMovement)
	{
		//just inform that we are capturing events. they will be processed by heroMoved() in client thread.
		return true;
	}

	if (ignoreEvents)
	{
		boost::unique_lock<boost::mutex> un(eventsM);
		while(!SDLEventsQueue.empty())
		{
			SDLEventsQueue.pop();
		}
		return true;
	}

	return false;
}

void CPlayerInterface::setMovementStatus(bool value)
{
	duringMovement = value;
	if (value)
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
		if (action != CGPathNode::TELEPORT_NORMAL &&
			action != CGPathNode::TELEPORT_BLOCKING_VISIT &&
			action != CGPathNode::TELEPORT_BATTLE)
		{
			return false;
		}

		return true;
	};

	auto getDestTeleportObj = [&](const CGObjectInstance * currentObject, const CGObjectInstance * nextObjectTop, const CGObjectInstance * nextObject) -> const CGObjectInstance *
	{
		if (CGTeleport::isConnected(currentObject, nextObjectTop))
			return nextObjectTop;
		if (nextObjectTop && nextObjectTop->ID == Obj::HERO &&
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
		TerrainId currentTerrain = Terrain::BORDER; // not init yet
		TerrainId newTerrain;
		int sh = -1;

		auto canStop = [&](CGPathNode * node) -> bool
		{
			if (node->layer == EPathfindingLayer::LAND || node->layer == EPathfindingLayer::SAIL)
				return true;

			if (node->accessible == CGPathNode::ACCESSIBLE)
				return true;

			return false;
		};

		for (i=(int)path.nodes.size()-1; i>0 && (stillMoveHero.data == CONTINUE_MOVE || !canStop(&path.nodes[i])); i--)
		{
			int3 currentCoord = path.nodes[i].coord;
			int3 nextCoord = path.nodes[i-1].coord;

			auto currentObject = getObj(currentCoord, currentCoord == h->pos);
			auto nextObjectTop = getObj(nextCoord, false);
			auto nextObject = getObj(nextCoord, true);
			auto destTeleportObj = getDestTeleportObj(currentObject, nextObjectTop, nextObject);
			if (isTeleportAction(path.nodes[i-1].action) && destTeleportObj != nullptr)
			{
				CCS->soundh->stopSound(sh);
				destinationTeleport = destTeleportObj->id;
				destinationTeleportPos = nextCoord;
				doMovement(h->pos, false);
				if (path.nodes[i-1].action == CGPathNode::TELEPORT_BLOCKING_VISIT
					|| path.nodes[i-1].action == CGPathNode::TELEPORT_BATTLE)
				{
					destinationTeleport = ObjectInstanceID();
					destinationTeleportPos = int3(-1);
				}
				if(i != path.nodes.size() - 1)
				{
					sh = CCS->soundh->playSound(CCS->soundh->horseSounds[currentTerrain], -1);
				}
				continue;
			}

			if (path.nodes[i-1].turns)
			{ //stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
				stillMoveHero.data = STOP_MOVE;
				break;
			}

			// Start a new sound for the hero movement or let the existing one carry on.
#if 0
			// TODO
			if (hero is flying && sh == -1)
				sh = CCS->soundh->playSound(soundBase::horseFlying, -1);
#endif
			{
				newTerrain = cb->getTile(CGHeroInstance::convertPosition(currentCoord, false))->terType->id;
				if(newTerrain != currentTerrain)
				{
					CCS->soundh->stopSound(sh);
					sh = CCS->soundh->playSound(CCS->soundh->horseSounds[newTerrain], -1);
					currentTerrain = newTerrain;
				}
			}

			assert(h->pos.z == nextCoord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all
			int3 endpos(nextCoord.x, nextCoord.y, h->pos.z);
			logGlobal->trace("Requesting hero movement to %s", endpos.toString());

			bool useTransit = false;
			if ((i-2 >= 0) // Check there is node after next one; otherwise transit is pointless
				&& (CGTeleport::isConnected(nextObjectTop, getObj(path.nodes[i-2].coord, false))
					|| CGTeleport::isTeleport(nextObjectTop)))
			{ // Hero should be able to go through object if it's allow transit
				useTransit = true;
			}
			else if (path.nodes[i-1].layer == EPathfindingLayer::AIR)
				useTransit = true;

			doMovement(endpos, useTransit);

			logGlobal->trace("Resuming %s", __FUNCTION__);
			bool guarded = cb->isInTheMap(cb->getGuardingCreaturePosition(endpos - int3(1, 0, 0)));
			if ((!useTransit && guarded) || showingDialog->get() == true) // Abort movement if a guard was fought or there is a dialog to display (Mantis #1136)
				break;
		}

		CCS->soundh->stopSound(sh);
	}

	//Update cursor so icon can change if needed when it reappears; doesn;'t apply if a dialog box pops up at the end of the movement
	if (!showingDialog->get())
		GH.fakeMouseMove();


	//todo: this should be in main thread
	if (adventureInt)
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

void CPlayerInterface::updateAmbientSounds(bool resetAll)
{
	if(castleInt || battleInt || !makingTurn || !currentSelection)
	{
		CCS->soundh->ambientStopAllChannels();
		return;
	}
	else if(!dynamic_cast<CAdvMapInt *>(GH.topInt().get()))
	{
		return;
	}
	if(resetAll)
		CCS->soundh->ambientStopAllChannels();

	std::map<std::string, int> currentSounds;
	auto updateSounds = [&](std::string soundId, int distance) -> void
	{
		if(vstd::contains(currentSounds, soundId))
			currentSounds[soundId] = std::max(currentSounds[soundId], distance);
		else
			currentSounds.insert(std::make_pair(soundId, distance));
	};

	int3 pos = currentSelection->getSightCenter();
	std::unordered_set<int3, ShashInt3> tiles;
	cb->getVisibleTilesInRange(tiles, pos, CCS->soundh->ambientGetRange(), int3::DIST_CHEBYSHEV);
	for(int3 tile : tiles)
	{
		int dist = pos.dist(tile, int3::DIST_CHEBYSHEV);
		// We want sound for every special terrain on tile and not just one on top
		for(auto & ttObj : CGI->mh->ttiles[tile.z][tile.x][tile.y].objects)
		{
			if(ttObj.ambientSound)
				updateSounds(ttObj.ambientSound.get(), dist);
		}
		if(CGI->mh->map->isCoastalTile(tile))
			updateSounds("LOOPOCEA", dist);
	}
	CCS->soundh->ambientUpdateChannels(currentSounds);
}

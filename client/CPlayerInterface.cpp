#include "../stdafx.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "../CCallback.h"
#include "CCastleInterface.h"
#include "CKingdomInterface.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
//#include "SDL_Extensions.h"
#include "SDL_Extensions.h"
//#include "SDL_framerate.h"

#include "SDL_framerate.h"
#include "CConfigHandler.h"
#include "CCreatureAnimation.h"
#include "Graphics.h"
#include "../hch/CArtHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CObjectHandler.h"
#include "../lib/Connection.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CTownHandler.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "../lib/map.h"
#include "../lib/VCMIDirs.h"
#include "../mapHandler.h"
#include "../timeHandler.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <cmath>
#include <queue>
#include <sstream>
#include <boost/filesystem.hpp>
#include "../StartInfo.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/*
 * CPlayerInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::assign;
using namespace CSDL_Ext;

void processCommand(const std::string &message, CClient *&client);

extern std::queue<SDL_Event*> events;
extern boost::mutex eventsM;

CPlayerInterface * LOCPLINT;

CBattleInterface * CPlayerInterface::battleInt;

enum  EMoveState {STOP_MOVE, WAITING_MOVE, CONTINUE_MOVE, DURING_MOVE};
CondSh<EMoveState> stillMoveHero; //used during hero movement

int howManyPeople = 0;


struct OCM_HLP_CGIN
{
	bool inline operator ()(const std::pair<const CGObjectInstance*,SDL_Rect>  & a, const std::pair<const CGObjectInstance*,SDL_Rect> & b) const
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo_cgin ;



CPlayerInterface::CPlayerInterface(int Player, int serial)
{
	howManyPeople++;
	GH.defActionsDef = 0;
	LOCPLINT = this;
	curAction = NULL;
	playerID=Player;
	serialID=serial;
	human=true;
	castleInt = NULL;
	battleInt = NULL;
	pim = new boost::recursive_mutex;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	sysOpts = GDefaultOptions;
	//initializing framerate keeper
	mainFPSmng = new FPSmanager;
	SDL_initFramerate(mainFPSmng);
	SDL_setFramerate(mainFPSmng, 48);
	//framerate keeper initialized
	cingconsole = new CInGameConsole;
	terminate_cond.set(false);
	firstCall = 1; //if loading will be overwritten in serialize
	autosaveCount = 0;
}
CPlayerInterface::~CPlayerInterface()
{
	howManyPeople--;
	delete pim;
	delete showingDialog;
	delete mainFPSmng;
	if(adventureInt)
	{
		if(adventureInt->active & CIntObject::KEYBOARD)
			adventureInt->deactivateKeys();
		delete adventureInt;
		adventureInt = NULL;
	}

	if(cingconsole->active) //TODO
		cingconsole->deactivate();
	delete cingconsole;

	for(std::map<int,SDL_Surface*>::iterator i=graphics->heroWins.begin(); i!= graphics->heroWins.end(); i++)
		SDL_FreeSurface(i->second);
	graphics->heroWins.clear();

	for(std::map<int,SDL_Surface*>::iterator i=graphics->townWins.begin(); i!= graphics->townWins.end(); i++)
		SDL_FreeSurface(i->second);
	graphics->townWins.clear();

	LOCPLINT = NULL;
}
void CPlayerInterface::init(ICallback * CB)
{
	cb = dynamic_cast<CCallback*>(CB);
	if(!adventureInt)
		adventureInt = new CAdvMapInt();

	std::vector<const CGTownInstance*> tt = cb->getTownsInfo(false);
	for(int i=0;i<tt.size();i++)
	{
		SDL_Surface * pom = infoWin(tt[i]);
		graphics->townWins.insert(std::pair<int,SDL_Surface*>(tt[i]->id,pom));
	}

	if(!towns.size() && !wanderingHeroes.size())
	{
		recreateHeroTownList();
	}
}
void CPlayerInterface::yourTurn()
{
	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);
		boost::unique_lock<boost::mutex> lock(eventsM); //block handling events until interface is ready

		LOCPLINT = this;
		GH.curInt = this;

		if(firstCall)
		{
			if(howManyPeople == 1)
				adventureInt->setPlayer(playerID);

			autosaveCount = getLastIndex("Autosave_");

			if(!GH.listInt.size())
			{
				GH.pushInt(adventureInt);
				adventureInt->activateKeys();
			}

			if(firstCall > 0) //new game, not loaded
			{
				int index = getLastIndex("Newgame_Autosave_");
				index %= SAVES_COUNT;
				cb->save("Newgame_Autosave_" + boost::lexical_cast<std::string>(index + 1));
			}
			firstCall = 0;
		}
		else
		{
			LOCPLINT->cb->save("Autosave_" + boost::lexical_cast<std::string>(autosaveCount++ + 1));
			autosaveCount %= 5;
		}

		if(howManyPeople > 1) //hot seat message
		{
			adventureInt->startHotSeatWait(playerID);

			makingTurn = true;
			std::string msg = CGI->generaltexth->allTexts[13];
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos[serialID].name);
			std::vector<SComponent*> cmp;
			cmp.push_back(new SComponent(SComponent::flag, playerID, 0));
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

inline void subRect(const int & x, const int & y, const int & z, const SDL_Rect & r, const int & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].first->id==hid)
		{
			hlp.objects[h].second = r;
			return;
		}
}

inline void delObjRect(const int & x, const int & y, const int & z, const int & hid)
{
	TerrainTile2 & hlp = CGI->mh->ttiles[x][y][z];
	for(int h=0; h<hlp.objects.size(); ++h)
		if(hlp.objects[h].first->id==hid)
		{
			hlp.objects.erase(hlp.objects.begin()+h);
			return;
		}
}
void CPlayerInterface::heroMoved(const TryMoveHero & details)
{
	if(LOCPLINT != this)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	const CGHeroInstance * ho = cb->getHeroInfo(details.id); //object representing this hero
	int3 hp = details.start;

	adventureInt->centerOn(ho->pos); //actualizing screen pos
	adventureInt->minimap.draw(screen2);
	adventureInt->heroList.draw(screen2);


	if(makingTurn  &&  ho->tempOwner == playerID) //we are moving our hero - we may need to update assigned path
	{
		if(details.result == TryMoveHero::TELEPORTATION	||  details.start == details.end)
		{
			if(adventureInt->terrain.currentPath)
				eraseCurrentPathOf(ho);
			return; //teleport - no fancy moving animation
					//TODO: smooth disappear / appear effect
		}

		if (details.result != TryMoveHero::SUCCESS && details.result != TryMoveHero::FAILED) //hero didn't change tile but visit succeeded
		{
			eraseCurrentPathOf(ho);
		}
		else if(adventureInt->terrain.currentPath  &&  details.result == TryMoveHero::SUCCESS) //&& hero is moving
		{
			//remove one node from the path (the one we went)
			adventureInt->terrain.currentPath->nodes.erase(adventureInt->terrain.currentPath->nodes.end()-1);
			if(!adventureInt->terrain.currentPath->nodes.size())  //if it was the last one, remove entire path
				eraseCurrentPathOf(ho);
		}
	}

	if (details.result != TryMoveHero::SUCCESS) //hero failed to move
	{
		ho->isStanding = true;
		stillMoveHero.setn(STOP_MOVE);
		GH.totalRedraw();
		return;
	}

	initMovement(details, ho, hp);

	//first initializing done
	SDL_framerateDelay(mainFPSmng); // after first move

	//main moving
	for(int i=1; i<32; i+=2*sysOpts.heroMoveSpeed)
	{
		movementPxStep(details, i, hp, ho);
		adventureInt->updateScreen = true;
		adventureInt->show(screen);
		CSDL_Ext::update(screen);
		SDL_framerateDelay(mainFPSmng); //for animation purposes
	} //for(int i=1; i<32; i+=4)
	//main moving done

	//finishing move
	finishMovement(details, hp, ho);
	ho->isStanding = true;

	//move finished
	adventureInt->minimap.draw(screen2);
	adventureInt->heroList.updateMove(ho);

	//check if user cancelled movement
	{
		boost::unique_lock<boost::mutex> un(eventsM);
		while(events.size())
		{
			SDL_Event *ev = events.front();
			events.pop();
			switch(ev->type)
			{
			case SDL_MOUSEBUTTONDOWN:
				stillMoveHero.setn(STOP_MOVE);
				break;
			case SDL_KEYDOWN:
				if(ev->key.keysym.sym < SDLK_F1  ||  ev->key.keysym.sym > SDLK_F15)
					stillMoveHero.setn(STOP_MOVE);
				break;
			}
			delete ev;
		}
	}

	if(stillMoveHero.get() == WAITING_MOVE)
		stillMoveHero.setn(DURING_MOVE);

}
void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	graphics->heroWins.erase(hero->ID);
	wanderingHeroes -= hero;
	adventureInt->heroList.updateHList(hero);
}
void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(graphics->heroWins.find(hero->subID)==graphics->heroWins.end())
		graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hero->subID,infoWin(hero)));
	wanderingHeroes.push_back(hero);
	adventureInt->heroList.updateHList();
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	castleInt = new CCastleInterface(town);
	GH.pushInt(castleInt);
}

SDL_Surface * CPlayerInterface::infoWin(const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if (specific)
	{
		switch (specific->ID)
		{
		case HEROI_TYPE:
			return graphics->drawHeroInfoWin(dynamic_cast<const CGHeroInstance*>(specific));
			break;
		case TOWNI_TYPE:
			return graphics->drawTownInfoWin(dynamic_cast<const CGTownInstance*>(specific));
			break;
		default:
			return NULL;
			break;
		}
	}
	else
	{
		switch (adventureInt->selection->ID)
		{
		case HEROI_TYPE:
			{
				const CGHeroInstance * curh = (const CGHeroInstance *)adventureInt->selection;
				return graphics->drawHeroInfoWin(curh);
			}
		case TOWNI_TYPE:
			{
				return graphics->drawTownInfoWin((const CGTownInstance *)adventureInt->selection);
			}
		default:
			tlog1 << "Strange... selection is neither hero nor town\n";
			return NULL;
		}
	}
}

int3 CPlayerInterface::repairScreenPos(int3 pos)
{
	if(pos.x<-CGI->mh->frameW)
		pos.x = -CGI->mh->frameW;
	if(pos.y<-CGI->mh->frameH)
		pos.y = -CGI->mh->frameH;
	if(pos.x>CGI->mh->map->width - adventureInt->terrain.tilesw + CGI->mh->frameW)
		pos.x = CGI->mh->map->width - adventureInt->terrain.tilesw + CGI->mh->frameW;
	if(pos.y>CGI->mh->map->height - adventureInt->terrain.tilesh + CGI->mh->frameH)
		pos.y = CGI->mh->map->height - adventureInt->terrain.tilesh + CGI->mh->frameH;
	return pos;
}
void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val)
{
	if(which >= PRIMARY_SKILLS) //no need to redraw infowin if this is experience (exp is treated as prim skill with id==4)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}
void CPlayerInterface::heroManaPointsChanged(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}
void CPlayerInterface::heroMovePointsChanged(const CGHeroInstance * hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	//adventureInt->heroList.draw();
}
void CPlayerInterface::receivedResource(int type, int val)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	GH.totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16>& skills, boost::function<void(ui32)> &callback)
{
	waitWhileDialog();
	CGI->soundh->playSound(soundBase::heroNewLevel);

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CLevelWindow *lw = new CLevelWindow(hero,pskill,skills,callback);
	GH.pushInt(lw);
}
void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	//redraw infowindow
	SDL_FreeSurface(graphics->townWins[town->id]);
	graphics->townWins[town->id] = infoWin(town);


	if(town->garrisonHero && vstd::contains(wanderingHeroes,town->garrisonHero)) //wandering hero moved to the garrison
	{
		CGI->mh->hideObject(town->garrisonHero);
		wanderingHeroes -= town->garrisonHero;
	}

	if(town->visitingHero && !vstd::contains(wanderingHeroes,town->visitingHero)) //hero leaves garrison
	{
		CGI->mh->printObject(town->visitingHero);
		wanderingHeroes.push_back(town->visitingHero);
	}

	//adventureInt->heroList.updateHList();

	CCastleInterface *c = castleInt;
	if(c)
	{
		c->garr->highlighted = NULL;
		c->hslotup.hero = town->garrisonHero;
		c->garr->odown = c->hslotdown.hero = town->visitingHero;
		c->garr->set2 = town->visitingHero ? &town->visitingHero->army : NULL;
		c->garr->recreateSlots();
	}
	GH.totalRedraw();
}
void CPlayerInterface::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	if(hero->tempOwner != town->tempOwner)
		return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	openTownWindow(town);
}
void CPlayerInterface::garrisonChanged(const CGObjectInstance * obj)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(obj->ID == HEROI_TYPE) //hero
	{
		const CGHeroInstance * hh;
		if(hh = dynamic_cast<const CGHeroInstance*>(obj))
		{
			SDL_FreeSurface(graphics->heroWins[hh->subID]);
			graphics->heroWins[hh->subID] = infoWin(hh);
		}
	}
	else if (obj->ID == TOWNI_TYPE) //town
	{
		const CGTownInstance * tt;
		if(tt = static_cast<const CGTownInstance*>(obj))
		{
			SDL_FreeSurface(graphics->townWins[tt->id]);
			graphics->townWins[tt->id] = infoWin(tt);
		}
		if(tt->visitingHero)
		{
			SDL_FreeSurface(graphics->heroWins[tt->visitingHero->subID]);
			graphics->heroWins[tt->visitingHero->subID] = infoWin(tt->visitingHero);
		}

	}

	bool wasGarrison = false;
	for(std::list<IShowActivable*>::iterator i = GH.listInt.begin(); i != GH.listInt.end(); i++)
	{
		if((*i)->type & IShowActivable::WITH_GARRISON)
		{
			CWindowWithGarrison *wwg = static_cast<CWindowWithGarrison*>(*i);
			wwg->garr->recreateSlots();
			wasGarrison = true;
		}
		else
		{//a cheat for Kingdom Overview window (it has CWindowWithGarrison-childrens which are not present in ListInt)
			CKingdomInterface *cki = dynamic_cast<CKingdomInterface*>(*i);//need to create "Garrison Holder" class thingy
			if (cki)
				cki->updateAllGarrisons();
		}
	}

	GH.totalRedraw();
}

void CPlayerInterface::buildChanged(const CGTownInstance *town, int buildingID, int what) //what: 1 - built, 2 - demolished
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	switch (buildingID)
	{
	case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 15:
		{
			SDL_FreeSurface(graphics->townWins[town->id]);
			graphics->townWins[town->id] = infoWin(town);
			break;
		}
	}
	if(!castleInt)
		return;
	if(castleInt->town!=town)
		return;
	switch(what)
	{
	case 1:
		CGI->soundh->playSound(soundBase::newBuilding);
		castleInt->addBuilding(buildingID);
		break;
	case 2:
		castleInt->removeBuilding(buildingID);
		break;
	}
}

void CPlayerInterface::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side) //called by engine when battle starts; side=0 - left, side=1 - right
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	while(showingDialog->get())
		SDL_Delay(20);

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CGI->musich->playMusicFromSet(CGI->musich->battleMusics, -1);
	GH.pushInt(battleInt);
}

void CPlayerInterface::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles) //called when battlefield is prepared, prior the battle beginning
{
}

void CPlayerInterface::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	for(int b=0; b<healedStacks.size(); ++b)
	{
		const CStack * healed = cb->battleGetStackByID(healedStacks[b].first);
		if(battleInt->creAnims[healed->ID]->getType() == 5)
		{
			//stack has been resurrected
			battleInt->creAnims[healed->ID]->setType(2);
		}
	}
}

void CPlayerInterface::battleNewStackAppeared(int stackID)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	//changing necessary things in battle interface
	battleInt->newStack(stackID);
}

void CPlayerInterface::battleObstaclesRemoved(const std::set<si32> & removedObstacles)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	for(std::set<si32>::const_iterator it = removedObstacles.begin(); it != removedObstacles.end(); ++it)
	{
		for(std::map< int, CDefHandler * >::iterator itBat = battleInt->idToObstacle.begin(); itBat != battleInt->idToObstacle.end(); ++itBat)
		{
			if(itBat->first == *it) //remove this obstacle
			{
				battleInt->idToObstacle.erase(itBat);
				break;
			}
		}
	}
	//update accessible hexes
	battleInt->redrawBackgroundWithHexes(battleInt->activeStack);
}

void CPlayerInterface::battleCatapultAttacked(const CatapultAttack & ca)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	battleInt->stackIsCatapulting(ca);
}

void CPlayerInterface::battleStacksRemoved(const BattleStacksRemoved & bsr)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	for(std::set<ui32>::const_iterator it = bsr.stackIDs.begin(); it != bsr.stackIDs.end(); ++it) //for each removed stack
	{
		battleInt->stackRemoved(*it);
	}
}

void CPlayerInterface::battleNewRound(int round) //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->newRound(round);
}

void CPlayerInterface::actionStarted(const BattleAction* action)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	curAction = new BattleAction(*action);
	battleInt->startAction(action);
}

void CPlayerInterface::actionFinished(const BattleAction* action)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	delete curAction;
	curAction = NULL;
	battleInt->endAction(action);
}

BattleAction CPlayerInterface::activeStack(int stackID) //called when it's turn of that stack
{

	CBattleInterface *b = battleInt;
	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);

		const CStack *stack = cb->battleGetStackByID(stackID);
		if(vstd::contains(stack->state,MOVED)) //this stack has moved and makes second action -> high morale
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->amount != 1) ? stack->creature->namePl : stack->creature->nameSing);
			battleInt->displayEffect(20,stack->position);
			battleInt->console->addText(hlp);
		}

		b->stackActivated(stackID);
	}
	//wait till BattleInterface sets its command
	boost::unique_lock<boost::mutex> lock(b->givenCommand->mx);
	while(!b->givenCommand->data)
		b->givenCommand->cond.wait(lock);

	//tidy up
	BattleAction ret = *(b->givenCommand->data);
	delete b->givenCommand->data;
	b->givenCommand->data = NULL;

	//return command
	return ret;
}

void CPlayerInterface::battleEnd(BattleResult *br)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->battleFinished(*br);
}

void CPlayerInterface::battleStackMoved(int ID, int dest, int distance, bool end)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->stackMoved(ID, dest, end, distance);
}
void CPlayerInterface::battleSpellCast(SpellCast *sc)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->spellCast(sc);
}
void CPlayerInterface::battleStacksEffectsSet(SetStackEffect & sse)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	battleInt->battleStacksEffectsSet(sse);
}
void CPlayerInterface::battleStacksAttacked(std::vector<BattleStackAttacked> & bsa)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	tlog5 << "CPlayerInterface::battleStackAttacked - locking...";
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	tlog5 << "done!\n";


	std::vector<SStackAttackedInfo> arg;
	for(std::vector<BattleStackAttacked>::iterator i = bsa.begin(); i != bsa.end(); i++)
	{
		if(i->isEffect() && i->effect != 12) //and not armageddon
		{
			const CStack *stack = cb->battleGetStackByID(i->stackAttacked, false);
			if (stack != NULL)
				battleInt->displayEffect(i->effect, stack->position);
		}
		SStackAttackedInfo to_put = {i->stackAttacked, i->damageAmount, i->killedAmount, i->attackerID, LOCPLINT->curAction->actionType==7, i->killed()};
		arg.push_back(to_put);
	}

	if(bsa.begin()->isEffect() && bsa.begin()->effect == 12) //for armageddon - I hope this condition is enough
	{
		battleInt->displayEffect(bsa.begin()->effect, -1);
	}

	battleInt->stacksAreAttacked(arg);
}
void CPlayerInterface::battleAttack(BattleAttack *ba)
{
	if(LOCPLINT != this)
	{ //another local interface should do this
		return;
	}

	tlog5 << "CPlayerInterface::battleAttack - locking...";
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	tlog5 << "done!\n";
	assert(curAction);
	if(ba->lucky()) //lucky hit
	{
		const CStack *stack = cb->battleGetStackByID(ba->stackAttacking);
		std::string hlp = CGI->generaltexth->allTexts[45];
		boost::algorithm::replace_first(hlp,"%s",(stack->amount != 1) ? stack->creature->namePl.c_str() : stack->creature->nameSing.c_str());
		battleInt->console->addText(hlp);
		battleInt->displayEffect(18,stack->position);
	}
	//TODO: bad luck?

	if(ba->shot())
	{
		for(std::vector<BattleStackAttacked>::iterator i = ba->bsa.begin(); i != ba->bsa.end(); i++)
			battleInt->stackIsShooting(ba->stackAttacking,cb->battleGetPos(i->stackAttacked), i->stackAttacked);
	}
	else
	{//WARNING: does not support multiple attacked creatures
		const CStack * attacker = cb->battleGetStackByID(ba->stackAttacking);
		int shift = 0;
		if(ba->counter() && BattleInfo::mutualPosition(curAction->destinationTile, attacker->position) < 0)
		{
			if( BattleInfo::mutualPosition(curAction->destinationTile + 1, attacker->position) >= 0 )
				shift = 1;
			else
				shift = -1;
		}
		battleInt->stackAttacking( ba->stackAttacking, ba->counter() ? curAction->destinationTile + shift : curAction->additionalInfo, ba->bsa.begin()->stackAttacked );
	}
}

void CPlayerInterface::showComp(SComponent comp)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	CGI->soundh->playSoundFromSet(CGI->soundh->pickupSounds);

	adventureInt->infoBar.showComp(&comp,4000);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<Component*> &components, int soundID)
{
	std::vector<SComponent*> intComps;
	for(int i=0;i<components.size();i++)
		intComps.push_back(new SComponent(*components[i]));
	showInfoDialog(text,intComps,soundID);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<SComponent*> & components, int soundID)
{
	waitWhileDialog();
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	if(stillMoveHero.get() == DURING_MOVE)//if we are in the middle of hero movement
		stillMoveHero.setn(STOP_MOVE); //after showing dialog movement will be stopped

	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text,playerID,0,components,pom,false);

	if(makingTurn && GH.listInt.size() && LOCPLINT == this)
	{
		CGI->soundh->playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->set(true);
		GH.pushInt(temp);
	}
	else
	{
		dialogs.push_back(temp);
	}
}

void CPlayerInterface::showYesNoDialog(const std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool DelComps)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	LOCPLINT->showingDialog->setn(true);
	CInfoWindow::showYesNoDialog(text, &components, onYes, onNo, DelComps, playerID);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel )
{
	waitWhileDialog();
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	CGI->soundh->playSound(static_cast<soundBase::soundID>(soundID));

	if(!selection && cancel) //simple yes/no dialog
	{
		std::vector<SComponent*> intComps;
		for(int i=0;i<components.size();i++)
			intComps.push_back(new SComponent(components[i])); //will be deleted by close in window

		showYesNoDialog(text,intComps,boost::bind(&CCallback::selectionMade,cb,1,askID),boost::bind(&CCallback::selectionMade,cb,0,askID),true);
	}
	else if(selection)
	{
		std::vector<CSelectableComponent*> intComps;
		for(int i=0;i<components.size();i++)
			intComps.push_back(new CSelectableComponent(components[i])); //will be deleted by CSelWindow::close

		std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
		pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
		if(cancel)
		{
			pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
		}

		int charperline = 35;
		if (pom.size() > 1)
			charperline = 50;
		CSelWindow * temp = new CSelWindow(text, playerID, charperline, intComps, pom, askID);
		GH.pushInt(temp);
		intComps[0]->clickLeft(true, false);
	}

}

void CPlayerInterface::tileRevealed(const std::set<int3> &pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::set<int3>::const_iterator i=pos.begin(); i!=pos.end();i++)
		adventureInt->minimap.showTile(*i);
	if(pos.size())
		GH.totalRedraw();
}

void CPlayerInterface::tileHidden(const std::set<int3> &pos)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::set<int3>::const_iterator i=pos.begin(); i!=pos.end();i++)
		adventureInt->minimap.hideTile(*i);
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	adventureInt->heroWindow->setHero(hero);
	adventureInt->heroWindow->quitButton->callback = boost::bind(&CHeroWindow::quit,adventureInt->heroWindow);
	GH.pushInt(adventureInt->heroWindow);
}

void CPlayerInterface::heroArtifactSetChanged(const CGHeroInstance*hero)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(adventureInt->heroWindow->curHero) //hero window is opened
	{
		adventureInt->heroWindow->deactivate();
		adventureInt->heroWindow->setHero(adventureInt->heroWindow->curHero);
		adventureInt->heroWindow->activate();
		return;
	}
	CExchangeWindow* cew = dynamic_cast<CExchangeWindow*>(GH.topInt());
	if(cew) //exchange window is open
	{
		cew->deactivate();
		for(int g=0; g<ARRAY_COUNT(cew->heroInst); ++g)
		{
			if(cew->heroInst[g] == hero)
			{
				cew->artifs[g]->updateState = true;
				cew->artifs[g]->setHero(hero);
				cew->artifs[g]->updateState = false;
			}
		}
		cew->prepareBackground();
		cew->activate();
	}
}

void CPlayerInterface::availableCreaturesChanged( const CGDwelling *town )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(castleInt && town->ID == TOWNI_TYPE)
	{
		CFortScreen *fs = dynamic_cast<CFortScreen*>(GH.topInt());
		if(fs)
			fs->draw(castleInt,false);
	}
	else if(GH.listInt.size() && (town->ID == 17  ||  town->ID == 20)) //external dwelling
	{
		CRecruitmentWindow *crw = dynamic_cast<CRecruitmentWindow*>(GH.topInt());
		if(crw)
			crw->initCres();
	}
}

void CPlayerInterface::heroBonusChanged( const CGHeroInstance *hero, const HeroBonus &bonus, bool gain )
{
	if(bonus.type == HeroBonus::NONE)	return;
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	redrawHeroWin(hero);
}

template <typename Handler> void CPlayerInterface::serializeTempl( Handler &h, const int version )
{
	h & playerID & serialID;
	h & sysOpts;
}

void CPlayerInterface::serialize( COSer<CSaveFile> &h, const int version )
{
	serializeTempl(h,version);
}

void CPlayerInterface::serialize( CISer<CLoadFile> &h, const int version )
{
	serializeTempl(h,version);
	sysOpts.apply();
	firstCall = -1;
}

void CPlayerInterface::redrawHeroWin(const CGHeroInstance * hero)
{
	if(!vstd::contains(graphics->heroWins,hero->subID))
	{
		tlog1 << "Cannot redraw infowindow for hero with subID=" << hero->subID << " - not present in our map\n";
		return;
	}
	SDL_FreeSurface(graphics->heroWins[hero->subID]);
	graphics->heroWins[hero->subID] = infoWin(hero);
	if (adventureInt->selection == hero)
		adventureInt->infoBar.draw(screen);
}

bool CPlayerInterface::moveHero( const CGHeroInstance *h, CGPath path )
{
	if (!h)
		return false; //can't find hero

	bool result = false;
	path.convert(0);
	boost::unique_lock<boost::mutex> un(stillMoveHero.mx);
	stillMoveHero.data = CONTINUE_MOVE;

	enum TerrainTile::EterrainType currentTerrain = TerrainTile::border; // not init yet
	enum TerrainTile::EterrainType newTerrain;
	int sh = -1;

	for(int i=path.nodes.size()-1; i>0 && stillMoveHero.data == CONTINUE_MOVE; i--)
	{
		//stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
		if(path.nodes[i-1].turns)
		{
			stillMoveHero.data = STOP_MOVE;
			break;
		}
		// Start a new sound for the hero movement or let the existing one carry on.
#if 0
		// TODO
		if (hero is flying && sh == -1)
			sh = CGI->soundh->playSound(soundBase::horseFlying, -1);
#endif
		{
			newTerrain = cb->getTileInfo(CGHeroInstance::convertPosition(path.nodes[i].coord, false))->tertype;

			if (newTerrain != currentTerrain) {
				CGI->soundh->stopSound(sh);
				sh = CGI->soundh->playSound(CGI->soundh->horseSounds[newTerrain], -1);
				currentTerrain = newTerrain;
			}
		}

		stillMoveHero.data = WAITING_MOVE;

		int3 endpos(path.nodes[i-1].coord.x, path.nodes[i-1].coord.y, h->pos.z);
		cb->moveHero(h,endpos);
		while(stillMoveHero.data != STOP_MOVE  &&  stillMoveHero.data != CONTINUE_MOVE)
			stillMoveHero.cond.wait(un);
	}

	CGI->soundh->stopSound(sh);

	//stillMoveHero = false;
	cb->recalculatePaths();
	return result;
}

bool CPlayerInterface::shiftPressed() const
{
	return SDL_GetKeyState(NULL)[SDLK_LSHIFT]  ||  SDL_GetKeyState(NULL)[SDLK_RSHIFT];
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd )
{
	if(stillMoveHero.get() == DURING_MOVE  &&  adventureInt->terrain.currentPath->nodes.size() > 1) //to ignore calls on passing through garrisons
		return;

	{
		boost::unique_lock<boost::mutex> un(showingDialog->mx);
		while(showingDialog->data)
			showingDialog->cond.wait(un);
	}

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	while(dialogs.size())
	{
		pim->unlock();
		SDL_Delay(20);
		pim->lock();
	}
	CGarrisonWindow *cgw = new CGarrisonWindow(up,down,removableUnits);
	cgw->quit->callback += onEnd;
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
void CPlayerInterface::showArtifactAssemblyDialog (ui32 artifactID, ui32 assembleTo, bool assemble, CFunctionList<void()> onYes, CFunctionList<void()> onNo)
{
	const CArtifact &artifact = CGI->arth->artifacts[artifactID];
	std::string text = artifact.Description();
	text += "\n\n";
	std::vector<SComponent*> scs;

	if (assemble) {
		const CArtifact &assembledArtifact = CGI->arth->artifacts[assembleTo];

		// You possess all of the components to...
		text += boost::str(boost::format(CGI->generaltexth->allTexts[732]) % assembledArtifact.Name());

		// Picture of assembled artifact at bottom.
		SComponent* sc = new SComponent;
		sc->type = SComponent::artifact;
		sc->subtype = assembledArtifact.id;
		sc->description = assembledArtifact.Description();
		sc->subtitle = assembledArtifact.Name();
		scs.push_back(sc);
	} else {
		// Do you wish to disassemble this artifact?
		text += CGI->generaltexth->allTexts[733];
	}

	showYesNoDialog(text, scs, onYes, onNo, true); 
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	if(stillMoveHero.get() == DURING_MOVE)
		stillMoveHero.setn(CONTINUE_MOVE);
}

void CPlayerInterface::heroExchangeStarted(si32 hero1, si32 hero2)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	GH.pushInt(new CExchangeWindow(hero2, hero1));
}

void CPlayerInterface::objectPropertyChanged(const SetObjectProperty * sop)
{
	//redraw minimap if owner changed
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	if(sop->what == 1)
	{
		const CGObjectInstance * obj = cb->getObjectInfo(sop->id);
		std::set<int3> pos = obj->getBlockedPos();
		for(std::set<int3>::const_iterator it = pos.begin(); it != pos.end(); ++it)
		{
			if(cb->isVisible(*it))
				adventureInt->minimap.showTile(*it);
		}

		if(obj->ID == TOWNI_TYPE)
			if(obj->tempOwner == playerID)
				towns.push_back(static_cast<const CGTownInstance *>(obj));
			else
				towns -= obj;

		assert(cb->getTownsInfo().size() == towns.size());
	}

}

void CPlayerInterface::recreateHeroTownList()
{
	wanderingHeroes.clear();
	std::vector<const CGHeroInstance*> heroes = cb->getHeroesInfo();
	for(size_t i = 0; i < heroes.size(); i++)
		if(!heroes[i]->inTownGarrison)
			wanderingHeroes.push_back(heroes[i]);

	towns.clear();
	std::vector<const CGTownInstance*> townInfo = cb->getTownsInfo();
	for(size_t i = 0; i < townInfo.size(); i++)
		towns.push_back(townInfo[i]);
}

const CGHeroInstance * CPlayerInterface::getWHero( int pos )
{
	if(pos < 0 || pos >= wanderingHeroes.size())
		return NULL;
	return wanderingHeroes[pos];
}

void CPlayerInterface::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level)
{
	waitWhileDialog();
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CRecruitmentWindow *cr = new CRecruitmentWindow(dwelling, level, dst, boost::bind(&CCallback::recruitCreatures, cb, dwelling, _1, _2));
	GH.pushInt(cr);
}

void CPlayerInterface::waitWhileDialog()
{
	boost::unique_lock<boost::mutex> un(showingDialog->mx);
	while(showingDialog->data)
		showingDialog->cond.wait(un);
}

void CPlayerInterface::showShipyardDialog(const IShipyard *obj)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	int state = obj->state();
	std::vector<si32> cost;
	obj->getBoatCost(cost);
	CShipyardWindow *csw = new CShipyardWindow(cost, state, obj->getBoatType(), boost::bind(&CCallback::buildBoat, cb, obj));
	GH.pushInt(csw);
}

void CPlayerInterface::newObject( const CGObjectInstance * obj )
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	CGI->mh->printObject(obj);
	//we might have built a boat in shipyard in opened town screen
	if(obj->ID == 8
		&& LOCPLINT->castleInt
		&&  obj->pos-obj->getVisitableOffset() == LOCPLINT->castleInt->town->bestLocation())
	{
		CGI->soundh->playSound(soundBase::newBuilding);
		LOCPLINT->castleInt->recreateBuildings();
	}
}

void CPlayerInterface::centerView (int3 pos, int focusTime)
{
	adventureInt->centerOn (pos);
	if(focusTime)
	{
		bool activeAdv = (GH.topInt() == adventureInt  &&  adventureInt->isActive());
		if(activeAdv)
			adventureInt->deactivate();

		SDL_Delay(focusTime);

		if(activeAdv)
			adventureInt->activate();
	}
}

void CPlayerInterface::objectRemoved( const CGObjectInstance *obj )
{
	if(obj->ID == HEROI_TYPE  &&  obj->tempOwner == playerID)
	{
		const CGHeroInstance *h = static_cast<const CGHeroInstance*>(obj);
		heroKilled(h);
	}
}

bool CPlayerInterface::ctrlPressed() const
{
	return SDL_GetKeyState(NULL)[SDLK_LCTRL]  ||  SDL_GetKeyState(NULL)[SDLK_RCTRL];
}

void CPlayerInterface::update()
{
	while(!terminate_cond.get() && !pim->try_lock()) //try acquiring long until it succeeds or we are told to terminate
		boost::this_thread::sleep(boost::posix_time::milliseconds(15));

	if(terminate_cond.get())
		return;

	//if there are any waiting dialogs, show them
	if(dialogs.size() && !showingDialog->get())
	{
		showingDialog->set(true);
		GH.pushInt(dialogs.front());
		dialogs.pop_front();
	}

	GH.updateTime();
	GH.handleEvents();

	if(!adventureInt->isActive() && adventureInt->scrollingDir) //player forces map scrolling though interface is disabled
		GH.totalRedraw();
	else
		GH.simpleRedraw();

	CGI->curh->draw1();
	CSDL_Ext::update(screen);
	CGI->curh->draw2();

	pim->unlock();

	SDL_framerateDelay(mainFPSmng);
}

int CPlayerInterface::getLastIndex( std::string namePrefix)
{
	using namespace boost::filesystem;
	using namespace boost::algorithm;

	std::map<std::time_t, int> dates; //save number => datestamp

	directory_iterator enddir;
	for (directory_iterator dir(GVCMIDirs.UserPath + "/Games"); dir!=enddir; dir++)
	{
		if(is_regular(dir->status()))
		{
			std::string name = dir->path().leaf();
			if(starts_with(name, namePrefix) && ends_with(name, ".vlgm1"))
			{
				char nr = name[namePrefix.size()];
				if(std::isdigit(nr))
				{
					dates[last_write_time(dir->path())] = boost::lexical_cast<int>(nr);
				}
			}
		}
	}

	if(dates.size())
		return (--dates.end())->second; //return latest file number
	return 0;
}

void CPlayerInterface::initMovement( const TryMoveHero &details, const CGHeroInstance * ho, const int3 &hp )
{
	if(details.end.x+1 == details.start.x && details.end.y+1 == details.start.y) //tl
	{
		//ho->moveDir = 1;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, -31)));
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 33, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 65, -31)));

		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 33)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.end.x == details.start.x && details.end.y+1 == details.start.y) //t
	{
		//ho->moveDir = 2;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 0, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 32, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 64, 1), ho->id);

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 0, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 32, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 64, 33), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.end.x-1 == details.start.x && details.end.y+1 == details.start.y) //tr
	{
		//ho->moveDir = 3;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -1, -31)));
		CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 31, -31)));
		CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 63, -31)));
		CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, -31)));

		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 33), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 33), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 33), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 33)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-2][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.end.x-1 == details.start.x && details.end.y == details.start.y) //r
	{
		//ho->moveDir = 4;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, 0), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 0)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 32), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 32)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.end.x-1 == details.start.x && details.end.y-1 == details.start.y) //br
	{
		//ho->moveDir = 5;
		ho->isStanding = false;
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, -1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 31, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 63, -1), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, -1)));

		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, -1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 31, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 63, 31), ho->id);
		CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 31)));

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 31, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 63, 63)));
		CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 95, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x+1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
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

		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 0, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 32, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 64, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.end.x+1 == details.start.x && details.end.y-1 == details.start.y) //bl
	{
		//ho->moveDir = 7;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, -1)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, -1), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, -1), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, -1), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 31)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 31), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 31), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 31), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 63)));
		CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 1, 63)));
		CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 33, 63)));
		CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, 65, 63)));

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-2][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-1][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
		std::stable_sort(CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x][hp.y+1][hp.z].objects.end(), ocmptwo_cgin);
	}
	else if(details.end.x+1 == details.start.x && details.end.y == details.start.y) //l
	{
		//ho->moveDir = 8;
		ho->isStanding = false;
		CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 0)));
		subRect(hp.x-2, hp.y-1, hp.z, genRect(32, 32, 1, 0), ho->id);
		subRect(hp.x-1, hp.y-1, hp.z, genRect(32, 32, 33, 0), ho->id);
		subRect(hp.x, hp.y-1, hp.z, genRect(32, 32, 65, 0), ho->id);

		CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.push_back(std::make_pair(ho, genRect(32, 32, -31, 32)));
		subRect(hp.x-2, hp.y, hp.z, genRect(32, 32, 1, 32), ho->id);
		subRect(hp.x-1, hp.y, hp.z, genRect(32, 32, 33, 32), ho->id);
		subRect(hp.x, hp.y, hp.z, genRect(32, 32, 65, 32), ho->id);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y-1][hp.z].objects.end(), ocmptwo_cgin);

		std::stable_sort(CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.begin(), CGI->mh->ttiles[hp.x-3][hp.y][hp.z].objects.end(), ocmptwo_cgin);
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
	std::stable_sort(CGI->mh->ttiles[details.end.x-2][details.end.y-1][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-2][details.end.y-1][details.end.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.end.x-1][details.end.y-1][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-1][details.end.y-1][details.end.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.end.x][details.end.y-1][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x][details.end.y-1][details.end.z].objects.end(), ocmptwo_cgin);

	std::stable_sort(CGI->mh->ttiles[details.end.x-2][details.end.y][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-2][details.end.y][details.end.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.end.x-1][details.end.y][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x-1][details.end.y][details.end.z].objects.end(), ocmptwo_cgin);
	std::stable_sort(CGI->mh->ttiles[details.end.x][details.end.y][details.end.z].objects.begin(), CGI->mh->ttiles[details.end.x][details.end.y][details.end.z].objects.end(), ocmptwo_cgin);
}

void CPlayerInterface::gameOver(ui8 player, bool victory )
{
	if(LOCPLINT != this)
		return;

	if(player == playerID)
	{
		if(!victory)
			showInfoDialog(CGI->generaltexth->allTexts[95]);
// 		else
// 			showInfoDialog("Placeholder message: you won!");

		makingTurn = true;
		while(showingDialog->get() || dialogs.size()); //wait till all dialogs are displayed and closed
		makingTurn = false;

 		//return to main menu
 		SDL_Event event;
 		event.type = SDL_USEREVENT;
 		event.user.code = 2;
 		SDL_PushEvent(&event);
	}
	else
	{
		if(!victory) //enemy has lost
		{
			std::string txt = CGI->generaltexth->allTexts[5]; //%s has been vanquished!
			boost::algorithm::replace_first(txt, "%s", CGI->generaltexth->capColors[player]);
			showInfoDialog(txt,std::vector<SComponent*>(1, new SComponent(SComponent::flag, player, 0)));
		}
	}
}

void CPlayerInterface::playerBonusChanged( const HeroBonus &bonus, bool gain )
{

}

void CPlayerInterface::showPuzzleMap()
{
	waitWhileDialog();

	//TODO: interface should not know the real position of Grail...
	float ratio = 0;
	int3 grailPos = cb->getGrailPos(ratio);

	GH.pushInt(new CPuzzleWindow(grailPos, ratio));
}

void SystemOptions::setMusicVolume( int newVolume )
{
	musicVolume = newVolume;
	CGI->musich->setVolume(newVolume);
	settingsChanged();
}

void SystemOptions::setSoundVolume( int newVolume )
{
	soundVolume = newVolume;
	CGI->soundh->setVolume(newVolume);
	settingsChanged();
}

void SystemOptions::setHeroMoveSpeed( int newSpeed )
{
	heroMoveSpeed = newSpeed;
	settingsChanged();
}

void SystemOptions::setMapScrollingSpeed( int newSpeed )
{
	mapScrollingSpeed = newSpeed;
	settingsChanged();
}

void SystemOptions::settingsChanged()
{
	CSaveFile settings(GVCMIDirs.UserPath + "/config/sysopts.bin");

	if(settings.sfile)
		settings << *this;
	else
		tlog1 << "Cannot save settings to config/sysopts.bin!\n";
}

void SystemOptions::apply()
{
	if(CGI->musich->getVolume() != musicVolume)
		CGI->musich->setVolume(musicVolume);
	if(CGI->soundh->getVolume() != soundVolume)
		CGI->soundh->setVolume(soundVolume);

	settingsChanged();
}

SystemOptions::SystemOptions()
{
	heroMoveSpeed = 2;
	mapScrollingSpeed = 2;
	musicVolume = 88;
	soundVolume = 88;

	printCellBorders = true;
	printStackRange = true;
	animSpeed = 2;
	printMouseShadow = true;
	showQueue = true;
}

void CPlayerInterface::eraseCurrentPathOf( const CGHeroInstance * ho )
{
	assert(vstd::contains(paths, ho));
	assert(ho == adventureInt->selection);

	paths.erase(ho);
	adventureInt->terrain.currentPath = NULL;
}

CGPath * CPlayerInterface::getAndVerifyPath(const CGHeroInstance * h)
{
	if(vstd::contains(paths,h)) //hero has assigned path
	{
		CGPath &path = paths[h];
		if(!path.nodes.size())
		{
			tlog3 << "Warning: empty path found...\n";
			paths.erase(h);
		}
		else
		{
			assert(h->getPosition(false) == path.startPos()); 
			//update the hero path in case of something has changed on map
			if(LOCPLINT->cb->getPath2(path.endPos(), path))
				return &path;
			else
				paths.erase(h);
		}
	}

	return NULL;
}

void CPlayerInterface::acceptTurn()
{
	waitWhileDialog();

	if(howManyPeople > 1)
		adventureInt->startTurn();

	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for(std::map<int,SDL_Surface*>::iterator i=graphics->heroWins.begin(); i!=graphics->heroWins.end();i++) //redraw hero infoboxes
		SDL_FreeSurface(i->second);
	graphics->heroWins.clear();
	std::vector <const CGHeroInstance *> hh = cb->getHeroesInfo(false);
	for(int i=0;i<hh.size();i++)
	{
		SDL_Surface * pom = infoWin(hh[i]);
		graphics->heroWins.insert(std::pair<int,SDL_Surface*>(hh[i]->subID,pom));
	}

	/* TODO: This isn't quite right. First day in game should play
	 * NEWDAY. And we don't play NEWMONTH. */
	int day = cb->getDate(1);
	if (day != 1)
		CGI->soundh->playSound(soundBase::newDay);
	else
		CGI->soundh->playSound(soundBase::newWeek);

	adventureInt->infoBar.newDay(day);

	//select first hero if available.
	//TODO: check if hero is slept
	if(wanderingHeroes.size())
		adventureInt->select(wanderingHeroes.front());
	else
		adventureInt->select(towns.front());

	adventureInt->showAll(screen);
}

void CPlayerInterface::tryDiggging(const CGHeroInstance *h)
{
	std::string hlp;
	if(h->movement < h->maxMovePoints(true))
		showInfoDialog(CGI->generaltexth->allTexts[56]); //"Digging for artifacts requires a whole day, try again tomorrow."
	else if(cb->getTileInfo(h->getPosition(false))->tertype == TerrainTile::water)
		showInfoDialog(CGI->generaltexth->allTexts[60]); //Try looking on land!
	else
	{
		CGI->mh->getTerrainDescr(h->getPosition(false), hlp, false);
		if(hlp.length())
			showInfoDialog(CGI->generaltexth->allTexts[97]); //Try searching on clear ground.
		else
			cb->dig(h);
	}
}

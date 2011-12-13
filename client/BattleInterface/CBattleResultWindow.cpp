#include "StdInc.h"
#include "CBattleResultWindow.h"

#include "CBattleInterface.h"
#include "../AdventureMapButton.h"
#include "../CGameInfo.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"
#include "../../CCallback.h"
#include "../CVideoHandler.h"
#include "../SDL_Extensions.h"
#include "../CBitmapHandler.h"

CBattleResultWindow::CBattleResultWindow(const BattleResult &br, const SDL_Rect & pos, CBattleInterface * _owner)
: owner(_owner)
{
	this->pos = pos;
	background = BitmapHandler::loadBitmap("CPRESULT.BMP", true);
	graphics->blueToPlayersAdv(background, owner->curInt->playerID);
	SDL_Surface * pom = SDL_ConvertSurface(background, screen->format, screen->flags);
	SDL_FreeSurface(background);
	background = pom;
	exit = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleResultWindow::bExitf,this), 384 + pos.x, 505 + pos.y, "iok6432.def", SDLK_RETURN);
	exit->borderColor = Colors::MetallicGold;
	exit->borderEnabled = true;

	if(br.winner==0) //attacker won
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 59, 124, FONT_SMALL, zwykly, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 408, 124, FONT_SMALL, zwykly, background);
	}
	else //if(br.winner==1)
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 59, 124, FONT_SMALL, zwykly, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 412, 124, FONT_SMALL, zwykly, background);
	}


	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[407], 232, 302, FONT_BIG, tytulowy, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[408], 232, 332, FONT_BIG, zwykly, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[409], 237, 428, FONT_BIG, zwykly, background);

	std::string attackerName, defenderName;

	if(owner->attackingHeroInstance) //a hero attacked
	{
		SDL_Rect temp_rect = genRect(64, 58, 21, 38);
		SDL_BlitSurface(graphics->portraitLarge[owner->attackingHeroInstance->portrait], NULL, background, &temp_rect);
		//setting attackerName
		attackerName = owner->attackingHeroInstance->name;
	}
	else //a monster attacked
	{
		int bestMonsterID = -1;
		ui32 bestPower = 0;
		for(TSlots::const_iterator it = owner->army1->Slots().begin(); it!=owner->army1->Slots().end(); ++it)
		{
			if(it->second->type->AIValue > bestPower)
			{
				bestPower = it->second->type->AIValue;
				bestMonsterID = it->second->type->idNumber;
			}
		}
		SDL_Rect temp_rect = genRect(64, 58, 21, 38);
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &temp_rect);
		//setting attackerName
		attackerName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}
	if(owner->defendingHeroInstance) //a hero defended
	{
		SDL_Rect temp_rect = genRect(64, 58, 392, 38);
		SDL_BlitSurface(graphics->portraitLarge[owner->defendingHeroInstance->portrait], NULL, background, &temp_rect);
		//setting defenderName
		defenderName = owner->defendingHeroInstance->name;
	}
	else //a monster defended
	{
		int bestMonsterID = -1;
		ui32 bestPower = 0;
		for(TSlots::const_iterator it = owner->army2->Slots().begin(); it!=owner->army2->Slots().end(); ++it)
		{
			if( it->second->type->AIValue > bestPower)
			{
				bestPower = it->second->type->AIValue;
				bestMonsterID = it->second->type->idNumber;
			}
		}
		SDL_Rect temp_rect = genRect(64, 58, 392, 38);
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &temp_rect);
		//setting defenderName
		defenderName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}

	//printing attacker and defender's names
	CSDL_Ext::printAt(attackerName, 89, 37, FONT_SMALL, zwykly, background);
	CSDL_Ext::printTo(defenderName, 381, 53, FONT_SMALL, zwykly, background);
	//printing casualities
	for(int step = 0; step < 2; ++step)
	{
		if(br.casualties[step].size()==0)
		{
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[523], 235, 360 + 97*step, FONT_SMALL, zwykly, background);
		}
		else
		{
			int xPos = 235 - (br.casualties[step].size()*32 + (br.casualties[step].size() - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + step*97;
			for(std::map<ui32,si32>::const_iterator it=br.casualties[step].begin(); it!=br.casualties[step].end(); ++it)
			{
				blitAt(graphics->smallImgs[it->first], xPos, yPos, background);
				std::ostringstream amount;
				amount<<it->second;
				CSDL_Ext::printAtMiddle(amount.str(), xPos+16, yPos + 42, FONT_SMALL, zwykly, background);
				xPos += 42;
			}
		}
	}
	//printing result description
	bool weAreAttacker = (owner->curInt->playerID == owner->attackingHeroInstance->tempOwner);
	if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
	{
		int text=-1;
		switch(br.result)
		{
		case 0: text = 304; break;
		case 1: text = 303; break;
		case 2: text = 302; break;
		}

		CCS->musich->playMusic(musicBase::winBattle);
		CCS->videoh->open(VIDEO_WIN);
		std::string str = CGI->generaltexth->allTexts[text];

		const CGHeroInstance * ourHero = weAreAttacker? owner->attackingHeroInstance : owner->defendingHeroInstance;
		if (ourHero)
		{
			str += CGI->generaltexth->allTexts[305];
			boost::algorithm::replace_first(str,"%s",ourHero->name);
			boost::algorithm::replace_first(str,"%d",boost::lexical_cast<std::string>(br.exp[weAreAttacker?0:1]));
		}
		CSDL_Ext::printAtMiddleWB(str, 235, 235, FONT_SMALL, 55, zwykly, background);
	}
	else // we lose
	{
		switch(br.result)
		{
		case 0: //normal victory
			{
				CCS->musich->playMusic(musicBase::loseCombat);
				CCS->videoh->open(VIDEO_LOSE_BATTLE_START);
				CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[311], 235, 235, FONT_SMALL, zwykly, background);
				break;
			}
		case 1: //flee
			{
				CCS->musich->playMusic(musicBase::retreatBattle);
				CCS->videoh->open(VIDEO_RETREAT_START);
				CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[310], 235, 235, FONT_SMALL, zwykly, background);
				break;
			}
		case 2: //surrender
			{
				CCS->musich->playMusic(musicBase::surrenderBattle);
				CCS->videoh->open(VIDEO_SURRENDER);
				CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[309], 235, 220, FONT_SMALL, zwykly, background);
				break;
			}
		}
	}
}

CBattleResultWindow::~CBattleResultWindow()
{
	SDL_FreeSurface(background);
}

void CBattleResultWindow::activate()
{
	owner->curInt->showingDialog->set(true);
	exit->activate();
}

void CBattleResultWindow::deactivate()
{
	exit->deactivate();
}

void CBattleResultWindow::show(SDL_Surface *to)
{
	//evaluating to
	if(!to)
		to = screen;

	CCS->videoh->update(107, 70, background, false, true);

	SDL_BlitSurface(background, NULL, to, &pos);
	exit->showAll(to);
}

void CBattleResultWindow::bExitf()
{
	if(LOCPLINT->cb->getStartInfo()->mode == StartInfo::DUEL)
	{
		std::exit(0);
	}

	CPlayerInterface * intTmp = owner->curInt;
	GH.popInts(2); //first - we; second - battle interface
	intTmp->showingDialog->setn(false);
	CCS->videoh->close();
}
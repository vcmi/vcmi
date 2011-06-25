#include "../stdafx.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CKingdomInterface.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "Graphics.h"
#include "CSpellWindow.h"
#include "CConfigHandler.h"
#include "CPlayerInterface.h"
#include "../global.h"
#include "../lib/CArtHandler.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CLodHandler.h"
#include "../lib/CObjectHandler.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/std/vector.hpp>
#include <cstdlib>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

#undef min

/*
 * CHeroWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface * screen;
using namespace boost::assign;

const boost::shared_ptr<BonusList> CHeroWithMaybePickedArtifact::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/) const
{
	boost::shared_ptr<BonusList> out(new BonusList);
	boost::shared_ptr<BonusList> heroBonuses = hero->getAllBonuses(selector, limit, hero);
	boost::shared_ptr<BonusList> bonusesFromPickedUpArtifact;

	CArtifactsOfHero::SCommonPart *cp = cww->artSets.size() ? cww->artSets.front()->commonInfo : NULL;
	if(cp && cp->src.art && cp->src.AOH && cp->src.AOH->getHero() == hero)
	{
		bonusesFromPickedUpArtifact = cp->src.art->getAllBonuses(selector, limit, hero);
	}
	else
		bonusesFromPickedUpArtifact = boost::shared_ptr<BonusList>(new BonusList);

	BOOST_FOREACH(Bonus *b, *bonusesFromPickedUpArtifact)
		*heroBonuses -= b;
	BOOST_FOREACH(Bonus *b, *heroBonuses)
		out->push_back(b);
	return out;
}

CHeroWithMaybePickedArtifact::CHeroWithMaybePickedArtifact(CWindowWithArtifacts *Cww, const CGHeroInstance *Hero)
	:  hero(Hero), cww(Cww)
{
}

void CHeroSwitcher::clickLeft(tribool down, bool previousState)
{
	if(!down)
	{
		const CGHeroInstance * buf = LOCPLINT->getWHero(id);
		if(!buf)
			return;
		GH.popIntTotally(getOwner());
		GH.pushInt(new CHeroWindow(buf));
	}
}

CHeroWindow * CHeroSwitcher::getOwner()
{
	return dynamic_cast<CHeroWindow*>(parent);
}

CHeroSwitcher::CHeroSwitcher(int serial)
{
	pos = Rect(612, 87 + serial * 54, 48, 32)  +  pos;
	id = serial;
	used = LCLICK;
}

CHeroWindow::CHeroWindow(const CGHeroInstance *hero)
	:  heroWArt(this, hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	garr = NULL;
	curHero = hero;
	player = LOCPLINT->playerID;//hero->tempOwner;

	background = new CPicture("HeroScr4.BMP");
	background->colorizeAndConvert(player);
	pos = background->center();


	//artifs = new CArtifactsOfHero(pos.topLeft(), true);
	ourBar = new CGStatusBar(7, 559, "ADROLLVR.bmp", 660); // new CStatusBar(pos.x+72, pos.y+567, "ADROLLVR.bmp", 660);

	quitButton = new AdventureMapButton(CGI->generaltexth->heroscrn[17], std::string(),boost::bind(&CHeroWindow::quit,this), 609, 516, "hsbtns.def", SDLK_RETURN);
	dismissButton = new AdventureMapButton(std::string(), CGI->generaltexth->heroscrn[28], boost::bind(&CHeroWindow::dismissCurrent,this), 454, 429, "hsbtns2.def", SDLK_d);
	questlogButton = new AdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CHeroWindow::questlog,this), 314, 429, "hsbtns4.def", SDLK_q);

	formations = new CHighlightableButtonsGroup(0);
	formations->addButton(map_list_of(0,CGI->generaltexth->heroscrn[23]),CGI->generaltexth->heroscrn[29], "hsbtns6.def", 481, 483, 0, 0, SDLK_t);
	formations->addButton(map_list_of(0,CGI->generaltexth->heroscrn[24]),CGI->generaltexth->heroscrn[30], "hsbtns7.def", 481, 519, 1, 0, SDLK_l);

	tacticsButton = new CHighlightableButton(0, 0, map_list_of(0,CGI->generaltexth->heroscrn[26])(3,CGI->generaltexth->heroscrn[25]), CGI->generaltexth->heroscrn[31], false, "hsbtns8.def", NULL, 539, 483, SDLK_b);


	//right list of heroes
	for(int g=0; g<8; ++g)
		heroListMi.push_back(new CHeroSwitcher(g));
	
	flags = CDefHandler::giveDefEss("CREST58.DEF");

	//areas
	portraitArea = new LRClickableAreaWText(Rect(18, 18, 58, 64));

	for(int v=0; v<PRIMARY_SKILLS; ++v)
	{
		LRClickableAreaWTextComp *area = new LRClickableAreaWTextComp(Rect(30 + 70*v, 109, 42, 64), SComponent::primskill);
		area->text = CGI->generaltexth->arraytxt[2+v];
		area->type = v;
		area->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % CGI->generaltexth->primarySkillNames[v]);
		primSkillAreas.push_back(area);
	}

	specArea = new LRClickableAreaWText(Rect(18, 180, 136, 42), CGI->generaltexth->heroscrn[27]);
	expArea = new LRClickableAreaWText(Rect(18, 228, 136, 42), CGI->generaltexth->heroscrn[9]);
	morale = new MoraleLuckBox(true, Rect(175,179,53,45));
	luck = new MoraleLuckBox(false, Rect(233,179,53,45));
	spellPointsArea = new LRClickableAreaWText(Rect(162,228, 136, 42), CGI->generaltexth->heroscrn[22]);

	for(int i = 0; i < std::min<size_t>(hero->secSkills.size(), 8u); ++i)
	{
		Rect r = Rect(i%2 == 0  ?  18  :  162,  276 + 48 * (i/2),  136,  42);
		secSkillAreas.push_back(new LRClickableAreaWTextComp(r, SComponent::secskill));
	}

	//////////////////////////////////////////////////////////////////////////???????????????
// 	pos.x += 65;
// 	pos.y += 8;
// 	
	//primary skills & exp and mana
	new CPicture(graphics->pskillsm->ourImages[0].bitmap, 32, 111, false);
	new CPicture(graphics->pskillsm->ourImages[1].bitmap, 102, 111, false);
	new CPicture(graphics->pskillsm->ourImages[2].bitmap, 172, 111, false);
	new CPicture(graphics->pskillsm->ourImages[5].bitmap, 242, 111, false);
	new CPicture(graphics->pskillsm->ourImages[4].bitmap, 20, 230, false);
	new CPicture(graphics->pskillsm->ourImages[3].bitmap, 162, 230, false);

	update(hero);
}

CHeroWindow::~CHeroWindow()
{
	delete flags;	
	//SDL_FreeSurface(curBack);
	//curBack = NULL;
	curHero = NULL;

	//artifs->dispose();
}

void CHeroWindow::update(const CGHeroInstance * hero, bool redrawNeeded /*= false*/)
{	
	if(!hero) //something strange... no hero? it shouldn't happen
	{
		tlog1 << "Set NULL hero? no way...\n";
		return;
	}

	assert(hero == curHero);
	//assert(hero->tempOwner == LOCPLINT->playerID || hero->tempOwner == NEUTRAL_PLAYER); //for now we won't show hero windows for non-our heroes
	
	specArea->text = CGI->generaltexth->hTxts[curHero->subID].longBonus;

	tacticsButton->callback.clear();
	tacticsButton->callback2.clear();

	dismissButton->hoverTexts[0] = boost::str(boost::format(CGI->generaltexth->heroscrn[16]) % curHero->name % curHero->type->heroClass->name);
	portraitArea->hoverText = boost::str(boost::format(CGI->generaltexth->allTexts[15]) % curHero->name % curHero->type->heroClass->name);
	portraitArea->text = curHero->getBiography();

	{
		AdventureMapButton * split = NULL;
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		if(!garr)
		{
			garr = new CGarrisonInt(15, 485, 8, Point(), background->bg, Point(15,485), curHero);
			{
				BLOCK_CAPTURING;
				split = new AdventureMapButton(CGI->generaltexth->allTexts[256], CGI->generaltexth->heroscrn[32], boost::bind(&CGarrisonInt::splitClick,garr), pos.x + 539, pos.y + 519, "hsbtns9.def", false, NULL, false); //deleted by garrison destructor
				boost::algorithm::replace_first(split->hoverTexts[0],"%s",CGI->generaltexth->allTexts[43]);
			}
			garr->addSplitBtn(split);
		}
		if(!artSets.size())
		{
			CArtifactsOfHero *arts = new CArtifactsOfHero(Point(-65, -8), true);
			arts->setHero(curHero);
			artSets.push_back(arts);
		}
	}


	//primary skills support
	for(size_t g=0; g<primSkillAreas.size(); ++g)
	{
		primSkillAreas[g]->bonusValue = heroWArt.getPrimSkillLevel(g);
	}

	//secondary skills support
	for(size_t g=0; g< secSkillAreas.size(); ++g)
	{
		int skill = curHero->secSkills[g].first,
			level = curHero->getSecSkillLevel(static_cast<CGHeroInstance::SecondarySkill>(curHero->secSkills[g].first));
		secSkillAreas[g]->type = skill;
		secSkillAreas[g]->bonusValue = level;
		secSkillAreas[g]->text = CGI->generaltexth->skillInfoTexts[skill][level-1];
		secSkillAreas[g]->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[21]) % CGI->generaltexth->levels[level-1] % CGI->generaltexth->skillName[skill]);
	}

	//printing experience - original format does not support ui64
	expArea->text = CGI->generaltexth->allTexts[2];
	boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(curHero->level));
	boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(curHero->level+1)));
	boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(curHero->exp));

	//printing spell points, boost::format can't be used due to locale issues
	spellPointsArea->text = CGI->generaltexth->allTexts[205];
	boost::replace_first(spellPointsArea->text, "%s", boost::lexical_cast<std::string>(curHero->name));
	boost::replace_first(spellPointsArea->text, "%d", boost::lexical_cast<std::string>(curHero->mana));
	boost::replace_first(spellPointsArea->text, "%d", boost::lexical_cast<std::string>(heroWArt.manaLimit()));

	//if we have exchange window with this curHero open
	bool noDismiss=false;
	BOOST_FOREACH(IShowActivable *isa, GH.listInt)
	{
		if(CExchangeWindow * cew = dynamic_cast<CExchangeWindow*>(isa))
			for(int g=0; g < ARRAY_COUNT(cew->heroInst); ++g)
				if(cew->heroInst[g] == curHero)
					noDismiss = true;

		if (dynamic_cast<CKingdomInterface*>(isa))
			noDismiss = true;
	}
	dismissButton->block(!!curHero->visitedTown || noDismiss);

	if(curHero->getSecSkillLevel(CGHeroInstance::TACTICS) == 0)
		tacticsButton->block(true);
	else
	{
		tacticsButton->block(false);
		tacticsButton->callback = vstd::assigno(curHero->tacticFormationEnabled,true);
		tacticsButton->callback2 = vstd::assigno(curHero->tacticFormationEnabled,false);
	}

	//setting formations
	formations->onChange = 0;
	formations->select(curHero->formation,true);
	formations->onChange = boost::bind(&CCallback::setFormation, LOCPLINT->cb, curHero, _1);

	morale->set(&heroWArt);
	luck->set(&heroWArt);

	if(redrawNeeded)
		redraw();
}

void CHeroWindow::quit()
{
	GH.popIntTotally(this);
}

void CHeroWindow::dismissCurrent()
{
	CFunctionList<void()> ony = boost::bind(&CHeroWindow::quit,this);
	ony += boost::bind(&CCallback::dismissHero,LOCPLINT->cb,curHero);
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[22],std::vector<SComponent*>(), ony, 0, false);
}

void CHeroWindow::questlog()
{
}

void CHeroWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	//blitting portrait
	blitAtLoc(graphics->portraitLarge[curHero->portrait], 19, 19, to);
	 
	//printing hero's name
	printAtMiddleLoc(curHero->name, 190, 38, FONT_BIG, tytulowy, to);
	 
	//printing hero's level
	std::string secondLine= CGI->generaltexth->allTexts[342];
	boost::algorithm::replace_first(secondLine,"%d",boost::lexical_cast<std::string>(curHero->level));
	boost::algorithm::replace_first(secondLine,"%s",curHero->type->heroClass->name);
	printAtMiddleLoc(secondLine, 190, 65, FONT_MEDIUM, zwykly, to);
	 	
	//primary skills names
	printAtMiddleLoc(CGI->generaltexth->jktexts[1], 52, 99, FONT_SMALL, tytulowy, to);
	printAtMiddleLoc(CGI->generaltexth->jktexts[2], 123, 99, FONT_SMALL, tytulowy, to);
	printAtMiddleLoc(CGI->generaltexth->jktexts[3], 193, 99, FONT_SMALL, tytulowy, to);
	printAtMiddleLoc(CGI->generaltexth->jktexts[4], 262, 99, FONT_SMALL, tytulowy, to);
	 
	//dismiss / quest log
	std::vector<std::string> toPrin = CMessage::breakText(CGI->generaltexth->jktexts[8]);
	if(toPrin.size()==1)
	{
	 	printAtLoc(toPrin[0], 372, 439, FONT_SMALL, zwykly, to);
	}
	else
	{
	 	printAtLoc(toPrin[0], 372, 430, FONT_SMALL, zwykly, to);
	 	printAtLoc(toPrin[1], 372, 446, FONT_SMALL, zwykly, to);
	}
	 
	toPrin = CMessage::breakText(CGI->generaltexth->jktexts[9]);
	if(toPrin.size()==1)
	{
	 	printAtLoc(toPrin[0], 512, 439, FONT_SMALL, zwykly, to);
	}
	else
	{
	 	printAtLoc(toPrin[0], 512, 430, FONT_SMALL, zwykly, to);
	 	printAtLoc(toPrin[1], 512, 446, FONT_SMALL, zwykly, to);
	}
	 
	//printing primary skills' amounts
	for(int m=0; m<4; ++m)
	{
	 	std::ostringstream primarySkill;
	 	primarySkill << primSkillAreas[m]->bonusValue;
	 	printAtMiddleLoc(primarySkill.str(), 53 + 70 * m, 166, FONT_SMALL, zwykly, to);
	}
	 
	blitAtLoc(flags->ourImages[player].bitmap, 606, 8, to);
	 
	//hero list blitting
	 
	for(int slotPos=0, g=0; g<LOCPLINT->wanderingHeroes.size(); ++g)
	{
	 	const CGHeroInstance * cur = LOCPLINT->wanderingHeroes[g];
	 	if (cur->inTownGarrison)
	 		// Only display heroes that are not in garrison
	 		continue;
	 
	 	blitAtLoc(graphics->portraitSmall[cur->portrait], 611, 87+slotPos*54, to);
	 	//printing yellow border
	 	if(cur->name == curHero->name)
	 	{
	 		for(int f=0; f<graphics->portraitSmall[cur->portrait]->w; ++f)
	 		{
	 			for(int h=0; h<graphics->portraitSmall[cur->portrait]->h; ++h)
	 				if(f==0 || h==0 || f==graphics->portraitSmall[cur->portrait]->w-1 || h==graphics->portraitSmall[cur->portrait]->h-1)
	 					CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + 611+f, pos.y + 87+slotPos*54+h, 240, 220, 120);
	 		}
	 	}
	 
	 	slotPos ++;
	}
	 
	//secondary skills
	for(size_t v=0; v<std::min(secSkillAreas.size(), curHero->secSkills.size()); ++v)
	{
	 	blitAtLoc(graphics->abils44->ourImages[curHero->secSkills[v].first*3+3+curHero->secSkills[v].second-1].bitmap, v%2 ? 161 : 18, 276 + 48 * (v/2), to);
	 	printAtLoc(CGI->generaltexth->levels[curHero->secSkills[v].second-1], v%2 ? 212 : 68, 280 + 48 * (v/2), FONT_SMALL, zwykly, to);
	 	printAtLoc(CGI->generaltexth->skillName[curHero->secSkills[v].first], v%2 ? 212 : 68, 300 + 48 * (v/2), FONT_SMALL, zwykly, to);
	}
	 
	//printing special ability
	blitAtLoc(graphics->un44->ourImages[curHero->subID].bitmap, 18, 180, to);
	printAtLoc(CGI->generaltexth->jktexts[5].substr(1, CGI->generaltexth->jktexts[5].size()-2), 69, 183, FONT_SMALL, tytulowy, to);
	printAtLoc(CGI->generaltexth->hTxts[curHero->subID].bonusName, 69, 205, FONT_SMALL, zwykly, to);
	 
	//printing necessery texts
	printAtLoc(CGI->generaltexth->jktexts[6].substr(1, CGI->generaltexth->jktexts[6].size()-2), 69, 232, FONT_SMALL, tytulowy, to);
	std::ostringstream expstr;
	expstr << curHero->exp;
	printAtLoc(expstr.str(), 68, 252, FONT_SMALL, zwykly, to);
	printAtLoc(CGI->generaltexth->jktexts[7].substr(1, CGI->generaltexth->jktexts[7].size()-2), 213, 232, FONT_SMALL, tytulowy, to);
	std::ostringstream manastr;
	manastr << curHero->mana << '/' << heroWArt.manaLimit();
	printAtLoc(manastr.str(), 211, 252, FONT_SMALL, zwykly, to);
}
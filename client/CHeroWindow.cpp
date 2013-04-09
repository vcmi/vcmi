#include "StdInc.h"

#include "CAnimation.h"
#include "CAdvmapInterface.h"
#include "../CCallback.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CKingdomInterface.h"
#include "CCreatureWindow.h"
#include "SDL.h"
#include "gui/SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "Graphics.h"
#include "CSpellWindow.h"
#include "../lib/CConfigHandler.h"
#include "CPlayerInterface.h"

#include "../lib/CArtHandler.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/NetPacks.h"

#include "gui/CGuiHandler.h"
#include "gui/CIntObjectClasses.h"
#include "CMT.h"

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

using namespace boost::assign;

const TBonusListPtr CHeroWithMaybePickedArtifact::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/, const std::string &cachingStr /*= ""*/) const
{
	TBonusListPtr out(new BonusList);
	TBonusListPtr heroBonuses = hero->getAllBonuses(selector, limit, hero);
	TBonusListPtr bonusesFromPickedUpArtifact;

	CArtifactsOfHero::SCommonPart *cp = cww->artSets.size() ? cww->artSets.front()->commonInfo : NULL;
	if(cp && cp->src.art && cp->src.valid() && cp->src.AOH && cp->src.AOH->getHero() == hero)
	{
		bonusesFromPickedUpArtifact = cp->src.art->getAllBonuses(selector, limit, hero);
	}
	else
		bonusesFromPickedUpArtifact = TBonusListPtr(new BonusList);

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
		const CGHeroInstance * buf = hero;
		GH.popIntTotally(parent);
		GH.pushInt(new CHeroWindow(buf));
	}
}

CHeroSwitcher::CHeroSwitcher(Point _pos, const CGHeroInstance * _hero):
	hero(_hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos += _pos;
	addUsedEvents(LCLICK);

	image = new CAnimImage("PortraitsSmall", hero->portrait);
	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

CHeroWindow::CHeroWindow(const CGHeroInstance *hero):
    CWindowObject(PLAYER_COLORED, "HeroScr4"),
	heroWArt(this, hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	garr = nullptr;
	curHero = hero;
	listSelection = nullptr;

	new CAnimImage("CREST58", LOCPLINT->playerID.getNum(), 0, 606, 8);

	//artifs = new CArtifactsOfHero(pos.topLeft(), true);
	ourBar = new CGStatusBar(7, 559, "ADROLLVR.bmp", 660); // new CStatusBar(pos.x+72, pos.y+567, "ADROLLVR.bmp", 660);

	quitButton = new CAdventureMapButton(CGI->generaltexth->heroscrn[17], std::string(),boost::bind(&CHeroWindow::close,this), 609, 516, "hsbtns.def", SDLK_RETURN);
	quitButton->assignedKeys.insert(SDLK_ESCAPE);
	dismissButton = new CAdventureMapButton(std::string(), CGI->generaltexth->heroscrn[28], boost::bind(&CHeroWindow::dismissCurrent,this), 454, 429, "hsbtns2.def", SDLK_d);
	questlogButton = new CAdventureMapButton(CGI->generaltexth->heroscrn[0], std::string(), boost::bind(&CHeroWindow::questlog,this), 314, 429, "hsbtns4.def", SDLK_q);

	formations = new CHighlightableButtonsGroup(0);
	formations->addButton(map_list_of(0,CGI->generaltexth->heroscrn[23]),CGI->generaltexth->heroscrn[29], "hsbtns6.def", 481, 483, 0, 0, SDLK_t);
	formations->addButton(map_list_of(0,CGI->generaltexth->heroscrn[24]),CGI->generaltexth->heroscrn[30], "hsbtns7.def", 481, 519, 1, 0, SDLK_l);

	tacticsButton = new CHighlightableButton(0, 0, map_list_of(0,CGI->generaltexth->heroscrn[26])(3,CGI->generaltexth->heroscrn[25]), CGI->generaltexth->heroscrn[31], false, "hsbtns8.def", NULL, 539, 483, SDLK_b);

	if (hero->commander)
	{
		commanderButton = new CAdventureMapButton ("Commander", "Commander info", boost::bind(&CHeroWindow::commanderWindow, this), 317, 18, "chftke.def", SDLK_c, NULL, false);
	}


	//right list of heroes
	for(int i=0; i < std::min(LOCPLINT->cb->howManyHeroes(false), 8); i++)
		heroList.push_back(new CHeroSwitcher(Point(612, 87 + i * 54), LOCPLINT->cb->getHeroBySerial(i, false)));

	//areas
	portraitArea = new LRClickableAreaWText(Rect(18, 18, 58, 64));
	portraitImage = new CAnimImage("PortraitsLarge", 0, 0, 19, 19);

	for(int v=0; v<GameConstants::PRIMARY_SKILLS; ++v)
	{
		LRClickableAreaWTextComp *area = new LRClickableAreaWTextComp(Rect(30 + 70*v, 109, 42, 64), CComponent::primskill);
		area->text = CGI->generaltexth->arraytxt[2+v];
		area->type = v;
		area->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % CGI->generaltexth->primarySkillNames[v]);
		primSkillAreas.push_back(area);
	}

	specImage = new CAnimImage("UN44", 0, 0, 18, 180);

	specArea = new LRClickableAreaWText(Rect(18, 180, 136, 42), CGI->generaltexth->heroscrn[27]);
	expArea = new LRClickableAreaWText(Rect(18, 228, 136, 42), CGI->generaltexth->heroscrn[9]);
	morale = new MoraleLuckBox(true, Rect(175,179,53,45));
	luck = new MoraleLuckBox(false, Rect(233,179,53,45));
	spellPointsArea = new LRClickableAreaWText(Rect(162,228, 136, 42), CGI->generaltexth->heroscrn[22]);

	for(int i = 0; i < std::min<size_t>(hero->secSkills.size(), 8u); ++i)
	{
		Rect r = Rect(i%2 == 0  ?  18  :  162,  276 + 48 * (i/2),  136,  42);
		secSkillAreas.push_back(new LRClickableAreaWTextComp(r, CComponent::secskill));
		secSkillImages.push_back(new CAnimImage("SECSKILL", 0, 0, r.x, r.y));
	}

	//dismiss / quest log
	new CTextBox(CGI->generaltexth->jktexts[8], Rect(370, 430, 65, 35), 0, FONT_SMALL, TOPLEFT, Colors::WHITE);
	new CTextBox(CGI->generaltexth->jktexts[9], Rect(510, 430, 65, 35), 0, FONT_SMALL, TOPLEFT, Colors::WHITE);

	//////////////////////////////////////////////////////////////////////////???????????????

	//primary skills & exp and mana
	new CAnimImage("PSKIL42", 0, 0, 32, 111, false);
	new CAnimImage("PSKIL42", 1, 0, 102, 111, false);
	new CAnimImage("PSKIL42", 2, 0, 172, 111, false);
	new CAnimImage("PSKIL42", 3, 0, 162, 230, false);
	new CAnimImage("PSKIL42", 4, 0, 20, 230, false);
	new CAnimImage("PSKIL42", 5, 0, 242, 111, false);

	update(hero);
}

void CHeroWindow::update(const CGHeroInstance * hero, bool redrawNeeded /*= false*/)
{	
	if(!hero) //something strange... no hero? it shouldn't happen
	{
        logGlobal->errorStream() << "Set NULL hero? no way...";
		return;
	}

	assert(hero == curHero);

	specArea->text = curHero->type->specDescr;
	specImage->setFrame(curHero->type->imageIndex);

	tacticsButton->callback.clear();
	tacticsButton->callback2.clear();

	dismissButton->hoverTexts[0] = boost::str(boost::format(CGI->generaltexth->heroscrn[16]) % curHero->name % curHero->type->heroClass->name);
	portraitArea->hoverText = boost::str(boost::format(CGI->generaltexth->allTexts[15]) % curHero->name % curHero->type->heroClass->name);
	portraitArea->text = curHero->getBiography();
	portraitImage->setFrame(curHero->portrait);

	{
		CAdventureMapButton * split = NULL;
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		if(!garr)
		{
			garr = new CGarrisonInt(15, 485, 8, Point(), background->bg, Point(15,485), curHero);
			split = new CAdventureMapButton(CGI->generaltexth->allTexts[256], CGI->generaltexth->heroscrn[32],
					boost::bind(&CGarrisonInt::splitClick,garr), 539, 519, "hsbtns9.def", false, NULL, false); //deleted by garrison destructor
			boost::algorithm::replace_first(split->hoverTexts[0],"%s",CGI->generaltexth->allTexts[43]);

			garr->addSplitBtn(split);
		}
		if(!artSets.size())
		{
			CArtifactsOfHero *arts = new CArtifactsOfHero(Point(-65, -8), true);
			arts->setHero(curHero);
			artSets.push_back(arts);
		}

		int serial = LOCPLINT->cb->getHeroSerial(curHero, false);

		vstd::clear_pointer(listSelection);
		if (serial >= 0)
			listSelection = new CPicture("HPSYYY", 612, 33 + serial * 54);
	}

	//primary skills support
	for(size_t g=0; g<primSkillAreas.size(); ++g)
	{
		primSkillAreas[g]->bonusValue = heroWArt.getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(g));
	}

	//secondary skills support
	for(size_t g=0; g< secSkillAreas.size(); ++g)
	{
		int skill = curHero->secSkills[g].first,
			level = curHero->getSecSkillLevel(SecondarySkill(curHero->secSkills[g].first));
		secSkillAreas[g]->type = skill;
		secSkillAreas[g]->bonusValue = level;
		secSkillAreas[g]->text = CGI->generaltexth->skillInfoTexts[skill][level-1];
		secSkillAreas[g]->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[21]) % CGI->generaltexth->levels[level-1] % CGI->generaltexth->skillName[skill]);
		secSkillImages[g]->setFrame(skill*3 + level + 2);
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
	BOOST_FOREACH(IShowActivatable *isa, GH.listInt)
	{
		if(CExchangeWindow * cew = dynamic_cast<CExchangeWindow*>(isa))
			for(int g=0; g < ARRAY_COUNT(cew->heroInst); ++g)
				if(cew->heroInst[g] == curHero)
					noDismiss = true;

		if (dynamic_cast<CKingdomInterface*>(isa))
			noDismiss = true;
	}
	dismissButton->block(!!curHero->visitedTown || noDismiss);

	if(curHero->getSecSkillLevel(SecondarySkill::TACTICS) == 0)
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

void CHeroWindow::dismissCurrent()
{
	CFunctionList<void()> ony = boost::bind(&CHeroWindow::close,this);
	ony += boost::bind(&CCallback::dismissHero,LOCPLINT->cb,curHero);
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[22], ony, 0, false);
}

void CHeroWindow::questlog()
{
}

void CHeroWindow::commanderWindow()
{
	//TODO: allow equipping commander artifacts by drag / drop
	//bool artSelected = false;
	const CArtifactsOfHero::SCommonPart *commonInfo = artSets.front()->commonInfo;

	if (const CArtifactInstance *art = commonInfo->src.art)
	{
		const CGHeroInstance *srcHero = commonInfo->src.AOH->getHero();
		//artSelected = true;
		ArtifactPosition freeSlot = art->firstAvailableSlot (curHero->commander);
		if (freeSlot < ArtifactPosition::COMMANDER_AFTER_LAST) //we don't want to put it in commander's backpack!
		{
			ArtifactLocation src (srcHero, commonInfo->src.slotID);
			ArtifactLocation dst (curHero->commander.get(), freeSlot);

			if (art->canBePutAt(dst, true))
			{	//equip clicked stack
				if(dst.getArt())
				{
					LOCPLINT->cb->swapArtifacts (dst, ArtifactLocation(srcHero, dst.getArt()->firstBackpackSlot(srcHero)));
				}
				LOCPLINT->cb->swapArtifacts(src, dst);
			}
		}
	}
	else
		GH.pushInt(new CCreatureWindow (curHero->commander));

}

void CHeroWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	 
	//printing hero's name
	printAtMiddleLoc(curHero->name, 190, 38, FONT_BIG, Colors::YELLOW, to);
	 
	//printing hero's level
	std::string secondLine= CGI->generaltexth->allTexts[342];
	boost::algorithm::replace_first(secondLine,"%d",boost::lexical_cast<std::string>(curHero->level));
	boost::algorithm::replace_first(secondLine,"%s",curHero->type->heroClass->name);
	printAtMiddleLoc(secondLine, 190, 65, FONT_MEDIUM, Colors::WHITE, to);
	 	
	//primary skills names
	printAtMiddleLoc(CGI->generaltexth->jktexts[1], 52, 99, FONT_SMALL, Colors::YELLOW, to);
	printAtMiddleLoc(CGI->generaltexth->jktexts[2], 123, 99, FONT_SMALL, Colors::YELLOW, to);
	printAtMiddleLoc(CGI->generaltexth->jktexts[3], 193, 99, FONT_SMALL, Colors::YELLOW, to);
	printAtMiddleLoc(CGI->generaltexth->jktexts[4], 262, 99, FONT_SMALL, Colors::YELLOW, to);

	//printing primary skills' amounts
	for(int m=0; m<4; ++m)
	{
	 	std::ostringstream primarySkill;
	 	primarySkill << primSkillAreas[m]->bonusValue;
	 	printAtMiddleLoc(primarySkill.str(), 53 + 70 * m, 166, FONT_SMALL, Colors::WHITE, to);
	}
	 
	//secondary skills
	for(size_t v=0; v<std::min(secSkillAreas.size(), curHero->secSkills.size()); ++v)
	{
	 	printAtLoc(CGI->generaltexth->levels[curHero->secSkills[v].second-1], (v%2) ? 212 : 68, 280 + 48 * (v/2), FONT_SMALL, Colors::WHITE, to);
	 	printAtLoc(CGI->generaltexth->skillName[curHero->secSkills[v].first], (v%2) ? 212 : 68, 300 + 48 * (v/2), FONT_SMALL, Colors::WHITE, to);
	}
	 
	//printing special ability
	printAtLoc(CGI->generaltexth->jktexts[5].substr(1, CGI->generaltexth->jktexts[5].size()-2), 69, 183, FONT_SMALL, Colors::YELLOW, to);
	printAtLoc(curHero->type->specName, 69, 205, FONT_SMALL, Colors::WHITE, to);
	 
	//printing necessery texts
	printAtLoc(CGI->generaltexth->jktexts[6].substr(1, CGI->generaltexth->jktexts[6].size()-2), 69, 232, FONT_SMALL, Colors::YELLOW, to);
	std::ostringstream expstr;
	expstr << curHero->exp;
	printAtLoc(expstr.str(), 68, 252, FONT_SMALL, Colors::WHITE, to);
	printAtLoc(CGI->generaltexth->jktexts[7].substr(1, CGI->generaltexth->jktexts[7].size()-2), 213, 232, FONT_SMALL, Colors::YELLOW, to);
	std::ostringstream manastr;
	manastr << curHero->mana << '/' << heroWArt.manaLimit();
	printAtLoc(manastr.str(), 211, 252, FONT_SMALL, Colors::WHITE, to);
}

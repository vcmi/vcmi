#include "StdInc.h"
#include "CSpellWindow.h"

#include "CAdvmapInterface.h"
#include "GUIClasses.h"
#include "InfoWindows.h"

#include "../CBitmapHandler.h"
#include "../CDefHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"

#include "../battle/CBattleInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"

#include "../../CCallback.h"

#include "../../lib/BattleState.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"

/*
 * CSpellWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

SpellbookInteractiveArea::SpellbookInteractiveArea(const SDL_Rect & myRect, std::function<void()> funcL,
	const std::string & textR, std::function<void()> funcHon, std::function<void()> funcHoff, CPlayerInterface * _myInt)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos = myRect;
	onLeft = funcL;
	textOnRclick = textR;
	onHoverOn = funcHon;
	onHoverOff = funcHoff;
	myInt = _myInt;
}

void SpellbookInteractiveArea::clickLeft(tribool down, bool previousState)
{
	if(!down)
	{
		onLeft();
	}
}

void SpellbookInteractiveArea::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(textOnRclick, down);
}

void SpellbookInteractiveArea::hover(bool on)
{
	//Hoverable::hover(on);
	if(on)
	{
		onHoverOn();
	}
	else
	{
		onHoverOff();
	}
}

CSpellWindow::CSpellWindow(const SDL_Rect &, const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells):
    CWindowObject(PLAYER_COLORED, "SpelBack"),
	battleSpellsOnly(openOnBattleSpells),
	selectedTab(4),
	currentPage(0),
	myHero(_myHero),
	myInt(_myInt)
{
	//initializing castable spells
	for(ui32 v=0; v<CGI->spellh->objects.size(); ++v)
	{
		if( !CGI->spellh->objects[v]->creatureAbility && myHero->canCastThisSpell(CGI->spellh->objects[v]) )
			mySpells.insert(SpellID(v));
	}

	//initializing sizes of spellbook's parts
	for(auto & elem : sitesPerTabAdv)
		elem = 0;
	for(auto & elem : sitesPerTabBattle)
		elem = 0;

	for(auto g : mySpells)
	{
		const CSpell &s = *CGI->spellh->objects[g];
		Uint8 *sitesPerOurTab = s.combatSpell ? sitesPerTabBattle : sitesPerTabAdv;

		++sitesPerOurTab[4];

		if(s.air)
			++sitesPerOurTab[0];
		if(s.fire)
			++sitesPerOurTab[1];
		if(s.water)
			++sitesPerOurTab[2];
		if(s.earth)
			++sitesPerOurTab[3];
	}
	if(sitesPerTabAdv[4] % 12 == 0)
		sitesPerTabAdv[4]/=12;
	else
		sitesPerTabAdv[4] = sitesPerTabAdv[4]/12 + 1;

	for(int v=0; v<4; ++v)
	{
		if(sitesPerTabAdv[v] <= 10)
			sitesPerTabAdv[v] = 1;
		else
		{
			if((sitesPerTabAdv[v] - 10) % 12 == 0)
				sitesPerTabAdv[v] = (sitesPerTabAdv[v] - 10) / 12 + 1;
			else
				sitesPerTabAdv[v] = (sitesPerTabAdv[v] - 10) / 12 + 2;
		}
	}

	if(sitesPerTabBattle[4] % 12 == 0)
		sitesPerTabBattle[4]/=12;
	else
		sitesPerTabBattle[4] = sitesPerTabBattle[4]/12 + 1;

	for(int v=0; v<4; ++v)
	{
		if(sitesPerTabBattle[v] <= 10)
			sitesPerTabBattle[v] = 1;
		else
		{
			if((sitesPerTabBattle[v] - 10) % 12 == 0)
				sitesPerTabBattle[v] = (sitesPerTabBattle[v] - 10) / 12 + 1;
			else
				sitesPerTabBattle[v] = (sitesPerTabBattle[v] - 10) / 12 + 2;
		}
	}


	//numbers of spell pages computed

	leftCorner = BitmapHandler::loadBitmap("SpelTrnL.bmp", true);
	rightCorner = BitmapHandler::loadBitmap("SpelTrnR.bmp", true);
	
	spells = new CAnimation("Spells.def");
		
	spellTab = CDefHandler::giveDef("SpelTab.def");
	schools = CDefHandler::giveDef("Schools.def");
	schoolBorders[0] = CDefHandler::giveDef("SplevA.def");
	schoolBorders[1] = CDefHandler::giveDef("SplevF.def");
	schoolBorders[2] = CDefHandler::giveDef("SplevW.def");
	schoolBorders[3] = CDefHandler::giveDef("SplevE.def");



	statusBar = new CGStatusBar(7 + pos.x, 569 + pos.y, "Spelroll.bmp");
	SDL_Rect temp_rect = genRect(45, 35, 479 + pos.x, 405 + pos.y);
	exitBtn = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::fexitb, this), CGI->generaltexth->zelp[460].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[460].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(45, 35, 221 + pos.x, 405 + pos.y);
	battleSpells = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::fbattleSpellsb, this), CGI->generaltexth->zelp[453].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[453].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(45, 35, 355 + pos.x, 405 + pos.y);
	adventureSpells = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::fadvSpellsb, this), CGI->generaltexth->zelp[452].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[452].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(45, 35, 418 + pos.x, 405 + pos.y);
	manaPoints = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::fmanaPtsb, this), CGI->generaltexth->zelp[459].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[459].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);

	temp_rect = genRect(36, 56, 549 + pos.x, 94 + pos.y);
	selectSpellsA = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 0), CGI->generaltexth->zelp[454].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[454].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 151 + pos.y);
	selectSpellsE = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 3), CGI->generaltexth->zelp[457].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[457].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 210 + pos.y);
	selectSpellsF = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 1), CGI->generaltexth->zelp[455].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[455].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 270 + pos.y);
	selectSpellsW = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 2), CGI->generaltexth->zelp[456].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[456].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 330 + pos.y);
	selectSpellsAll = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 4), CGI->generaltexth->zelp[458].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[458].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);

	temp_rect = genRect(leftCorner->h, leftCorner->w, 97 + pos.x, 77 + pos.y);
	lCorner = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::fLcornerb, this), CGI->generaltexth->zelp[450].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[450].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(rightCorner->h, rightCorner->w, 487 + pos.x, 72 + pos.y);
	rCorner = new SpellbookInteractiveArea(temp_rect, std::bind(&CSpellWindow::fRcornerb, this), CGI->generaltexth->zelp[451].second, std::bind(&CGStatusBar::setText, statusBar, (CGI->generaltexth->zelp[451].first)), std::bind(&CGStatusBar::clear, statusBar), myInt);

	//areas for spells
	int xpos = 117 + pos.x, ypos = 90 + pos.y;

	for(int v=0; v<12; ++v)
	{
		temp_rect = genRect(65, 78, xpos, ypos);
		spellAreas[v] = new SpellArea(temp_rect, this);

		if(v == 5) //to right page
		{
			xpos = 336 + pos.x; ypos = 90 + pos.y;
		}
		else
		{
			if(v%2 == 0)
			{
				xpos+=85;
			}
			else
			{
				xpos -= 85; ypos+=97;
			}
		}
	}

	selectedTab = battleSpellsOnly ? myInt->spellbookSettings.spellbookLastTabBattle : myInt->spellbookSettings.spellbookLastTabAdvmap;
	currentPage = battleSpellsOnly ? myInt->spellbookSettings.spellbookLastPageBattle : myInt->spellbookSettings.spellbokLastPageAdvmap;

	// spellbook last page battle index is not reset after battle, so this needs to stay here
	vstd::abetween(currentPage, 0, pagesWithinCurrentTab() - 1);

	computeSpellsPerArea();
	addUsedEvents(KEYBOARD);
}

CSpellWindow::~CSpellWindow()
{
	SDL_FreeSurface(leftCorner);
	SDL_FreeSurface(rightCorner);
	spells->unload();
	delete spells;
	delete spellTab;
	delete schools;
	for(auto & elem : schoolBorders)
		delete elem;

	delete exitBtn;
	delete battleSpells;
	delete adventureSpells;
	delete manaPoints;
	delete statusBar;

	delete selectSpellsA;
	delete selectSpellsE;
	delete selectSpellsF;
	delete selectSpellsW;
	delete selectSpellsAll;

	delete lCorner;
	delete rCorner;

	for(auto & elem : spellAreas)
	{
		delete elem;
	}
}

void CSpellWindow::fexitb()
{
	(LOCPLINT->battleInt ? myInt->spellbookSettings.spellbookLastTabBattle : myInt->spellbookSettings.spellbookLastTabAdvmap) = selectedTab;
	(LOCPLINT->battleInt ? myInt->spellbookSettings.spellbookLastPageBattle : myInt->spellbookSettings.spellbokLastPageAdvmap) = currentPage;

	GH.popIntTotally(this);
}

void CSpellWindow::fadvSpellsb()
{
	if (battleSpellsOnly == true)
	{
		turnPageRight();
		battleSpellsOnly = false;
		currentPage = 0;
	}
	computeSpellsPerArea();
}

void CSpellWindow::fbattleSpellsb()
{
	if (battleSpellsOnly == false)
	{
		turnPageLeft();
		battleSpellsOnly = true;
		currentPage = 0;
	}
	computeSpellsPerArea();
}

void CSpellWindow::fmanaPtsb()
{
}

void CSpellWindow::selectSchool(int school)
{
	if (selectedTab != school)
	{
		if (selectedTab < school)
			turnPageLeft();
		else
			turnPageRight();
		selectedTab = school;
		currentPage = 0;
	}
	computeSpellsPerArea();
}

void CSpellWindow::fLcornerb()
{
	if(currentPage>0)
	{
		turnPageLeft();
		--currentPage;
	}
	computeSpellsPerArea();
	GH.breakEventHandling();
}

void CSpellWindow::fRcornerb()
{
	if((currentPage + 1) < (pagesWithinCurrentTab()))
	{
		turnPageRight();
		++currentPage;
	}
	computeSpellsPerArea();
	GH.breakEventHandling();
}

void CSpellWindow::showAll(SDL_Surface * to)
{
	CWindowObject::showAll(to);
	blitAt(spellTab->ourImages[selectedTab].bitmap, 524 + pos.x, 88 + pos.y, to);

	std::ostringstream mana;
	mana<<myHero->mana;
	printAtMiddleLoc(mana.str(), 435, 426, FONT_SMALL, Colors::YELLOW, to);

	statusBar->showAll(to);

	//printing school images
	if(selectedTab!=4 && currentPage == 0)
	{
		blitAt(schools->ourImages[selectedTab].bitmap, 117 + pos.x, 74 + pos.y, to);
	}

	//printing corners
	if(currentPage!=0)
	{
		blitAt(leftCorner, lCorner->pos.x, lCorner->pos.y, to);
	}
	if((currentPage+1) < (pagesWithinCurrentTab()) )
	{
		blitAt(rightCorner, rCorner->pos.x, rCorner->pos.y, to);
	}

	//printing spell info
	for(auto & elem : spellAreas)
	{
		elem->showAll(to);
	}
}

void CSpellWindow::show(SDL_Surface * to)
{
	statusBar->show(to);
}

class SpellbookSpellSorter
{
public:
	bool operator()(const int & a, const int & b)
	{
		const CSpell & A = *CGI->spellh->objects[a];
		const CSpell & B = *CGI->spellh->objects[b];
		if(A.level<B.level)
			return true;
		if(A.level>B.level)
			return false;
		if(A.air && !B.air)
			return true;
		if(!A.air && B.air)
			return false;
		if(A.fire && !B.fire)
			return true;
		if(!A.fire && B.fire)
			return false;
		if(A.water && !B.water)
			return true;
		if(!A.water && B.water)
			return false;
		if(A.earth && !B.earth)
			return true;
		if(!A.earth && B.earth)
			return false;
		return A.name < B.name;
	}
} spellsorter;

void CSpellWindow::computeSpellsPerArea()
{
	std::vector<SpellID> spellsCurSite;
	for(auto it = mySpells.cbegin(); it != mySpells.cend(); ++it)
	{
		if(CGI->spellh->objects[*it]->combatSpell ^ !battleSpellsOnly
			&& ((CGI->spellh->objects[*it]->air && selectedTab == 0) ||
				(CGI->spellh->objects[*it]->fire && selectedTab == 1) ||
				(CGI->spellh->objects[*it]->water && selectedTab == 2) ||
				(CGI->spellh->objects[*it]->earth && selectedTab == 3) ||
				selectedTab == 4 )
			)
		{
			spellsCurSite.push_back(*it);
		}
	}
	std::sort(spellsCurSite.begin(), spellsCurSite.end(), spellsorter);
	if(selectedTab == 4)
	{
		if(spellsCurSite.size() > 12)
		{
			spellsCurSite = std::vector<SpellID>(spellsCurSite.begin() + currentPage*12, spellsCurSite.end());
			if(spellsCurSite.size() > 12)
			{
				spellsCurSite.erase(spellsCurSite.begin()+12, spellsCurSite.end());
			}
		}
	}
	else //selectedTab == 0, 1, 2 or 3
	{
		if(spellsCurSite.size() > 10)
		{
			if(currentPage == 0)
			{
				spellsCurSite.erase(spellsCurSite.begin()+10, spellsCurSite.end());
			}
			else
			{
				spellsCurSite = std::vector<SpellID>(spellsCurSite.begin() + (currentPage-1)*12 + 10, spellsCurSite.end());
				if(spellsCurSite.size() > 12)
				{
					spellsCurSite.erase(spellsCurSite.begin()+12, spellsCurSite.end());
				}
			}
		}
	}
	//applying
	if(selectedTab == 4 || currentPage != 0)
	{
		for(size_t c=0; c<12; ++c)
		{
			if(c<spellsCurSite.size())
			{
				spellAreas[c]->setSpell(spellsCurSite[c]);
			}
			else
			{
				spellAreas[c]->setSpell(SpellID::NONE);
			}
		}
	}
	else
	{
		spellAreas[0]->setSpell(SpellID::NONE);
		spellAreas[1]->setSpell(SpellID::NONE);
		for(size_t c=0; c<10; ++c)
		{
			if(c<spellsCurSite.size())
				spellAreas[c+2]->setSpell(spellsCurSite[c]);
			else
				spellAreas[c+2]->setSpell(SpellID::NONE);
		}
	}
	redraw();
}

void CSpellWindow::activate()
{
	CIntObject::activate();
	exitBtn->activate();
	battleSpells->activate();
	adventureSpells->activate();
	manaPoints->activate();

	selectSpellsA->activate();
	selectSpellsE->activate();
	selectSpellsF->activate();
	selectSpellsW->activate();
	selectSpellsAll->activate();

	for(auto & elem : spellAreas)
	{
		elem->activate();
	}

	lCorner->activate();
	rCorner->activate();
}

void CSpellWindow::deactivate()
{
	CIntObject::deactivate();
	exitBtn->deactivate();
	battleSpells->deactivate();
	adventureSpells->deactivate();
	manaPoints->deactivate();

	selectSpellsA->deactivate();
	selectSpellsE->deactivate();
	selectSpellsF->deactivate();
	selectSpellsW->deactivate();
	selectSpellsAll->deactivate();

	for(auto & elem : spellAreas)
	{
		elem->deactivate();
	}

	lCorner->deactivate();
	rCorner->deactivate();
}

void CSpellWindow::turnPageLeft()
{
	if (settings["video"]["spellbookAnimation"].Bool())
		CCS->videoh->openAndPlayVideo("PGTRNLFT.SMK", pos.x+13, pos.y+15, screen);
}

void CSpellWindow::turnPageRight()
{
	if (settings["video"]["spellbookAnimation"].Bool())
		CCS->videoh->openAndPlayVideo("PGTRNRGH.SMK", pos.x+13, pos.y+15, screen);
}

void CSpellWindow::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_ESCAPE ||  key.keysym.sym == SDLK_RETURN)
		fexitb();

	if(key.state == SDL_PRESSED)
	{
		switch(key.keysym.sym)
		{
		case SDLK_LEFT:
			fLcornerb();
			break;
		case SDLK_RIGHT:
			fRcornerb();
			break;
		case SDLK_UP:
		case SDLK_DOWN:
		{
			bool down = key.keysym.sym == SDLK_DOWN;
			static const int schoolsOrder[] = { 0, 3, 1, 2, 4 };
			int index = -1;
			while(schoolsOrder[++index] != selectedTab);
			index += (down ? 1 : -1);
			vstd::abetween(index, 0, ARRAY_COUNT(schoolsOrder) - 1);
			if(selectedTab != schoolsOrder[index])
				selectSchool(schoolsOrder[index]);
			break;
		}
		case SDLK_c:
			fbattleSpellsb();
			break;
		case SDLK_a:
			fadvSpellsb();
			break;
		default://to get rid of warnings
			break;
		}

		//alt + 1234567890-= casts spell from 1 - 12 slot
		if(LOCPLINT->altPressed())
		{
			SDLKey hlpKey = key.keysym.sym;
			if(CGuiHandler::isNumKey(hlpKey, false))
			{
				if(hlpKey == SDLK_KP_PLUS)
					hlpKey = SDLK_EQUALS;
				else
					hlpKey = CGuiHandler::numToDigit(hlpKey);
			}

			static const SDLKey spellSelectors[] = {SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_MINUS, SDLK_EQUALS};

			int index = -1;
			while(++index < ARRAY_COUNT(spellSelectors) && spellSelectors[index] != hlpKey);
			if(index >= ARRAY_COUNT(spellSelectors))
				return;

			//try casting spell
			spellAreas[index]->clickLeft(false, true);
		}
	}
}

Uint8 CSpellWindow::pagesWithinCurrentTab()
{
	return battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab];
}

void CSpellWindow::teleportTo( int town, const CGHeroInstance * hero )
{
	const CGTownInstance * dest = LOCPLINT->cb->getTown(ObjectInstanceID(town));
	LOCPLINT->cb->castSpell(hero, SpellID::TOWN_PORTAL, dest->visitablePos());
}

CSpellWindow::SpellArea::SpellArea(SDL_Rect pos, CSpellWindow * owner)
{
	this->pos = pos;
	this->owner = owner;
	addUsedEvents(LCLICK | RCLICK | HOVER);

	spellCost = whichSchool = schoolLevel = -1;
	mySpell = SpellID::NONE;
}

void CSpellWindow::SpellArea::clickLeft(tribool down, bool previousState)
{
	if(!down && mySpell!=-1)
	{
		const CSpell *sp = CGI->spellh->objects[mySpell];

		int spellCost = owner->myInt->cb->getSpellCost(sp, owner->myHero);
		if(spellCost > owner->myHero->mana) //insufficient mana
		{
			char msgBuf[500];
			sprintf(msgBuf, CGI->generaltexth->allTexts[206].c_str(), spellCost, owner->myHero->mana);
			owner->myInt->showInfoDialog(std::string(msgBuf));
			return;
		}

		//battle spell on adv map or adventure map spell during combat => display infowindow, not cast
		if((sp->combatSpell && !owner->myInt->battleInt)
			|| (!sp->combatSpell && owner->myInt->battleInt))
		{
			std::vector<CComponent*> hlp(1, new CComponent(CComponent::spell, mySpell, 0));
			LOCPLINT->showInfoDialog(sp->getLevelInfo(schoolLevel).description, hlp);
			return;
		}

		//we will cast a spell
		if(sp->combatSpell && owner->myInt->battleInt && owner->myInt->cb->battleCanCastSpell()) //if battle window is open
		{
			ESpellCastProblem::ESpellCastProblem problem = owner->myInt->cb->battleCanCastThisSpell(sp);
			switch (problem)
			{
			case ESpellCastProblem::OK:
				{
					int spell = mySpell;
					owner->fexitb();
					owner->myInt->battleInt->castThisSpell(spell);
				}
				break;
			case ESpellCastProblem::ANOTHER_ELEMENTAL_SUMMONED:
				{
					std::string text = CGI->generaltexth->allTexts[538], summoner, elemental, caster;
					std::vector<const CStack *> stacks = owner->myInt->cb->battleGetStacks();
					for(const CStack * s : stacks)
					{
						if(vstd::contains(s->state, EBattleStackState::SUMMONED))
						{
							elemental = s->getCreature()->namePl;
							summoner = owner->myInt->cb->battleGetHeroInfo(!s->attackerOwned).name;
							break;
						}
					}
					if (owner->myHero->type->sex)
					{ //female
						caster = CGI->generaltexth->allTexts[540];
					}
					else
					{ //male
						caster = CGI->generaltexth->allTexts[539];
					}
					text = boost::str(boost::format(text) % summoner % elemental % caster);


					owner->myInt->showInfoDialog(text);
				}
				break;
			case ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED:
				{
					//Recanter's Cloak or similar effect. Try to retrieve bonus
					const Bonus *b = owner->myHero->getBonusLocalFirst(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
					//TODO what about other values and non-artifact sources?
					if(b && b->val == 2 && b->source == Bonus::ARTIFACT)
					{
						std::string artName = CGI->arth->artifacts[b->sid]->Name();
						//The %s prevents %s from casting 3rd level or higher spells.
						owner->myInt->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[536])
							% artName % owner->myHero->name));
					}
					else
					{
						// General message:
						// %s recites the incantations but they seem to have no effect.
						std::string text = CGI->generaltexth->allTexts[541], caster = owner->myHero->name;
						text = boost::str(boost::format(text) % caster);
						owner->myInt->showInfoDialog(text);
					}
				}
				break;
			case ESpellCastProblem::NO_APPROPRIATE_TARGET:
				{
					owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[185]);
				}
				break;
			}
		}
		else if(!sp->combatSpell && !owner->myInt->battleInt) //adventure spell
		{
			SpellID spell = mySpell;
			const CGHeroInstance *h = owner->myHero;
			owner->fexitb();

			switch(spell)
			{
			case SpellID::SUMMON_BOAT:
				{
					int3 pos = h->bestLocation();
					if(pos.x < 0)
					{
						LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[334]); //There is no place to put the boat.
						return;
					}
				}
				break;
			case SpellID::SCUTTLE_BOAT:
			case SpellID::DIMENSION_DOOR:
				adventureInt->enterCastingMode(sp);
				return;
			case SpellID::VISIONS:
			case SpellID::VIEW_EARTH:
			case SpellID::DISGUISE:
			case SpellID::VIEW_AIR:
			case SpellID::FLY:
			case SpellID::WATER_WALK:
				break;
			case SpellID::TOWN_PORTAL:
				{
					std::vector <int> availableTowns;
					std::vector <const CGTownInstance*> Towns = LOCPLINT->cb->getTownsInfo(true);
					if (Towns.empty())
					{
						LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[124]);
						return;
					}

					if (h->getSpellSchoolLevel(CGI->spellh->objects[spell]) < 2) //not advanced or expert - teleport to nearest available city
					{
						auto nearest = Towns.cbegin(); //nearest town's iterator
						si32 dist = LOCPLINT->cb->getTown((*nearest)->id)->pos.dist2dSQ(h->pos);

						for (auto i = nearest + 1; i != Towns.cend(); ++i)
						{
							const CGTownInstance * dest = LOCPLINT->cb->getTown((*i)->id);
							si32 curDist = dest->pos.dist2dSQ(h->pos);

							if (curDist < dist)
							{
								nearest = i;
								dist = curDist;
							}
						}

						if ((*nearest)->visitingHero)
							LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[123]);
						else
						{
							const CGTownInstance * town = LOCPLINT->cb->getTown((*nearest)->id);
							LOCPLINT->cb->castSpell(h, spell, town->visitablePos());// - town->getVisitableOffset());
						}
					}
					else
					{ //let the player choose
						for(auto & Town : Towns)
						{
							const CGTownInstance *t = Town;
							if (t->visitingHero == nullptr) //empty town and this is
							{
								availableTowns.push_back(t->id.getNum());//add to the list
							}
						}
						if (availableTowns.empty())
							LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[124]);
						else
							GH.pushInt (new CObjectListWindow(availableTowns,
								new CAnimImage("SPELLSCR",spell),
								CGI->generaltexth->jktexts[40], CGI->generaltexth->jktexts[41],
								std::bind (&CSpellWindow::teleportTo, owner, _1, h)));
					}
					return;
				}
				break;
			default:
				assert(0);
			}

			//can return earlier in some cases
			LOCPLINT->cb->castSpell(h, spell);
		}
	}
}

void CSpellWindow::SpellArea::clickRight(tribool down, bool previousState)
{
	if(down && mySpell != -1)
	{
		std::string dmgInfo;
		const CGHeroInstance * hero = owner->myHero;
		int causedDmg = owner->myInt->cb->estimateSpellDamage( CGI->spellh->objects[mySpell], (hero ? hero : nullptr));
		if(causedDmg == 0 || mySpell == 57) //Titan's Lightning Bolt already has damage info included
			dmgInfo = "";
		else
		{
			dmgInfo = CGI->generaltexth->allTexts[343];
			boost::algorithm::replace_first(dmgInfo, "%d", boost::lexical_cast<std::string>(causedDmg));
		}

		CRClickPopup::createAndPush(CGI->spellh->objects[mySpell]->getLevelInfo(schoolLevel).description + dmgInfo,
		                            new CComponent(CComponent::spell, mySpell));
	}
}

void CSpellWindow::SpellArea::hover(bool on)
{
	//Hoverable::hover(on);
	if(mySpell != -1)
	{
		if(on)
		{
			std::ostringstream ss;
			ss<<CGI->spellh->objects[mySpell]->name<<" ("<<CGI->generaltexth->allTexts[171+CGI->spellh->objects[mySpell]->level]<<")";
			owner->statusBar->setText(ss.str());
		}
		else
		{
			owner->statusBar->clear();
		}
	}
}

void CSpellWindow::SpellArea::showAll(SDL_Surface * to)
{
	if(mySpell < 0)
		return;

	const CSpell * spell = mySpell.toSpell();	
	owner->spells->load(mySpell);
	
	IImage * icon = owner->spells->getImage(mySpell,0,false);
	
	if(icon != nullptr)
		icon->draw(to, pos.x, pos.y);
	else
		logGlobal->errorStream() << __FUNCTION__ << ": failed to load spell icon for spell with id " << mySpell;

	blitAt(owner->schoolBorders[owner->selectedTab >= 4 ? whichSchool : owner->selectedTab]->ourImages[schoolLevel].bitmap, pos.x, pos.y, to); //printing border (indicates level of magic school)

	SDL_Color firstLineColor, secondLineColor;
	if(spellCost > owner->myHero->mana) //hero cannot cast this spell
	{
		static const SDL_Color unavailableSpell = {239, 189, 33, 0};
		firstLineColor = Colors::WHITE;
		secondLineColor = unavailableSpell;
	}
	else
	{
		firstLineColor = Colors::YELLOW;
		secondLineColor = Colors::WHITE;
	}
	//printing spell's name
	printAtMiddleLoc(spell->name, 39, 70, FONT_TINY, firstLineColor, to);
	//printing lvl
	printAtMiddleLoc(CGI->generaltexth->allTexts[171 + spell->level], 39, 82, FONT_TINY, secondLineColor, to);
	//printing  cost
	std::ostringstream ss;
	ss << CGI->generaltexth->allTexts[387] << ": " << spellCost;
	printAtMiddleLoc(ss.str(), 39, 94, FONT_TINY, secondLineColor, to);
}

void CSpellWindow::SpellArea::setSpell(SpellID spellID)
{
	mySpell = spellID;
	if(mySpell < 0)
		return;

	const CSpell * spell = CGI->spellh->objects[mySpell];
	schoolLevel = owner->myHero->getSpellSchoolLevel(spell, &whichSchool);
	spellCost = owner->myInt->cb->getSpellCost(spell, owner->myHero);
}

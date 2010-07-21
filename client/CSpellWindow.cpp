#include "CSpellWindow.h"
#include "Graphics.h"
#include "../hch/CDefHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CVideoHandler.h"
#include "CAdvmapInterface.h"
#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
#include "../CCallback.h"
#include <boost/bind.hpp>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

/*
 * CSpellWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface * screen;
extern SDL_Color tytulowy, zwykly, darkTitle;

SpellbookInteractiveArea::SpellbookInteractiveArea(const SDL_Rect & myRect, boost::function<void()> funcL,
	const std::string & textR, boost::function<void()> funcHon, boost::function<void()> funcHoff, CPlayerInterface * _myInt)
{
	used = LCLICK | RCLICK | HOVER;
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

CSpellWindow::CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells):
	battleSpellsOnly(openOnBattleSpells),
	selectedTab(4),
	currentPage(0),
	myHero(_myHero),
	myInt(_myInt)
{
	//initializing castable spells
	for(ui32 v=0; v<CGI->spellh->spells.size(); ++v)
	{
		if( !CGI->spellh->spells[v].creatureAbility && myHero->canCastThisSpell(&CGI->spellh->spells[v]) )
			mySpells.insert(v);
	}

	//initializing sizes of spellbook's parts
	for(int b=0; b<5; ++b)
		sitesPerTabAdv[b] = 0;
	for(int b=0; b<5; ++b)
		sitesPerTabBattle[b] = 0;

	for(std::set<ui32>::const_iterator g = mySpells.begin(); g!=mySpells.end(); ++g)
	{
		const CSpell &s = CGI->spellh->spells[*g];
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

	pos = myRect;
	background = BitmapHandler::loadBitmap("SpelBack.bmp");
	graphics->blueToPlayersAdv(background, myHero->tempOwner);

	leftCorner = BitmapHandler::loadBitmap("SpelTrnL.bmp", true);
	rightCorner = BitmapHandler::loadBitmap("SpelTrnR.bmp", true);
	spells = CDefHandler::giveDef("Spells.def");
	spellTab = CDefHandler::giveDef("SpelTab.def");
	schools = CDefHandler::giveDef("Schools.def");
	schoolBorders[0] = CDefHandler::giveDef("SplevA.def");
	schoolBorders[1] = CDefHandler::giveDef("SplevF.def");
	schoolBorders[2] = CDefHandler::giveDef("SplevW.def");
	schoolBorders[3] = CDefHandler::giveDef("SplevE.def");



	statusBar = new CStatusBar(7 + pos.x, 569 + pos.y, "Spelroll.bmp");
	SDL_Rect temp_rect = genRect(45, 35, 479 + pos.x, 405 + pos.y);
	exitBtn = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fexitb, this), CGI->generaltexth->zelp[460].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[460].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(45, 35, 221 + pos.x, 405 + pos.y);
	battleSpells = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fbattleSpellsb, this), CGI->generaltexth->zelp[453].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[453].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(45, 35, 355 + pos.x, 405 + pos.y);
	adventureSpells = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fadvSpellsb, this), CGI->generaltexth->zelp[452].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[452].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(45, 35, 418 + pos.x, 405 + pos.y);
	manaPoints = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fmanaPtsb, this), CGI->generaltexth->zelp[459].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[459].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);

	temp_rect = genRect(36, 56, 549 + pos.x, 94 + pos.y);
	selectSpellsA = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::selectSchool, this, 0), CGI->generaltexth->zelp[454].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[454].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 151 + pos.y);
	selectSpellsE = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::selectSchool, this, 3), CGI->generaltexth->zelp[457].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[457].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 210 + pos.y);
	selectSpellsF = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::selectSchool, this, 1), CGI->generaltexth->zelp[455].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[455].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 270 + pos.y);
	selectSpellsW = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::selectSchool, this, 2), CGI->generaltexth->zelp[456].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[456].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(36, 56, 549 + pos.x, 330 + pos.y);
	selectSpellsAll = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::selectSchool, this, 4), CGI->generaltexth->zelp[458].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[458].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);

	temp_rect = genRect(leftCorner->h, leftCorner->w, 97 + pos.x, 77 + pos.y);
	lCorner = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fLcornerb, this), CGI->generaltexth->zelp[450].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[450].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);
	temp_rect = genRect(rightCorner->h, rightCorner->w, 487 + pos.x, 72 + pos.y);
	rCorner = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fRcornerb, this), CGI->generaltexth->zelp[451].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[451].first)), boost::bind(&CStatusBar::clear, statusBar), myInt);

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

	selectedTab = battleSpellsOnly ? LOCPLINT->spellbookSettings.spellbookLastTabBattle : LOCPLINT->spellbookSettings.spellbookLastTabAdvmap;
	currentPage = battleSpellsOnly ? LOCPLINT->spellbookSettings.spellbookLastPageBattle : LOCPLINT->spellbookSettings.spellbokLastPageAdvmap;
	abetw(currentPage, 0, pagesWithinCurrentTab());
	computeSpellsPerArea();
}

CSpellWindow::~CSpellWindow()
{
	SDL_FreeSurface(background);
	SDL_FreeSurface(leftCorner);
	SDL_FreeSurface(rightCorner);
	delete spells;
	delete spellTab;
	delete schools;
	for(int b=0; b<4; ++b)
		delete schoolBorders[b];

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

	for(int g=0; g<12; ++g)
	{
		delete spellAreas[g];
	}
}

void CSpellWindow::fexitb()
{
	(LOCPLINT->battleInt ? LOCPLINT->spellbookSettings.spellbookLastTabBattle : LOCPLINT->spellbookSettings.spellbookLastTabAdvmap) = selectedTab;
	(LOCPLINT->battleInt ? LOCPLINT->spellbookSettings.spellbookLastPageBattle : LOCPLINT->spellbookSettings.spellbokLastPageAdvmap) = currentPage;

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

void CSpellWindow::showAll(SDL_Surface *to)
{
	SDL_BlitSurface(background, NULL, to, &pos);
	blitAt(spellTab->ourImages[selectedTab].bitmap, 524 + pos.x, 88 + pos.y, to);

	std::ostringstream mana;
	mana<<myHero->mana;
	CSDL_Ext::printAtMiddle(mana.str(), pos.x+435, pos.y +426, FONT_SMALL, tytulowy, to);
	
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
	for(int b=0; b<12; ++b)
	{
		spellAreas[b]->showAll(to);
	}
}

class SpellbookSpellSorter
{
public:
	bool operator()(const int & a, const int & b)
	{
		CSpell A = CGI->spellh->spells[a];
		CSpell B = CGI->spellh->spells[b];
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
	std::vector<ui32> spellsCurSite;
	for(std::set<ui32>::const_iterator it = mySpells.begin(); it != mySpells.end(); ++it)
	{
		if(CGI->spellh->spells[*it].combatSpell ^ !battleSpellsOnly
			&& ((CGI->spellh->spells[*it].air && selectedTab == 0) || 
				(CGI->spellh->spells[*it].fire && selectedTab == 1) ||
				(CGI->spellh->spells[*it].water && selectedTab == 2) ||
				(CGI->spellh->spells[*it].earth && selectedTab == 3) ||
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
			spellsCurSite = std::vector<ui32>(spellsCurSite.begin() + currentPage*12, spellsCurSite.end());
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
				spellsCurSite = std::vector<ui32>(spellsCurSite.begin() + (currentPage-1)*12 + 10, spellsCurSite.end());
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
				spellAreas[c]->setSpell(-1);
			}
		}
	}
	else
	{
		spellAreas[0]->setSpell(-1);
		spellAreas[1]->setSpell(-1);
		for(size_t c=0; c<10; ++c)
		{
			if(c<spellsCurSite.size())
				spellAreas[c+2]->setSpell(spellsCurSite[c]);
			else
				spellAreas[c+2]->setSpell(-1);
		}
	}
	redraw();
}

void CSpellWindow::activate()
{
	activateKeys();
	exitBtn->activate();
	battleSpells->activate();
	adventureSpells->activate();
	manaPoints->activate();

	selectSpellsA->activate();
	selectSpellsE->activate();
	selectSpellsF->activate();
	selectSpellsW->activate();
	selectSpellsAll->activate();

	for(int g=0; g<12; ++g)
	{
		spellAreas[g]->activate();
	}

	lCorner->activate();
	rCorner->activate();
}

void CSpellWindow::deactivate()
{
	deactivateKeys();
	exitBtn->deactivate();
	battleSpells->deactivate();
	adventureSpells->deactivate();
	manaPoints->deactivate();

	selectSpellsA->deactivate();
	selectSpellsE->deactivate();
	selectSpellsF->deactivate();
	selectSpellsW->deactivate();
	selectSpellsAll->deactivate();

	for(int g=0; g<12; ++g)
	{
		spellAreas[g]->deactivate();
	}

	lCorner->deactivate();
	rCorner->deactivate();
}

void CSpellWindow::turnPageLeft()
{
	CGI->videoh->openAndPlayVideo("PGTRNLFT.SMK", pos.x+13, pos.y+15, screen);
}

void CSpellWindow::turnPageRight()
{
	CGI->videoh->openAndPlayVideo("PGTRNRGH.SMK", pos.x+13, pos.y+15, screen);
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
			bool down = key.keysym.sym == SDLK_DOWN;
			static const int schoolsOrder[] = {0, 3, 1, 2, 4};
			int index = -1;
			while(schoolsOrder[++index] != selectedTab);
			index += (down ? 1 : -1);
			abetw(index, 0, ARRAY_COUNT(schoolsOrder) - 1);
			if(selectedTab != schoolsOrder[index])
				selectSchool(schoolsOrder[index]);
			break;
		}

		//alt + 1234567890-= casts spell from 1 - 12 slot
		if(LOCPLINT->altPressed())
		{
			SDLKey hlpKey = key.keysym.sym;
			if(isNumKey(hlpKey, false))
			{
				if(hlpKey == SDLK_KP_PLUS)
					hlpKey = SDLK_EQUALS;
				else
					hlpKey = numToDigit(hlpKey);
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

CSpellWindow::SpellArea::SpellArea(SDL_Rect pos, CSpellWindow * owner)
{
	this->pos = pos;
	this->owner = owner;
	used = LCLICK | RCLICK | HOVER;

	spellCost = mySpell = whichSchool = schoolLevel = -1;
}

void CSpellWindow::SpellArea::clickLeft(tribool down, bool previousState)
{
	if(!down && mySpell!=-1)
	{
		const CSpell *sp = &CGI->spellh->spells[mySpell];

		int spellCost = owner->myInt->cb->getSpellCost(sp, owner->myHero);
		if(spellCost > owner->myHero->mana) //insufficient mana
		{
			char msgBuf[500];
			sprintf(msgBuf, CGI->generaltexth->allTexts[206].c_str(), spellCost, owner->myHero->mana);
			owner->myInt->showInfoDialog(std::string(msgBuf));
		}

		//battle spell on adv map or adventure map spell during combat => display infowindow, not cast
		if(sp->combatSpell && !owner->myInt->battleInt
			|| !sp->combatSpell && owner->myInt->battleInt)
		{
			std::vector<SComponent*> hlp(1, new SComponent(SComponent::spell, mySpell, 0));
			LOCPLINT->showInfoDialog(sp->descriptions[schoolLevel], hlp);
			return;
		}

		//we will cast a spell
		if(sp->combatSpell && owner->myInt->battleInt && owner->myInt->cb->battleCanCastSpell()) //if battle window is open
		{
			int spell = mySpell;
			owner->fexitb();
			owner->myInt->battleInt->castThisSpell(spell);
		}
		else //adventure spell
		{
			using namespace Spells;
			int spell = mySpell;
			const CGHeroInstance *h = owner->myHero;
			owner->fexitb();

			switch(spell)
			{
			case SUMMON_BOAT:
				{
					int3 pos = h->bestLocation();
					if(pos.x < 0)
					{
						LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[334]); //There is no place to put the boat.
						return;
					}
				}
				break;
			case SCUTTLE_BOAT:
			case DIMENSION_DOOR:
				adventureInt->enterCastingMode(sp);
				return;
			case VISIONS:
			case VIEW_EARTH:
			case DISGUISE:
			case VIEW_AIR:
			case FLY:
			case WATER_WALK:
			case TOWN_PORTAL:
				break;
			default:
				assert(0);
			}

			LOCPLINT->cb->castSpell(h, spell);
		}
	}
}

void CSpellWindow::SpellArea::clickRight(tribool down, bool previousState)
{
	if(down && mySpell != -1)
	{
		std::string dmgInfo;
		int causedDmg = owner->myInt->cb->estimateSpellDamage( &CGI->spellh->spells[mySpell] );
		if(causedDmg == 0)
			dmgInfo = "";
		else
		{
			dmgInfo = CGI->generaltexth->allTexts[343];
			boost::algorithm::replace_first(dmgInfo, "%d", boost::lexical_cast<std::string>(causedDmg));
		}

		SDL_Surface *spellBox = CMessage::drawBoxTextBitmapSub(
			owner->myInt->playerID,
			CGI->spellh->spells[mySpell].descriptions[schoolLevel] + dmgInfo, this->owner->spells->ourImages[mySpell].bitmap,
			CGI->spellh->spells[mySpell].name,30,30);
		CInfoPopup *vinya = new CInfoPopup(spellBox, true);
		GH.pushInt(vinya);
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
			ss<<CGI->spellh->spells[mySpell].name<<" ("<<CGI->generaltexth->allTexts[171+CGI->spellh->spells[mySpell].level]<<")";
			owner->statusBar->print(ss.str());
		}
		else
		{
			owner->statusBar->clear();
		}
	}
}

void CSpellWindow::SpellArea::showAll(SDL_Surface *to)
{
	if(mySpell < 0)
		return;

	const CSpell * spell = &CGI->spellh->spells[mySpell];

	blitAt(owner->spells->ourImages[mySpell].bitmap, pos.x, pos.y, to);
	blitAt(owner->schoolBorders[owner->selectedTab >= 4 ? whichSchool : owner->selectedTab]->ourImages[schoolLevel].bitmap, pos.x, pos.y, to); //printing border (indicates level of magic school)

	SDL_Color firstLineColor, secondLineColor;
	if(spellCost > owner->myHero->mana) //hero cannot cast this spell
	{
		static const SDL_Color unavailableSpell = {239, 189, 33, 0};
		firstLineColor = zwykly;
		secondLineColor = unavailableSpell;
	}
	else
	{
		firstLineColor = tytulowy;
		secondLineColor = zwykly;
	}
	//printing spell's name
	CSDL_Ext::printAtMiddle(spell->name, pos.x + 39, pos.y + 70, FONT_TINY, firstLineColor, to);
	//printing lvl
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[171 + spell->level], pos.x + 39, pos.y + 82, FONT_TINY, secondLineColor, to);
	//printing  cost
	std::ostringstream ss;
	ss << CGI->generaltexth->allTexts[387] << ": " << spellCost;
	CSDL_Ext::printAtMiddle(ss.str(), pos.x + 39, pos.y + 94, FONT_TINY, secondLineColor, to);
}

void CSpellWindow::SpellArea::setSpell(int spellID)
{
	mySpell = spellID;
	if(mySpell < 0)
		return;

	const CSpell * spell = &CGI->spellh->spells[mySpell];
	schoolLevel = owner->myHero->getSpellSchoolLevel(spell, &whichSchool);
	spellCost = owner->myInt->cb->getSpellCost(spell, owner->myHero);
}
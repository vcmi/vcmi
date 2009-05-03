#include "CSpellWindow.h"
#include "Graphics.h"
#include "../hch/CDefHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../CAdvmapInterface.h"
#include "../CBattleInterface.h"
#include "../CGameInfo.h"
#include "../SDL_Extensions.h"
#include "../CMessage.h"
#include <boost/bind.hpp>
#include <sstream>

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
extern SDL_Color tytulowy, zwykly ;
extern TTF_Font *GEOR16;

SpellbookInteractiveArea::SpellbookInteractiveArea(const SDL_Rect & myRect, boost::function<void()> funcL, const std::string & textR, boost::function<void()> funcHon, boost::function<void()> funcHoff)
{
	pos = myRect;
	onLeft = funcL;
	textOnRclick = textR;
	onHoverOn = funcHon;
	onHoverOff = funcHoff;
}

void SpellbookInteractiveArea::clickLeft(boost::logic::tribool down)
{
	if(!down)
	{
		onLeft();
	}
}

void SpellbookInteractiveArea::clickRight(boost::logic::tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(textOnRclick, down, this);
}

void SpellbookInteractiveArea::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		onHoverOn();
	}
	else
	{
		onHoverOff();
	}
}

void SpellbookInteractiveArea::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();

}
void SpellbookInteractiveArea::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}

CSpellWindow::CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * myHero):
	battleSpellsOnly(true),
	selectedTab(4),
	spellSite(0)
{
	//initializing castable spells
	for(ui32 v=0; v<CGI->spellh->spells.size(); ++v)
	{
		if( !CGI->spellh->spells[v].creatureAbility && myHero->canCastThisSpell(&CGI->spellh->spells[v]) )
			mySpells.insert(v);
	}

	//initializing schools' levels
	for(int b=0; b<4; ++b) schoolLvls[b] = 0;
	for(size_t b=0; b<myHero->secSkills.size(); ++b)
	{
		switch(myHero->secSkills[b].first)
		{
		case 14: //fire magic
			schoolLvls[1] = myHero->secSkills[b].second;
			break;
		case 15: //air magic
			schoolLvls[0] = myHero->secSkills[b].second;
			break;
		case 16: //water magic
			schoolLvls[2] = myHero->secSkills[b].second;
			break;
		case 17: //earth magic
			schoolLvls[3] = myHero->secSkills[b].second;
			break;
		}
	}

	//initializing sizes of spellbook's parts
	for(int b=0; b<5; ++b)
		sitesPerTabAdv[b] = 0;
	for(int b=0; b<5; ++b)
		sitesPerTabBattle[b] = 0;
	for(std::set<ui32>::const_iterator g = mySpells.begin(); g!=mySpells.end(); ++g)
	{
		if(CGI->spellh->spells[*g].combatSpell)
		{
			++(sitesPerTabBattle[4]);
		}
		else
		{
			++(sitesPerTabAdv[4]);
		}

		if(CGI->spellh->spells[*g].air)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[0]);
			}
			else
			{
				++(sitesPerTabAdv[0]);
			}
		}
		if(CGI->spellh->spells[*g].fire)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[1]);
			}
			else
			{
				++(sitesPerTabAdv[1]);
			}
		}
		if(CGI->spellh->spells[*g].water)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[2]);
			}
			else
			{
				++(sitesPerTabAdv[2]);
			}
		}
		if(CGI->spellh->spells[*g].earth)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[3]);
			}
			else
			{
				++(sitesPerTabAdv[3]);
			}
		}
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

	std::stringstream mana;
	mana<<myHero->mana;
	CSDL_Ext::printAtMiddle(mana.str(), 434, 425, GEOR16, tytulowy, background);

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
	exitBtn = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fexitb, this), CGI->generaltexth->zelp[460].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[460].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(45, 35, 221 + pos.x, 405 + pos.y);
	battleSpells = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fbattleSpellsb, this), CGI->generaltexth->zelp[453].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[453].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(45, 35, 355 + pos.x, 405 + pos.y);
	adventureSpells = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fadvSpellsb, this), CGI->generaltexth->zelp[452].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[452].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(45, 35, 418 + pos.x, 405 + pos.y);
	manaPoints = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fmanaPtsb, this), CGI->generaltexth->zelp[459].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[459].first)), boost::bind(&CStatusBar::clear, statusBar));

	temp_rect = genRect(36, 56, 549 + pos.x, 94 + pos.y);
	selectSpellsA = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsAb, this), CGI->generaltexth->zelp[454].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[454].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 549 + pos.x, 151 + pos.y);
	selectSpellsE = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsEb, this), CGI->generaltexth->zelp[457].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[457].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 549 + pos.x, 210 + pos.y);
	selectSpellsF = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsFb, this), CGI->generaltexth->zelp[455].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[455].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 549 + pos.x, 270 + pos.y);
	selectSpellsW = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsWb, this), CGI->generaltexth->zelp[456].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[456].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 549 + pos.x, 330 + pos.y);
	selectSpellsAll = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsAllb, this), CGI->generaltexth->zelp[458].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[458].first)), boost::bind(&CStatusBar::clear, statusBar));

	temp_rect = genRect(leftCorner->h, leftCorner->w, 97 + pos.x, 77 + pos.y);
	lCorner = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fLcornerb, this), CGI->generaltexth->zelp[450].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[450].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(rightCorner->h, rightCorner->w, 487 + pos.x, 72 + pos.y);
	rCorner = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fRcornerb, this), CGI->generaltexth->zelp[451].second, boost::bind(&CStatusBar::print, statusBar, (CGI->generaltexth->zelp[451].first)), boost::bind(&CStatusBar::clear, statusBar));

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
	LOCPLINT->popIntTotally(this);
}

void CSpellWindow::fadvSpellsb()
{
	battleSpellsOnly = false;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fbattleSpellsb()
{
	battleSpellsOnly = true;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fmanaPtsb()
{
}

void CSpellWindow::fspellsAb()
{
	selectedTab = 0;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fspellsEb()
{
	selectedTab = 3;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fspellsFb()
{
	selectedTab = 1;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fspellsWb()
{
	selectedTab = 2;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fspellsAllb()
{
	selectedTab = 4;
	spellSite = 0;
	computeSpellsPerArea();
}

void CSpellWindow::fLcornerb()
{
	if(spellSite>0)
		--spellSite;
	computeSpellsPerArea();
}

void CSpellWindow::fRcornerb()
{
	if((spellSite + 1) < (battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab]))
		++spellSite;
	computeSpellsPerArea();
}

void CSpellWindow::show(SDL_Surface *to)
{
	SDL_BlitSurface(background, NULL, to, &pos);
	blitAt(spellTab->ourImages[selectedTab].bitmap, 524 + pos.x, 94 + pos.y, to);
	
	statusBar->show(to);

	//printing school images
	if(selectedTab!=4 && spellSite == 0)
	{
		blitAt(schools->ourImages[selectedTab].bitmap, 117 + pos.x, 74 + pos.y, to);
	}

	//printing corners
	if(spellSite!=0)
	{
		blitAt(leftCorner, lCorner->pos.x, lCorner->pos.y, to);
	}
	if((spellSite+1) != (battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab]) )
	{
		blitAt(rightCorner, rCorner->pos.x, rCorner->pos.y, to);
	}

	//printing spell info
	for(int b=0; b<12; ++b)
	{
		if(spellAreas[b]->mySpell == -1)
			continue;
		//int b2 = -1; //TODO use me

		blitAt(spells->ourImages[spellAreas[b]->mySpell].bitmap, spellAreas[b]->pos.x, spellAreas[b]->pos.y, to);

		Uint8 bestSchool = -1, bestslvl = 0;
		if(CGI->spellh->spells[spellAreas[b]->mySpell].air)
			if(schoolLvls[0] >= bestslvl)
			{
				bestSchool = 0;
				bestslvl = schoolLvls[0];
			}
		if(CGI->spellh->spells[spellAreas[b]->mySpell].fire)
			if(schoolLvls[1] >= bestslvl)
			{
				bestSchool = 1;
				bestslvl = schoolLvls[1];
			}
		if(CGI->spellh->spells[spellAreas[b]->mySpell].water)
			if(schoolLvls[2] >= bestslvl)
			{
				bestSchool = 2;
				bestslvl = schoolLvls[2];
			}
		if(CGI->spellh->spells[spellAreas[b]->mySpell].earth)
			if(schoolLvls[3] >= bestslvl)
			{
				bestSchool = 3;
				bestslvl = schoolLvls[3];
			}

		//printing border (indicates level of magic school)
		blitAt(schoolBorders[bestSchool]->ourImages[bestslvl].bitmap, spellAreas[b]->pos.x, spellAreas[b]->pos.y, to);

		//printing spell's name
		CSDL_Ext::printAtMiddle(CGI->spellh->spells[spellAreas[b]->mySpell].name, spellAreas[b]->pos.x + 39, spellAreas[b]->pos.y + 70, GEORM, tytulowy, to);
		//printing lvl
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[171 + CGI->spellh->spells[spellAreas[b]->mySpell].level], spellAreas[b]->pos.x + 39, spellAreas[b]->pos.y + 82, GEORM, zwykly, to);
		//printing  cost
		std::stringstream ss;
		ss<<CGI->generaltexth->allTexts[387]<<": "<<CGI->spellh->spells[spellAreas[b]->mySpell].costs[bestslvl];

		CSDL_Ext::printAtMiddle(ss.str(), spellAreas[b]->pos.x + 39, spellAreas[b]->pos.y + 94, GEORM, zwykly, to);
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
			spellsCurSite = std::vector<ui32>(spellsCurSite.begin() + spellSite*12, spellsCurSite.end());
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
			if(spellSite == 0)
			{
				spellsCurSite.erase(spellsCurSite.begin()+10, spellsCurSite.end());
			}
			else
			{
				spellsCurSite = std::vector<ui32>(spellsCurSite.begin() + (spellSite-1)*12 + 10, spellsCurSite.end());
				if(spellsCurSite.size() > 12)
				{
					spellsCurSite.erase(spellsCurSite.begin()+12, spellsCurSite.end());
				}
			}
		}
	}
	//applying
	if(selectedTab == 4 || spellSite != 0)
	{
		for(size_t c=0; c<12; ++c)
		{
			if(c<spellsCurSite.size())
			{
				spellAreas[c]->mySpell = spellsCurSite[c];
			}
			else
			{
				spellAreas[c]->mySpell = -1;
			}
		}
	}
	else
	{
		spellAreas[0]->mySpell = -1;
		spellAreas[1]->mySpell = -1;
		for(size_t c=0; c<10; ++c)
		{
			if(c<spellsCurSite.size())
				spellAreas[c+2]->mySpell = spellsCurSite[c];
			else
				spellAreas[c+2]->mySpell = -1;
		}
	}

}

void CSpellWindow::activate()
{
	exitBtn->activate();
	battleSpells->activate();
	adventureSpells->activate();
	manaPoints->activate();

	selectSpellsA->activate();
	selectSpellsE->activate();
	selectSpellsF->activate();
	selectSpellsW->activate();
	selectSpellsAll->activate();

	lCorner->activate();
	rCorner->activate();

	for(int g=0; g<12; ++g)
	{
		spellAreas[g]->activate();
	}
}

void CSpellWindow::deactivate()
{
	exitBtn->deactivate();
	battleSpells->deactivate();
	adventureSpells->deactivate();
	manaPoints->deactivate();

	selectSpellsA->deactivate();
	selectSpellsE->deactivate();
	selectSpellsF->deactivate();
	selectSpellsW->deactivate();
	selectSpellsAll->deactivate();
	
	lCorner->deactivate();
	rCorner->deactivate();

	for(int g=0; g<12; ++g)
	{
		spellAreas[g]->deactivate();
	}
}

CSpellWindow::SpellArea::SpellArea(SDL_Rect pos, CSpellWindow * owner)
{
	this->pos = pos;
	this->owner = owner;
}

void CSpellWindow::SpellArea::clickLeft(boost::logic::tribool down)
{
	if(!down && mySpell!=-1)
	{
		//we will cast a spell
		if(LOCPLINT->battleInt) //if battle window is open
		{
			LOCPLINT->battleInt->castThisSpell(mySpell);
		}
		owner->fexitb();
	}
}

void CSpellWindow::SpellArea::clickRight(boost::logic::tribool down)
{
	if(down && mySpell != -1)
	{
		CInfoPopup *vinya = new CInfoPopup();
		vinya->free = true;
		vinya->bitmap = CMessage::drawBoxTextBitmapSub(
			LOCPLINT->playerID,
			CGI->spellh->spells[mySpell].descriptions[0], this->owner->spells->ourImages[mySpell].bitmap,
			CGI->spellh->spells[mySpell].name,30,30);
		vinya->pos.x = screen->w/2 - vinya->bitmap->w/2;
		vinya->pos.y = screen->h/2 - vinya->bitmap->h/2;
		LOCPLINT->pushInt(vinya);
	}
}

void CSpellWindow::SpellArea::hover(bool on)
{
	Hoverable::hover(on);
	if(mySpell != -1)
	{
		if(on)
		{
			std::stringstream ss;
			ss<<CGI->spellh->spells[mySpell].name<<" ("<<CGI->generaltexth->allTexts[171+CGI->spellh->spells[mySpell].level]<<")";
			owner->statusBar->print(ss.str());
		}
		else
		{
			owner->statusBar->clear();
		}
	}
}

void CSpellWindow::SpellArea::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}

void CSpellWindow::SpellArea::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}

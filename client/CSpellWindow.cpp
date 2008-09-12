#include "CSpellWindow.h"
#include "Graphics.h"
#include "../hch/CDefHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CPreGameTextHandler.h"
#include "../CAdvmapInterface.h"
#include "../CGameInfo.h"
#include "../SDL_Extensions.h"
#include <boost/bind.hpp>
#include <sstream>

extern SDL_Surface * screen;
extern SDL_Color tytulowy, zwykly ;
extern TTF_Font *GEOR16;

SpellbookInteractiveArea::SpellbookInteractiveArea(SDL_Rect & myRect, boost::function<void()> funcL, std::string textR, boost::function<void()> funcHon, boost::function<void()> funcHoff)
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

CSpellWindow::CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * myHero): selectedTab(4), spellSite(0), battleSpellsOnly(true)
{
	//for testing only
	for(ui32 v=0; v<CGI->spellh->spells.size(); ++v)
	{
		((CGHeroInstance*)(myHero))->spells.insert(v);
	}

	//initializing sizes of spellbook's parts
	for(int b=0; b<5; ++b)
		sitesPerTabAdv[b] = 0;
	for(int b=0; b<5; ++b)
		sitesPerTabBattle[b] = 0;
	for(std::set<ui32>::const_iterator g = myHero->spells.begin(); g!=myHero->spells.end(); ++g)
	{
		if(CGI->spellh->spells[*g].creatureAbility)
			continue; //currently we don't want tu put here creature abilities
		if(CGI->spellh->spells[*g].air)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[0]);
				++(sitesPerTabBattle[4]);
			}
			else
			{
				++(sitesPerTabAdv[0]);
				++(sitesPerTabAdv[4]);
			}
		}
		if(CGI->spellh->spells[*g].fire)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[1]);
				++(sitesPerTabBattle[4]);
			}
			else
			{
				++(sitesPerTabAdv[1]);
				++(sitesPerTabAdv[4]);
			}
		}
		if(CGI->spellh->spells[*g].water)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[2]);
				++(sitesPerTabBattle[4]);
			}
			else
			{
				++(sitesPerTabAdv[2]);
				++(sitesPerTabAdv[4]);
			}
		}
		if(CGI->spellh->spells[*g].earth)
		{
			if(CGI->spellh->spells[*g].combatSpell)
			{
				++(sitesPerTabBattle[3]);
				++(sitesPerTabBattle[4]);
			}
			else
			{
				++(sitesPerTabAdv[3]);
				++(sitesPerTabAdv[4]);
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
	schoolW = CDefHandler::giveDef("SplevW.def");
	schoolE = CDefHandler::giveDef("SplevE.def");
	schoolF = CDefHandler::giveDef("SplevF.def");
	schoolA = CDefHandler::giveDef("SplevA.def");



	statusBar = new CStatusBar(97, 571, "Spelroll.bmp");
	SDL_Rect temp_rect = genRect(45, 35, 569, 407);
	exitBtn = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fexitb, this), CGI->preth->zelp[460].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[460].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(45, 35, 311, 407);
	battleSpells = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fbattleSpellsb, this), CGI->preth->zelp[453].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[453].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(45, 35, 445, 407);
	adventureSpells = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fadvSpellsb, this), CGI->preth->zelp[452].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[452].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(45, 35, 508, 407);
	manaPoints = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fmanaPtsb, this), CGI->preth->zelp[459].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[459].first)), boost::bind(&CStatusBar::clear, statusBar));

	temp_rect = genRect(36, 56, 639, 96);
	selectSpellsA = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsAb, this), CGI->preth->zelp[454].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[454].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 639, 153);
	selectSpellsE = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsEb, this), CGI->preth->zelp[457].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[457].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 639, 212);
	selectSpellsF = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsFb, this), CGI->preth->zelp[455].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[455].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 639, 272);
	selectSpellsW = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsWb, this), CGI->preth->zelp[456].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[456].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(36, 56, 639, 332);
	selectSpellsAll = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fspellsAllb, this), CGI->preth->zelp[458].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[458].first)), boost::bind(&CStatusBar::clear, statusBar));

	temp_rect = genRect(leftCorner->h, leftCorner->w, 187, 79);
	lCorner = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fLcornerb, this), CGI->preth->zelp[450].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[450].first)), boost::bind(&CStatusBar::clear, statusBar));
	temp_rect = genRect(rightCorner->h, rightCorner->w, 577, 76);
	rCorner = new SpellbookInteractiveArea(temp_rect, boost::bind(&CSpellWindow::fRcornerb, this), CGI->preth->zelp[451].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[451].first)), boost::bind(&CStatusBar::clear, statusBar));
}

CSpellWindow::~CSpellWindow()
{
	SDL_FreeSurface(background);
	SDL_FreeSurface(leftCorner);
	SDL_FreeSurface(rightCorner);
	delete spells;
	delete spellTab;
	delete schools;
	delete schoolW;
	delete schoolE;
	delete schoolF;
	delete schoolA;

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
}

void CSpellWindow::fexitb()
{
	deactivate();

	for(int g=0; g<LOCPLINT->objsToBlit.size(); ++g)
	{
		if(dynamic_cast<CSpellWindow*>(LOCPLINT->objsToBlit[g]))
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+g);
			break;
		}
	}

	delete this;
	LOCPLINT->curint->activate();
}

void CSpellWindow::fadvSpellsb()
{
	battleSpellsOnly = false;
}

void CSpellWindow::fbattleSpellsb()
{
	battleSpellsOnly = true;
}

void CSpellWindow::fmanaPtsb()
{
}

void CSpellWindow::fspellsAb()
{
	selectedTab = 0;
	spellSite = 0;
}

void CSpellWindow::fspellsEb()
{
	selectedTab = 3;
	spellSite = 0;
}

void CSpellWindow::fspellsFb()
{
	selectedTab = 1;
	spellSite = 0;
}

void CSpellWindow::fspellsWb()
{
	selectedTab = 2;
	spellSite = 0;
}

void CSpellWindow::fspellsAllb()
{
	selectedTab = 4;
	spellSite = 0;
}

void CSpellWindow::fLcornerb()
{
	if(spellSite>0)
		--spellSite;
}

void CSpellWindow::fRcornerb()
{
	if((spellSite + 1) < (battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab]))
		++spellSite;
}

void CSpellWindow::show(SDL_Surface *to)
{
	if(to == NULL)  //evaluating to
		to = screen;

	SDL_BlitSurface(background, NULL, to, &pos);
	blitAt(spellTab->ourImages[selectedTab].bitmap, 614, 96, to);
	
	statusBar->show();

	if(selectedTab!=4 && spellSite == 0)
	{
		blitAt(schools->ourImages[selectedTab].bitmap, 207, 76, to);
	}

	if(spellSite!=0)
	{
		blitAt(leftCorner, lCorner->pos.x, lCorner->pos.y, to);
	}
	if((spellSite+1) != (battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab]) )
	{
		blitAt(rightCorner, rCorner->pos.x, rCorner->pos.y, to);
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
}

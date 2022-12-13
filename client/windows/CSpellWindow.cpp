/*
 * CSpellWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSpellWindow.h"

#include "../../lib/ScopeGuard.h"

#include "CAdvmapInterface.h"
#include "GUIClasses.h"
#include "InfoWindows.h"
#include "CCastleInterface.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"

#include "../battle/BattleInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/GameConstants.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

CSpellWindow::InteractiveArea::InteractiveArea(const SDL_Rect & myRect, std::function<void()> funcL, int helpTextId, CSpellWindow * _owner)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos = myRect;
	onLeft = funcL;
	hoverText = CGI->generaltexth->zelp[helpTextId].first;
	helpText = CGI->generaltexth->zelp[helpTextId].second;
	owner = _owner;
}

void CSpellWindow::InteractiveArea::clickLeft(tribool down, bool previousState)
{
	if(!down)
		onLeft();
}

void CSpellWindow::InteractiveArea::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(helpText, down);
}

void CSpellWindow::InteractiveArea::hover(bool on)
{
	if(on)
		owner->statusBar->setText(hoverText);
	else
		owner->statusBar->clear();
}

class SpellbookSpellSorter
{
public:
	bool operator()(const CSpell * A, const CSpell * B)
	{
		if(A->level < B->level)
			return true;
		if(A->level > B->level)
			return false;


		for(ui8 schoolId = 0; schoolId < 4; schoolId++)
		{
			if(A->school.at((ESpellSchool)schoolId) && !B->school.at((ESpellSchool)schoolId))
				return true;
			if(!A->school.at((ESpellSchool)schoolId) && B->school.at((ESpellSchool)schoolId))
				return false;
		}

		return A->name < B->name;
	}
} spellsorter;

CSpellWindow::CSpellWindow(const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells):
    CWindowObject(PLAYER_COLORED, "SpelBack"),
	battleSpellsOnly(openOnBattleSpells),
	selectedTab(4),
	currentPage(0),
	myHero(_myHero),
	myInt(_myInt)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	//initializing castable spells
	mySpells.reserve(CGI->spellh->objects.size());
	for(const CSpell * spell : CGI->spellh->objects)
	{
		if(!spell->isCreatureAbility() && myHero->canCastThisSpell(spell))
			mySpells.push_back(spell);
	}
	std::sort(mySpells.begin(), mySpells.end(), spellsorter);

	//initializing sizes of spellbook's parts
	for(auto & elem : sitesPerTabAdv)
		elem = 0;
	for(auto & elem : sitesPerTabBattle)
		elem = 0;

	for(const auto spell : mySpells)
	{
		int * sitesPerOurTab = spell->isCombat() ? sitesPerTabBattle : sitesPerTabAdv;

		++sitesPerOurTab[4];

		spell->forEachSchool([&sitesPerOurTab](const spells::SchoolInfo & school, bool & stop)
		{
			++sitesPerOurTab[(ui8)school.id];
		});
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

	leftCorner = std::make_shared<CPicture>("SpelTrnL.bmp", 97, 77);
	rightCorner = std::make_shared<CPicture>("SpelTrnR.bmp", 487, 72);

	spellIcons = std::make_shared<CAnimation>("Spells");

	schoolTab = std::make_shared<CAnimImage>("SpelTab", selectedTab, 0, 524, 88);
	schoolPicture = std::make_shared<CAnimImage>("Schools", 0, 0, 117, 74);

	schoolBorders[0] = std::make_shared<CAnimation>("SplevA.def");
	schoolBorders[1] = std::make_shared<CAnimation>("SplevF.def");
	schoolBorders[2] = std::make_shared<CAnimation>("SplevW.def");
	schoolBorders[3] = std::make_shared<CAnimation>("SplevE.def");

	for(auto item : schoolBorders)
		item->preload();
	mana = std::make_shared<CLabel>(435, 426, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, boost::lexical_cast<std::string>(myHero->mana));
	statusBar = CGStatusBar::create(7, 569, "Spelroll.bmp");

	SDL_Rect temp_rect = genRect(45, 35, 479 + pos.x, 405 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::fexitb, this), 460, this));
	temp_rect = genRect(45, 35, 221 + pos.x, 405 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::fbattleSpellsb, this), 453, this));
	temp_rect = genRect(45, 35, 355 + pos.x, 405 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::fadvSpellsb, this), 452, this));
	temp_rect = genRect(45, 35, 418 + pos.x, 405 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::fmanaPtsb, this), 459, this));

	temp_rect = genRect(36, 56, 549 + pos.x, 94 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 0), 454, this));
	temp_rect = genRect(36, 56, 549 + pos.x, 151 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 3), 457, this));
	temp_rect = genRect(36, 56, 549 + pos.x, 210 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 1), 455, this));
	temp_rect = genRect(36, 56, 549 + pos.x, 270 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 2), 456, this));
	temp_rect = genRect(36, 56, 549 + pos.x, 330 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 4), 458, this));

	temp_rect = genRect(leftCorner->bg->h, leftCorner->bg->w, 97 + pos.x, 77 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::fLcornerb, this), 450, this));
	temp_rect = genRect(rightCorner->bg->h, rightCorner->bg->w, 487 + pos.x, 72 + pos.y);
	interactiveAreas.push_back(std::make_shared<InteractiveArea>(temp_rect, std::bind(&CSpellWindow::fRcornerb, this), 451, this));

	//areas for spells
	int xpos = 117 + pos.x, ypos = 90 + pos.y;

	for(int v=0; v<12; ++v)
	{
		temp_rect = genRect(65, 78, xpos, ypos);
		spellAreas[v] = std::make_shared<SpellArea>(temp_rect, this);

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
	schoolTab->setFrame(selectedTab, 0);
	int cp = battleSpellsOnly ? myInt->spellbookSettings.spellbookLastPageBattle : myInt->spellbookSettings.spellbokLastPageAdvmap;
	// spellbook last page battle index is not reset after battle, so this needs to stay here
	vstd::abetween(cp, 0, std::max(0, pagesWithinCurrentTab() - 1));
	setCurrentPage(cp);
	computeSpellsPerArea();
	addUsedEvents(KEYBOARD);
}

CSpellWindow::~CSpellWindow()
{
}

void CSpellWindow::fexitb()
{
	(myInt->battleInt ? myInt->spellbookSettings.spellbookLastTabBattle : myInt->spellbookSettings.spellbookLastTabAdvmap) = selectedTab;
	(myInt->battleInt ? myInt->spellbookSettings.spellbookLastPageBattle : myInt->spellbookSettings.spellbokLastPageAdvmap) = currentPage;

	close();
}

void CSpellWindow::fadvSpellsb()
{
	if(battleSpellsOnly == true)
	{
		turnPageRight();
		battleSpellsOnly = false;
		setCurrentPage(0);
	}
	computeSpellsPerArea();
}

void CSpellWindow::fbattleSpellsb()
{
	if(battleSpellsOnly == false)
	{
		turnPageLeft();
		battleSpellsOnly = true;
		setCurrentPage(0);
	}
	computeSpellsPerArea();
}

void CSpellWindow::fmanaPtsb()
{
}

void CSpellWindow::selectSchool(int school)
{
	if(selectedTab != school)
	{
		if(selectedTab < school)
			turnPageLeft();
		else
			turnPageRight();
		selectedTab = school;
		schoolTab->setFrame(selectedTab, 0);
		setCurrentPage(0);
	}
	computeSpellsPerArea();
}

void CSpellWindow::fLcornerb()
{
	if(currentPage>0)
	{
		turnPageLeft();
		setCurrentPage(currentPage - 1);
	}
	computeSpellsPerArea();
	GH.breakEventHandling();
}

void CSpellWindow::fRcornerb()
{
	if((currentPage + 1) < (pagesWithinCurrentTab()))
	{
		turnPageRight();
		setCurrentPage(currentPage + 1);
	}
	computeSpellsPerArea();
	GH.breakEventHandling();
}

void CSpellWindow::show(SDL_Surface * to)
{
	statusBar->show(to);
}

void CSpellWindow::computeSpellsPerArea()
{
	std::vector<const CSpell *> spellsCurSite;
	spellsCurSite.reserve(mySpells.size());
	for(const CSpell * spell : mySpells)
	{
		if(spell->isCombat() ^ !battleSpellsOnly
			&& ((selectedTab == 4) || spell->school.at((ESpellSchool)selectedTab))
			)
		{
			spellsCurSite.push_back(spell);
		}
	}

	if(selectedTab == 4)
	{
		if(spellsCurSite.size() > 12)
		{
			spellsCurSite = std::vector<const CSpell *>(spellsCurSite.begin() + currentPage*12, spellsCurSite.end());
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
				spellsCurSite = std::vector<const CSpell *>(spellsCurSite.begin() + (currentPage-1)*12 + 10, spellsCurSite.end());
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
			if(c < spellsCurSite.size())
			{
				spellAreas[c]->setSpell(spellsCurSite[c]);
			}
			else
			{
				spellAreas[c]->setSpell(nullptr);
			}
		}
	}
	else
	{
		spellAreas[0]->setSpell(nullptr);
		spellAreas[1]->setSpell(nullptr);
		for(size_t c=0; c<10; ++c)
		{
			if(c < spellsCurSite.size())
				spellAreas[c+2]->setSpell(spellsCurSite[c]);
			else
				spellAreas[c+2]->setSpell(nullptr);
		}
	}
	redraw();
}

void CSpellWindow::setCurrentPage(int value)
{
	currentPage = value;
	schoolPicture->visible = selectedTab!=4 && currentPage == 0;
	if(selectedTab != 4)
		schoolPicture->setFrame(selectedTab, 0);
	leftCorner->visible = currentPage != 0;
	rightCorner->visible = (currentPage+1) < pagesWithinCurrentTab();

	mana->setText(boost::lexical_cast<std::string>(myHero->mana));//just in case, it will be possible to cast spell without closing book
}

void CSpellWindow::turnPageLeft()
{
	if(settings["video"]["spellbookAnimation"].Bool())
		CCS->videoh->openAndPlayVideo("PGTRNLFT.SMK", pos.x+13, pos.y+15);
}

void CSpellWindow::turnPageRight()
{
	if(settings["video"]["spellbookAnimation"].Bool())
		CCS->videoh->openAndPlayVideo("PGTRNRGH.SMK", pos.x+13, pos.y+15);
}

void CSpellWindow::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_ESCAPE ||  key.keysym.sym == SDLK_RETURN)
	{
		fexitb();
		return;
	}

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
		if(myInt->altPressed())
		{
			SDL_Keycode hlpKey = key.keysym.sym;
			if(CGuiHandler::isNumKey(hlpKey, false))
			{
				if(hlpKey == SDLK_KP_PLUS)
					hlpKey = SDLK_EQUALS;
				else
					hlpKey = CGuiHandler::numToDigit(hlpKey);
			}

			static const SDL_Keycode spellSelectors[] = {SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_MINUS, SDLK_EQUALS};

			int index = -1;
			while(++index < ARRAY_COUNT(spellSelectors) && spellSelectors[index] != hlpKey);
			if(index >= ARRAY_COUNT(spellSelectors))
				return;

			//try casting spell
			spellAreas[index]->clickLeft(false, true);
		}
	}
}

int CSpellWindow::pagesWithinCurrentTab()
{
	return battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab];
}

CSpellWindow::SpellArea::SpellArea(SDL_Rect pos, CSpellWindow * owner)
{
	this->pos = pos;
	this->owner = owner;
	addUsedEvents(LCLICK | RCLICK | HOVER);

	schoolLevel = -1;
	mySpell = nullptr;

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	image = std::make_shared<CAnimImage>(owner->spellIcons, 0, 0);
	image->visible = false;

	name = std::make_shared<CLabel>(39, 70, FONT_TINY, ETextAlignment::CENTER);
	level = std::make_shared<CLabel>(39, 82, FONT_TINY, ETextAlignment::CENTER);
	cost = std::make_shared<CLabel>(39, 94, FONT_TINY, ETextAlignment::CENTER);

	for(auto l : {name, level, cost})
		l->autoRedraw = false;
}

CSpellWindow::SpellArea::~SpellArea() = default;

void CSpellWindow::SpellArea::clickLeft(tribool down, bool previousState)
{
	if(mySpell && !down)
	{
		auto spellCost = owner->myInt->cb->getSpellCost(mySpell, owner->myHero);
		if(spellCost > owner->myHero->mana) //insufficient mana
		{
			owner->myInt->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[206]) % spellCost % owner->myHero->mana));
			return;
		}

		//anything that is not combat spell is adventure spell
		//this not an error in general to cast even creature ability with hero
		const bool combatSpell = mySpell->isCombat();
		if(combatSpell == mySpell->isAdventure())
		{
			logGlobal->error("Spell have invalid flags");
			return;
		}

		const bool inCombat = owner->myInt->battleInt != nullptr;
		const bool inCastle = owner->myInt->castleInt != nullptr;

		//battle spell on adv map or adventure map spell during combat => display infowindow, not cast
		if((combatSpell ^ inCombat) || inCastle)
		{
			std::vector<std::shared_ptr<CComponent>> hlp(1, std::make_shared<CComponent>(CComponent::spell, mySpell->id, 0));
			owner->myInt->showInfoDialog(mySpell->getLevelDescription(schoolLevel), hlp);
		}
		else if(combatSpell)
		{
			spells::detail::ProblemImpl problem;
			if(mySpell->canBeCast(problem, owner->myInt->cb.get(), spells::Mode::HERO, owner->myHero))
			{
				owner->myInt->battleInt->castThisSpell(mySpell->id);
				owner->fexitb();
			}
			else
			{
				std::vector<std::string> texts;
				problem.getAll(texts);
				if(!texts.empty())
					owner->myInt->showInfoDialog(texts.front());
				else
					owner->myInt->showInfoDialog(CGI->generaltexth->localizedTexts["adventureMap"]["spellUnknownProblem"].String());
			}
		}
		else //adventure spell
		{
			const CGHeroInstance * h = owner->myHero;
			GH.popInts(1);

			auto guard = vstd::makeScopeGuard([this]()
			{
				owner->myInt->spellbookSettings.spellbookLastTabAdvmap = owner->selectedTab;
				owner->myInt->spellbookSettings.spellbokLastPageAdvmap = owner->currentPage;
			});

			if(mySpell->getTargetType() == spells::AimType::LOCATION)
				adventureInt->enterCastingMode(mySpell);
			else if(mySpell->getTargetType() == spells::AimType::NO_TARGET)
				owner->myInt->cb->castSpell(h, mySpell->id);
			else
				logGlobal->error("Invalid spell target type");
		}
	}
}

void CSpellWindow::SpellArea::clickRight(tribool down, bool previousState)
{
	if(mySpell && down)
	{
		std::string dmgInfo;
		auto causedDmg = owner->myInt->cb->estimateSpellDamage(mySpell, owner->myHero);
		if(causedDmg == 0 || mySpell->id == SpellID::TITANS_LIGHTNING_BOLT) //Titan's Lightning Bolt already has damage info included
			dmgInfo.clear();
		else
		{
			dmgInfo = CGI->generaltexth->allTexts[343];
			boost::algorithm::replace_first(dmgInfo, "%d", boost::lexical_cast<std::string>(causedDmg));
		}

		CRClickPopup::createAndPush(mySpell->getLevelDescription(schoolLevel) + dmgInfo, std::make_shared<CComponent>(CComponent::spell, mySpell->id));
	}
}

void CSpellWindow::SpellArea::hover(bool on)
{
	if(mySpell)
	{
		if(on)
			owner->statusBar->setText(boost::to_string(boost::format("%s (%s)") % mySpell->name % CGI->generaltexth->allTexts[171+mySpell->level]));
		else
			owner->statusBar->clear();
	}
}

void CSpellWindow::SpellArea::setSpell(const CSpell * spell)
{
	schoolBorder.reset();
	image->visible = false;
	name->setText("");
	level->setText("");
	cost->setText("");
	mySpell = spell;
	if(mySpell)
	{
		int32_t whichSchool = 0; //0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic,
		schoolLevel = owner->myHero->getSpellSchoolLevel(mySpell, &whichSchool);
		auto spellCost = owner->myInt->cb->getSpellCost(mySpell, owner->myHero);

		image->setFrame(mySpell->id);
		image->visible = true;

		{
			OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
			schoolBorder = std::make_shared<CAnimImage>(owner->schoolBorders[owner->selectedTab >= 4 ? whichSchool : owner->selectedTab], schoolLevel);
		}

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

		name->color = firstLineColor;
		name->setText(mySpell->name);

		level->color = secondLineColor;
		if(schoolLevel > 0)
		{
			boost::format fmt("%s/%s");
			fmt % CGI->generaltexth->allTexts[171 + mySpell->level];
			fmt % CGI->generaltexth->levels.at(3+(schoolLevel-1));//lines 4-6
			level->setText(fmt.str());
		}
		else
			level->setText(CGI->generaltexth->allTexts[171 + mySpell->level]);

		cost->color = secondLineColor;
		boost::format costfmt("%s: %d");
		costfmt % CGI->generaltexth->allTexts[387] % spellCost;
		cost->setText(costfmt.str());
	}
}

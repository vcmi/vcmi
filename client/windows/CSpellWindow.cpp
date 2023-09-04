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

#include "GUIClasses.h"
#include "InfoWindows.h"
#include "CCastleInterface.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../CVideoHandler.h"

#include "../battle/BattleInterface.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"
#include "../widgets/TextControls.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../render/CAnimation.h"
#include "../render/IRenderHandler.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/GameConstants.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

CSpellWindow::InteractiveArea::InteractiveArea(const Rect & myRect, std::function<void()> funcL, int helpTextId, CSpellWindow * _owner)
{
	addUsedEvents(LCLICK | SHOW_POPUP | HOVER);
	pos = myRect;
	onLeft = funcL;
	hoverText = CGI->generaltexth->zelp[helpTextId].first;
	helpText = CGI->generaltexth->zelp[helpTextId].second;
	owner = _owner;
}

void CSpellWindow::InteractiveArea::clickPressed(const Point & cursorPosition)
{
	onLeft();
}

void CSpellWindow::InteractiveArea::showPopupWindow(const Point & cursorPosition)
{
	CRClickPopup::createAndPush(helpText);
}

void CSpellWindow::InteractiveArea::hover(bool on)
{
	if(on)
		owner->statusBar->write(hoverText);
	else
		owner->statusBar->clear();
}

class SpellbookSpellSorter
{
public:
	bool operator()(const CSpell * A, const CSpell * B)
	{
		if(A->getLevel() < B->getLevel())
			return true;
		if(A->getLevel() > B->getLevel())
			return false;


		for(auto schoolId = 0; schoolId < GameConstants::DEFAULT_SCHOOLS; schoolId++)
		{
			if(A->school.at(SpellSchool(schoolId)) && !B->school.at(SpellSchool(schoolId)))
				return true;
			if(!A->school.at(SpellSchool(schoolId)) && B->school.at(SpellSchool(schoolId)))
				return false;
		}

		return A->getNameTranslated() < B->getNameTranslated();
	}
} spellsorter;

CSpellWindow::CSpellWindow(const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells):
	CWindowObject(PLAYER_COLORED, ImagePath::builtin("SpelBack")),
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

		spell->forEachSchool([&sitesPerOurTab](const SpellSchool & school, bool & stop)
		{
			++sitesPerOurTab[school];
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

	leftCorner = std::make_shared<CPicture>(ImagePath::builtin("SpelTrnL.bmp"), 97, 77);
	rightCorner = std::make_shared<CPicture>(ImagePath::builtin("SpelTrnR.bmp"), 487, 72);

	spellIcons = GH.renderHandler().loadAnimation(AnimationPath::builtin("Spells"));

	schoolTab = std::make_shared<CAnimImage>(AnimationPath::builtin("SpelTab"), selectedTab, 0, 524, 88);
	schoolPicture = std::make_shared<CAnimImage>(AnimationPath::builtin("Schools"), 0, 0, 117, 74);

	schoolBorders[0] = GH.renderHandler().loadAnimation(AnimationPath::builtin("SplevA.def"));
	schoolBorders[1] = GH.renderHandler().loadAnimation(AnimationPath::builtin("SplevF.def"));
	schoolBorders[2] = GH.renderHandler().loadAnimation(AnimationPath::builtin("SplevW.def"));
	schoolBorders[3] = GH.renderHandler().loadAnimation(AnimationPath::builtin("SplevE.def"));

	for(auto item : schoolBorders)
		item->preload();
	mana = std::make_shared<CLabel>(435, 426, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, std::to_string(myHero->mana));
	statusBar = CGStatusBar::create(7, 569, ImagePath::builtin("Spelroll.bmp"));

	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 479 + pos.x, 405 + pos.y, 36, 56), std::bind(&CSpellWindow::fexitb,         this),    460, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 221 + pos.x, 405 + pos.y, 36, 56), std::bind(&CSpellWindow::fbattleSpellsb, this),    453, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 355 + pos.x, 405 + pos.y, 36, 56), std::bind(&CSpellWindow::fadvSpellsb,    this),    452, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 418 + pos.x, 405 + pos.y, 36, 56), std::bind(&CSpellWindow::fmanaPtsb,      this),    459, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 549 + pos.x,  94 + pos.y, 36, 56), std::bind(&CSpellWindow::selectSchool,   this, 0), 454, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 549 + pos.x, 151 + pos.y, 45, 35), std::bind(&CSpellWindow::selectSchool,   this, 3), 457, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 549 + pos.x, 210 + pos.y, 45, 35), std::bind(&CSpellWindow::selectSchool,   this, 1), 455, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 549 + pos.x, 270 + pos.y, 45, 35), std::bind(&CSpellWindow::selectSchool,   this, 2), 456, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 549 + pos.x, 330 + pos.y, 45, 35), std::bind(&CSpellWindow::selectSchool,   this, 4), 458, this));

	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect(  97 + pos.x, 77 + pos.y, leftCorner->pos.h,  leftCorner->pos.w  ), std::bind(&CSpellWindow::fLcornerb, this), 450, this));
	interactiveAreas.push_back(std::make_shared<InteractiveArea>( Rect( 487 + pos.x, 72 + pos.y, rightCorner->pos.h, rightCorner->pos.w ), std::bind(&CSpellWindow::fRcornerb, this), 451, this));

	//areas for spells
	int xpos = 117 + pos.x, ypos = 90 + pos.y;

	for(int v=0; v<12; ++v)
	{
		spellAreas[v] = std::make_shared<SpellArea>( Rect(xpos, ypos, 65, 78), this);

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

	selectedTab = battleSpellsOnly ? myInt->localState->spellbookSettings.spellbookLastTabBattle : myInt->localState->spellbookSettings.spellbookLastTabAdvmap;
	schoolTab->setFrame(selectedTab, 0);
	int cp = battleSpellsOnly ? myInt->localState->spellbookSettings.spellbookLastPageBattle : myInt->localState->spellbookSettings.spellbokLastPageAdvmap;
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
	(myInt->battleInt ? myInt->localState->spellbookSettings.spellbookLastTabBattle : myInt->localState->spellbookSettings.spellbookLastTabAdvmap) = selectedTab;
	(myInt->battleInt ? myInt->localState->spellbookSettings.spellbookLastPageBattle : myInt->localState->spellbookSettings.spellbokLastPageAdvmap) = currentPage;

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
}

void CSpellWindow::fRcornerb()
{
	if((currentPage + 1) < (pagesWithinCurrentTab()))
	{
		turnPageRight();
		setCurrentPage(currentPage + 1);
	}
	computeSpellsPerArea();
}

void CSpellWindow::show(Canvas & to)
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
			&& ((selectedTab == 4) || spell->school.at(SpellSchool(selectedTab)))
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

	mana->setText(std::to_string(myHero->mana));//just in case, it will be possible to cast spell without closing book
}

void CSpellWindow::turnPageLeft()
{
	if(settings["video"]["spellbookAnimation"].Bool())
		CCS->videoh->openAndPlayVideo(VideoPath::builtin("PGTRNLFT.SMK"), pos.x+13, pos.y+15);
}

void CSpellWindow::turnPageRight()
{
	if(settings["video"]["spellbookAnimation"].Bool())
		CCS->videoh->openAndPlayVideo(VideoPath::builtin("PGTRNRGH.SMK"), pos.x+13, pos.y+15);
}

void CSpellWindow::keyPressed(EShortcut key)
{
	switch(key)
	{
		case EShortcut::GLOBAL_RETURN:
			fexitb();
			break;

		case EShortcut::MOVE_LEFT:
			fLcornerb();
			break;
		case EShortcut::MOVE_RIGHT:
			fRcornerb();
			break;
		case EShortcut::MOVE_UP:
		case EShortcut::MOVE_DOWN:
		{
			bool down = key == EShortcut::MOVE_DOWN;
			static const int schoolsOrder[] = { 0, 3, 1, 2, 4 };
			int index = -1;
			while(schoolsOrder[++index] != selectedTab);
			index += (down ? 1 : -1);
			vstd::abetween<int>(index, 0, std::size(schoolsOrder) - 1);
			if(selectedTab != schoolsOrder[index])
				selectSchool(schoolsOrder[index]);
			break;
		}
		case EShortcut::SPELLBOOK_TAB_COMBAT:
			fbattleSpellsb();
			break;
		case EShortcut::SPELLBOOK_TAB_ADVENTURE:
			fadvSpellsb();
			break;
	}
}

int CSpellWindow::pagesWithinCurrentTab()
{
	return battleSpellsOnly ? sitesPerTabBattle[selectedTab] : sitesPerTabAdv[selectedTab];
}

CSpellWindow::SpellArea::SpellArea(Rect pos, CSpellWindow * owner)
{
	this->pos = pos;
	this->owner = owner;
	addUsedEvents(LCLICK | SHOW_POPUP | HOVER);

	schoolLevel = -1;
	mySpell = nullptr;

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	image = std::make_shared<CAnimImage>(owner->spellIcons, 0, 0);
	image->visible = false;

	name = std::make_shared<CLabel>(39, 70, FONT_TINY, ETextAlignment::CENTER);
	level = std::make_shared<CLabel>(39, 82, FONT_TINY, ETextAlignment::CENTER);
	cost = std::make_shared<CLabel>(39, 94, FONT_TINY, ETextAlignment::CENTER);

	for(auto l : {name, level, cost})
		l->setAutoRedraw(false);
}

CSpellWindow::SpellArea::~SpellArea() = default;

void CSpellWindow::SpellArea::clickPressed(const Point & cursorPosition)
{
	if(mySpell)
	{
		auto spellCost = owner->myInt->cb->getSpellCost(mySpell, owner->myHero);
		if(spellCost > owner->myHero->mana) //insufficient mana
		{
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[206]) % spellCost % owner->myHero->mana));
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
			LOCPLINT->showInfoDialog(mySpell->getDescriptionTranslated(schoolLevel), hlp);
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
					LOCPLINT->showInfoDialog(texts.front());
				else
					LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.spellUnknownProblem"));
			}
		}
		else //adventure spell
		{
			const CGHeroInstance * h = owner->myHero;
			GH.windows().popWindows(1);

			auto guard = vstd::makeScopeGuard([this]()
			{
				owner->myInt->localState->spellbookSettings.spellbookLastTabAdvmap = owner->selectedTab;
				owner->myInt->localState->spellbookSettings.spellbokLastPageAdvmap = owner->currentPage;
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

void CSpellWindow::SpellArea::showPopupWindow(const Point & cursorPosition)
{
	if(mySpell)
	{
		std::string dmgInfo;
		auto causedDmg = owner->myInt->cb->estimateSpellDamage(mySpell, owner->myHero);
		if(causedDmg == 0 || mySpell->id == SpellID::TITANS_LIGHTNING_BOLT) //Titan's Lightning Bolt already has damage info included
			dmgInfo.clear();
		else
		{
			dmgInfo = CGI->generaltexth->allTexts[343];
			boost::algorithm::replace_first(dmgInfo, "%d", std::to_string(causedDmg));
		}

		CRClickPopup::createAndPush(mySpell->getDescriptionTranslated(schoolLevel) + dmgInfo, std::make_shared<CComponent>(CComponent::spell, mySpell->id));
	}
}

void CSpellWindow::SpellArea::hover(bool on)
{
	if(mySpell)
	{
		if(on)
			owner->statusBar->write(boost::str(boost::format("%s (%s)") % mySpell->getNameTranslated() % CGI->generaltexth->allTexts[171+mySpell->getLevel()]));
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

		ColorRGBA firstLineColor, secondLineColor;
		if(spellCost > owner->myHero->mana) //hero cannot cast this spell
		{
			firstLineColor = Colors::WHITE;
			secondLineColor = Colors::ORANGE;
		}
		else
		{
			firstLineColor = Colors::YELLOW;
			secondLineColor = Colors::WHITE;
		}

		name->color = firstLineColor;
		name->setText(mySpell->getNameTranslated());

		level->color = secondLineColor;
		if(schoolLevel > 0)
		{
			boost::format fmt("%s/%s");
			fmt % CGI->generaltexth->allTexts[171 + mySpell->getLevel()];
			fmt % CGI->generaltexth->levels[3+(schoolLevel-1)];//lines 4-6
			level->setText(fmt.str());
		}
		else
			level->setText(CGI->generaltexth->allTexts[171 + mySpell->getLevel()]);

		cost->color = secondLineColor;
		boost::format costfmt("%s: %d");
		costfmt % CGI->generaltexth->allTexts[387] % spellCost;
		cost->setText(costfmt.str());
	}
}

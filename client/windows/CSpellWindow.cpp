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

#include "../battle/CBattleInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"

#include "../../lib/BattleState.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/spells/CSpellHandler.h"
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
	OBJ_CONSTRUCTION_CAPTURING_ALL;
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
		int * sitesPerOurTab = spell->isCombatSpell() ? sitesPerTabBattle : sitesPerTabAdv;

		++sitesPerOurTab[4];

		spell->forEachSchool([&sitesPerOurTab](const SpellSchoolInfo & school, bool & stop)
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

	leftCorner = new CPicture("SpelTrnL.bmp", 97, 77);
	rightCorner = new CPicture("SpelTrnR.bmp", 487, 72);

	spells = std::make_shared<CAnimation>("Spells");

	spellTab = new CAnimImage("SpelTab", selectedTab, 0, 524, 88);
	schools = new CAnimImage("Schools",0,0,117,74);

	schoolBorders[0] = std::make_shared<CAnimation>("SplevA.def");
	schoolBorders[1] = std::make_shared<CAnimation>("SplevF.def");
	schoolBorders[2] = std::make_shared<CAnimation>("SplevW.def");
	schoolBorders[3] = std::make_shared<CAnimation>("SplevE.def");

	for(auto item : schoolBorders)
		item->load();
	mana = new CLabel(435, 426, FONT_SMALL, CENTER, Colors::YELLOW, boost::lexical_cast<std::string>(myHero->mana));
	statusBar = new CGStatusBar(7, 569, "Spelroll.bmp");

	SDL_Rect temp_rect = genRect(45, 35, 479 + pos.x, 405 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::fexitb, this), 460, this);
	temp_rect = genRect(45, 35, 221 + pos.x, 405 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::fbattleSpellsb, this), 453, this);
	temp_rect = genRect(45, 35, 355 + pos.x, 405 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::fadvSpellsb, this), 452, this);
	temp_rect = genRect(45, 35, 418 + pos.x, 405 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::fmanaPtsb, this), 459, this);

	temp_rect = genRect(36, 56, 549 + pos.x, 94 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 0), 454, this);
	temp_rect = genRect(36, 56, 549 + pos.x, 151 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 3), 457, this);
	temp_rect = genRect(36, 56, 549 + pos.x, 210 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 1), 455, this);
	temp_rect = genRect(36, 56, 549 + pos.x, 270 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 2), 456, this);
	temp_rect = genRect(36, 56, 549 + pos.x, 330 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::selectSchool, this, 4), 458, this);

	temp_rect = genRect(leftCorner->bg->h, leftCorner->bg->w, 97 + pos.x, 77 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::fLcornerb, this), 450, this);
	temp_rect = genRect(rightCorner->bg->h, rightCorner->bg->w, 487 + pos.x, 72 + pos.y);
	new InteractiveArea(temp_rect, std::bind(&CSpellWindow::fRcornerb, this), 451, this);

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
	spellTab->setFrame(selectedTab, 0);
	int cp = battleSpellsOnly ? myInt->spellbookSettings.spellbookLastPageBattle : myInt->spellbookSettings.spellbokLastPageAdvmap;
	// spellbook last page battle index is not reset after battle, so this needs to stay here
	vstd::abetween(cp, 0, std::max(0, pagesWithinCurrentTab() - 1));
	setCurrentPage(cp);
	computeSpellsPerArea();
	addUsedEvents(KEYBOARD);
}

CSpellWindow::~CSpellWindow()
{
	for(auto item : schoolBorders)
		item->unload();
	spells->unload();
}

void CSpellWindow::fexitb()
{
	(myInt->battleInt ? myInt->spellbookSettings.spellbookLastTabBattle : myInt->spellbookSettings.spellbookLastTabAdvmap) = selectedTab;
	(myInt->battleInt ? myInt->spellbookSettings.spellbookLastPageBattle : myInt->spellbookSettings.spellbokLastPageAdvmap) = currentPage;

	GH.popIntTotally(this);
}

void CSpellWindow::fadvSpellsb()
{
	if (battleSpellsOnly == true)
	{
		turnPageRight();
		battleSpellsOnly = false;
		setCurrentPage(0);
	}
	computeSpellsPerArea();
}

void CSpellWindow::fbattleSpellsb()
{
	if (battleSpellsOnly == false)
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
	if (selectedTab != school)
	{
		if (selectedTab < school)
			turnPageLeft();
		else
			turnPageRight();
		selectedTab = school;
		spellTab->setFrame(selectedTab, 0);
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
		if(spell->combatSpell ^ !battleSpellsOnly
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
	schools->visible = selectedTab!=4 && currentPage == 0;
	if(selectedTab != 4)
		schools->setFrame(selectedTab, 0);
	leftCorner->visible = currentPage != 0;
	rightCorner->visible = (currentPage+1) < pagesWithinCurrentTab();

	mana->setText(boost::lexical_cast<std::string>(myHero->mana));//just in case, it will be possible to cast spell without closing book
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

	spellCost = whichSchool = schoolLevel = -1;
	mySpell = nullptr;
	schoolBorder = nullptr;

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	image = new CAnimImage(owner->spells, 0, 0);
	image->visible = false;

	name  = new CLabel(39, 70, FONT_TINY, CENTER);
	level = new CLabel(39, 82, FONT_TINY, CENTER);
	cost  = new CLabel(39, 94, FONT_TINY, CENTER);

	for(auto l : {name, level, cost})
		l->autoRedraw = false;
}

CSpellWindow::SpellArea::~SpellArea()
{

}

void CSpellWindow::SpellArea::clickLeft(tribool down, bool previousState)
{
	if(!down && mySpell)
	{
		int spellCost = owner->myInt->cb->getSpellCost(mySpell, owner->myHero);
		if(spellCost > owner->myHero->mana) //insufficient mana
		{
			owner->myInt->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[206]) % spellCost % owner->myHero->mana));
			return;
		}
		//battle spell on adv map or adventure map spell during combat => display infowindow, not cast
		if((mySpell->isCombatSpell() && !owner->myInt->battleInt)
		   || (mySpell->isAdventureSpell() && (owner->myInt->battleInt || owner->myInt->castleInt)))
		{
			std::vector<CComponent*> hlp(1, new CComponent(CComponent::spell, mySpell->id, 0));
			owner->myInt->showInfoDialog(mySpell->getLevelInfo(schoolLevel).description, hlp);
			return;
		}

		//we will cast a spell
		if(mySpell->combatSpell && owner->myInt->battleInt && owner->myInt->cb->battleCanCastSpell()) //if battle window is open
		{
			ESpellCastProblem::ESpellCastProblem problem = owner->myInt->cb->battleCanCastThisSpell(mySpell);
			switch (problem)
			{
			case ESpellCastProblem::OK:
				{
					owner->myInt->battleInt->castThisSpell(mySpell->id);
					owner->fexitb();
					return;
				}
				break;
			case ESpellCastProblem::ANOTHER_ELEMENTAL_SUMMONED:
				{
					std::string text = CGI->generaltexth->allTexts[538], elemental, caster;
					const PlayerColor player = owner->myInt->playerID;

					const TStacks stacks = owner->myInt->cb->battleGetStacksIf([player](const CStack * s)
					{
						return s->owner == player
							&& vstd::contains(s->state, EBattleStackState::SUMMONED)
							&& !vstd::contains(s->state, EBattleStackState::CLONED);
					});
					for(const CStack * s : stacks)
					{
						elemental = s->getCreature()->namePl;
					}
					if (owner->myHero->type->sex)
					{ //female
						caster = CGI->generaltexth->allTexts[540];
					}
					else
					{ //male
						caster = CGI->generaltexth->allTexts[539];
					}
					std::string summoner = owner->myHero->name;

					text = boost::str(boost::format(text) % summoner % elemental % caster);

					owner->myInt->showInfoDialog(text);
				}
				break;
			case ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED:
				{
					//Recanter's Cloak or similar effect. Try to retrieve bonus
					const auto b = owner->myHero->getBonusLocalFirst(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
					//TODO what about other values and non-artifact sources?
					if(b && b->val == 2 && b->source == Bonus::ARTIFACT)
					{
						std::string artName = CGI->arth->artifacts[b->sid]->Name();
						//The %s prevents %s from casting 3rd level or higher spells.
						owner->myInt->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[536])
							% artName % owner->myHero->name));
					}
					else if(b && b->source == Bonus::TERRAIN_OVERLAY && b->sid == BFieldType::CURSED_GROUND)
					{
						owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[537]);
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
			default:
				{
					// General message:
					std::string text = CGI->generaltexth->allTexts[541], caster = owner->myHero->name;
					text = boost::str(boost::format(text) % caster);
					owner->myInt->showInfoDialog(text);
				}
			}
		}
		else if(mySpell->isAdventureSpell() && !owner->myInt->battleInt) //adventure spell and not in battle
		{
			const CGHeroInstance *h = owner->myHero;
			GH.popInt(owner);

			auto guard = vstd::makeScopeGuard([this]
			{
				(owner->myInt->battleInt ? owner->myInt->spellbookSettings.spellbookLastTabBattle : owner->myInt->spellbookSettings.spellbookLastTabAdvmap) = owner->selectedTab;
				(owner->myInt->battleInt ? owner->myInt->spellbookSettings.spellbookLastPageBattle : owner->myInt->spellbookSettings.spellbokLastPageAdvmap) = owner->currentPage;
				delete owner;
			});

			if(mySpell->id == SpellID::TOWN_PORTAL)
			{
				//special case
				//todo: move to mechanics

				std::vector <int> availableTowns;
				std::vector <const CGTownInstance*> Towns = owner->myInt->cb->getTownsInfo(false);

				vstd::erase_if(Towns, [this](const CGTownInstance * t)
				{
					const auto relations = owner->myInt->cb->getPlayerRelations(t->tempOwner, owner->myInt->playerID);
					return relations == PlayerRelations::ENEMIES;
				});

				if (Towns.empty())
				{
					owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[124]);
					return;
				}

				const int movementCost = (h->getSpellSchoolLevel(mySpell) >= 3) ? 200 : 300;

				if(h->movement < movementCost)
				{
					owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[125]);
					return;
				}

				if (h->getSpellSchoolLevel(mySpell) < 2) //not advanced or expert - teleport to nearest available city
				{
					auto nearest = Towns.cbegin(); //nearest town's iterator
					si32 dist = owner->myInt->cb->getTown((*nearest)->id)->pos.dist2dSQ(h->pos);

					for (auto i = nearest + 1; i != Towns.cend(); ++i)
					{
						const CGTownInstance * dest = owner->myInt->cb->getTown((*i)->id);
						si32 curDist = dest->pos.dist2dSQ(h->pos);

						if (curDist < dist)
						{
							nearest = i;
							dist = curDist;
						}
					}

					if ((*nearest)->visitingHero)
						owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[123]);
					else
					{
						const CGTownInstance * town = owner->myInt->cb->getTown((*nearest)->id);
						owner->myInt->cb->castSpell(h, mySpell->id, town->visitablePos());// - town->getVisitableOffset());
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

					auto castTownPortal = [this, h](int townId)
					{
						const CGTownInstance * dest = owner->myInt->cb->getTown(ObjectInstanceID(townId));
						owner->myInt->cb->castSpell(h, SpellID::TOWN_PORTAL, dest->visitablePos());
					};

					if (availableTowns.empty())
						owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[124]);
					else
						GH.pushInt (new CObjectListWindow(availableTowns,
							new CAnimImage("SPELLSCR",mySpell->id),
							CGI->generaltexth->jktexts[40], CGI->generaltexth->jktexts[41],
							castTownPortal));
				}
				return;
			}

			if(mySpell->id == SpellID::SUMMON_BOAT)
			{
				//special case
				//todo: move to mechanics
				int3 pos = h->bestLocation();
				if(pos.x < 0)
				{
					owner->myInt->showInfoDialog(CGI->generaltexth->allTexts[334]); //There is no place to put the boat.
					return;
				}
			}

			if(mySpell->getTargetType() == CSpell::LOCATION)
			{
				adventureInt->enterCastingMode(mySpell);
			}
			else if(mySpell->getTargetType() == CSpell::NO_TARGET)
			{
				owner->myInt->cb->castSpell(h, mySpell->id);
			}
			else
			{
				logGlobal->error("Invalid spell target type");
			}
		}
	}
}

void CSpellWindow::SpellArea::clickRight(tribool down, bool previousState)
{
	if(down && mySpell)
	{
		std::string dmgInfo;
		int causedDmg = owner->myInt->cb->estimateSpellDamage(mySpell, owner->myHero);
		if(causedDmg == 0 || mySpell->id == SpellID::TITANS_LIGHTNING_BOLT) //Titan's Lightning Bolt already has damage info included
			dmgInfo = "";
		else
		{
			dmgInfo = CGI->generaltexth->allTexts[343];
			boost::algorithm::replace_first(dmgInfo, "%d", boost::lexical_cast<std::string>(causedDmg));
		}

		CRClickPopup::createAndPush(mySpell->getLevelInfo(schoolLevel).description + dmgInfo,
		                            new CComponent(CComponent::spell, mySpell->id));
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

void CSpellWindow::SpellArea::showAll(SDL_Surface * to)
{
	if(mySpell)
	{
		//printing border (indicates level of magic school)
		if(schoolBorder)
			schoolBorder->draw(to, pos.x, pos.y);
		CIntObject::showAll(to);
	}
}

void CSpellWindow::SpellArea::setSpell(const CSpell * spell)
{
	schoolBorder = nullptr;
	image->visible = false;
	name->setText("");
	level->setText("");
	cost->setText("");
	mySpell = spell;
	if(mySpell)
	{
		schoolLevel = owner->myHero->getSpellSchoolLevel(mySpell, &whichSchool);
		spellCost = owner->myInt->cb->getSpellCost(mySpell, owner->myHero);

		image->setFrame(mySpell->id);
		image->visible = true;
		schoolBorder = owner->schoolBorders[owner->selectedTab >= 4 ? whichSchool : owner->selectedTab]->getImage(schoolLevel,0);

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

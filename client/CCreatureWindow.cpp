#include "CCreatureWindow.h"
#include "../lib/CCreatureSet.h"
#include "CGameInfo.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/BattleState.h"
#include "../CCallback.h"

#include <SDL.h>
#include "SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "Graphics.h"
#include "AdventureMapButton.h"
#include "CPlayerInterface.h"
#include "CConfigHandler.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include "../lib/CGameState.h"
#include "../lib/CSpellHandler.h"

using namespace CSDL_Ext;

class CBonusItem;

/*
 * CCreatureWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CCreatureWindow::CCreatureWindow (const CStack &stack, int Type)
	: type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (stack.base)
		init(stack.base, &stack, dynamic_cast<const CGHeroInstance*>(stack.base->armyObj));
	else
	{
		CStackInstance * s = new CStackInstance(stack.type, 1); //TODO: war machines and summons should be regular stacks
		init(s, stack.type, NULL);
		delete s;
	}

}

CCreatureWindow::CCreatureWindow (const CStackInstance &stack, int Type)
	: type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	init(&stack, &stack, dynamic_cast<const CGHeroInstance*>(stack.armyObj));
}

CCreatureWindow::CCreatureWindow(int Cid, int Type, int creatureCount)
	:type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	CStackInstance * stack = new CStackInstance(Cid, creatureCount); //TODO: simplify?
	init(stack, CGI->creh->creatures[Cid], NULL);
	delete stack;
}

CCreatureWindow::CCreatureWindow(const CStackInstance &st, int Type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui)
	: type(Type), dsm(Dsm), dismiss(0), upgrade(0), ok(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(&st, &st,dynamic_cast<const CGHeroInstance*>(st.armyObj));

	//print abilities text - if r-click popup
	if(type)
	{
		if(Upg && ui)
		{
			bool enough = true;
			for(std::set<std::pair<int,int> >::iterator i=ui->cost[0].begin(); i!=ui->cost[0].end(); i++) //calculate upgrade cost
			{
				BLOCK_CAPTURING;
				if(LOCPLINT->cb->getResourceAmount(i->first) < i->second*st.count)
					enough = false;
				upgResCost.push_back(new SComponent(SComponent::resource,i->first,i->second*st.count)); 
			}

			if(enough)
			{
				CFunctionList<void()> fs;
				fs += Upg;
				fs += boost::bind(&CCreatureWindow::close,this);
				CFunctionList<void()> cfl;
				cfl = boost::bind(&CPlayerInterface::showYesNoDialog, LOCPLINT, CGI->generaltexth->allTexts[207], boost::ref(upgResCost), fs, 0, false);
				upgrade = new AdventureMapButton("",CGI->generaltexth->zelp[446].second,cfl,385, 148,"IVIEWCR.DEF",SDLK_u);
			}
			else
			{
				upgrade = new AdventureMapButton("",CGI->generaltexth->zelp[446].second,boost::function<void()>(),385, 148,"IVIEWCR.DEF");
				upgrade->callback.funcs.clear();
				upgrade->setOffset(2);
			}

		}
		if(Dsm)
		{
			CFunctionList<void()> fs[2];
			//on dismiss confirmed
			fs[0] += Dsm; //dismiss
			fs[0] += boost::bind(&CCreatureWindow::close,this);//close this window
			CFunctionList<void()> cfl;
			cfl = boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],std::vector<SComponent*>(),fs[0],fs[1],false);
			dismiss = new AdventureMapButton("",CGI->generaltexth->zelp[445].second,cfl,333, 148,"IVIEWCR2.DEF",SDLK_d);
		}
	}
}

void CCreatureWindow::init(const CStackInstance *Stack, const CBonusSystemNode *StackNode, const CGHeroInstance *HeroOwner)
{
	stack = Stack;
	c = stack->type;
	if(!StackNode)
		stackNode = c;
	else
		stackNode = StackNode;
	heroOwner = HeroOwner;

	//Basic graphics - need to calculate size

	CBonusSystemNode node = CBonusSystemNode() ;
	node.bonuses = stackNode->getBonuses(Selector::durationType(Bonus::PERMANENT));
	BonusList bl;

	while (node.bonuses.size())
	{
		Bonus * b = node.bonuses.front();

		bl.push_back (new Bonus(*b));
		bl.back()->val = node.valOfBonuses(Selector::typeSybtype(b->type, b->subtype)); //merge multiple bonuses into one
		node.bonuses.remove_if (Selector::typeSybtype(b->type, b->subtype)); //remove used bonuses
	}

	std::string text;
	BOOST_FOREACH(Bonus* b, bl)
	{
		text = stack->bonusToString(b, false);
		if (text.size()) //if it's possible to give any description for this kind of bonus
		{
			bonusItems.push_back (new CBonusItem(genRect(0, 0, 251, 57), text, stack->bonusToString(b, true), stack->bonusToGraphics(b)));
		}
	}

	bonusRows = std::min ((int)((bonusItems.size() + 1) / 2), (conf.cc.resy - 230) / 60);
	amin(bonusRows, 4);
	amax(bonusRows, 1);

	bitmap = new CPicture("CreWin" + boost::lexical_cast<std::string>(bonusRows) + ".pcx"); //1 to 4 rows for now
	bitmap->colorizeAndConvert(LOCPLINT->playerID);
	pos = bitmap->center();

	//Buttons
	ok = new AdventureMapButton("",CGI->generaltexth->zelp[445].second, boost::bind(&CCreatureWindow::close,this), 489, 148, "hsbtns.def", SDLK_RETURN);

	if (type <= BATTLE) //in battle or info window
	{
		upgrade = NULL;
		dismiss = NULL;
	}
	anim = new CCreaturePic(22, 48, c);

	//Stats
	morale = new MoraleLuckBox(true, genRect(42, 42, 335, 100));
	morale->set(stack);
	luck = new MoraleLuckBox(false, genRect(42, 42, 387, 100));
	luck->set(stack);

	new CPicture(graphics->pskillsm->ourImages[4].bitmap, 335, 50, false); //exp icon - Print it always?
	if (type) //not in fort window
	{
		if (STACK_EXP)
		{
			int rank = std::min(stack->getExpRank(), 10); //hopefully nobody adds more
			printAtMiddle(CGI->generaltexth->zcrexp[rank] + " [" + boost::lexical_cast<std::string>(rank) + "]", 436, 62, FONT_MEDIUM, tytulowy,*bitmap);
			printAtMiddle(boost::lexical_cast<std::string>(stack->experience), 436, 82, FONT_SMALL, zwykly,*bitmap);
			if (type > BATTLE) //we need it only on adv. map
			{
				int tier = stack->type->level;
				if (!iswith(tier, 1, 7))
					tier = 0;
				int number;
				std::string expText = CGI->generaltexth->zcrexp[324];
				boost::replace_first (expText, "%s", c->namePl);
				boost::replace_first (expText, "%s", CGI->generaltexth->zcrexp[rank]);
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(rank));
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(stack->experience));
				number = CGI->creh->expRanks[tier][rank] - stack->experience;
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number));

				number = CGI->creh->maxExpPerBattle[tier]; //percent
				boost::replace_first (expText, "%i%", boost::lexical_cast<std::string>(number));
				number *= CGI->creh->expRanks[tier].back() / 100; //actual amount
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number));

				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(stack->count)); //Number of Creatures in stack

				int expmin = std::max(CGI->creh->expRanks[tier][std::max(rank-1, 0)], (ui32)1);
				number = (stack->count * (stack->experience - expmin)) / expmin; //Maximum New Recruits without losing current Rank
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number)); //TODO

				boost::replace_first (expText, "%.2f", boost::lexical_cast<std::string>(1)); //TODO Experience Multiplier
				number = CGI->creh->expAfterUpgrade;
				boost::replace_first (expText, "%.2f", boost::lexical_cast<std::string>(number) + "%"); //Upgrade Multiplier

				expmin = CGI->creh->expRanks[tier][9];
				int expmax = CGI->creh->expRanks[tier][10];
				number = expmax - expmin;
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number)); //Experience after Rank 10 
				number = (stack->count * (expmax - expmin)) / expmin; 
				boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number)); //Maximum New Recruits to remain at Rank 10 if at Maximum Experience 
			
				expArea = new LRClickableAreaWTextComp(Rect(334, 49, 160, 44),SComponent::experience);
				expArea->text = expText;
				expArea->bonusValue = 0; //TDO: some specific value or no number at all
			}
		}

		if (STACK_ARTIFACT && type > BATTLE)
		{
			//SDL_Rect rect = genRect(44,44,465,98);
			//creatureArtifact = new CArtPlace(NULL);
			//creatureArtifact->pos = rect;
			//creatureArtifact->ourOwner = NULL; //hmm?
			leftArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind (&CCreatureWindow::scrollArt, this, -1), 437, 98, "hsbtns3.def", SDLK_LEFT);
			rightArtRoll = new AdventureMapButton(std::string(), std::string(), boost::bind (&CCreatureWindow::scrollArt, this, +1), 516, 98, "hsbtns5.def", SDLK_RIGHT);
		}
		else
			creatureArtifact = NULL;
	}

	if(const CStack *battleStack = dynamic_cast<const CStack*>(stackNode)) //only during battle
	{
		//spell effects
		int printed=0; //how many effect pics have been printed
		std::vector<si32> spells = battleStack->activeSpells();
		BOOST_FOREACH(si32 effect, spells)
		{
			std::string spellText;
			if (effect < graphics->spellEffectsPics->ourImages.size()) //not all effects have graphics (for eg. Acid Breath)
			{
				spellText = CGI->generaltexth->allTexts[610]; //"%s, duration: %d rounds."	
				boost::replace_first (spellText, "%s", CGI->spellh->spells[effect]->name);
				int duration = battleStack->getBonus(Selector::source(Bonus::SPELL_EFFECT,effect))->turnsRemain;
				boost::replace_first (spellText, "%d", boost::lexical_cast<std::string>(duration)); 
				blitAt(graphics->spellEffectsPics->ourImages[effect + 1].bitmap, 20 + 52 * printed, 184, *bitmap);
				spellEffects.push_back(new LRClickableAreaWText(Rect(20 + 52 * printed, 184, 50, 38), spellText, spellText));
				if (++printed >= 10) //we can fit only 10 effects
					break;
			}
		}
		//print current health
		printLine (5, CGI->generaltexth->allTexts[200], battleStack->firstHPleft);
	}

	if (bonusItems.size() > (bonusRows << 1)) //only after graphics are created
	{
		slider = new CSlider(528, 231, bonusRows*60, boost::bind (&CCreatureWindow::sliderMoved, this, _1),
		bonusRows, (bonusItems.size() + 1) >> 1, 0, false, 0);
	}
	else //slider automatically places bonus Items
		recreateSkillList (0);

	showAll(screen2);

	//AUIDAT.DEF
}

void CCreatureWindow::printLine(int nr, const std::string &text, int baseVal, int val/*=-1*/, bool range/*=false*/)
{
	printAt(text, 162, 48 + nr*19, FONT_SMALL, zwykly, *bitmap);

	std::string hlp;
	if(range && baseVal != val)
		hlp = boost::str(boost::format("%d - %d") % baseVal % val);
	else if(baseVal != val && val>=0)
		hlp = boost::str(boost::format("%d (%d)") % baseVal % val);
	else
		hlp = boost::lexical_cast<std::string>(baseVal);

	printTo(hlp, 325, 64 + nr*19, FONT_SMALL, zwykly, *bitmap);
}

void CCreatureWindow::recreateSkillList(int Pos)
{
	int n = 0, i = 0, j = 0;
	int numSkills = std::min ((bonusRows + Pos) << 1, (int)bonusItems.size());
	std::string gfxName;
	for (n = 0; n < Pos << 1; ++n)
	{
		bonusItems[n]->visible = false;
	}
	for (n = Pos << 1; n < numSkills; ++n)
	{
		int offsetx = 257*j - (bonusRows == 4 ? 1 : 0);
		int offsety = 60*i + (bonusRows > 1 ? 1 : 0); //lack of precision :/

		bonusItems[n]->moveTo (Point(pos.x + offsetx + 10, pos.y + offsety + 230), true);
		bonusItems[n]->visible = true;

		if (++j > 1) //next line
		{
			++i;
			j = 0;
		}
	}
	for (n = numSkills; n < bonusItems.size(); ++n)
	{
		bonusItems[n]->visible = false;
	}
}

void CCreatureWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	count = boost::lexical_cast<std::string>(stack->count);
	if (count.size()) //TODO
		printTo(count, 117, 174, FONT_SMALL, tytulowy,*bitmap);
	printAtMiddle(c->namePl, 180, 30, FONT_SMALL, tytulowy,*bitmap); //creature name

	printLine(0, CGI->generaltexth->primarySkillNames[0], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), stackNode->Attack());
	printLine(1, CGI->generaltexth->primarySkillNames[1], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), stackNode->Defense());
	//if(c->shots)
	//	printLine(2, CGI->generaltexth->allTexts[198], c->shots);
	if(stackNode->valOfBonuses(Bonus::SHOTS))
		printLine(2, CGI->generaltexth->allTexts[198], stackNode->valOfBonuses(Bonus::SHOTS));

	//TODO
	int dmgMultiply = 1;
	if(heroOwner && stackNode->hasBonusOfType(Bonus::SIEGE_WEAPON))
		dmgMultiply += heroOwner->Attack(); 

	printLine(3, CGI->generaltexth->allTexts[199], stackNode->getMinDamage() * dmgMultiply, stackNode->getMaxDamage() * dmgMultiply, true);
	printLine(4, CGI->generaltexth->allTexts[388], c->valOfBonuses(Bonus::STACK_HEALTH), stackNode->valOfBonuses(Bonus::STACK_HEALTH));
	printLine(6, CGI->generaltexth->zelp[441].first, c->valOfBonuses(Bonus::STACKS_SPEED), stackNode->valOfBonuses(Bonus::STACKS_SPEED));

	BOOST_FOREACH(CBonusItem* b, bonusItems)
		b->showAll (to);
}

void CCreatureWindow::sliderMoved(int newpos)
{
	recreateSkillList(newpos); //move components
	redraw();
}

void CCreatureWindow::scrollArt(int dir)
{
}

void CCreatureWindow::clickRight(tribool down, bool previousState)
{
	if(down)
		return;
	if (type < 3)
		close();
}

//void CCreatureWindow::activate()
//{
//	CIntObject::activate();
//	if(type < 3)
//		activateRClick();
//}

//void CCreatureWindow::deactivate()
//{
//
//}

void CCreatureWindow::close()
{
	GH.popIntTotally(this);
}

CCreatureWindow::~CCreatureWindow()
{
 	for (int i=0; i<upgResCost.size(); ++i)
 		delete upgResCost[i];
	bonusItems.clear();
}

CBonusItem::CBonusItem()
{

}

CBonusItem::CBonusItem(const Rect &Pos, const std::string &Name, const std::string &Description, const std::string &graphicsName)
{
	OBJ_CONSTRUCTION;
	SHARE_POS;
	visible = false;

	name = Name;
	description = Description;
	if (graphicsName.size())
		bonusGraphics = new CPicture(graphicsName, 26, 232);
	else
		bonusGraphics = NULL;

	used = 0; //no actions atm
}

void CBonusItem::showAll (SDL_Surface * to)
{
	if (visible)
	{
		printAt(name, pos.x + 72, pos.y + 6, FONT_SMALL, tytulowy, to);
		printAt(description, pos.x + 72, pos.y + 30, FONT_SMALL, zwykly, to);
		if (bonusGraphics && bonusGraphics->bg)
			blitAtLoc(bonusGraphics->bg, 12, 2, to);
	}
}

CBonusItem::~CBonusItem()
{
	//delete bonusGraphics; //automatic destruction
}

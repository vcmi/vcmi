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

using namespace CSDL_Ext;

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

	init(stack.base, &stack, dynamic_cast<const CGHeroInstance*>(stack.base->armyObj));
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

	CStackInstance * stack = new CStackInstance(Type,creatureCount); //TODO: simplify?
	init(stack, CGI->creh->creatures[Cid], NULL);
	delete stack;
}

void CCreatureWindow::init(const CStackInstance *stack, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner)
{
	c = stack->type;
	if(!stackNode) stackNode = c;

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

	std::vector<std::pair<std::string, std::string>> descriptions; //quick, yet slow solution
	std::string text, text2;
	BOOST_FOREACH(Bonus* b, bl)
	{
		text = stack->bonusToString(b, false);
		if (text.size())
		{
			text2 = stack->bonusToString(b, true);
			descriptions.push_back(std::pair<std::string,std::string>(text, text2));
		}
	}

	int bonusRows = std::min ((int)((descriptions.size() + 1) / 2), (conf.cc.resy - 230) / 60);
	amin(bonusRows, 4);
	amax(bonusRows, 1);
	//TODO: Scroll them

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
	count = boost::lexical_cast<std::string>(stack->count);
	if (count.size()) //TODO
		printTo(count, 117, 174, FONT_SMALL, tytulowy,*bitmap);
	printAtMiddle(c->namePl, 180, 30, FONT_SMALL, tytulowy,*bitmap); //creature name

	//Stats
	morale = new MoraleLuckBox(true, genRect(42, 42, 335, 100));
	morale->set(stack);
	luck = new MoraleLuckBox(false, genRect(42, 42, 387, 100));
	luck->set(stack);

	printLine(0, CGI->generaltexth->primarySkillNames[0], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), stackNode->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK));
	printLine(1, CGI->generaltexth->primarySkillNames[1], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), stackNode->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE));
	if(c->shots)
		printLine(2, CGI->generaltexth->allTexts[198], c->shots);

	//TODO
	int dmgMultiply = 1;
	if(heroOwner && stackNode->hasBonusOfType(Bonus::SIEGE_WEAPON))
		dmgMultiply += heroOwner->Attack(); 

	printLine(3, CGI->generaltexth->allTexts[199], stackNode->getMinDamage() * dmgMultiply, stackNode->getMaxDamage() * dmgMultiply, true);
	printLine(4, CGI->generaltexth->allTexts[388], c->valOfBonuses(Bonus::STACK_HEALTH), stackNode->valOfBonuses(Bonus::STACK_HEALTH));
	printLine(6, CGI->generaltexth->zelp[441].first, c->valOfBonuses(Bonus::STACKS_SPEED), stackNode->valOfBonuses(Bonus::STACKS_SPEED));

	if (stack)
	{
		if (STACK_EXP)
		{
			printAtMiddle("Rank " + boost::lexical_cast<std::string>(stack->getExpRank()), 436, 62, FONT_MEDIUM, tytulowy,*bitmap);
			printAtMiddle(boost::lexical_cast<std::string>(stack->experience), 436, 82, FONT_SMALL, zwykly,*bitmap);
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
			blitAt(graphics->spellEffectsPics->ourImages[effect + 1].bitmap, 20 + 52 * printed, 184, *bitmap); 
			if (++printed >= 10) //we can fit only 10 effects
				break;
		}
		//print current health
		printLine (5, CGI->generaltexth->allTexts[200], battleStack->firstHPleft);
	}

	//All bonuses - abilities

	int i = 0, j = 0;
	typedef std::pair<std::string, std::string> stringpair; //jeez
	BOOST_FOREACH(stringpair p, descriptions)
	{
		int offsetx = 257*j;
		int offsety = 60*i;

		printAt(p.first, 84 + offsetx, 238 + offsety, FONT_SMALL, tytulowy, *bitmap);
		printAt(p.second, 84 + offsetx, 258 + offsety, FONT_SMALL, zwykly, *bitmap);

		if (++j > 1) //next line
		{
			++i;
			j = 0;
		}
			//text = stack->bonusToString(*it, true);
			//if (text.size())
			//	printAt(text, 80 + offsety, 262 + offsetx, FONT_SMALL, zwykly, *bitmap);
	}

	//AUIDAT.DEF
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

//void CCreatureWindow::activate()
//{
//	CIntObject::activate();
//	if(type < 3)
//		activateRClick();
//}

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

void CCreatureWindow::close()
{
	GH.popIntTotally(this);
}

CCreatureWindow::~CCreatureWindow()
{
 	for(int i=0; i<upgResCost.size();i++)
 		delete upgResCost[i];
}
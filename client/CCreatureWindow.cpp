#include "StdInc.h"
#include "CCreatureWindow.h"

#include "../lib/CCreatureSet.h"
#include "CGameInfo.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/BattleState.h"
#include "../CCallback.h"

#include <SDL.h>
#include "gui/SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "Graphics.h"
#include "CPlayerInterface.h"
#include "../lib/CConfigHandler.h"
#include "CAnimation.h"

#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/CSpellHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/NetPacks.h" //ArtifactLocation
#include "../lib/CModHandler.h"
#include "../lib/IBonusTypeHandler.h"

#include "gui/CGuiHandler.h"
#include "gui/CIntObjectClasses.h"

using namespace CSDL_Ext;

class CCreatureArtifactInstance;
class CSelectableSkill;

/*
 * CCreatureWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CCreatureWindow::CCreatureWindow (const CStack &stack, CreWinType Type):
    CWindowObject(PLAYER_COLORED | (Type == OTHER ? RCLICK_POPUP : 0 ) ),
    type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (stack.base)
		init(stack.base, &stack, dynamic_cast<const CGHeroInstance*>(stack.base->armyObj));
	else
	{
		auto   s = new CStackInstance(stack.type, 1); //TODO: war machines and summons should be regular stacks
		init(s, &stack, nullptr);
		delete s;
	}
}

CCreatureWindow::CCreatureWindow (const CStackInstance &stack, CreWinType Type):
    CWindowObject(PLAYER_COLORED | (Type == OTHER ? RCLICK_POPUP : 0 ) ),
    type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	init(&stack, &stack, dynamic_cast<const CGHeroInstance*>(stack.armyObj));
}

CCreatureWindow::CCreatureWindow(CreatureID Cid, CreWinType Type, int creatureCount):
   CWindowObject(PLAYER_COLORED | (Type == OTHER ? RCLICK_POPUP : 0 ) ),
    type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	auto   stack = new CStackInstance(Cid, creatureCount); //TODO: simplify?
	init(stack, CGI->creh->creatures[Cid], nullptr);
	delete stack;
}

CCreatureWindow::CCreatureWindow(const CStackInstance &st, CreWinType Type, std::function<void()> Upg, std::function<void()> Dsm, UpgradeInfo *ui):
    CWindowObject(PLAYER_COLORED | (Type == OTHER ? RCLICK_POPUP : 0 ) ),
    type(Type),
    dismiss(nullptr),
    upgrade(nullptr),
    ok(nullptr),
    dsm(Dsm)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(&st, &st,dynamic_cast<const CGHeroInstance*>(st.armyObj));

	//print abilities text - if r-click popup
	if(type)
	{
		if(Upg && ui)
		{
			TResources upgradeCost = ui->cost[0] * st.count;
			for(TResources::nziterator i(upgradeCost); i.valid(); i++)
			{
				BLOCK_CAPTURING;
				upgResCost.push_back(new CComponent(CComponent::resource, i->resType, i->resVal));
			}

			if(LOCPLINT->cb->getResourceAmount().canAfford(upgradeCost))
			{
				CFunctionList<void()> fs;
				fs += Upg;
				fs += std::bind(&CCreatureWindow::close,this);
				CFunctionList<void()> cfl;
				cfl = std::bind(&CPlayerInterface::showYesNoDialog, LOCPLINT, CGI->generaltexth->allTexts[207], fs, nullptr, false, std::ref(upgResCost));
				upgrade = new CAdventureMapButton("",CGI->generaltexth->zelp[446].second,cfl,385, 148,"IVIEWCR.DEF",SDLK_u);
			}
			else
			{
				upgrade = new CAdventureMapButton("",CGI->generaltexth->zelp[446].second,std::function<void()>(),385, 148,"IVIEWCR.DEF");
				upgrade->callback.funcs.clear();
				upgrade->setOffset(2);
			}

		}
		if(Dsm)
		{
			CFunctionList<void()> fs[2];
			//on dismiss confirmed
			fs[0] += Dsm; //dismiss
			fs[0] += std::bind(&CCreatureWindow::close,this);//close this window
			CFunctionList<void()> cfl;
			cfl = std::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],fs[0],fs[1],false,std::vector<CComponent*>());
			dismiss = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second,cfl,333, 148,"IVIEWCR2.DEF",SDLK_d);
		}
	}
}

CCreatureWindow::CCreatureWindow (const CCommanderInstance * Commander, const CStack * stack):
    CWindowObject(PLAYER_COLORED),
	commander (Commander)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (stack)
	{
		type = COMMANDER_BATTLE;
		init(commander, stack, dynamic_cast<const CGHeroInstance*>(commander->armyObj));
	}
	else
	{
		type = COMMANDER;
		init(commander, commander, dynamic_cast<const CGHeroInstance*>(commander->armyObj));
	}

	std::function<void()> Dsm;
	CFunctionList<void()> fs[2];
	//on dismiss confirmed
	fs[0] += Dsm; //dismiss
	fs[0] += std::bind(&CCreatureWindow::close,this);//close this window
	CFunctionList<void()> cfl;
	cfl = std::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],fs[0],fs[1],false,std::vector<CComponent*>());
	if (type < COMMANDER_LEVEL_UP) //can dismiss only in regular window
		dismiss = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, cfl, 333, 148,"IVIEWCR2.DEF", SDLK_d);
}

CCreatureWindow::CCreatureWindow (std::vector<ui32> &skills, const CCommanderInstance * Commander, std::function<void(ui32)> callback):
    CWindowObject(PLAYER_COLORED),
    type(COMMANDER_LEVEL_UP),
	commander (Commander),
	selectedOption (0), //choose something before drawing
	upgradeOptions(skills), //copy skills to choose from
	levelUp (callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	init(commander, commander, dynamic_cast<const CGHeroInstance*>(commander->armyObj));

	std::function<void()> Dsm;
	CFunctionList<void()> fs[2];
	//on dismiss confirmed
	fs[0] += Dsm; //dismiss
	fs[0] += std::bind(&CCreatureWindow::close,this);//close this window
	CFunctionList<void()> cfl;
	cfl = std::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],fs[0],fs[1],false,std::vector<CComponent*>());
	if (type < COMMANDER_LEVEL_UP) //can dismiss only in regular window
		dismiss = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, cfl, 333, 148,"IVIEWCR2.DEF", SDLK_d);
}

void CCreatureWindow::init(const CStackInstance *Stack, const CBonusSystemNode *StackNode, const CGHeroInstance *HeroOwner)
{
	creatureArtifact = nullptr; //may be set later
	artifactImage = nullptr;
	stack = Stack;
	c = stack->type;
	if(!StackNode)
		stackNode = c;
	else
		stackNode = StackNode;
	const CStack *battleStack = dynamic_cast<const CStack*>(stackNode); //only during battle
	heroOwner = HeroOwner;

	if (battleStack)
		count = boost::lexical_cast<std::string>(battleStack->count);
	else if (Stack->count)
		count = boost::lexical_cast<std::string>(Stack->count);

	if (type < COMMANDER)
		commander = nullptr;

	bool creArt = false;
	displayedArtifact = ArtifactPosition::CREATURE_SLOT; // 0

	//Basic graphics - need to calculate size

	int commanderOffset = 0;
	if (type >= COMMANDER)
		commanderOffset = 74;

	if (commander) //secondary skills
	{
		creArt = true;
		for (int i = ECommander::ATTACK; i <= ECommander::SPELL_POWER; ++i)
		{
			if (commander->secondarySkills[i] || vstd::contains(upgradeOptions, i))
			{
				std::string file = skillToFile(i);

				skillPictures.push_back(new CPicture(file, 0,0));
			}
		}

		if (type == COMMANDER_LEVEL_UP)
		{
			for (auto option : upgradeOptions)
			{
				ui32 index = selectableSkills.size();
				auto   selectableSkill = new CSelectableSkill();
				selectableSkill->callback = std::bind(&CCreatureWindow::selectSkill, this, index);

				if (option < 100)
				{
					selectableSkill->pos = skillPictures[index]->pos; //resize
					selectableSkills.push_back (selectableSkill);
				}
				else
				{
					selectableSkill->pos = Rect (95, 256, 55, 55); //TODO: scroll
					const Bonus *b = CGI->creh->skillRequirements[option-100].first; 
					bonusItems.push_back (new CBonusItem (genRect(0, 0, 251, 57), stack->bonusToString(b, false), stack->bonusToString(b, true), stack->bonusToGraphics(b)));
					selectableBonuses.push_back (selectableSkill); //insert these before other bonuses
				}
			}
		}
	}

	BonusList bl, blTemp;
	blTemp = (*(stackNode->getBonuses(Selector::durationType(Bonus::PERMANENT).And(Selector::anyRange()))));

	while (blTemp.size())
	{
		Bonus * b = blTemp.front();

		bl.push_back (new Bonus(*b));
		bl.back()->val = blTemp.valOfBonuses(Selector::typeSubtype(b->type, b->subtype)); //merge multiple bonuses into one
		blTemp.remove_if (Selector::typeSubtype(b->type, b->subtype)); //remove used bonuses
	}

	std::string text;
	for(Bonus* b : bl)
	{
		text = stack->bonusToString(b, false);
		if (text.size()) //if it's possible to give any description for this kind of bonus
		{
			bonusItems.push_back (new CBonusItem(genRect(0, 0, 251, 57), text, stack->bonusToString(b, true), stack->bonusToGraphics(b)));
		}
	}


	//handle Magic resistance separately :/
	const IBonusBearer *temp = stack;
	
	if (battleStack)
	{
		temp = battleStack;
	}

	int magicResistance = temp->magicResistance(); 
	
	if (magicResistance)
	{
		Bonus b;
		b.type = Bonus::MAGIC_RESISTANCE;
		
		text = VLC->getBth()->bonusToString(&b,temp,false);
		const std::string description = VLC->getBth()->bonusToString(&b,temp,true);
		bonusItems.push_back (new CBonusItem(genRect(0, 0, 251, 57), text, description, stack->bonusToGraphics(&b)));
	}

	bonusRows = std::min ((int)((bonusItems.size() + 1) / 2), (screen->h - 230) / 60);
	if (type >= COMMANDER)
		vstd::amin(bonusRows, 3);
	else
		vstd::amin(bonusRows, 4);
	vstd::amax(bonusRows, 1);

	if (type >= COMMANDER)
	{
		setBackground("CommWin" + boost::lexical_cast<std::string>(bonusRows) + ".pcx");
		for (int i = 0; i < skillPictures.size(); ++i)
		{
			skillPictures[i]->moveTo (Point (pos.x + 37 + i * 84, pos.y + 224));
		}
		for (int i = 0; i < selectableSkills.size(); ++i)
		{
			if (upgradeOptions[i] < skillPictures.size()) // it's secondary skill
			{
				selectableSkills[i]->pos = skillPictures[upgradeOptions[i]]->pos; //dirty workaround
			}
			else
				break;
		}
		//print commander level
		new CLabel(488, 62, FONT_MEDIUM, CENTER, Colors::YELLOW,
		           boost::lexical_cast<std::string>((ui16)(commander->level)));

		new CLabel(488, 82, FONT_SMALL, CENTER, Colors::WHITE,
		           boost::lexical_cast<std::string>(stack->experience));
	}
	else
		setBackground("CreWin" + boost::lexical_cast<std::string>(bonusRows) + ".pcx"); //1 to 4 rows for now

	//Buttons
	ok = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, std::bind(&CCreatureWindow::close,this), 489, 148, "hsbtns.def", SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);

	if (type <= BATTLE) //in battle or info window
	{
		upgrade = nullptr;
		dismiss = nullptr;
	}
	anim = new CCreaturePic(22, 48, c);

	//Stats
	morale = new MoraleLuckBox(true, genRect(42, 42, 335, 100));
	morale->set(stackNode);
	luck = new MoraleLuckBox(false, genRect(42, 42, 387, 100));
	luck->set(stackNode);

	new CAnimImage("PSKIL42", 4, 0, 387, 51); //exp icon - Print it always?
	if (type) //not in fort window
	{
		if (CGI->modh->modules.STACK_EXP && type < COMMANDER)
		{
			int rank = std::min(stack->getExpRank(), 10); //hopefully nobody adds more
			new CLabel(488, 82, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(stack->experience));
			new CLabel(488, 62, FONT_MEDIUM, CENTER, Colors::YELLOW,
			           CGI->generaltexth->zcrexp[rank] + " [" + boost::lexical_cast<std::string>(rank) + "]");

			if (type > BATTLE) //we need it only on adv. map
			{
				int tier = stack->type->level;
				if (!vstd::iswithin(tier, 1, 7))
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

				expArea = new LRClickableAreaWTextComp(Rect(334, 49, 160, 44),CComponent::experience);
				expArea->text = expText;
				expArea->bonusValue = 0; //TDO: some specific value or no number at all
			}
		}

		if (CGI->modh->modules.STACK_ARTIFACT)
		{
			creArt = true;
		}
	}
	if (creArt) //stack or commander artifacts
	{
		setArt (stack->getArt(ArtifactPosition::CREATURE_SLOT));
		if (type > BATTLE && type < COMMANDER_BATTLE) //artifact buttons inactive in battle
		{
			//TODO: disable buttons if no artifact is equipped
			leftArtRoll = new CAdventureMapButton(std::string(), std::string(), std::bind (&CCreatureWindow::scrollArt, this, -1), 437, 98, "hsbtns3.def", SDLK_LEFT);
			rightArtRoll = new CAdventureMapButton(std::string(), std::string(), std::bind (&CCreatureWindow::scrollArt, this, +1), 516, 98, "hsbtns5.def", SDLK_RIGHT);
			if (heroOwner)
				passArtToHero = new CAdventureMapButton(std::string(), std::string(), std::bind (&CCreatureWindow::passArtifactToHero, this), 437, 148, "OVBUTN1.DEF", SDLK_HOME);
		}
	}

	if (battleStack) //only during battle
	{
		//spell effects
		int printed=0; //how many effect pics have been printed
		std::vector<si32> spells = battleStack->activeSpells();
		for(si32 effect : spells)
		{
			std::string spellText;
			if (effect < graphics->spellEffectsPics->ourImages.size()) //not all effects have graphics (for eg. Acid Breath)
			{
				spellText = CGI->generaltexth->allTexts[610]; //"%s, duration: %d rounds."
				boost::replace_first (spellText, "%s", CGI->spellh->spells[effect]->name);
				int duration = battleStack->getBonusLocalFirst(Selector::source(Bonus::SPELL_EFFECT,effect))->turnsRemain;
				boost::replace_first (spellText, "%d", boost::lexical_cast<std::string>(duration));

				new CAnimImage("SpellInt", effect + 1, 0, 20 + 52 * printed, 184);
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
		slider = new CSlider(528, 231 + commanderOffset, bonusRows*60, std::bind (&CCreatureWindow::sliderMoved, this, _1),
		bonusRows, (bonusItems.size() + 1) >> 1, 0, false, 0);
	}
	else //slider automatically places bonus Items
		recreateSkillList (0);

	showAll(screen2);

	//AUIDAT.DEF
}

void CCreatureWindow::printLine(int nr, const std::string &text, int baseVal, int val/*=-1*/, bool range/*=false*/)
{
	new CLabel(162, 48 + nr*19, FONT_SMALL, TOPLEFT, Colors::WHITE, text);

	std::string hlp;
	if(range && baseVal != val)
		hlp = boost::str(boost::format("%d - %d") % baseVal % val);
	else if(baseVal != val && val>=0)
		hlp = boost::str(boost::format("%d (%d)") % baseVal % val);
	else
		hlp = boost::lexical_cast<std::string>(baseVal);

	new CLabel(325, 64 + nr*19, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, hlp);
}

void CCreatureWindow::recreateSkillList(int Pos)
{
	int commanderOffset = 0;
	if (type >= COMMANDER)
		commanderOffset = 74;

	int n = 0, i = 0, j = 0;
	int numSkills = std::min ((bonusRows + Pos) << 1, (int)bonusItems.size());
	for (n = 0; n < Pos << 1; ++n)
	{
		bonusItems[n]->visible = false;
		if (n < selectableBonuses.size())
			selectableBonuses[n]->deactivate(); //we assume that bonuses are at front of the list
	}
	for (n = Pos << 1; n < numSkills; ++n)
	{
		int offsetx = 257*j - (bonusRows == 4 ? 1 : 0);
		int offsety = 60*i + (bonusRows > 1 ? 1 : 0) + commanderOffset; //lack of precision :/

		bonusItems[n]->moveTo (Point(pos.x + offsetx + 10, pos.y + offsety + 230), true);
		bonusItems[n]->visible = true;
		if (n < selectableBonuses.size())
		{
			selectableBonuses[n]->moveTo (Point(bonusItems[n]->pos.x + 12, bonusItems[n]->pos.y + 2)); //for some reason bonusItems have dimensions 0?
			//selectableBonuses[n]->pos = bonusItems[n]->bonusGraphics->pos;
			selectableBonuses[n]->activate();
		}

		if (++j > 1) //next line
		{
			++i;
			j = 0;
		}
	}
	for (n = numSkills; n < bonusItems.size(); ++n)
	{
		bonusItems[n]->visible = false;
		if (n < selectableBonuses.size())
			selectableBonuses[n]->deactivate();
	}
}

void CCreatureWindow::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	printAtMiddleLoc((type >= COMMANDER ? c->nameSing : c->namePl), 180, 30, FONT_SMALL, Colors::YELLOW, to); //creature name

	printLine(0, CGI->generaltexth->primarySkillNames[0], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), stackNode->Attack());
	printLine(1, CGI->generaltexth->primarySkillNames[1], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), stackNode->Defense());

	if (stackNode->valOfBonuses(Bonus::SHOTS) && stackNode->hasBonusOfType(Bonus::SHOOTER))
	{//only for shooting units - important with wog exp shooters
		if (type == BATTLE)
			printLine(2, CGI->generaltexth->allTexts[198], stackNode->valOfBonuses(Bonus::SHOTS), dynamic_cast<const CStack*>(stackNode)->shots);
		else
			printLine(2, CGI->generaltexth->allTexts[198], stackNode->valOfBonuses(Bonus::SHOTS));
	}
	if (stackNode->valOfBonuses(Bonus::CASTS))
	{
		printAtMiddleLoc(CGI->generaltexth->allTexts[399], 356, 62, FONT_SMALL, Colors::WHITE, to);
		std::string casts;
		if (type == BATTLE)
			casts = boost::lexical_cast<std::string>((ui16)dynamic_cast<const CStack*>(stackNode)->casts); //ui8 is converted to char :(
		else
			casts = boost::lexical_cast<std::string>(stackNode->valOfBonuses(Bonus::CASTS));
		printAtMiddleLoc(casts, 356, 82, FONT_SMALL, Colors::WHITE, to);
	}

	//TODO
	int dmgMultiply = 1;
	if(heroOwner && stackNode->hasBonusOfType(Bonus::SIEGE_WEAPON))
		dmgMultiply += heroOwner->Attack();

	printLine(3, CGI->generaltexth->allTexts[199], stackNode->getMinDamage() * dmgMultiply, stackNode->getMaxDamage() * dmgMultiply, true);
	printLine(4, CGI->generaltexth->allTexts[388], c->valOfBonuses(Bonus::STACK_HEALTH), stackNode->valOfBonuses(Bonus::STACK_HEALTH));
	printLine(6, CGI->generaltexth->zelp[441].first, c->valOfBonuses(Bonus::STACKS_SPEED), stackNode->valOfBonuses(Bonus::STACKS_SPEED));

	for(CBonusItem* b : bonusItems)
		b->showAll (to);

	for(auto s : selectableSkills)
		s->showAll (to);

	for (int i = 0; i < skillPictures.size(); i++)
	{
		skillPictures[i]->bg = BitmapHandler::loadBitmap (skillToFile(i));
		skillPictures[i]->showAll (to);
	}

	if (upgradeOptions.size() && (type == COMMANDER_LEVEL_UP && upgradeOptions[selectedOption] >= 100)) //add frame to selected skill
	{
		int index = selectedOption - selectableSkills.size(); //this is screwed
		CSDL_Ext::drawBorder(to, Rect::around(selectableBonuses[index]->pos), int3(Colors::METALLIC_GOLD.r, Colors::METALLIC_GOLD.g, Colors::METALLIC_GOLD.b)); 
	}
}

void CCreatureWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
	if (!count.empty()) //army stack
		graphics->fonts[FONT_TIMES]->renderTextRight(to, count, Colors::WHITE, Point(pos.x + 114, pos.y + 174));
}


void CCreatureWindow::close()
{
	if (upgradeOptions.size()) //a skill for commander was chosen
		levelUp (selectedOption); //callback value is vector index

	GH.popIntTotally(this);
}

void CCreatureWindow::sliderMoved(int newpos)
{
	recreateSkillList(newpos); //move components
	redraw();
}

std::string CCreatureWindow::skillToFile (int skill)
{
		std::string file = "zvs/Lib1.res/_";
		switch (skill)
		{
			case ECommander::ATTACK:
				file += "AT";
				break;
			case ECommander::DEFENSE:
				file += "DF";
				break;
			case ECommander::HEALTH:
				file += "HP";
				break;
			case ECommander::DAMAGE:
				file += "DM";
				break;
			case ECommander::SPEED:
				file += "SP";
				break;
			case ECommander::SPELL_POWER:
				file += "MP";
				break;
		}
		std::string sufix = boost::lexical_cast<std::string>((int)(commander->secondarySkills[skill])); //casting ui8 causes ascii char conversion
		if (type == COMMANDER_LEVEL_UP)
		{
			if (upgradeOptions.size() && upgradeOptions[selectedOption] == skill)//that one specific skill is selected
				sufix += "="; //level-up highlight
			else if (!vstd::contains(upgradeOptions, skill))
				sufix = "no"; //not avaliable - no number
		}
		file += sufix += ".bmp";

		return file;
}

void CCreatureWindow::setArt(const CArtifactInstance *art)
{
	creatureArtifact = art;
	if (creatureArtifact)
	{
		if (artifactImage == nullptr)
			artifactImage = new CAnimImage("ARTIFACT", creatureArtifact->artType->iconIndex, 0, 466, 100);
		else
			artifactImage->setFrame(creatureArtifact->artType->iconIndex);
	}
	else
		artifactImage = nullptr;
	
	redraw();
}

void CCreatureWindow::scrollArt(int dir)
{
	//TODO: get next artifact
	int size = stack->artifactsWorn.size();
	displayedArtifact  =  size ? static_cast<ArtifactPosition>((displayedArtifact + dir) % size)
	                           : static_cast<ArtifactPosition>(ArtifactPosition::CREATURE_SLOT);
	setArt (stack->getArt(displayedArtifact));
}

void CCreatureWindow::passArtifactToHero()
{
	const CGHeroInstance * h = dynamic_cast<const CGHeroInstance *>(stack->armyObj);
	if (h && creatureArtifact)
	{
		LOCPLINT->cb->swapArtifacts (ArtifactLocation (stack, displayedArtifact), ArtifactLocation(h, creatureArtifact->firstBackpackSlot(h)));
	}
	else
        logGlobal->warnStream() << "Pass artifact to hero should be disabled, no hero or no artifact!";

	//redraw is handled via CArtifactHolder interface
}

void CCreatureWindow::artifactRemoved (const ArtifactLocation &artLoc)
{
	//align artifacts to remove holes
	for (auto al : stack->artifactsWorn)
	{
		ArtifactPosition freeSlot = al.second.artifact->firstAvailableSlot(stack); 
		if (freeSlot < al.first)
			LOCPLINT->cb->swapArtifacts (ArtifactLocation(stack, al.first), ArtifactLocation(stack, freeSlot));
	}
	int size = stack->artifactsWorn.size();
	displayedArtifact  =  size ? static_cast<ArtifactPosition>(displayedArtifact % size)
	                           : static_cast<ArtifactPosition>(ArtifactPosition::CREATURE_SLOT); //0
	setArt (stack->getArt(displayedArtifact));
}
void CCreatureWindow::artifactMoved (const ArtifactLocation &artLoc, const ArtifactLocation &destLoc)
{
	artifactRemoved (artLoc); //same code
}

void CCreatureWindow::selectSkill (ui32 which)
{
	selectedOption = which;
	redraw();
}

CCreatureWindow::~CCreatureWindow()
{
 	for (auto & elem : upgResCost)
 		delete elem;
	bonusItems.clear();
}

CBonusItem::CBonusItem()
{

}

CBonusItem::CBonusItem(const Rect &Pos, const std::string &Name, const std::string &Description, const std::string &graphicsName)
{
	OBJ_CONSTRUCTION;
	visible = false;

	name = Name;
	description = Description;
	if (graphicsName.size())
		bonusGraphics = new CPicture(graphicsName, 26, 232);
	else
		bonusGraphics = nullptr;

	removeUsedEvents(ALL); //no actions atm
}

void CBonusItem::showAll (SDL_Surface * to)
{
	if (visible)
	{
		graphics->fonts[FONT_SMALL]->renderTextLeft(to, name, Colors::YELLOW, Point(pos.x + 72, pos.y + 6));
		graphics->fonts[FONT_SMALL]->renderTextLeft(to, description, Colors::WHITE,  Point(pos.x + 72, pos.y + 30));
		if (bonusGraphics && bonusGraphics->bg)
			blitAtLoc(bonusGraphics->bg, 12, 2, to);
	}
}

CBonusItem::~CBonusItem()
{
	//delete bonusGraphics; //automatic destruction
}

void CSelectableSkill::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

void CCreInfoWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
	creatureCount->showAll(to);
}

CCreInfoWindow::CCreInfoWindow(const CStackInstance &stack, bool LClicked, std::function<void()> upgradeFunc, std::function<void()> dismissFunc, UpgradeInfo *upgradeInfo):
    CWindowObject(PLAYER_COLORED | (LClicked ? 0 : RCLICK_POPUP), "CRSTKPU")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(stack.type, &stack, dynamic_cast<const CGHeroInstance*>(stack.armyObj), stack.count, LClicked);

	//additional buttons if opened with left click
	if(LClicked)
	{
		std::function<void()> closeFunc = std::bind(&CCreInfoWindow::close,this);

		if(upgradeFunc && upgradeInfo)
		{
			TResources upgradeCost = upgradeInfo->cost[0] * stack.count;
			for(TResources::nziterator i(upgradeCost); i.valid(); i++)
			{
				BLOCK_CAPTURING;
				upgResCost.push_back(new CComponent(CComponent::resource, i->resType, i->resVal));
			}

			CFunctionList<void()> onUpgrade;
			onUpgrade += upgradeFunc;
			onUpgrade += closeFunc;

			std::function<void()> dialog = std::bind(&CPlayerInterface::showYesNoDialog,
				LOCPLINT,
				CGI->generaltexth->allTexts[207],
				onUpgrade, 0, false,
				std::ref(upgResCost));

			upgrade = new CAdventureMapButton("", CGI->generaltexth->zelp[446].second, dialog, 76, 237, "IVIEWCR", SDLK_u);
			upgrade->block(!LOCPLINT->cb->getResourceAmount().canAfford(upgradeCost));
		}

		if(dismissFunc)
		{
			CFunctionList<void()> onDismiss;
			onDismiss += dismissFunc;
			onDismiss += closeFunc;

			std::function<void()> dialog = std::bind(&CPlayerInterface::showYesNoDialog,
				LOCPLINT,
				CGI->generaltexth->allTexts[12],
				onDismiss, 0, true, std::vector<CComponent*>());

			dismiss = new CAdventureMapButton("", CGI->generaltexth->zelp[445].second, dialog, 21, 237, "IVIEWCR2",SDLK_d);
		}
	}
}

CCreInfoWindow::CCreInfoWindow(int creatureID, bool LClicked, int creatureCount):
    CWindowObject(PLAYER_COLORED | (LClicked ? 0 : RCLICK_POPUP), "CRSTKPU")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	const CCreature *creature = CGI->creh->creatures[creatureID];
	init(creature, nullptr, nullptr, creatureCount, LClicked);
}

CCreInfoWindow::CCreInfoWindow(const CStack &stack, bool LClicked):
    CWindowObject(PLAYER_COLORED | (LClicked ? 0 : RCLICK_POPUP), "CRSTKPU")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(stack.getCreature(), &stack, stack.getMyHero(), stack.count, LClicked);
}

CCreInfoWindow::~CCreInfoWindow()
{
	for(CComponent* object : upgResCost)
		delete object;
}

void CCreInfoWindow::printLine(int position, const std::string &text, int baseVal, int val/*=-1*/, bool range/*=false*/)
{
	infoTexts[position].first = new CLabel(155, 48 + position*19, FONT_SMALL, TOPLEFT, Colors::WHITE, text);
	std::string valueStr;

	if(range && baseVal != val)
		valueStr = boost::str(boost::format("%d - %d") % baseVal % val);

	else if(baseVal != val && val>=0)
		valueStr = boost::str(boost::format("%d (%d)") % baseVal % val);

	else
		valueStr = boost::lexical_cast<std::string>(baseVal);

	infoTexts[position].second = new CLabel(276, 63 + position*19, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, valueStr);
}

void CCreInfoWindow::init(const CCreature *creature, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner, int count, bool LClicked)
{
	removeUsedEvents(ALL);
	if (!LClicked)
		addUsedEvents(RCLICK);

	if(!stackNode)
		stackNode = creature;

	animation = new CCreaturePic(21, 48, creature);

	std::string countStr = boost::lexical_cast<std::string>(count);
	creatureCount = new CLabel(114, 174, FONT_TIMES, BOTTOMRIGHT, Colors::WHITE, countStr);

	creatureName = new CLabel(149, 30, FONT_SMALL, CENTER, Colors::YELLOW, creature->namePl);

	printLine(0, CGI->generaltexth->primarySkillNames[0], creature->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), stackNode->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK));
	printLine(1, CGI->generaltexth->primarySkillNames[1], creature->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), stackNode->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE));

	if(stackNode->valOfBonuses(Bonus::SHOTS))
		printLine(2, CGI->generaltexth->allTexts[198], stackNode->valOfBonuses(Bonus::SHOTS));

	//TODO
	int dmgMultiply = 1;
	if(heroOwner && stackNode->hasBonusOfType(Bonus::SIEGE_WEAPON))
		dmgMultiply += heroOwner->Attack();

	printLine(3, CGI->generaltexth->allTexts[199],   stackNode->getMinDamage() * dmgMultiply, stackNode->getMaxDamage() * dmgMultiply, true);
	printLine(4, CGI->generaltexth->allTexts[388],   creature->valOfBonuses(Bonus::STACK_HEALTH), stackNode->valOfBonuses(Bonus::STACK_HEALTH));
	printLine(6, CGI->generaltexth->zelp[441].first, creature->valOfBonuses(Bonus::STACKS_SPEED), stackNode->valOfBonuses(Bonus::STACKS_SPEED));

	//setting morale
	morale = new MoraleLuckBox(true, genRect(42, 42, 22, 186));
	morale->set(stackNode);
	//setting luck
	luck = new MoraleLuckBox(false, genRect(42, 42, 75, 186));
	luck->set(stackNode);

	if(!LClicked)
	{
		abilityText = new CLabel(17, 231, FONT_SMALL, TOPLEFT, Colors::WHITE, creature->abilityText);
	}
	else
	{
		abilityText = nullptr;
		ok = new CAdventureMapButton("", CGI->generaltexth->zelp[445].second,
			std::bind(&CCreInfoWindow::close,this), 216, 237, "IOKAY.DEF", SDLK_RETURN);
		ok->assignedKeys.insert(SDLK_ESCAPE);
	}

	//if we are displying window fo r stack in battle, there are several more things that we need to display
	if(const CStack *battleStack = dynamic_cast<const CStack*>(stackNode))
	{
		//print at most 3 spell effects
		std::vector<si32> spells = battleStack->activeSpells();
		for (size_t i=0; i< std::min(spells.size(), size_t(3)); i++)
			effects.push_back(new CAnimImage("SpellInt", spells[i]+1, 0, 127 + 52*i, 186));

		//print current health
		printLine(5, CGI->generaltexth->allTexts[200], battleStack->firstHPleft);
	}
}

CIntObject * createCreWindow(
	const CStack *s, bool lclick/* = false*/)
{
	auto c = dynamic_cast<const CCommanderInstance *>(s->base);
	if (c)
	{
		return new CCreatureWindow (c, s);
	}
	else
	{
		if(settings["general"]["classicCreatureWindow"].Bool())
			return new CCreInfoWindow(*s, lclick);
		else
			return new CCreatureWindow(*s, LOCPLINT->battleInt ? CCreatureWindow::BATTLE : CCreatureWindow::OTHER);
	}
}

CIntObject * createCreWindow(CreatureID Cid, CCreatureWindow::CreWinType Type, int creatureCount)
{
	if(settings["general"]["classicCreatureWindow"].Bool())
		return new CCreInfoWindow(Cid, Type, creatureCount);
	else
		return new CCreatureWindow(Cid, Type, creatureCount);
}

CIntObject * createCreWindow(const CStackInstance *s, CCreatureWindow::CreWinType type, std::function<void()> Upg, std::function<void()> Dsm, UpgradeInfo *ui)
{
	if(settings["general"]["classicCreatureWindow"].Bool())
		return new CCreInfoWindow(*s, type==CCreatureWindow::HERO, Upg, Dsm, ui);
	else
		return  new CCreatureWindow(*s, type, Upg, Dsm, ui);
}

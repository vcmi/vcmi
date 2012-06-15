#include "StdInc.h"
#include "CCreatureWindow.h"

#include "../lib/CCreatureSet.h"
#include "CGameInfo.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/BattleState.h"
#include "../CCallback.h"

#include <SDL.h>
#include "UIFramework/SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "Graphics.h"
#include "CPlayerInterface.h"
#include "CConfigHandler.h"
#include "CAnimation.h"

#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/CSpellHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/NetPacks.h" //ArtifactLocation

#include "UIFramework/CGuiHandler.h"
#include "UIFramework/CIntObjectClasses.h"

using namespace CSDL_Ext;

class CBonusItem;
class CCreatureArtifactInstance;

/*
 * CCreatureWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CCreatureWindow::CCreatureWindow (const CStack &stack, int Type):
    CWindowObject(PLAYER_COLORED | (Type < 3 ? RCLICK_POPUP : 0 ) ),
    type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (stack.base)
		init(stack.base, &stack, dynamic_cast<const CGHeroInstance*>(stack.base->armyObj));
	else
	{
		CStackInstance * s = new CStackInstance(stack.type, 1); //TODO: war machines and summons should be regular stacks
		init(s, &stack, NULL);
		delete s;
	}
}

CCreatureWindow::CCreatureWindow (const CStackInstance &stack, int Type):
    CWindowObject(PLAYER_COLORED | (Type < 3 ? RCLICK_POPUP : 0 ) ),
    type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	init(&stack, &stack, dynamic_cast<const CGHeroInstance*>(stack.armyObj));
}

CCreatureWindow::CCreatureWindow(int Cid, int Type, int creatureCount):
   CWindowObject(PLAYER_COLORED | (Type < 3 ? RCLICK_POPUP : 0 ) ),
    type(Type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	CStackInstance * stack = new CStackInstance(Cid, creatureCount); //TODO: simplify?
	init(stack, CGI->creh->creatures[Cid], NULL);
	delete stack;
}

CCreatureWindow::CCreatureWindow(const CStackInstance &st, int Type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui):
    CWindowObject(PLAYER_COLORED | (Type < 3 ? RCLICK_POPUP : 0 ) ),
    type(Type),
    dismiss(0),
    upgrade(0),
    ok(0),
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
				fs += boost::bind(&CCreatureWindow::close,this);
				CFunctionList<void()> cfl;
				cfl = boost::bind(&CPlayerInterface::showYesNoDialog, LOCPLINT, CGI->generaltexth->allTexts[207], fs, 0, false, boost::ref(upgResCost));
				upgrade = new CAdventureMapButton("",CGI->generaltexth->zelp[446].second,cfl,385, 148,"IVIEWCR.DEF",SDLK_u);
			}
			else
			{
				upgrade = new CAdventureMapButton("",CGI->generaltexth->zelp[446].second,boost::function<void()>(),385, 148,"IVIEWCR.DEF");
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
			cfl = boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],fs[0],fs[1],false,std::vector<CComponent*>());
			dismiss = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second,cfl,333, 148,"IVIEWCR2.DEF",SDLK_d);
		}
	}
}

CCreatureWindow::CCreatureWindow (const CCommanderInstance * Commander):
    CWindowObject(PLAYER_COLORED),
    type(COMMANDER),
	commander (Commander)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(commander, commander, dynamic_cast<const CGHeroInstance*>(commander->armyObj));

	boost::function<void()> Dsm;
	CFunctionList<void()> fs[2];
	//on dismiss confirmed
	fs[0] += Dsm; //dismiss
	fs[0] += boost::bind(&CCreatureWindow::close,this);//close this window
	CFunctionList<void()> cfl;
	cfl = boost::bind(&CPlayerInterface::showYesNoDialog,LOCPLINT,CGI->generaltexth->allTexts[12],fs[0],fs[1],false,std::vector<CComponent*>());
	dismiss = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, cfl, 333, 148,"IVIEWCR2.DEF", SDLK_d);
}

void CCreatureWindow::init(const CStackInstance *Stack, const CBonusSystemNode *StackNode, const CGHeroInstance *HeroOwner)
{
	creatureArtifact = NULL; //may be set later
	artifactImage = NULL;
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
		commander = NULL;

	bool creArt = false;
	displayedArtifact = ArtifactPosition::CREATURE_SLOT; // 0

	//Basic graphics - need to calculate size

	int commanderOffset = 0;
	if (type >= COMMANDER)
		commanderOffset = 74;

	BonusList bl, blTemp;
	blTemp = (*(stackNode->getBonuses(Selector::durationType(Bonus::PERMANENT))));


	while (blTemp.size())
	{
		Bonus * b = blTemp.front();

		bl.push_back (new Bonus(*b));
		bl.back()->val = blTemp.valOfBonuses(Selector::typeSubtype(b->type, b->subtype)); //merge multiple bonuses into one
		blTemp.remove_if (Selector::typeSubtype(b->type, b->subtype)); //remove used bonuses
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

	int magicResistance = 0; //handle it separately :/
	if (battleStack)
	{
		magicResistance = battleStack->magicResistance(); //include Aura of Resistance
	}
	else
	{
		magicResistance = stack->magicResistance(); //include Resiatance hero skill
	}
	if (magicResistance)
	{
		std::map<TBonusType, std::pair<std::string, std::string> >::const_iterator it = CGI->creh->stackBonuses.find(Bonus::MAGIC_RESISTANCE);
		std::string description;
		text = it->second.first;
		description = it->second.second;
		boost::algorithm::replace_first(description, "%d", boost::lexical_cast<std::string>(magicResistance));
		Bonus b;
		b.type = Bonus::MAGIC_RESISTANCE;
		bonusItems.push_back (new CBonusItem(genRect(0, 0, 251, 57), text, description, stack->bonusToGraphics(&b)));
	}

	bonusRows = std::min ((int)((bonusItems.size() + 1) / 2), (screen->h - 230) / 60);
	if (type >= COMMANDER)
		vstd::amin(bonusRows, 3);
	else
		vstd::amin(bonusRows, 4);
	vstd::amax(bonusRows, 1);

	if (type >= COMMANDER)
		setBackground("CommWin" + boost::lexical_cast<std::string>(bonusRows) + ".pcx");
	else
		setBackground("CreWin" + boost::lexical_cast<std::string>(bonusRows) + ".pcx"); //1 to 4 rows for now

	//Buttons
	ok = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, boost::bind(&CCreatureWindow::close,this), 489, 148, "hsbtns.def", SDLK_RETURN);
	ok->assignedKeys.insert(SDLK_ESCAPE);

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

	new CAnimImage("PSKIL42", 4, 0, 387, 51); //exp icon - Print it always?
	if (type) //not in fort window
	{
		if (GameConstants::STACK_EXP && type < COMMANDER)
		{
			int rank = std::min(stack->getExpRank(), 10); //hopefully nobody adds more
			new CLabel(488, 82, FONT_SMALL, CENTER, Colors::Cornsilk, boost::lexical_cast<std::string>(stack->experience));
			new CLabel(488, 62, FONT_MEDIUM, CENTER, Colors::Jasmine,
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

		if (GameConstants::STACK_ARTIFACT)
		{
			creArt = true;
		}
	}
	if (commander) //secondary skills
	{
		creArt = true;
		for (int i = ECommander::ATTACK; i <= ECommander::SPELL_POWER; ++i)
		{
			if (commander->secondarySkills[i])
			{
				std::string file = "zvs/Lib1.res/_";
				switch (i)
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
				std::string sufix = boost::lexical_cast<std::string>((int)(commander->secondarySkills[i] - 1)); //casting ui8 causes ascii char conversion
				if (type == COMMANDER_LEVEL_UP)
				{
					if (commander->secondarySkills[i] < ECommander::MAX_SKILL_LEVEL)
						sufix += "="; //level-up highlight
					else
						sufix = "no"; //not avaliable - no number
				}
				file += sufix += ".bmp";

				new CPicture(file, 37 + i * 84, 224);
			}
		}
		//print commander level
		new CLabel(488, 62, FONT_MEDIUM, CENTER, Colors::Jasmine,
		           boost::lexical_cast<std::string>((ui16)(commander->level)));

		new CLabel(488, 82, FONT_SMALL, CENTER, Colors::Cornsilk,
		           boost::lexical_cast<std::string>(stack->experience));
	}
	if (creArt) //stack or commander artifacts
	{
		setArt (stack->getArt(ArtifactPosition::CREATURE_SLOT));
		if (type > BATTLE) //artifact buttons inactive in battle
		{
			//TODO: disable buttons if no artifact is equipped
			leftArtRoll = new CAdventureMapButton(std::string(), std::string(), boost::bind (&CCreatureWindow::scrollArt, this, -1), 437, 98, "hsbtns3.def", SDLK_LEFT);
			rightArtRoll = new CAdventureMapButton(std::string(), std::string(), boost::bind (&CCreatureWindow::scrollArt, this, +1), 516, 98, "hsbtns5.def", SDLK_RIGHT);
			if (heroOwner)
				passArtToHero = new CAdventureMapButton(std::string(), std::string(), boost::bind (&CCreatureWindow::passArtifactToHero, this), 437, 148, "OVBUTN1.DEF", SDLK_HOME);
		}
	}

	if (battleStack) //only during battle
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
		slider = new CSlider(528, 231 + commanderOffset, bonusRows*60, boost::bind (&CCreatureWindow::sliderMoved, this, _1),
		bonusRows, (bonusItems.size() + 1) >> 1, 0, false, 0);
	}
	else //slider automatically places bonus Items
		recreateSkillList (0);

	showAll(screen2);

	//AUIDAT.DEF
}

void CCreatureWindow::printLine(int nr, const std::string &text, int baseVal, int val/*=-1*/, bool range/*=false*/)
{
	new CLabel(162, 48 + nr*19, FONT_SMALL, TOPLEFT, Colors::Cornsilk, text);

	std::string hlp;
	if(range && baseVal != val)
		hlp = boost::str(boost::format("%d - %d") % baseVal % val);
	else if(baseVal != val && val>=0)
		hlp = boost::str(boost::format("%d (%d)") % baseVal % val);
	else
		hlp = boost::lexical_cast<std::string>(baseVal);

	new CLabel(325, 64 + nr*19, FONT_SMALL, BOTTOMRIGHT, Colors::Cornsilk, hlp);
}

void CCreatureWindow::recreateSkillList(int Pos)
{
	int commanderOffset = 0;
	if (type >= COMMANDER)
		commanderOffset = 74;

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
		int offsety = 60*i + (bonusRows > 1 ? 1 : 0) + commanderOffset; //lack of precision :/

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

	printAtMiddleLoc((type >= COMMANDER ? c->nameSing : c->namePl), 180, 30, FONT_SMALL, Colors::Jasmine, to); //creature name

	printLine(0, CGI->generaltexth->primarySkillNames[0], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), stackNode->Attack());
	printLine(1, CGI->generaltexth->primarySkillNames[1], c->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), stackNode->Defense());

	if (stackNode->valOfBonuses(Bonus::SHOTS) && stackNode->hasBonusOfType(Bonus::SHOOTER))
	{//only for shooting units - important with wog exp shooters
		if (type == BATTLE)
			printLine(2, CGI->generaltexth->allTexts[198], dynamic_cast<const CStack*>(stackNode)->shots);
		else
			printLine(2, CGI->generaltexth->allTexts[198], stackNode->valOfBonuses(Bonus::SHOTS));
	}
	if (stackNode->valOfBonuses(Bonus::CASTS))
	{
		printAtMiddleLoc(CGI->generaltexth->allTexts[399], 356, 62, FONT_SMALL, Colors::Cornsilk, to);
		std::string casts;
		if (type == BATTLE)
			casts = boost::lexical_cast<std::string>((ui16)dynamic_cast<const CStack*>(stackNode)->casts); //ui8 is converted to char :(
		else
			casts = boost::lexical_cast<std::string>(stackNode->valOfBonuses(Bonus::CASTS));
		printAtMiddleLoc(casts, 356, 82, FONT_SMALL, Colors::Cornsilk, to);
	}

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

void CCreatureWindow::show(SDL_Surface * to)
{
	if (count.size()) //army stack
		printTo(count, pos.x + 114, pos.y + 174,FONT_TIMES, Colors::Cornsilk, to);
}


void CCreatureWindow::sliderMoved(int newpos)
{
	recreateSkillList(newpos); //move components
	redraw();
}

void CCreatureWindow::setArt(const CArtifactInstance *art)
{
	creatureArtifact = art;
	if (creatureArtifact)
	{
		if (artifactImage == NULL)
			artifactImage = new CAnimImage("ARTIFACT", creatureArtifact->artType->id, 0, 466, 100);
		else
			artifactImage->setFrame(creatureArtifact->artType->id);
	}
	else
		artifactImage = NULL;
	
	redraw();
}

void CCreatureWindow::scrollArt(int dir)
{
	//TODO: get next artifact
	int size = stack->artifactsWorn.size();
	displayedArtifact  =  size ? (displayedArtifact + dir) % size : ArtifactPosition::CREATURE_SLOT;
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
		tlog2 << "Pass artifact to hero should be disabled, no hero or no artifact!\n";

	//redraw is handled via CArtifactHolder interface
}

void CCreatureWindow::artifactRemoved (const ArtifactLocation &artLoc)
{
	//align artifacts to remove holes
	BOOST_FOREACH (auto al, stack->artifactsWorn)
	{
		int freeSlot = al.second.artifact->firstAvailableSlot(stack); 
		if (freeSlot < al.first)
			LOCPLINT->cb->swapArtifacts (ArtifactLocation(stack, al.first), ArtifactLocation(stack, freeSlot));
	}
	int size = stack->artifactsWorn.size();
	displayedArtifact  =  size ? (displayedArtifact % size) : ArtifactPosition::CREATURE_SLOT; //0
	setArt (stack->getArt(displayedArtifact));
}
void CCreatureWindow::artifactMoved (const ArtifactLocation &artLoc, const ArtifactLocation &destLoc)
{
	artifactRemoved (artLoc); //same code
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
	visible = false;

	name = Name;
	description = Description;
	if (graphicsName.size())
		bonusGraphics = new CPicture(graphicsName, 26, 232);
	else
		bonusGraphics = NULL;

	removeUsedEvents(ALL); //no actions atm
}

void CBonusItem::showAll (SDL_Surface * to)
{
	if (visible)
	{
		printAt(name, pos.x + 72, pos.y + 6, FONT_SMALL, Colors::Jasmine, to);
		printAt(description, pos.x + 72, pos.y + 30, FONT_SMALL, Colors::Cornsilk, to);
		if (bonusGraphics && bonusGraphics->bg)
			blitAtLoc(bonusGraphics->bg, 12, 2, to);
	}
}

CBonusItem::~CBonusItem()
{
	//delete bonusGraphics; //automatic destruction
}

void CCreInfoWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
	creatureCount->showAll(to);
}

CCreInfoWindow::CCreInfoWindow(const CStackInstance &stack, bool LClicked, boost::function<void()> upgradeFunc, boost::function<void()> dismissFunc, UpgradeInfo *upgradeInfo):
    CWindowObject(PLAYER_COLORED | (LClicked ? 0 : RCLICK_POPUP), "CRSTKPU")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(stack.type, &stack, dynamic_cast<const CGHeroInstance*>(stack.armyObj), stack.count, LClicked);

	//additional buttons if opened with left click
	if(LClicked)
	{
		boost::function<void()> closeFunc = boost::bind(&CCreInfoWindow::close,this);

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

			boost::function<void()> dialog = boost::bind(&CPlayerInterface::showYesNoDialog,
				LOCPLINT,
				CGI->generaltexth->allTexts[207],
				onUpgrade, 0, false,
				boost::ref(upgResCost));

			upgrade = new CAdventureMapButton("", CGI->generaltexth->zelp[446].second, dialog, 76, 237, "IVIEWCR", SDLK_u);
			upgrade->block(!LOCPLINT->cb->getResourceAmount().canAfford(upgradeCost));
		}

		if(dismissFunc)
		{
			CFunctionList<void()> onDismiss;
			onDismiss += dismissFunc;
			onDismiss += closeFunc;

			boost::function<void()> dialog = boost::bind(&CPlayerInterface::showYesNoDialog,
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
	init(creature, NULL, NULL, creatureCount, LClicked);
}

CCreInfoWindow::CCreInfoWindow(const CStack &stack, bool LClicked):
    CWindowObject(PLAYER_COLORED | (LClicked ? 0 : RCLICK_POPUP), "CRSTKPU")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init(stack.getCreature(), &stack, stack.getMyHero(), stack.count, LClicked);
}

CCreInfoWindow::~CCreInfoWindow()
{
	BOOST_FOREACH(CComponent* object, upgResCost)
		delete object;
}

void CCreInfoWindow::printLine(int position, const std::string &text, int baseVal, int val/*=-1*/, bool range/*=false*/)
{
	infoTexts[position].first = new CLabel(155, 48 + position*19, FONT_SMALL, TOPLEFT, Colors::Cornsilk, text);
	std::string valueStr;

	if(range && baseVal != val)
		valueStr = boost::str(boost::format("%d - %d") % baseVal % val);

	else if(baseVal != val && val>=0)
		valueStr = boost::str(boost::format("%d (%d)") % baseVal % val);

	else
		valueStr = boost::lexical_cast<std::string>(baseVal);

	infoTexts[position].second = new CLabel(276, 63 + position*19, FONT_SMALL, BOTTOMRIGHT, Colors::Cornsilk, valueStr);
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
	creatureCount = new CLabel(114, 174, FONT_TIMES, BOTTOMRIGHT, Colors::Cornsilk, countStr);

	creatureName = new CLabel(149, 30, FONT_SMALL, CENTER, Colors::Jasmine, creature->namePl);

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
		abilityText = new CLabel(17, 231, FONT_SMALL, TOPLEFT, Colors::Cornsilk, creature->abilityText);
	}
	else
	{
		abilityText = NULL;
		ok = new CAdventureMapButton("", CGI->generaltexth->zelp[445].second,
			boost::bind(&CCreInfoWindow::close,this), 216, 237, "IOKAY.DEF", SDLK_RETURN);
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
	if(settings["general"]["classicCreatureWindow"].Bool())
		return new CCreInfoWindow(*s, lclick);
	else
		return new CCreatureWindow(*s, CCreatureWindow::BATTLE);
}

CIntObject * createCreWindow(int Cid, int Type, int creatureCount)
{
	if(settings["general"]["classicCreatureWindow"].Bool())
		return new CCreInfoWindow(Cid, Type, creatureCount);
	else
		return new CCreatureWindow(Cid, Type, creatureCount);
}

CIntObject * createCreWindow(const CStackInstance *s, int type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui)
{
	if(settings["general"]["classicCreatureWindow"].Bool())
		return new CCreInfoWindow(*s, type==3, Upg, Dsm, ui);
	else
		return  new CCreatureWindow(*s, type, Upg, Dsm, ui);
}

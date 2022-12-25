/*
 * BattleControlPanel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleControlPanel.h"

#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleStacksController.h"
#include "BattleActionsController.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CAnimation.h"
#include "../windows/CSpellWindow.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/filesystem/ResourceID.h"

BattleControlPanel::BattleControlPanel(BattleInterface & owner, const Point & position):
	owner(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	REGISTER_BUILDER("battleConsole", &BattleControlPanel::buildBattleConsole);
	
	pos += position;
	
	const JsonNode config(ResourceID("config/widgets/battleControlPanel.json"));
	
	addCallback("options", std::bind(&BattleControlPanel::bOptionsf, this));
	addCallback("surrender", std::bind(&BattleControlPanel::bSurrenderf, this));
	addCallback("flee", std::bind(&BattleControlPanel::bFleef, this));
	addCallback("autofight", std::bind(&BattleControlPanel::bAutofightf, this));
	addCallback("spellbook", std::bind(&BattleControlPanel::bSpellf, this));
	addCallback("wait", std::bind(&BattleControlPanel::bWaitf, this));
	addCallback("defence", std::bind(&BattleControlPanel::bDefencef, this));
	addCallback("consoleUp", std::bind(&BattleControlPanel::bConsoleUpf, this));
	addCallback("consoleDown", std::bind(&BattleControlPanel::bConsoleDownf, this));
	addCallback("tacticNext", std::bind(&BattleControlPanel::bTacticNextStack, this));
	addCallback("tacticEnd", std::bind(&BattleControlPanel::bTacticPhaseEnd, this));
	addCallback("alternativeAction", std::bind(&BattleControlPanel::bSwitchActionf, this));
	
	build(config);
	
	console = widget<BattleConsole>("console");
	GH.statusbar = console;

	if ( owner.tacticsMode )
		tacticPhaseStarted();
	else
		tacticPhaseEnded();
}

std::shared_ptr<BattleConsole> BattleControlPanel::buildBattleConsole(const JsonNode & config) const
{
	return std::make_shared<BattleConsole>(readRect(config["rect"]));
}

void BattleControlPanel::show(SDL_Surface * to)
{
	//show menu before all other elements to keep it in background
	if(auto w = widget<CPicture>("menu"))
		w->show(to);
	CIntObject::show(to);
}

void BattleControlPanel::showAll(SDL_Surface * to)
{
	//show menu before all other elements to keep it in background
	if(auto w = widget<CPicture>("menu"))
		w->showAll(to);
	CIntObject::showAll(to);
}


void BattleControlPanel::tacticPhaseStarted()
{
	build(variables["tacticItems"]);

	if(auto w = widget<CPicture>("menu"))
	{
		w->colorize(owner.curInt->playerID);
		w->recActions &= ~(SHOWALL | UPDATE);
	}
}
void BattleControlPanel::tacticPhaseEnded()
{
	deleteWidget("tacticNext");
	deleteWidget("tacticEnd");

	build(variables["battleItems"]);

	if(auto w = widget<CPicture>("menu"))
	{
		w->colorize(owner.curInt->playerID);
		w->recActions &= ~(SHOWALL | UPDATE);
	}
}

void BattleControlPanel::bOptionsf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	GH.pushIntT<BattleOptionsWindow>(owner);
}

void BattleControlPanel::bSurrenderf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	int cost = owner.curInt->cb->battleGetSurrenderCost();
	if(cost >= 0)
	{
		std::string enemyHeroName = owner.curInt->cb->battleGetEnemyHero().name;
		if(enemyHeroName.empty())
		{
			logGlobal->warn("Surrender performed without enemy hero, should not happen!");
			enemyHeroName = "#ENEMY#";
		}

		std::string surrenderMessage = boost::str(boost::format(CGI->generaltexth->allTexts[32]) % enemyHeroName % cost); //%s states: "I will accept your surrender and grant you and your troops safe passage for the price of %d gold."
		owner.curInt->showYesNoDialog(surrenderMessage, [this](){ reallySurrender(); }, nullptr);
	}
}

void BattleControlPanel::bFleef()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if ( owner.curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = std::bind(&BattleControlPanel::reallyFlee,this);
		owner.curInt->showYesNoDialog(CGI->generaltexth->allTexts[28], ony, nullptr); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<std::shared_ptr<CComponent>> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if (owner.attackingHeroInstance)
			if (owner.attackingHeroInstance->tempOwner == owner.curInt->cb->getMyColor())
				heroName = owner.attackingHeroInstance->name;
		if (owner.defendingHeroInstance)
			if (owner.defendingHeroInstance->tempOwner == owner.curInt->cb->getMyColor())
				heroName = owner.defendingHeroInstance->name;
		//calculating text
		auto txt = boost::format(CGI->generaltexth->allTexts[340]) % heroName; //The Shackles of War are present.  %s can not retreat!

		//printing message
		owner.curInt->showInfoDialog(boost::to_string(txt), comps);
	}
}

void BattleControlPanel::reallyFlee()
{
	owner.giveCommand(EActionType::RETREAT);
	CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
}

void BattleControlPanel::reallySurrender()
{
	if (owner.curInt->cb->getResourceAmount(Res::GOLD) < owner.curInt->cb->battleGetSurrenderCost())
	{
		owner.curInt->showInfoDialog(CGI->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		owner.giveCommand(EActionType::SURRENDER);
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
	}
}

void BattleControlPanel::showAlternativeActionIcon(PossiblePlayerBattleAction action)
{
	auto w = widget<CButton>("alternativeAction");
	if(!w)
		return;
	
	std::string iconName = variables["actionIconDefault"].String();
	switch(action)
	{
		case PossiblePlayerBattleAction::ATTACK:
			iconName = variables["actionIconAttack"].String();
			break;
			
		case PossiblePlayerBattleAction::SHOOT:
			iconName = variables["actionIconShoot"].String();
			break;
			
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			iconName = variables["actionIconSpell"].String();
			break;
			
		//TODO: figure out purpose of this icon
		//case PossiblePlayerBattleAction::???:
			//iconName = variables["actionIconWalk"].String();
			//break;
			
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			iconName = variables["actionIconReturn"].String();
			break;
			
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			iconName = variables["actionIconNoReturn"].String();
			break;
	}
		
	auto anim = std::make_shared<CAnimation>(iconName);
	w->setImage(anim, false);
}

void BattleControlPanel::setAlternativeActions(const std::list<PossiblePlayerBattleAction> & actions)
{
	alternativeActions = actions;
	defaultAction = PossiblePlayerBattleAction::INVALID;
	if(alternativeActions.size() > 1)
		defaultAction = alternativeActions.back();
	if(!alternativeActions.empty())
		showAlternativeActionIcon(alternativeActions.front());
	else
		showAlternativeActionIcon(defaultAction);
}

void BattleControlPanel::bAutofightf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	//Stop auto-fight mode
	if(owner.curInt->isAutoFightOn)
	{
		assert(owner.curInt->autofightingAI);
		owner.curInt->isAutoFightOn = false;
		logGlobal->trace("Stopping the autofight...");
	}
	else if(!owner.curInt->autofightingAI)
	{
		owner.curInt->isAutoFightOn = true;
		blockUI(true);

		auto ai = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());
		ai->init(owner.curInt->env, owner.curInt->cb);
		ai->battleStart(owner.army1, owner.army2, int3(0,0,0), owner.attackingHeroInstance, owner.defendingHeroInstance, owner.curInt->cb->battleGetMySide());
		owner.curInt->autofightingAI = ai;
		owner.curInt->cb->registerBattleInterface(ai);

		owner.requestAutofightingAIToTakeAction();
	}
}

void BattleControlPanel::bSpellf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (!owner.myTurn)
		return;

	auto myHero = owner.currentHero();
	if(!myHero)
		return;

	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	ESpellCastProblem::ESpellCastProblem spellCastProblem = owner.curInt->cb->battleCanCastSpell(myHero, spells::Mode::HERO);

	if(spellCastProblem == ESpellCastProblem::OK)
	{
		GH.pushIntT<CSpellWindow>(myHero, owner.curInt.get());
	}
	else if (spellCastProblem == ESpellCastProblem::MAGIC_IS_BLOCKED)
	{
		//TODO: move to spell mechanics, add more information to spell cast problem
		//Handle Orb of Inhibition-like effects -> we want to display dialog with info, why casting is impossible
		auto blockingBonus = owner.currentHero()->getBonusLocalFirst(Selector::type()(Bonus::BLOCK_ALL_MAGIC));
		if (!blockingBonus)
			return;

		if (blockingBonus->source == Bonus::ARTIFACT)
		{
			const auto artID = ArtifactID(blockingBonus->sid);
			//If we have artifact, put name of our hero. Otherwise assume it's the enemy.
			//TODO check who *really* is source of bonus
			std::string heroName = myHero->hasArt(artID) ? myHero->name : owner.enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[683])
										% heroName % CGI->artifacts()->getByIndex(artID)->getName()));
		}
	}
}

void BattleControlPanel::bSwitchActionf()
{
	if(alternativeActions.empty())
		return;
	
	if(alternativeActions.front() == defaultAction)
	{
		alternativeActions.push_back(alternativeActions.front());
		alternativeActions.pop_front();
	}
	
	auto actions = owner.actionsController->getPossibleActions();
	if(!actions.empty() && actions.front() == alternativeActions.front())
	{
		owner.actionsController->removePossibleAction(alternativeActions.front());
		showAlternativeActionIcon(defaultAction);
	}
	else
	{
		owner.actionsController->pushFrontPossibleAction(alternativeActions.front());
		showAlternativeActionIcon(alternativeActions.front());
	}
	
	alternativeActions.push_back(alternativeActions.front());
	alternativeActions.pop_front();
}

void BattleControlPanel::bWaitf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::WAIT);
}

void BattleControlPanel::bDefencef()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	if (owner.stacksController->getActiveStack() != nullptr)
		owner.giveCommand(EActionType::DEFEND);
}

void BattleControlPanel::bConsoleUpf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	console->scrollUp();
}

void BattleControlPanel::bConsoleDownf()
{
	if (owner.actionsController->spellcastingModeActive())
		return;

	console->scrollDown();
}

void BattleControlPanel::bTacticNextStack()
{
	owner.tacticNextStack(nullptr);
}

void BattleControlPanel::bTacticPhaseEnd()
{
	owner.tacticPhaseEnd();
}

void BattleControlPanel::blockUI(bool on)
{
	bool canCastSpells = false;
	auto hero = owner.curInt->cb->battleGetMyHero();

	if(hero)
	{
		ESpellCastProblem::ESpellCastProblem spellcastingProblem = owner.curInt->cb->battleCanCastSpell(hero, spells::Mode::HERO);

		//if magic is blocked, we leave button active, so the message can be displayed after button click
		canCastSpells = spellcastingProblem == ESpellCastProblem::OK || spellcastingProblem == ESpellCastProblem::MAGIC_IS_BLOCKED;
	}

	bool canWait = owner.stacksController->getActiveStack() ? !owner.stacksController->getActiveStack()->waitedThisTurn : false;

	if(auto w = widget<CButton>("options"))
		w->block(on);
	if(auto w = widget<CButton>("flee"))
		w->block(on || !owner.curInt->cb->battleCanFlee());
	if(auto w = widget<CButton>("surrender"))
		w->block(on || owner.curInt->cb->battleGetSurrenderCost() < 0);
	if(auto w = widget<CButton>("cast"))
		w->block(on || owner.tacticsMode || !canCastSpells);
	if(auto w = widget<CButton>("wait"))
		w->block(on || owner.tacticsMode || !canWait);
	if(auto w = widget<CButton>("defence"))
		w->block(on || owner.tacticsMode);
	if(auto w = widget<CButton>("alternativeAction"))
		w->block(on || owner.tacticsMode);

	// block only if during enemy turn and auto-fight is off
	// otherwise - crash on accessing non-exisiting active stack
	if(auto w = widget<CButton>("options"))
		w->block(!owner.curInt->isAutoFightOn && !owner.stacksController->getActiveStack());

	auto btactEnd = widget<CButton>("tacticEnd");
	auto btactNext = widget<CButton>("tacticNext");
	if(owner.tacticsMode && btactEnd && btactNext)
	{
		btactNext->block(on);
		btactEnd->block(on);
	}
	else
	{
		auto bConsoleUp = widget<CButton>("consoleUp");
		auto bConsoleDown = widget<CButton>("consoleDown");
		if(bConsoleUp && bConsoleDown)
		{
			bConsoleUp->block(on);
			bConsoleDown->block(on);
		}
	}
}

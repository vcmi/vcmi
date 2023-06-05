/*
 * GUIClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GUIClasses.h"

#include "CCastleInterface.h"
#include "CCreatureWindow.h"
#include "CHeroWindow.h"
#include "InfoWindows.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../CServerHandler.h"

#include "../battle/BattleInterfaceClasses.h"
#include "../battle/BattleInterface.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/TextAlignment.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"

#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CreatureCostBox.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"

#include "../lobby/CSavingScreen.h"
#include "../render/Canvas.h"
#include "../render/CAnimation.h"
#include "../CMT.h"

#include "../../CCallback.h"

#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGMarket.h"
#include "../lib/ArtifactUtils.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/GameSettings.h"
#include "../lib/CondSh.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/CStopWatch.h"
#include "../lib/CTownHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/bonuses/Bonus.h"
#include "../lib/NetPacksBase.h"
#include "../lib/StartInfo.h"
#include "../lib/TextOperations.h"

CRecruitmentWindow::CCreatureCard::CCreatureCard(CRecruitmentWindow * window, const CCreature * crea, int totalAmount)
	: CIntObject(LCLICK | RCLICK),
	parent(window),
	selected(false),
	creature(crea),
	amount(totalAmount)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	animation = std::make_shared<CCreaturePic>(1, 1, creature, true, true);
	// 1 + 1 px for borders
	pos.w = animation->pos.w + 2;
	pos.h = animation->pos.h + 2;
}

void CRecruitmentWindow::CCreatureCard::select(bool on)
{
	selected = on;
	redraw();
}

void CRecruitmentWindow::CCreatureCard::clickLeft(tribool down, bool previousState)
{
	if(down)
		parent->select(this->shared_from_this());
}

void CRecruitmentWindow::CCreatureCard::clickRight(tribool down, bool previousState)
{
	if(down)
		GH.windows().createAndPushWindow<CStackWindow>(creature, true);
}

void CRecruitmentWindow::CCreatureCard::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	if(selected)
		to.drawBorder(pos, Colors::RED);
	else
		to.drawBorder(pos, Colors::YELLOW);
}

void CRecruitmentWindow::select(std::shared_ptr<CCreatureCard> card)
{
	if(card == selected)
		return;

	if(selected)
		selected->select(false);

	selected = card;

	if(selected)
		selected->select(true);

	if(card)
	{
		si32 maxAmount = card->creature->maxAmount(LOCPLINT->cb->getResourceAmount());

		vstd::amin(maxAmount, card->amount);

		slider->setAmount(maxAmount);

		if(slider->getValue() != maxAmount)
			slider->scrollTo(maxAmount);
		else // if slider already at 0 - emulate call to sliderMoved()
			sliderMoved(maxAmount);

		costPerTroopValue->createItems(card->creature->getFullRecruitCost());
		totalCostValue->createItems(card->creature->getFullRecruitCost());

		costPerTroopValue->set(card->creature->getFullRecruitCost());
		totalCostValue->set(card->creature->getFullRecruitCost() * maxAmount);

		//Recruit %s
		title->setText(boost::str(boost::format(CGI->generaltexth->tcommands[21]) % card->creature->getNamePluralTranslated()));

		maxButton->block(maxAmount == 0);
		slider->block(maxAmount == 0);
	}
}

void CRecruitmentWindow::buy()
{
	CreatureID crid =  selected->creature->getId();
	SlotID dstslot = dst-> getSlotFor(crid);

	if(!dstslot.validSlot() && (selected->creature->warMachine == ArtifactID::NONE)) //no available slot
	{
		std::string txt;
		if(dst->ID == Obj::HERO)
		{
			txt = CGI->generaltexth->allTexts[425]; //The %s would join your hero, but there aren't enough provisions to support them.
			boost::algorithm::replace_first(txt, "%s", slider->getValue() > 1 ? CGI->creh->objects[crid]->getNamePluralTranslated() : CGI->creh->objects[crid]->getNameSingularTranslated());
		}
		else
		{
			txt = CGI->generaltexth->allTexts[17]; //There is no room in the garrison for this army.
		}

		LOCPLINT->showInfoDialog(txt);
		return;
	}

	onRecruit(crid, slider->getValue());
	if(level >= 0)
		close();
}

void CRecruitmentWindow::showAll(Canvas & to)
{
	CWindowObject::showAll(to);

	Rect(172, 222, 67, 42) + pos.topLeft();

	// recruit\total values
	to.drawBorder(Rect(172, 222, 67, 42) + pos.topLeft(), Colors::YELLOW);
	to.drawBorder(Rect(246, 222, 67, 42) + pos.topLeft(), Colors::YELLOW);

	//cost boxes
	to.drawBorder(Rect( 64, 222, 99, 76) + pos.topLeft(), Colors::YELLOW);
	to.drawBorder(Rect(322, 222, 99, 76) + pos.topLeft(), Colors::YELLOW);

	//buttons borders
	to.drawBorder(Rect(133, 312, 66, 34) + pos.topLeft(), Colors::METALLIC_GOLD);
	to.drawBorder(Rect(211, 312, 66, 34) + pos.topLeft(), Colors::METALLIC_GOLD);
	to.drawBorder(Rect(289, 312, 66, 34) + pos.topLeft(), Colors::METALLIC_GOLD);
}

CRecruitmentWindow::CRecruitmentWindow(const CGDwelling * Dwelling, int Level, const CArmedInstance * Dst, const std::function<void(CreatureID,int)> & Recruit, int y_offset):
	CStatusbarWindow(PLAYER_COLORED, "TPRCRT"),
	onRecruit(Recruit),
	level(Level),
	dst(Dst),
	selected(nullptr),
	dwelling(Dwelling)
{
	moveBy(Point(0, y_offset));

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	slider = std::make_shared<CSlider>(Point(176,279),135,std::bind(&CRecruitmentWindow::sliderMoved,this, _1),0,0,0,true);

	maxButton = std::make_shared<CButton>(Point(134, 313), "IRCBTNS.DEF", CGI->generaltexth->zelp[553], std::bind(&CSlider::scrollToMax, slider), EShortcut::RECRUITMENT_MAX);
	buyButton = std::make_shared<CButton>(Point(212, 313), "IBY6432.DEF", CGI->generaltexth->zelp[554], std::bind(&CRecruitmentWindow::buy, this), EShortcut::GLOBAL_ACCEPT);
	cancelButton = std::make_shared<CButton>(Point(290, 313), "ICN6432.DEF", CGI->generaltexth->zelp[555], std::bind(&CRecruitmentWindow::close, this), EShortcut::GLOBAL_CANCEL);

	title = std::make_shared<CLabel>(243, 32, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW);
	availableValue = std::make_shared<CLabel>(205, 253, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	toRecruitValue = std::make_shared<CLabel>(279, 253, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	costPerTroopValue = std::make_shared<CreatureCostBox>(Rect(65, 222, 97, 74), CGI->generaltexth->allTexts[346]);
	totalCostValue = std::make_shared<CreatureCostBox>(Rect(323, 222, 97, 74), CGI->generaltexth->allTexts[466]);

	availableTitle = std::make_shared<CLabel>(205, 233, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[465]);
	toRecruitTitle = std::make_shared<CLabel>(279, 233, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[16]);

	availableCreaturesChanged();
}

void CRecruitmentWindow::availableCreaturesChanged()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	size_t selectedIndex = 0;

	if(!cards.empty() && selected) // find position of selected item
		selectedIndex = std::find(cards.begin(), cards.end(), selected) - cards.begin();

	select(nullptr);

	cards.clear();

	for(int i=0; i<dwelling->creatures.size(); i++)
	{
		//find appropriate level
		if(level >= 0 && i != level)
			continue;

		int amount = dwelling->creatures[i].first;

		//create new cards
		for(auto & creature : boost::adaptors::reverse(dwelling->creatures[i].second))
			cards.push_back(std::make_shared<CCreatureCard>(this, CGI->creh->objects[creature], amount));
	}

	const int creatureWidth = 102;

	//normal distance between cards - 18px
	int requiredSpace = 18;
	//maximum distance we can use without reaching window borders
	int availableSpace = pos.w - 50 - creatureWidth * (int)cards.size();

	if (cards.size() > 1) // avoid division by zero
		availableSpace /= (int)cards.size() - 1;
	else
		availableSpace = 0;

	assert(availableSpace >= 0);

	const int spaceBetween = std::min(requiredSpace, availableSpace);
	const int totalCreatureWidth = spaceBetween + creatureWidth;

	//now we know total amount of cards and can move them to correct position
	int curx = pos.w / 2 - (creatureWidth*(int)cards.size()/2) - (spaceBetween*((int)cards.size()-1)/2);
	for(auto & card : cards)
	{
		card->moveBy(Point(curx, 64));
		curx += totalCreatureWidth;
	}

	//restore selection
	select(cards[selectedIndex]);

	if(slider->getValue() == slider->getAmount())
		slider->scrollToMax();
	else // if slider already at 0 - emulate call to sliderMoved()
		sliderMoved(slider->getAmount());
}

void CRecruitmentWindow::sliderMoved(int to)
{
	if(!selected)
		return;

	buyButton->block(!to);
	availableValue->setText(std::to_string(selected->amount - to));
	toRecruitValue->setText(std::to_string(to));

	totalCostValue->set(selected->creature->getFullRecruitCost() * to);
}

CSplitWindow::CSplitWindow(const CCreature * creature, std::function<void(int, int)> callback_, int leftMin_, int rightMin_, int leftAmount_, int rightAmount_)
	: CWindowObject(PLAYER_COLORED, "GPUCRDIV"),
	callback(callback_),
	leftAmount(leftAmount_),
	rightAmount(rightAmount_),
	leftMin(leftMin_),
	rightMin(rightMin_)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	int total = leftAmount + rightAmount;
	int leftMax = total - rightMin;
	int rightMax = total - leftMin;

	ok = std::make_shared<CButton>(Point(20, 263), "IOK6432", CButton::tooltip(), std::bind(&CSplitWindow::apply, this), EShortcut::GLOBAL_ACCEPT);
	cancel = std::make_shared<CButton>(Point(214, 263), "ICN6432", CButton::tooltip(), std::bind(&CSplitWindow::close, this), EShortcut::GLOBAL_CANCEL);

	int sliderPosition = total - leftMin - rightMin;

	leftInput = std::make_shared<CTextInput>(Rect(20, 218, 100, 36), FONT_BIG, std::bind(&CSplitWindow::setAmountText, this, _1, true));
	rightInput = std::make_shared<CTextInput>(Rect(176, 218, 100, 36), FONT_BIG, std::bind(&CSplitWindow::setAmountText, this, _1, false));

	//add filters to allow only number input
	leftInput->filters += std::bind(&CTextInput::numberFilter, _1, _2, leftMin, leftMax);
	rightInput->filters += std::bind(&CTextInput::numberFilter, _1, _2, rightMin, rightMax);

	leftInput->setText(std::to_string(leftAmount), false);
	rightInput->setText(std::to_string(rightAmount), false);

	animLeft = std::make_shared<CCreaturePic>(20, 54, creature, true, false);
	animRight = std::make_shared<CCreaturePic>(177, 54,creature, true, false);

	slider = std::make_shared<CSlider>(Point(21, 194), 257, std::bind(&CSplitWindow::sliderMoved, this, _1), 0, sliderPosition, rightAmount - rightMin, true);

	std::string titleStr = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(titleStr,"%s", creature->getNamePluralTranslated());
	title = std::make_shared<CLabel>(150, 34, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, titleStr);
}

void CSplitWindow::setAmountText(std::string text, bool left)
{
	int amount = 0;
	if(text.length())
	{
		try
		{
			amount = boost::lexical_cast<int>(text);
		}
		catch(boost::bad_lexical_cast &)
		{
			amount = left ? leftAmount : rightAmount;
		}

		int total = leftAmount + rightAmount;
		if(amount > total)
			amount = total;
	}

	setAmount(amount, left);
	slider->scrollTo(rightAmount - rightMin);
}

void CSplitWindow::setAmount(int value, bool left)
{
	int total = leftAmount + rightAmount;
	leftAmount  = left ? value : total - value;
	rightAmount = left ? total - value : value;

	leftInput->setText(std::to_string(leftAmount));
	rightInput->setText(std::to_string(rightAmount));
}

void CSplitWindow::apply()
{
	callback(leftAmount, rightAmount);
	close();
}

void CSplitWindow::sliderMoved(int to)
{
	setAmount(rightMin + to, false);
}

CLevelWindow::CLevelWindow(const CGHeroInstance * hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> & skills, std::function<void(ui32)> callback)
	: CWindowObject(PLAYER_COLORED, "LVLUPBKG"),
	cb(callback)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	LOCPLINT->showingDialog->setn(true);

	if(!skills.empty())
	{
		std::vector<std::shared_ptr<CSelectableComponent>> comps;
		for(auto & skill : skills)
		{
			auto comp = std::make_shared<CSelectableComponent>(CComponent::secskill, skill, hero->getSecSkillLevel(SecondarySkill(skill))+1, CComponent::medium);
			comps.push_back(comp);
		}

		box = std::make_shared<CComponentBox>(comps, Rect(75, 300, pos.w - 150, 100));
	}

	portrait = std::make_shared<CAnimImage>("PortraitsLarge", hero->portrait, 0, 170, 66);
	ok = std::make_shared<CButton>(Point(297, 413), "IOKAY", CButton::tooltip(), std::bind(&CLevelWindow::close, this), EShortcut::GLOBAL_ACCEPT);

	//%s has gained a level.
	mainTitle = std::make_shared<CLabel>(192, 33, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->allTexts[444]) % hero->getNameTranslated()));

	//%s is now a level %d %s.
	std::string levelTitleText = CGI->generaltexth->translate("core.genrltxt.445");
	boost::replace_first(levelTitleText, "%s", hero->getNameTranslated());
	boost::replace_first(levelTitleText, "%d", std::to_string(hero->level));
	boost::replace_first(levelTitleText, "%s", hero->type->heroClass->getNameTranslated());

	levelTitle = std::make_shared<CLabel>(192, 162, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, levelTitleText);

	skillIcon = std::make_shared<CAnimImage>("PSKIL42", pskill, 0, 174, 190);

	skillValue = std::make_shared<CLabel>(192, 253, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->primarySkillNames[pskill] + " +1");
}


CLevelWindow::~CLevelWindow()
{
	//FIXME: call callback if there was nothing to select?
	if (box && box->selectedIndex() != -1)
		cb(box->selectedIndex());

	LOCPLINT->showingDialog->setn(false);
}

CTavernWindow::CTavernWindow(const CGObjectInstance * TavernObj)
	: CStatusbarWindow(PLAYER_COLORED, "TPTAVERN"),
	tavernObj(TavernObj)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	std::vector<const CGHeroInstance*> h = LOCPLINT->cb->getAvailableHeroes(TavernObj);
	if(h.size() < 2)
		h.resize(2, nullptr);

	selected = 0;
	if(!h[0])
		selected = 1;
	if(!h[0] && !h[1])
		selected = -1;

	oldSelected = -1;

	h1 = std::make_shared<HeroPortrait>(selected, 0, 72, 299, h[0]);
	h2 = std::make_shared<HeroPortrait>(selected, 1, 162, 299, h[1]);

	title = std::make_shared<CLabel>(197, 32, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[37]);
	cost = std::make_shared<CLabel>(320, 328, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(GameConstants::HERO_GOLD_COST));
	heroDescription = std::make_shared<CTextBox>("", Rect(30, 373, 233, 35), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	heroesForHire = std::make_shared<CLabel>(145, 283, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[38]);

	auto rumorText = boost::str(boost::format(CGI->generaltexth->allTexts[216]) % LOCPLINT->cb->getTavernRumor(tavernObj));
	rumor = std::make_shared<CTextBox>(rumorText, Rect(32, 188, 330, 66), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
	cancel = std::make_shared<CButton>(Point(310, 428), "ICANCEL.DEF", CButton::tooltip(CGI->generaltexth->tavernInfo[7]), std::bind(&CTavernWindow::close, this), EShortcut::GLOBAL_CANCEL);
	recruit = std::make_shared<CButton>(Point(272, 355), "TPTAV01.DEF", CButton::tooltip(), std::bind(&CTavernWindow::recruitb, this), EShortcut::GLOBAL_ACCEPT);
	thiefGuild = std::make_shared<CButton>(Point(22, 428), "TPTAV02.DEF", CButton::tooltip(CGI->generaltexth->tavernInfo[5]), std::bind(&CTavernWindow::thievesguildb, this), EShortcut::ADVENTURE_THIEVES_GUILD);

	if(LOCPLINT->cb->getResourceAmount(EGameResID::GOLD) < GameConstants::HERO_GOLD_COST) //not enough gold
	{
		recruit->addHoverText(CButton::NORMAL, CGI->generaltexth->tavernInfo[0]); //Cannot afford a Hero
		recruit->block(true);
	}
	else if(LOCPLINT->cb->howManyHeroes(true) >= CGI->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP))
	{
		//Cannot recruit. You already have %d Heroes.
		recruit->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->tavernInfo[1]) % LOCPLINT->cb->howManyHeroes(true)));
		recruit->block(true);
	}
	else if(LOCPLINT->cb->howManyHeroes(false) >= CGI->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP))
	{
		//Cannot recruit. You already have %d Heroes.
		recruit->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->tavernInfo[1]) % LOCPLINT->cb->howManyHeroes(false)));
		recruit->block(true);
	}
	else if(LOCPLINT->castleInt && LOCPLINT->castleInt->town->visitingHero)
	{
		recruit->addHoverText(CButton::NORMAL, CGI->generaltexth->tavernInfo[2]); //Cannot recruit. You already have a Hero in this town.
		recruit->block(true);
	}
	else
	{
		if(selected == -1)
			recruit->block(true);
	}
	if(LOCPLINT->castleInt)
		CCS->videoh->open(LOCPLINT->castleInt->town->town->clientInfo.tavernVideo);
	else
		CCS->videoh->open("TAVERN.BIK");
}

void CTavernWindow::recruitb()
{
	const CGHeroInstance *toBuy = (selected ? h2 : h1)->h;
	const CGObjectInstance *obj = tavernObj;
	close();
	LOCPLINT->cb->recruitHero(obj, toBuy);
}

void CTavernWindow::thievesguildb()
{
	GH.windows().createAndPushWindow<CThievesGuildWindow>(tavernObj);
}

CTavernWindow::~CTavernWindow()
{
	CCS->videoh->close();
}

void CTavernWindow::show(Canvas & to)
{
	CWindowObject::show(to);

	if(selected >= 0)
	{
		auto sel = selected ? h2 : h1;

		if(selected != oldSelected)
		{
			// Selected hero just changed. Update RECRUIT button hover text if recruitment is allowed.
			oldSelected = selected;

			heroDescription->setText(sel->description);

			//Recruit %s the %s
			if (!recruit->isBlocked())
				recruit->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->tavernInfo[3]) % sel->h->getNameTranslated() % sel->h->type->heroClass->getNameTranslated()));

		}

		to.drawBorder(Rect::createAround(sel->pos, 2), Colors::BRIGHT_YELLOW, 2);
	}

	CCS->videoh->update(pos.x+70, pos.y+56, to.getInternalSurface(), true, false);
}

void CTavernWindow::HeroPortrait::clickLeft(tribool down, bool previousState)
{
	if(h && previousState && !down)
		*_sel = _id;
}

void CTavernWindow::HeroPortrait::clickRight(tribool down, bool previousState)
{
	if(h && down)
		GH.windows().createAndPushWindow<CRClickPopupInt>(std::make_shared<CHeroWindow>(h));
}

CTavernWindow::HeroPortrait::HeroPortrait(int & sel, int id, int x, int y, const CGHeroInstance * H)
	: CIntObject(LCLICK | RCLICK | HOVER),
	h(H), _sel(&sel), _id(id)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	h = H;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;

	if(H)
	{
		hoverName = CGI->generaltexth->tavernInfo[4];
		boost::algorithm::replace_first(hoverName,"%s",H->getNameTranslated());

		int artifs = (int)h->artifactsWorn.size() + (int)h->artifactsInBackpack.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifactsWorn, ArtifactPosition(i)))
				artifs--;

		description = CGI->generaltexth->allTexts[215];
		boost::algorithm::replace_first(description, "%s", h->getNameTranslated());
		boost::algorithm::replace_first(description, "%d", std::to_string(h->level));
		boost::algorithm::replace_first(description, "%s", h->type->heroClass->getNameTranslated());
		boost::algorithm::replace_first(description, "%d", std::to_string(artifs));

		portrait = std::make_shared<CAnimImage>("portraitsLarge", h->portrait);
	}
}

void CTavernWindow::HeroPortrait::hover(bool on)
{
	//Hoverable::hover(on);
	if(on)
		GH.statusbar()->write(hoverName);
	else
		GH.statusbar()->clear();
}

static const std::string QUICK_EXCHANGE_MOD_PREFIX = "quick-exchange";
static const std::string QUICK_EXCHANGE_BG = QUICK_EXCHANGE_MOD_PREFIX + "/TRADEQE";

static bool isQuickExchangeLayoutAvailable()
{
	return CResourceHandler::get()->existsResource(ResourceID(std::string("SPRITES/") + QUICK_EXCHANGE_BG, EResType::IMAGE));
}

// Runs a task asynchronously with gamestate locking and waitTillRealize set to true
class GsThread
{
private:
	std::function<void()> action;
	std::shared_ptr<CCallback> cb;

public:

	static void run(std::function<void()> action)
	{
		std::shared_ptr<GsThread> instance(new GsThread(action));


		boost::thread(std::bind(&GsThread::staticRun, instance));
	}

private:
	GsThread(std::function<void()> action)
		:action(action), cb(LOCPLINT->cb)
	{
	}

	static void staticRun(std::shared_ptr<GsThread> instance)
	{
		instance->run();
	}

	void run()
	{
		boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);

		auto originalWaitTillRealize = cb->waitTillRealize;
		auto originalUnlockGsWhenWating = cb->unlockGsWhenWaiting;

		cb->waitTillRealize = true;
		cb->unlockGsWhenWaiting = true;

		action();

		cb->waitTillRealize = originalWaitTillRealize;
		cb->unlockGsWhenWaiting = originalUnlockGsWhenWating;
	}
};

CExchangeController::CExchangeController(CExchangeWindow * view, ObjectInstanceID hero1, ObjectInstanceID hero2)
	:left(LOCPLINT->cb->getHero(hero1)), right(LOCPLINT->cb->getHero(hero2)), cb(LOCPLINT->cb), view(view)
{
}

std::function<void()> CExchangeController::onMoveArmyToLeft()
{
	return [&]() { moveArmy(false); };
}

std::function<void()> CExchangeController::onMoveArmyToRight()
{
	return [&]() { moveArmy(true); };
}

std::vector<CArtifactInstance *> getBackpackArts(const CGHeroInstance * hero)
{
	std::vector<CArtifactInstance *> result;

	for(auto slot : hero->artifactsInBackpack)
	{
		result.push_back(slot.artifact);
	}

	return result;
}

std::function<void()> CExchangeController::onSwapArtifacts()
{
	return [&]()
	{
		GsThread::run([=]
		{
			cb->bulkMoveArtifacts(left->id, right->id, true);
		});
	};
}

std::function<void()> CExchangeController::onMoveArtifactsToLeft()
{
	return [&]() { moveArtifacts(false); };
}

std::function<void()> CExchangeController::onMoveArtifactsToRight()
{
	return [&]() { moveArtifacts(true); };
}

std::vector<std::pair<SlotID, CStackInstance *>> getStacks(const CArmedInstance * source)
{
	auto slots = source->Slots();

	return std::vector<std::pair<SlotID, CStackInstance *>>(slots.begin(), slots.end());
}

std::function<void()> CExchangeController::onSwapArmy()
{
	return [&]()
	{
		GsThread::run([=]
		{
			if(left->tempOwner != cb->getMyColor()
				|| right->tempOwner != cb->getMyColor())
			{
				return;
			}

			auto leftSlots = getStacks(left);
			auto rightSlots = getStacks(right);

			auto i = leftSlots.begin(), j = rightSlots.begin();

			for(; i != leftSlots.end() && j != rightSlots.end(); i++, j++)
			{
				cb->swapCreatures(left, right, i->first, j->first);
			}

			if(i != leftSlots.end())
			{
				for(; i != leftSlots.end(); i++)
				{
					cb->swapCreatures(left, right, i->first, right->getFreeSlot());
				}
			}
			else if(j != rightSlots.end())
			{
				for(; j != rightSlots.end(); j++)
				{
					cb->swapCreatures(left, right, left->getFreeSlot(), j->first);
				}
			}
		});
	};
}

std::function<void()> CExchangeController::onMoveStackToLeft(SlotID slotID)
{
	return [=]()
	{
		if(right->tempOwner != cb->getMyColor())
		{
			return;
		}

		moveStack(right, left, slotID);
	};
}

std::function<void()> CExchangeController::onMoveStackToRight(SlotID slotID)
{
	return [=]()
	{
		if(left->tempOwner != cb->getMyColor())
		{
			return;
		}

		moveStack(left, right, slotID);
	};
}

void CExchangeController::moveStack(
	const CGHeroInstance * source,
	const CGHeroInstance * target,
	SlotID sourceSlot)
{
	auto creature = source->getCreature(sourceSlot);
	SlotID targetSlot = target->getSlotFor(creature);

	if(targetSlot.validSlot())
	{
		if(source->stacksCount() == 1 && source->needsLastStack())
		{
			cb->splitStack(
				source,
				target,
				sourceSlot,
				targetSlot,
				target->getStackCount(targetSlot) + source->getStackCount(sourceSlot) - 1);
		}
		else
		{
			cb->mergeOrSwapStacks(source, target, sourceSlot, targetSlot);
		}
	}
}

void CExchangeController::moveArmy(bool leftToRight)
{
	const CGHeroInstance * source = leftToRight ? left : right;
	const CGHeroInstance * target = leftToRight ? right : left;
	const CGarrisonSlot * selection =  this->view->getSelectedSlotID();
	SlotID slot;

	if(source->tempOwner != cb->getMyColor())
	{
		return;
	}

	if(selection && selection->our() && selection->getObj() == source)
	{
		slot = selection->getSlot();
	}
	else
	{
		auto weakestSlot = vstd::minElementByFun(
			source->Slots(), 
			[](const std::pair<SlotID, CStackInstance *> & s) -> int
			{
				return s.second->getCreatureID().toCreature()->getAIValue();
			});

		slot = weakestSlot->first;
	}

	cb->bulkMoveArmy(source->id, target->id, slot);
}

void CExchangeController::moveArtifacts(bool leftToRight)
{
	const CGHeroInstance * source = leftToRight ? left : right;
	const CGHeroInstance * target = leftToRight ? right : left;

	if(source->tempOwner != cb->getMyColor())
	{
		return;
	}

	GsThread::run([=]
	{
		cb->bulkMoveArtifacts(source->id, target->id, false);
	});
}

void CExchangeController::moveArtifact(
	const CGHeroInstance * source,
	const CGHeroInstance * target,
	ArtifactPosition srcPosition)
{
	auto srcLocation = ArtifactLocation(source, srcPosition);
	auto dstLocation = ArtifactLocation(target,
		ArtifactUtils::getArtAnyPosition(target, source->getArt(srcPosition)->getTypeId()));

	cb->swapArtifacts(srcLocation, dstLocation);
}

CExchangeWindow::CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID)
	: CStatusbarWindow(PLAYER_COLORED | BORDERED, isQuickExchangeLayoutAvailable() ? QUICK_EXCHANGE_BG : "TRADE2"),
	controller(this, hero1, hero2),
	moveStackLeftButtons(),
	moveStackRightButtons()
{
	const bool qeLayout = isQuickExchangeLayoutAvailable();

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	heroInst[0] = LOCPLINT->cb->getHero(hero1);
	heroInst[1] = LOCPLINT->cb->getHero(hero2);

	auto genTitle = [](const CGHeroInstance * h)
	{
		boost::format fmt(CGI->generaltexth->allTexts[138]);
		fmt % h->getNameTranslated() % h->level % h->type->heroClass->getNameTranslated();
		return boost::str(fmt);
	};

	titles[0] = std::make_shared<CLabel>(147, 25, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, genTitle(heroInst[0]));
	titles[1] = std::make_shared<CLabel>(653, 25, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, genTitle(heroInst[1]));

	auto PSKIL32 = std::make_shared<CAnimation>("PSKIL32");
	PSKIL32->preload();

	auto SECSK32 = std::make_shared<CAnimation>("SECSK32");

	for(int g = 0; g < 4; ++g) 
	{
		if (qeLayout)
			primSkillImages.push_back(std::make_shared<CAnimImage>(PSKIL32, g, Rect(389, 12 + 26 * g, 22, 22)));
		else
			primSkillImages.push_back(std::make_shared<CAnimImage>(PSKIL32, g, 0, 385, 19 + 36 * g));
	}

	for(int leftRight : {0, 1})
	{
		const CGHeroInstance * hero = heroInst.at(leftRight);

		herosWArt[leftRight] = std::make_shared<CHeroWithMaybePickedArtifact>(this, hero);

		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
			primSkillValues[leftRight].push_back(std::make_shared<CLabel>(352 + (qeLayout ? 96 : 93) * leftRight, (qeLayout ? 22 : 35) + (qeLayout ? 26 : 36) * m, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE));


		for(int m=0; m < hero->secSkills.size(); ++m)
			secSkillIcons[leftRight].push_back(std::make_shared<CAnimImage>(SECSK32, 0, 0, 32 + 36 * m + 454 * leftRight, qeLayout ? 83 : 88));

		specImages[leftRight] = std::make_shared<CAnimImage>("UN32", hero->type->imageIndex, 0, 67 + 490 * leftRight, qeLayout ? 41 : 45);

		expImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 4, 0, 103 + 490 * leftRight, qeLayout ? 41 : 45);
		expValues[leftRight] = std::make_shared<CLabel>(119 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		manaImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 5, 0, 139 + 490 * leftRight, qeLayout ? 41 : 45);
		manaValues[leftRight] = std::make_shared<CLabel>(155 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	}

	portraits[0] = std::make_shared<CAnimImage>("PortraitsLarge", heroInst[0]->portrait, 0, 257, 13);
	portraits[1] = std::make_shared<CAnimImage>("PortraitsLarge", heroInst[1]->portrait, 0, 485, 13);

	artifs[0] = std::make_shared<CArtifactsOfHeroMain>(Point(-334, 150));
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = std::make_shared<CArtifactsOfHeroMain>(Point(98, 150));
	artifs[1]->setHero(heroInst[1]);

	addSet(artifs[0]);
	addSet(artifs[1]);

	for(int g=0; g<4; ++g)
	{
		primSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>());
		if (qeLayout)
			primSkillAreas[g]->pos = Rect(Point(pos.x + 324, pos.y + 12 + 26 * g), Point(152, 22));
		else
			primSkillAreas[g]->pos = Rect(Point(pos.x + 329, pos.y + 19 + 36 * g), Point(140, 32));
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->type = g;
		primSkillAreas[g]->bonusValue = 0;
		primSkillAreas[g]->baseType = 0;
		primSkillAreas[g]->hoverText = CGI->generaltexth->heroscrn[1];
		boost::replace_first(primSkillAreas[g]->hoverText, "%s", CGI->generaltexth->primarySkillNames[g]);
	}

	//heroes related thing
	for(int b=0; b < heroInst.size(); b++)
	{
		const CGHeroInstance * hero = heroInst.at(b);

		//secondary skill's clickable areas
		for(int g=0; g<hero->secSkills.size(); ++g)
		{
			int skill = hero->secSkills[g].first,
				level = hero->secSkills[g].second; // <1, 3>
			secSkillAreas[b].push_back(std::make_shared<LRClickableAreaWTextComp>());
			secSkillAreas[b][g]->pos = Rect(Point(pos.x + 32 + g * 36 + b * 454 , pos.y + (qeLayout ? 83 : 88)), Point(32, 32) );
			secSkillAreas[b][g]->baseType = 1;

			secSkillAreas[b][g]->type = skill;
			secSkillAreas[b][g]->bonusValue = level;
			secSkillAreas[b][g]->text = CGI->skillh->getByIndex(skill)->getDescriptionTranslated(level);

			secSkillAreas[b][g]->hoverText = CGI->generaltexth->heroscrn[21];
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->generaltexth->levels[level - 1]);
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->skillh->getByIndex(skill)->getNameTranslated());
		}

		heroAreas[b] = std::make_shared<CHeroArea>(257 + 228*b, 13, hero);

		specialtyAreas[b] = std::make_shared<LRClickableAreaWText>();
		specialtyAreas[b]->pos = Rect(Point(pos.x + 69 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		specialtyAreas[b]->hoverText = CGI->generaltexth->heroscrn[27];
		specialtyAreas[b]->text = hero->type->getSpecialtyDescriptionTranslated();

		experienceAreas[b] = std::make_shared<LRClickableAreaWText>();
		experienceAreas[b]->pos = Rect(Point(pos.x + 105 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		experienceAreas[b]->hoverText = CGI->generaltexth->heroscrn[9];
		experienceAreas[b]->text = CGI->generaltexth->allTexts[2];
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(hero->level));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(CGI->heroh->reqExp(hero->level+1)));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(hero->exp));

		spellPointsAreas[b] = std::make_shared<LRClickableAreaWText>();
		spellPointsAreas[b]->pos = Rect(Point(pos.x + 141 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		spellPointsAreas[b]->hoverText = CGI->generaltexth->heroscrn[22];
		spellPointsAreas[b]->text = CGI->generaltexth->allTexts[205];
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%s", hero->getNameTranslated());
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", std::to_string(hero->mana));
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", std::to_string(hero->manaLimit()));

		morale[b] = std::make_shared<MoraleLuckBox>(true, Rect(Point(176 + 490 * b, 39), Point(32, 32)), true);
		luck[b] = std::make_shared<MoraleLuckBox>(false,  Rect(Point(212 + 490 * b, 39), Point(32, 32)), true);
	}

	quit = std::make_shared<CButton>(Point(732, 567), "IOKAY.DEF", CGI->generaltexth->zelp[600], std::bind(&CExchangeWindow::close, this), EShortcut::GLOBAL_ACCEPT);
	if(queryID.getNum() > 0)
		quit->addCallback([=](){ LOCPLINT->cb->selectionMade(0, queryID); });

	questlogButton[0] = std::make_shared<CButton>(Point( 10, qeLayout ? 39 : 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 0));
	questlogButton[1] = std::make_shared<CButton>(Point(740, qeLayout ? 39 : 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 1));

	Rect barRect(5, 578, 725, 18);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), barRect, 5, 578));

	//garrison interface

	garr = std::make_shared<CGarrisonInt>(69, qeLayout ? 122 : 131, 4, Point(418,0), heroInst[0], heroInst[1], true, true);
	auto splitButtonCallback = [&](){ garr->splitClick(); };
	garr->addSplitBtn(std::make_shared<CButton>( Point( 10, qeLayout ? 122 : 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback));
	garr->addSplitBtn(std::make_shared<CButton>( Point(744, qeLayout ? 122 : 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback));

	if(qeLayout)
	{
		moveAllGarrButtonLeft    = std::make_shared<CButton>(Point(325, 118), QUICK_EXCHANGE_MOD_PREFIX + "/armRight.DEF", CButton::tooltip(CGI->generaltexth->qeModCommands[1]), controller.onMoveArmyToRight());
		echangeGarrButton        = std::make_shared<CButton>(Point(377, 118), QUICK_EXCHANGE_MOD_PREFIX + "/swapAll.DEF", CButton::tooltip(CGI->generaltexth->qeModCommands[2]), controller.onSwapArmy());
		moveAllGarrButtonRight   = std::make_shared<CButton>(Point(425, 118), QUICK_EXCHANGE_MOD_PREFIX + "/armLeft.DEF", CButton::tooltip(CGI->generaltexth->qeModCommands[1]), controller.onMoveArmyToLeft());
		moveArtifactsButtonLeft  = std::make_shared<CButton>(Point(325, 154), QUICK_EXCHANGE_MOD_PREFIX + "/artRight.DEF", CButton::tooltip(CGI->generaltexth->qeModCommands[3]), controller.onMoveArtifactsToRight());
		echangeArtifactsButton   = std::make_shared<CButton>(Point(377, 154), QUICK_EXCHANGE_MOD_PREFIX + "/swapAll.DEF", CButton::tooltip(CGI->generaltexth->qeModCommands[4]), controller.onSwapArtifacts());
		moveArtifactsButtonRight = std::make_shared<CButton>(Point(425, 154), QUICK_EXCHANGE_MOD_PREFIX + "/artLeft.DEF", CButton::tooltip(CGI->generaltexth->qeModCommands[3]), controller.onMoveArtifactsToLeft());

		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
		{
			moveStackLeftButtons.push_back(
				std::make_shared<CButton>(
					Point(484 + 35 * i, 154),
					QUICK_EXCHANGE_MOD_PREFIX + "/unitLeft.DEF",
					CButton::tooltip(CGI->generaltexth->qeModCommands[1]),
					controller.onMoveStackToLeft(SlotID(i))));

			moveStackRightButtons.push_back(
				std::make_shared<CButton>(
					Point(66 + 35 * i, 154),
					QUICK_EXCHANGE_MOD_PREFIX + "/unitRight.DEF",
					CButton::tooltip(CGI->generaltexth->qeModCommands[1]),
					controller.onMoveStackToRight(SlotID(i))));
		}
	}

	updateWidgets();
}

const CGarrisonSlot * CExchangeWindow::getSelectedSlotID() const
{
	return garr->getSelection();
}

void CExchangeWindow::updateGarrisons()
{
	garr->recreateSlots();

	updateWidgets();
}

void CExchangeWindow::questlog(int whichHero)
{
	CCS->curh->dragAndDropCursor(nullptr);
	LOCPLINT->showQuestLog();
}

void CExchangeWindow::updateWidgets()
{
	for(size_t leftRight : {0, 1})
	{
		const CGHeroInstance * hero = heroInst.at(leftRight);

		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
		{
			auto value = herosWArt[leftRight]->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(m));
			primSkillValues[leftRight][m]->setText(std::to_string(value));
		}

		for(int m=0; m < hero->secSkills.size(); ++m)
		{
			int id = hero->secSkills[m].first;
			int level = hero->secSkills[m].second;

			secSkillIcons[leftRight][m]->setFrame(2 + id * 3 + level);
		}

		expValues[leftRight]->setText(TextOperations::formatMetric(hero->exp, 3));
		manaValues[leftRight]->setText(TextOperations::formatMetric(hero->mana, 3));

		morale[leftRight]->set(hero);
		luck[leftRight]->set(hero);
	}
}

CShipyardWindow::CShipyardWindow(const TResources & cost, int state, BoatId boatType, const std::function<void()> & onBuy)
	: CStatusbarWindow(PLAYER_COLORED, "TPSHIP")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	bgWater = std::make_shared<CPicture>("TPSHIPBK", 100, 69);

	std::string boatFilenames[3] = {"AB01_", "AB02_", "AB03_"};

	Point waterCenter = Point(bgWater->pos.x+bgWater->pos.w/2, bgWater->pos.y+bgWater->pos.h/2);
	bgShip = std::make_shared<CAnimImage>(boatFilenames[boatType.getNum()], 0, 7, 120, 96, 0);
	bgShip->center(waterCenter);

	// Create resource icons and costs.
	std::string goldValue = std::to_string(cost[EGameResID::GOLD]);
	std::string woodValue = std::to_string(cost[EGameResID::WOOD]);

	goldCost = std::make_shared<CLabel>(118, 294, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, goldValue);
	woodCost = std::make_shared<CLabel>(212, 294, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, woodValue);

	goldPic = std::make_shared<CAnimImage>("RESOURCE",GameResID(EGameResID::GOLD), 0, 100, 244);
	woodPic = std::make_shared<CAnimImage>("RESOURCE", GameResID(EGameResID::WOOD), 0, 196, 244);

	quit = std::make_shared<CButton>(Point(224, 312), "ICANCEL", CButton::tooltip(CGI->generaltexth->allTexts[599]), std::bind(&CShipyardWindow::close, this), EShortcut::GLOBAL_CANCEL);
	build = std::make_shared<CButton>(Point(42, 312), "IBUY30", CButton::tooltip(CGI->generaltexth->allTexts[598]), std::bind(&CShipyardWindow::close, this), EShortcut::GLOBAL_ACCEPT);
	build->addCallback(onBuy);

	for(GameResID i = EGameResID::WOOD; i <= EGameResID::GOLD; ++i)
	{
		if(cost[i] > LOCPLINT->cb->getResourceAmount(i))
		{
			build->block(true);
			break;
		}
	}

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	title = std::make_shared<CLabel>(164, 27,  FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[13]);
	costLabel = std::make_shared<CLabel>(164, 220, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->jktexts[14]);
}

void CTransformerWindow::CItem::move()
{
	if(left)
		moveBy(Point(289, 0));
	else
		moveBy(Point(-289, 0));
	left = !left;
}

void CTransformerWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		move();
		parent->redraw();
	}
}

void CTransformerWindow::CItem::update()
{
	icon->setFrame(parent->army->getCreature(SlotID(id))->getId() + 2);
}

CTransformerWindow::CItem::CItem(CTransformerWindow * parent_, int size_, int id_)
	: CIntObject(LCLICK),
	id(id_),
	size(size_),
	parent(parent_)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	left = true;
	pos.w = 58;
	pos.h = 64;

	pos.x += 45  + (id%3)*83 + id/6*83;
	pos.y += 109 + (id/3)*98;
	icon = std::make_shared<CAnimImage>("TWCRPORT", parent->army->getCreature(SlotID(id))->getId() + 2);
	count = std::make_shared<CLabel>(28, 76,FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(size));
}

void CTransformerWindow::makeDeal()
{
	for(auto & elem : items)
	{
		if(!elem->left)
			LOCPLINT->cb->trade(market, EMarketMode::CREATURE_UNDEAD, elem->id, 0, 0, hero);
	}
}

void CTransformerWindow::addAll()
{
	for(auto & elem : items)
	{
		if(elem->left)
			elem->move();
	}
	redraw();
}

void CTransformerWindow::updateGarrisons()
{
	for(auto & item : items)
		item->update();
}

CTransformerWindow::CTransformerWindow(const IMarket * _market, const CGHeroInstance * _hero)
	: CStatusbarWindow(PLAYER_COLORED, "SKTRNBK"),
	hero(_hero),
	market(_market)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	if(hero)
		army = hero;
	else
		army = dynamic_cast<const CArmedInstance *>(market); //for town only
	
	if(army)
	{
		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
		{
			if(army->getCreature(SlotID(i)))
				items.push_back(std::make_shared<CItem>(this, army->getStackCount(SlotID(i)), i));
		}
	}

	all = std::make_shared<CButton>(Point(146, 416), "ALTARMY.DEF", CGI->generaltexth->zelp[590], [&](){ addAll(); }, EShortcut::RECRUITMENT_UPGRADE_ALL);
	convert = std::make_shared<CButton>(Point(269, 416), "ALTSACR.DEF", CGI->generaltexth->zelp[591], [&](){ makeDeal(); }, EShortcut::GLOBAL_ACCEPT);
	cancel = std::make_shared<CButton>(Point(392, 416), "ICANCEL.DEF", CGI->generaltexth->zelp[592], [&](){ close(); },EShortcut::GLOBAL_CANCEL);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	titleLeft = std::make_shared<CLabel>(153, 29,FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[485]);//holding area
	titleRight = std::make_shared<CLabel>(153+295, 29, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[486]);//transformer
	helpLeft = std::make_shared<CTextBox>(CGI->generaltexth->allTexts[487], Rect(26,  56, 255, 40), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW);//move creatures to create skeletons
	helpRight = std::make_shared<CTextBox>(CGI->generaltexth->allTexts[488], Rect(320, 56, 255, 40), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW);//creatures here will become skeletons
}

CUniversityWindow::CItem::CItem(CUniversityWindow * _parent, int _ID, int X, int Y)
	: CIntObject(LCLICK | RCLICK | HOVER),
	ID(_ID),
	parent(_parent)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.x += X;
	pos.y += Y;

	topBar = std::make_shared<CAnimImage>(parent->bars, 0, 0, -28, -22);
	bottomBar = std::make_shared<CAnimImage>(parent->bars, 0, 0, -28, 48);

	icon = std::make_shared<CAnimImage>("SECSKILL", _ID * 3 + 3, 0);

	name = std::make_shared<CLabel>(22, -13, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->skillh->getByIndex(ID)->getNameTranslated());
	level = std::make_shared<CLabel>(22, 57, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->levels[0]);

	pos.h = icon->pos.h;
	pos.w = icon->pos.w;
}

void CUniversityWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		if(state() == 2)
			GH.windows().createAndPushWindow<CUnivConfirmWindow>(parent, ID, LOCPLINT->cb->getResourceAmount(EGameResID::GOLD) >= 2000);
	}
}

void CUniversityWindow::CItem::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CRClickPopup::createAndPush(CGI->skillh->getByIndex(ID)->getDescriptionTranslated(1), std::make_shared<CComponent>(CComponent::secskill, ID, 1));
	}
}

void CUniversityWindow::CItem::hover(bool on)
{
	if(on)
		GH.statusbar()->write(CGI->skillh->getByIndex(ID)->getNameTranslated());
	else
		GH.statusbar()->clear();
}

int CUniversityWindow::CItem::state()
{
	if(parent->hero->getSecSkillLevel(SecondarySkill(ID)))//hero know this skill
		return 1;
	if(!parent->hero->canLearnSkill(SecondarySkill(ID)))//can't learn more skills
		return 0;
	return 2;
}

void CUniversityWindow::CItem::showAll(Canvas & to)
{
	//TODO: update when state actually changes
	auto stateIndex = state();
	topBar->setFrame(stateIndex);
	bottomBar->setFrame(stateIndex);

	CIntObject::showAll(to);
}

CUniversityWindow::CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market)
	: CStatusbarWindow(PLAYER_COLORED, "UNIVERS1"),
	hero(_hero),
	market(_market)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	bars = std::make_shared<CAnimation>();
	bars->setCustom("UNIVRED", 0, 0);
	bars->setCustom("UNIVGOLD", 1, 0);
	bars->setCustom("UNIVGREN", 2, 0);
	bars->preload();
	
	std::string titleStr = CGI->generaltexth->allTexts[602];
	std::string speechStr = CGI->generaltexth->allTexts[603];

	if(auto town = dynamic_cast<const CGTownInstance *>(_market))
	{
		auto faction = town->town->faction->getId();
		auto bid = town->town->getSpecialBuilding(BuildingSubID::MAGIC_UNIVERSITY)->bid;
		titlePic = std::make_shared<CAnimImage>((*CGI->townh)[faction]->town->clientInfo.buildingsIcons, bid);
	}
	else if(auto uni = dynamic_cast<const CGUniversity *>(_market); uni->appearance)
	{
		titlePic = std::make_shared<CAnimImage>(uni->appearance->animationFile, 0);
		titleStr = uni->title;
		speechStr = uni->speech;
	}
	else
	{
		titlePic = std::make_shared<CPicture>("UNIVBLDG");
	}

	titlePic->center(Point(232 + pos.x, 76 + pos.y));

	clerkSpeech = std::make_shared<CTextBox>(speechStr, Rect(24, 129, 413, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	title = std::make_shared<CLabel>(231, 26, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, titleStr);

	std::vector<int> goods = market->availableItemsIds(EMarketMode::RESOURCE_SKILL);

	for(int i=0; i<goods.size(); i++)//prepare clickable items
		items.push_back(std::make_shared<CItem>(this, goods[i], 54+i*104, 234));

	cancel = std::make_shared<CButton>(Point(200, 313), "IOKAY.DEF", CGI->generaltexth->zelp[632], [&](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUniversityWindow::makeDeal(int skill)
{
	LOCPLINT->cb->trade(market, EMarketMode::RESOURCE_SKILL, 6, skill, 1, hero);
}


CUnivConfirmWindow::CUnivConfirmWindow(CUniversityWindow * owner_, int SKILL, bool available)
	: CStatusbarWindow(PLAYER_COLORED, "UNIVERS2.PCX"),
	owner(owner_)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	std::string text = CGI->generaltexth->allTexts[608];
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->skillh->getByIndex(SKILL)->getNameTranslated());
	boost::replace_first(text, "%d", "2000");

	clerkSpeech = std::make_shared<CTextBox>(text, Rect(24, 129, 413, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	name = std::make_shared<CLabel>(230, 37,  FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->skillh->getByIndex(SKILL)->getNameTranslated());
	icon = std::make_shared<CAnimImage>("SECSKILL", SKILL*3+3, 0, 211, 51);
	level = std::make_shared<CLabel>(230, 107, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->levels[1]);

	costIcon = std::make_shared<CAnimImage>("RESOURCE", GameResID(EGameResID::GOLD), 0, 210, 210);
	cost = std::make_shared<CLabel>(230, 267, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "2000");

	std::string hoverText = CGI->generaltexth->allTexts[609];
	boost::replace_first(hoverText, "%s", CGI->generaltexth->levels[0]+ " " + CGI->skillh->getByIndex(SKILL)->getNameTranslated());

	text = CGI->generaltexth->zelp[633].second;
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->skillh->getByIndex(SKILL)->getNameTranslated());
	boost::replace_first(text, "%d", "2000");

	confirm = std::make_shared<CButton>(Point(148, 299), "IBY6432.DEF", CButton::tooltip(hoverText, text), [=](){makeDeal(SKILL);}, EShortcut::GLOBAL_ACCEPT);
	confirm->block(!available);

	cancel = std::make_shared<CButton>(Point(252,299), "ICANCEL.DEF", CGI->generaltexth->zelp[631], [&](){ close(); }, EShortcut::GLOBAL_CANCEL);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUnivConfirmWindow::makeDeal(int skill)
{
	owner->makeDeal(skill);
	close();
}

CGarrisonWindow::CGarrisonWindow(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits)
	: CWindowObject(PLAYER_COLORED, "GARRISON")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	garr = std::make_shared<CGarrisonInt>(92, 127, 4, Point(0,96), up, down, removableUnits);
	{
		auto split = std::make_shared<CButton>(Point(88, 314), "IDV6432.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3], ""), [&](){ garr->splitClick(); } );
		garr->addSplitBtn(split);
	}
	quit = std::make_shared<CButton>(Point(399, 314), "IOK6432.DEF", CButton::tooltip(CGI->generaltexth->tcommands[8], ""), [&](){ close(); }, EShortcut::GLOBAL_ACCEPT);

	std::string titleText;
	if(down->tempOwner == up->tempOwner)
	{
		titleText = CGI->generaltexth->allTexts[709];
	}
	else
	{
		//assume that this is joining monsters dialog
		if(up->Slots().size() > 0)
		{
			titleText = CGI->generaltexth->allTexts[35];
			boost::algorithm::replace_first(titleText, "%s", up->Slots().begin()->second->type->getNamePluralTranslated());
		}
		else
		{
			logGlobal->error("Invalid armed instance for garrison window.");
		}
	}
	title = std::make_shared<CLabel>(275, 30, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, titleText);

	banner = std::make_shared<CAnimImage>("CREST58", up->getOwner().getNum(), 0, 28, 124);
	portrait = std::make_shared<CAnimImage>("PortraitsLarge", down->portrait, 0, 29, 222);
}

void CGarrisonWindow::updateGarrisons()
{
	garr->recreateSlots();
}

CHillFortWindow::CHillFortWindow(const CGHeroInstance * visitor, const CGObjectInstance * object)
	: CStatusbarWindow(PLAYER_COLORED, "APHLFTBK"),
	fort(object),
	hero(visitor)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	title = std::make_shared<CLabel>(325, 32, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, fort->getObjectName());

	heroPic = std::make_shared<CHeroArea>(30, 60, hero);

	for(int i=0; i < resCount; i++)
	{
		totalIcons[i] = std::make_shared<CAnimImage>("SMALRES", i, 0, 104 + 76 * i, 237);
		totalLabels[i] = std::make_shared<CLabel>(166 + 76 * i, 253, FONT_SMALL, ETextAlignment::BOTTOMRIGHT);
	}

	for(int i = 0; i < slotsCount; i++)
	{
		upgrade[i] = std::make_shared<CButton>(Point(107 + i * 76, 171), "", CButton::tooltip(getTextForSlot(SlotID(i))), [=](){ makeDeal(SlotID(i)); }, vstd::next(EShortcut::SELECT_INDEX_1, i));
		for(auto image : { "APHLF1R.DEF", "APHLF1Y.DEF", "APHLF1G.DEF" })
			upgrade[i]->addImage(image);

		for(int j : {0,1})
		{
			slotIcons[i][j] = std::make_shared<CAnimImage>("SMALRES", 0, 0, 104 + 76 * i, 128 + 20 * j);
			slotLabels[i][j] = std::make_shared<CLabel>(168 + 76 * i, 144 + 20 * j, FONT_SMALL, ETextAlignment::BOTTOMRIGHT);
		}
	}

	upgradeAll = std::make_shared<CButton>(Point(30, 231), "", CButton::tooltip(CGI->generaltexth->allTexts[432]), [&](){ makeDeal(SlotID(slotsCount));}, EShortcut::RECRUITMENT_UPGRADE_ALL);
	for(auto image : { "APHLF4R.DEF", "APHLF4Y.DEF", "APHLF4G.DEF" })
		upgradeAll->addImage(image);

	quit = std::make_shared<CButton>(Point(294, 275), "IOKAY.DEF", CButton::tooltip(), std::bind(&CHillFortWindow::close, this), EShortcut::GLOBAL_ACCEPT);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	garr = std::make_shared<CGarrisonInt>(108, 60, 18, Point(), hero, nullptr);
	updateGarrisons();
}

void CHillFortWindow::updateGarrisons()
{
	std::array<TResources, slotsCount> costs;// costs [slot ID] [resource ID] = resource count for upgrade

	TResources totalSum; // totalSum[resource ID] = value

	for(int i=0; i<slotsCount; i++)
	{
		std::fill(costs[i].begin(), costs[i].end(), 0);
		int newState = getState(SlotID(i));
		if(newState != -1)
		{
			UpgradeInfo info;
			LOCPLINT->cb->fillUpgradeInfo(hero, SlotID(i), info);
			if(info.newID.size())//we have upgrades here - update costs
			{
				costs[i] = info.cost[0] * hero->getStackCount(SlotID(i));
				totalSum += costs[i];
			}
		}

		currState[i] = newState;
		upgrade[i]->setIndex(currState[i] == -1 ? 0 : currState[i]);
		upgrade[i]->block(currState[i] == -1);
		upgrade[i]->addHoverText(CButton::NORMAL, getTextForSlot(SlotID(i)));
	}

	//"Upgrade all" slot
	int newState = 2;
	{
		TResources myRes = LOCPLINT->cb->getResourceAmount();

		bool allUpgraded = true;//All creatures are upgraded?
		for(int i=0; i<slotsCount; i++)
			allUpgraded &= currState[i] == 1 || currState[i] == -1;

		if(allUpgraded)
			newState = 1;

		if(!totalSum.canBeAfforded(myRes))
			newState = 0;
	}

	currState[slotsCount] = newState;
	upgradeAll->setIndex(newState);

	garr->recreateSlots();

	for(int i = 0; i < slotsCount; i++)
	{
		//hide all first
		for(int j : {0,1})
		{
			slotIcons[i][j]->visible = false;
			slotLabels[i][j]->setText("");
		}
		//if can upgrade or can not afford, draw cost
		if(currState[i] == 0 || currState[i] == 2)
		{
			if(costs[i].nonZero())
			{
				//reverse iterator is used to display gold as first element
				int j = 0;
				for(int res = (int)costs[i].size()-1; (res >= 0) && (j < 2); res--)
				{
					int val = costs[i][res];
					if(!val)
						continue;

					slotIcons[i][j]->visible = true;
					slotIcons[i][j]->setFrame(res);

					slotLabels[i][j]->setText(std::to_string(val));
					j++;
				}
			}
			else//free upgrade - print gold image and "Free" text
			{
				slotIcons[i][0]->visible = true;
				slotIcons[i][0]->setFrame(GameResID(EGameResID::GOLD));
				slotLabels[i][0]->setText(CGI->generaltexth->allTexts[344]);
			}
		}
	}

	for(int i = 0; i < resCount; i++)
	{
		if(totalSum[i] == 0)
		{
			totalIcons[i]->visible = false;
			totalLabels[i]->setText("");
		}
		else
		{
			totalIcons[i]->visible = true;
			totalLabels[i]->setText(std::to_string(totalSum[i]));
		}
	}
}

void CHillFortWindow::makeDeal(SlotID slot)
{
	assert(slot.getNum()>=0);
	int offset = (slot.getNum() == slotsCount)?2:0;
	switch(currState[slot.getNum()])
	{
		case 0:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[314 + offset], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			break;
		case 1:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[313 + offset], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			break;
		case 2:
			for(int i=0; i<slotsCount; i++)
			{
				if(slot.getNum() ==i || ( slot.getNum() == slotsCount && currState[i] == 2 ))//this is activated slot or "upgrade all"
				{
					UpgradeInfo info;
					LOCPLINT->cb->fillUpgradeInfo(hero, SlotID(i), info);
					LOCPLINT->cb->upgradeCreature(hero, SlotID(i), info.newID[0]);
				}
			}
			break;
	}
}

std::string CHillFortWindow::getTextForSlot(SlotID slot)
{
	if(!hero->getCreature(slot))//we don`t have creature here
		return "";

	std::string str = CGI->generaltexth->allTexts[318];
	int amount = hero->getStackCount(slot);
	if(amount == 1)
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->getNameSingularTranslated());
	else
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->getNamePluralTranslated());

	return str;
}

int CHillFortWindow::getState(SlotID slot)
{
	TResources myRes = LOCPLINT->cb->getResourceAmount();

	if(hero->slotEmpty(slot))//no creature here
		return -1;

	UpgradeInfo info;
	LOCPLINT->cb->fillUpgradeInfo(hero, slot, info);
	if(!info.newID.size())//already upgraded
		return 1;

	if(!(info.cost[0] * hero->getStackCount(slot)).canBeAfforded(myRes))
		return 0;

	return 2;//can upgrade
}

CThievesGuildWindow::CThievesGuildWindow(const CGObjectInstance * _owner):
	CStatusbarWindow(PLAYER_COLORED | BORDERED, "TpRank"),
	owner(_owner)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	SThievesGuildInfo tgi; //info to be displayed
	LOCPLINT->cb->getThievesGuildInfo(tgi, owner);

	exitb = std::make_shared<CButton>(Point(748, 556), "TPMAGE1", CButton::tooltip(CGI->generaltexth->allTexts[600]), [&](){ close();}, EShortcut::GLOBAL_RETURN);
	statusbar = CGStatusBar::create(3, 555, "TStatBar.bmp", 742);

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);

	//data for information table:
	// fields[row][column] = list of id's of players for this box
	static std::vector< std::vector< PlayerColor > > SThievesGuildInfo::* fields[] =
		{ &SThievesGuildInfo::numOfTowns, &SThievesGuildInfo::numOfHeroes,       &SThievesGuildInfo::gold,
		  &SThievesGuildInfo::woodOre,    &SThievesGuildInfo::mercSulfCrystGems, &SThievesGuildInfo::obelisks,
		  &SThievesGuildInfo::artifacts,  &SThievesGuildInfo::army,              &SThievesGuildInfo::income };

	for(int g=0; g<12; ++g)
	{
		int posY[] = {400, 460, 510};
		int y;
		if(g < 9)
			y = 52 + 32*g;
		else
			y = posY[g-9];

		std::string text = CGI->generaltexth->jktexts[24+g];
		boost::algorithm::trim_if(text,boost::algorithm::is_any_of("\""));
		rowHeaders.push_back(std::make_shared<CLabel>(135, y, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, text));
	}

	auto PRSTRIPS = std::make_shared<CAnimation>("PRSTRIPS");
	PRSTRIPS->preload();

	for(int g=1; g<tgi.playerColors.size(); ++g)
		columnBackgrounds.push_back(std::make_shared<CAnimImage>(PRSTRIPS, g-1, 0, 250 + 66*g, 7));

	for(int g=0; g<tgi.playerColors.size(); ++g)
		columnHeaders.push_back(std::make_shared<CLabel>(283 + 66*g, 24, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[16+g]));

	auto itgflags = std::make_shared<CAnimation>("itgflags");
	itgflags->preload();

	//printing flags
	for(int g = 0; g < std::size(fields); ++g) //by lines
	{
		for(int b=0; b<(tgi .* fields[g]).size(); ++b) //by places (1st, 2nd, ...)
		{
			std::vector<PlayerColor> &players = (tgi .* fields[g])[b]; //get players with this place in this line

			//position of box
			int xpos = 259 + 66 * b;
			int ypos = 41 +  32 * g;

			size_t rowLength[2]; //size of each row
			rowLength[0] = std::min<size_t>(players.size(), 4);
			rowLength[1] = players.size() - rowLength[0];

			for(size_t j=0; j < 2; j++)
			{
				// origin of this row | offset for 2nd row| shift right for short rows
				//if we have 2 rows, start either from mid or beginning (depending on count), otherwise center the flags
				int rowStartX = xpos + (j ? 6 + ((int)rowLength[j] < 3 ? 12 : 0) : 24 - 6 * (int)rowLength[j]);
				int rowStartY = ypos + (j ? 4 : 0);

				for(size_t i=0; i < rowLength[j]; i++)
					cells.push_back(std::make_shared<CAnimImage>(itgflags, players[i + j*4].getNum(), 0, rowStartX + (int)i*12, rowStartY));
			}
		}
	}

	static const std::string colorToBox[] = {"PRRED.BMP", "PRBLUE.BMP", "PRTAN.BMP", "PRGREEN.BMP", "PRORANGE.BMP", "PRPURPLE.BMP", "PRTEAL.BMP", "PRROSE.bmp"};

	//printing best hero
	int counter = 0;
	for(auto & iter : tgi.colorToBestHero)
	{
		banners.push_back(std::make_shared<CPicture>(colorToBox[iter.first.getNum()], 253 + 66 * counter, 334));
		if(iter.second.portrait >= 0)
		{
			bestHeroes.push_back(std::make_shared<CAnimImage>("PortraitsSmall", iter.second.portrait, 0, 260 + 66 * counter, 360));
			//TODO: r-click info:
			// - r-click on hero
			// - r-click on primary skill label
			if(iter.second.details)
			{
				primSkillHeaders.push_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[184], Rect(260 + 66*counter, 396, 52, 64),
					0, FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE));

				for(int i=0; i<iter.second.details->primskills.size(); ++i)
				{
					primSkillValues.push_back(std::make_shared<CLabel>(310 + 66 * counter, 407 + 11*i, FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE,
							   std::to_string(iter.second.details->primskills[i])));
				}
			}
		}
		counter++;
	}

	//printing best creature
	counter = 0;
	for(auto & it : tgi.bestCreature)
	{
		if(it.second >= 0)
			bestCreatures.push_back(std::make_shared<CAnimImage>("TWCRPORT", it.second+2, 0, 255 + 66 * counter, 479));
		counter++;
	}

	//printing personality
	counter = 0;
	for(auto & it : tgi.personality)
	{
		std::string text;
		if(it.second == EAiTactic::NONE)
		{
			text = CGI->generaltexth->arraytxt[172];
		}
		else if(it.second != EAiTactic::RANDOM)
		{
			text = CGI->generaltexth->arraytxt[168 + it.second];
		}

		personalities.push_back(std::make_shared<CLabel>(283 + 66*counter, 459, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, text));

		counter++;
	}
}

CObjectListWindow::CItem::CItem(CObjectListWindow * _parent, size_t _id, std::string _text)
	: CIntObject(LCLICK | DOUBLECLICK),
	parent(_parent),
	index(_id)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	border = std::make_shared<CPicture>("TPGATES");
	pos = border->pos;

	type |= REDRAW_PARENT;

	text = std::make_shared<CLabel>(pos.w/2, pos.h/2, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, _text);
	select(index == parent->selected);
}

void CObjectListWindow::CItem::select(bool on)
{
	ui8 mask = UPDATE | SHOWALL;
	if(on)
		border->recActions |= mask;
	else
		border->recActions &= ~mask;
	redraw();//???
}

void CObjectListWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if( previousState && !down)
		parent->changeSelection(index);
}

void CObjectListWindow::CItem::clickDouble()
{
	parent->elementSelected();
}

CObjectListWindow::CObjectListWindow(const std::vector<int> & _items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection)
	: CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback),
	selected(initialSelection)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	items.reserve(_items.size());

	for(int id : _items)
	{
		items.push_back(std::make_pair(id, LOCPLINT->cb->getObjInstance(ObjectInstanceID(id))->getObjectName()));
	}

	init(titleWidget_, _title, _descr);
}

CObjectListWindow::CObjectListWindow(const std::vector<std::string> & _items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection)
	: CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback),
	selected(initialSelection)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	items.reserve(_items.size());

	for(size_t i=0; i<_items.size(); i++)
		items.push_back(std::make_pair(int(i), _items[i]));

	init(titleWidget_, _title, _descr);
}

void CObjectListWindow::init(std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr)
{
	titleWidget = titleWidget_;

	title = std::make_shared<CLabel>(152, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, _title);
	descr = std::make_shared<CLabel>(145, 133, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, _descr);
	exit = std::make_shared<CButton>( Point(228, 402), "ICANCEL.DEF", CButton::tooltip(), std::bind(&CObjectListWindow::exitPressed, this), EShortcut::GLOBAL_CANCEL);

	if(titleWidget)
	{
		addChild(titleWidget.get());
		titleWidget->recActions = 255-DISPOSE;
		titleWidget->pos.x = pos.w/2 + pos.x - titleWidget->pos.w/2;
		titleWidget->pos.y =75 + pos.y - titleWidget->pos.h/2;
	}
	list = std::make_shared<CListBox>(std::bind(&CObjectListWindow::genItem, this, _1),
		Point(14, 151), Point(0, 25), 9, items.size(), 0, 1, Rect(262, -32, 256, 256) );
	list->type |= REDRAW_PARENT;

	ok = std::make_shared<CButton>(Point(15, 402), "IOKAY.DEF", CButton::tooltip(), std::bind(&CObjectListWindow::elementSelected, this), EShortcut::GLOBAL_ACCEPT);
	ok->block(!list->size());
}

std::shared_ptr<CIntObject> CObjectListWindow::genItem(size_t index)
{
	if(index < items.size())
		return std::make_shared<CItem>(this, index, items[index].second);
	return std::shared_ptr<CIntObject>();
}

void CObjectListWindow::elementSelected()
{
	std::function<void(int)> toCall = onSelect;//save
	int where = items[selected].first;      //required variables
	close();//then destroy window
	toCall(where);//and send selected object
}

void CObjectListWindow::exitPressed()
{
	std::function<void()> toCall = onExit;//save
	close();//then destroy window
	if(toCall)
		toCall();
}

void CObjectListWindow::changeSelection(size_t which)
{
	ok->block(false);
	if(selected == which)
		return;

	for(std::shared_ptr<CIntObject> element : list->getItems())
	{
		CItem * item = dynamic_cast<CItem*>(element.get());
		if(item)
		{
			if(item->index == selected)
				item->select(false);

			if(item->index == which)
				item->select(true);
		}
	}
	selected = which;
}

void CObjectListWindow::keyPressed (EShortcut key)
{
	int sel = static_cast<int>(selected);

	switch(key)
	{
	break; case EShortcut::MOVE_UP:
		sel -=1;

	break; case EShortcut::MOVE_DOWN:
		sel +=1;

	break; case EShortcut::MOVE_PAGE_UP:
		sel -=9;

	break; case EShortcut::MOVE_PAGE_DOWN:
		sel +=9;

	break; case EShortcut::MOVE_FIRST:
		sel = 0;

	break; case EShortcut::MOVE_LAST:
		sel = static_cast<int>(items.size());

	break; default:
		return;
	}

	vstd::abetween<int>(sel, 0, items.size()-1);
	list->scrollTo(sel);
	changeSelection(sel);
}

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

#include "../CServerHandler.h"
#include "../Client.h"
#include "../CPlayerInterface.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/CursorHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"

#include "../widgets/CComponent.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/CreatureCostBox.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/VideoWidget.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"

#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"
#include "../render/IImage.h"
#include "../render/IFont.h"

#include "../lib/GameLibrary.h"
#include "../lib/callback/CCallback.h"
#include "../lib/entities/building/CBuilding.h"
#include "../lib/entities/faction/CTownHandler.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjectConstructors/CommonConstructors.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGMarket.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/gameState/SThievesGuildInfo.h"
#include "../lib/gameState/TavernHeroesPool.h"
#include "../lib/gameState/UpgradeInfo.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/texts/TextOperations.h"
#include "../lib/IGameSettings.h"
#include "../lib/ConditionalWait.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/CSkillHandler.h"
#include "../lib/CSoundBase.h"

#include <boost/lexical_cast.hpp>

CRecruitmentWindow::CCreatureCard::CCreatureCard(CRecruitmentWindow * window, const CCreature * crea, int totalAmount)
	: CIntObject(LCLICK | SHOW_POPUP),
	parent(window),
	selected(false),
	creature(crea),
	amount(totalAmount)
{
	OBJECT_CONSTRUCTION;
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

void CRecruitmentWindow::CCreatureCard::clickPressed(const Point & cursorPosition)
{
	parent->select(this->shared_from_this());
}

void CRecruitmentWindow::CCreatureCard::showPopupWindow(const Point & cursorPosition)
{
	ENGINE->windows().createAndPushWindow<CStackWindow>(creature, true);
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
		si32 maxAmount = card->creature->maxAmount(GAME->interface()->cb->getResourceAmount());

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
		title->setText(boost::str(boost::format(LIBRARY->generaltexth->tcommands[21]) % card->creature->getNamePluralTranslated()));

		maxButton->block(maxAmount == 0);
		slider->block(maxAmount == 0);
	}
}

void CRecruitmentWindow::close()
{
	if (onClose)
		onClose();
	CStatusbarWindow::close();
}

void CRecruitmentWindow::buy()
{
	CreatureID crid =  selected->creature->getId();
	SlotID dstslot = dst->getSlotFor(crid);

	if(!dstslot.validSlot() && (selected->creature->warMachine == ArtifactID::NONE)) //no available slot
	{
		std::pair<SlotID, SlotID> toMerge;
		bool allowMerge = GAME->interface()->cb->getSettings().getBoolean(EGameSettings::DWELLINGS_ACCUMULATE_WHEN_OWNED);

		if (allowMerge && dst->mergeableStacks(toMerge))
		{
			GAME->interface()->cb->mergeStacks( dst, dst, toMerge.first, toMerge.second);
		}
		else
		{
			std::string txt;
			if(dwelling->ID != Obj::TOWN)
			{
				txt = LIBRARY->generaltexth->allTexts[425]; //The %s would join your hero, but there aren't enough provisions to support them.
				boost::algorithm::replace_first(txt, "%s", slider->getValue() > 1 ? LIBRARY->creh->objects[crid]->getNamePluralTranslated() : LIBRARY->creh->objects[crid]->getNameSingularTranslated());
			}
			else
			{
				txt = LIBRARY->generaltexth->allTexts[17]; //There is no room in the garrison for this army.
			}

			GAME->interface()->showInfoDialog(txt);
			return;
		}
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

CRecruitmentWindow::CRecruitmentWindow(const CGDwelling * Dwelling, int Level, const CArmedInstance * Dst, const std::function<void(CreatureID,int)> & Recruit, const std::function<void()> & onClose, int y_offset):
	CWindowObject(PLAYER_COLORED, ImagePath::builtin("TPRCRT")),
	onRecruit(Recruit),
	onClose(onClose),
	level(Level),
	dst(Dst),
	selected(nullptr),
	dwelling(Dwelling)
{
	moveBy(Point(0, y_offset));

	OBJECT_CONSTRUCTION;

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	slider = std::make_shared<CSlider>(Point(176, 279), 135, std::bind(&CRecruitmentWindow::sliderMoved, this, _1), 0, 0, 0, Orientation::HORIZONTAL);

	maxButton = std::make_shared<CButton>(Point(134, 313), AnimationPath::builtin("IRCBTNS.DEF"), LIBRARY->generaltexth->zelp[553], std::bind(&CSlider::scrollToMax, slider), EShortcut::RECRUITMENT_MAX);
	buyButton = std::make_shared<CButton>(Point(212, 313), AnimationPath::builtin("IBY6432.DEF"), LIBRARY->generaltexth->zelp[554], std::bind(&CRecruitmentWindow::buy, this), EShortcut::GLOBAL_ACCEPT);
	cancelButton = std::make_shared<CButton>(Point(290, 313), AnimationPath::builtin("ICN6432.DEF"), LIBRARY->generaltexth->zelp[555], std::bind(&CRecruitmentWindow::close, this), EShortcut::GLOBAL_CANCEL);

	title = std::make_shared<CLabel>(243, 32, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW);
	availableValue = std::make_shared<CLabel>(205, 253, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	toRecruitValue = std::make_shared<CLabel>(279, 253, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	costPerTroopValue = std::make_shared<CreatureCostBox>(Rect(65, 222, 97, 74), LIBRARY->generaltexth->allTexts[346]);
	totalCostValue = std::make_shared<CreatureCostBox>(Rect(323, 222, 97, 74), LIBRARY->generaltexth->allTexts[466]);

	availableTitle = std::make_shared<CLabel>(205, 233, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[465]);
	toRecruitTitle = std::make_shared<CLabel>(279, 233, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[16]);

	availableCreaturesChanged();
}

void CRecruitmentWindow::availableCreaturesChanged()
{
	OBJECT_CONSTRUCTION;

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
			cards.push_back(std::make_shared<CCreatureCard>(this, creature.toCreature(), amount));
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

	buyButton->block(!to || !GAME->interface()->makingTurn);
	availableValue->setText(std::to_string(selected->amount - to));
	toRecruitValue->setText(std::to_string(to));

	totalCostValue->set(selected->creature->getFullRecruitCost() * to);
}

CSplitWindow::CSplitWindow(const CCreature * creature, std::function<void(int, int)> callback_, int leftMin_, int rightMin_, int leftAmount_, int rightAmount_)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("GPUCRDIV")),
	callback(callback_),
	leftAmount(leftAmount_),
	rightAmount(rightAmount_),
	leftMin(leftMin_),
	rightMin(rightMin_)
{
	OBJECT_CONSTRUCTION;

	int total = leftAmount + rightAmount;
	int leftMax = total - rightMin;
	int rightMax = total - leftMin;

	ok = std::make_shared<CButton>(Point(20, 263), AnimationPath::builtin("IOK6432"), CButton::tooltip(), std::bind(&CSplitWindow::apply, this), EShortcut::GLOBAL_ACCEPT);
	cancel = std::make_shared<CButton>(Point(214, 263), AnimationPath::builtin("ICN6432"), CButton::tooltip(), std::bind(&CSplitWindow::close, this), EShortcut::GLOBAL_CANCEL);

	int sliderPosition = total - leftMin - rightMin;

	leftInput = std::make_shared<CTextInput>(Rect(20, 218, 100, 36), FONT_BIG, ETextAlignment::CENTER, true);
	rightInput = std::make_shared<CTextInput>(Rect(176, 218, 100, 36), FONT_BIG, ETextAlignment::CENTER, true);

	leftInput->setCallback(std::bind(&CSplitWindow::setAmountText, this, _1, true));
	rightInput->setCallback(std::bind(&CSplitWindow::setAmountText, this, _1, false));

	//add filters to allow only number input
	leftInput->setFilterNumber(leftMin, leftMax);
	rightInput->setFilterNumber(rightMin, rightMax);

	leftInput->setText(std::to_string(leftAmount));
	rightInput->setText(std::to_string(rightAmount));

	animLeft = std::make_shared<CCreaturePic>(20, 54, creature, true, false);
	animRight = std::make_shared<CCreaturePic>(177, 54,creature, true, false);

	slider = std::make_shared<CSlider>(Point(21, 194), 257, std::bind(&CSplitWindow::sliderMoved, this, _1), 0, sliderPosition, rightAmount - rightMin, Orientation::HORIZONTAL);

	std::string titleStr = LIBRARY->generaltexth->allTexts[256];
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

CLevelWindow::CLevelWindow(const CGHeroInstance * hero, PrimarySkill pskill, std::vector<SecondarySkill> & skills, std::function<void(ui32)> callback)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("LVLUPBKG")),
	cb(callback)
{
	OBJECT_CONSTRUCTION;

	GAME->interface()->showingDialog->setBusy();

	if(!skills.empty())
	{
		std::vector<std::shared_ptr<CSelectableComponent>> comps;
		for(auto & skill : skills)
		{
			auto comp = std::make_shared<CSelectableComponent>(ComponentType::SEC_SKILL, skill, hero->getSecSkillLevel(SecondarySkill(skill))+1, CComponent::medium);
			comp->onChoose = std::bind(&CLevelWindow::close, this);
			comps.push_back(comp);
		}

		box = std::make_shared<CComponentBox>(comps, Rect(75, 300, pos.w - 150, 100));
	}

	portrait = std::make_shared<CHeroArea>(170, 66, hero);
	portrait->addClickCallback(nullptr);
	portrait->addRClickCallback([hero](){ ENGINE->windows().createAndPushWindow<CRClickPopupInt>(std::make_shared<CHeroWindow>(hero)); });
	ok = std::make_shared<CButton>(Point(297, 413), AnimationPath::builtin("IOKAY"), CButton::tooltip(), std::bind(&CLevelWindow::close, this), EShortcut::GLOBAL_ACCEPT);

	//%s has gained a level.
	mainTitle = std::make_shared<CLabel>(192, 33, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(LIBRARY->generaltexth->allTexts[444]) % hero->getNameTranslated()));

	//%s is now a level %d %s.
	std::string levelTitleText = LIBRARY->generaltexth->translate("core.genrltxt.445");
	boost::replace_first(levelTitleText, "%s", hero->getNameTranslated());
	boost::replace_first(levelTitleText, "%d", std::to_string(hero->level));
	boost::replace_first(levelTitleText, "%s", hero->getClassNameTranslated());

	levelTitle = std::make_shared<CLabel>(192, 162, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, levelTitleText);

	skillIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), pskill.getNum(), 0, 174, 190);

	skillValue = std::make_shared<CLabel>(192, 253, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->primarySkillNames[pskill.getNum()] + " +1");
}

void CLevelWindow::close()
{
	//FIXME: call callback if there was nothing to select?
	if (box && box->selectedIndex() != -1)
		cb(box->selectedIndex());

	GAME->interface()->showingDialog->setFree();

	CWindowObject::close();
}

CTavernWindow::CTavernWindow(const CGObjectInstance * TavernObj, const std::function<void()> & onWindowClosed)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("TPTAVERN")),
	onWindowClosed(onWindowClosed),
	tavernObj(TavernObj),
	heroToInvite(nullptr)
{
	OBJECT_CONSTRUCTION;

	std::vector<const CGHeroInstance*> h = GAME->interface()->cb->getAvailableHeroes(TavernObj);
	if(h.size() < 2)
		h.resize(2, nullptr);

	selected = 0;
	if(!h[0])
		selected = 1;
	if(!h[0] && !h[1])
		selected = -1;

	oldSelected = -1;

	h1 = std::make_shared<HeroPortrait>(selected, 0, 72, 299, h[0], [this]() { if(!recruit->isBlocked()) recruitb(); });
	h2 = std::make_shared<HeroPortrait>(selected, 1, 162, 299, h[1], [this]() { if(!recruit->isBlocked()) recruitb(); });

	title = std::make_shared<CLabel>(197, 32, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[37]);
	cost = std::make_shared<CLabel>(320, 328, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(GameConstants::HERO_GOLD_COST));
	heroDescription = std::make_shared<CTextBox>("", Rect(30, 373, 233, 35), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	heroesForHire = std::make_shared<CLabel>(145, 283, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[38]);

	rumor = std::make_shared<CTextBox>(GAME->interface()->cb->getTavernRumor(tavernObj), Rect(32, 188, 330, 66), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
	cancel = std::make_shared<CButton>(Point(310, 428), AnimationPath::builtin("ICANCEL.DEF"), CButton::tooltip(LIBRARY->generaltexth->tavernInfo[7]), std::bind(&CTavernWindow::close, this), EShortcut::GLOBAL_CANCEL);
	recruit = std::make_shared<CButton>(Point(272, 355), AnimationPath::builtin("TPTAV01.DEF"), CButton::tooltip(), std::bind(&CTavernWindow::recruitb, this), EShortcut::GLOBAL_ACCEPT);
	thiefGuild = std::make_shared<CButton>(Point(22, 428), AnimationPath::builtin("TPTAV02.DEF"), CButton::tooltip(LIBRARY->generaltexth->tavernInfo[5]), std::bind(&CTavernWindow::thievesguildb, this), EShortcut::ADVENTURE_THIEVES_GUILD);

	if(!GAME->interface()->makingTurn)
	{
		recruit->block(true);
	}
	else if(GAME->interface()->cb->getResourceAmount(EGameResID::GOLD) < GameConstants::HERO_GOLD_COST) //not enough gold
	{
		recruit->addHoverText(EButtonState::NORMAL, LIBRARY->generaltexth->tavernInfo[0]); //Cannot afford a Hero
		recruit->block(true);
	}
	else if(GAME->interface()->cb->howManyHeroes(true) >= GAME->interface()->cb->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP))
	{
		MetaString message;
		message.appendTextID("core.tvrninfo.1");
		message.replaceNumber(GAME->interface()->cb->howManyHeroes(true));

		//Cannot recruit. You already have %d Heroes.
		recruit->addHoverText(EButtonState::NORMAL, message.toString());
		recruit->block(true);
	}
	else if(GAME->interface()->cb->howManyHeroes(false) >= GAME->interface()->cb->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP))
	{
		MetaString message;
		message.appendTextID("core.tvrninfo.1");
		message.replaceNumber(GAME->interface()->cb->howManyHeroes(false));

		recruit->addHoverText(EButtonState::NORMAL, message.toString());
		recruit->block(true);
	}
	else if(dynamic_cast<const CGTownInstance *>(TavernObj) && dynamic_cast<const CGTownInstance *>(TavernObj)->getVisitingHero())
	{
		recruit->addHoverText(EButtonState::NORMAL, LIBRARY->generaltexth->tavernInfo[2]); //Cannot recruit. You already have a Hero in this town.
		recruit->block(true);
	}
	else
	{
		if(selected == -1)
			recruit->block(true);
	}
	if(GAME->interface()->castleInt)
		videoPlayer = std::make_shared<VideoWidget>(Point(70, 56), GAME->interface()->castleInt->town->getTown()->clientInfo.tavernVideo, false);
	else if(const auto * townObj = dynamic_cast<const CGTownInstance *>(TavernObj))
		videoPlayer = std::make_shared<VideoWidget>(Point(70, 56), townObj->getTown()->clientInfo.tavernVideo, false);
	else
		videoPlayer = std::make_shared<VideoWidget>(Point(70, 56), VideoPath::builtin("TAVERN.BIK"), false);

	addInvite();
}

void CTavernWindow::addInvite()
{
	OBJECT_CONSTRUCTION;

	if(!GAME->interface()->cb->getSettings().getBoolean(EGameSettings::HEROES_TAVERN_INVITE))
		return;

	const auto & heroesPool = GAME->server().client->gameState().heroesPool;
	for(auto & elem : heroesPool->unusedHeroesFromPool())
	{
		bool heroAvailable = heroesPool->isHeroAvailableFor(elem.first, tavernObj->getOwner());
		if(heroAvailable)
			inviteableHeroes[elem.first] = elem.second;
	}

	if(!inviteableHeroes.empty())
	{
		int imageIndex = heroToInvite ? heroToInvite->getIconIndex() : 156; // 156 => special id for random
		if(!heroToInvite)
			heroToInvite = (*RandomGeneratorUtil::nextItem(inviteableHeroes, CRandomGenerator::getDefault())).second;

		inviteHero = std::make_shared<CLabel>(170, 444, EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.tavernWindow.inviteHero"));
		inviteHeroImage = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), imageIndex, 0, 245, 428);
		inviteHeroImageArea = std::make_shared<LRClickableArea>(Rect(245, 428, 48, 32), [this](){ ENGINE->windows().createAndPushWindow<HeroSelector>(inviteableHeroes, [this](CGHeroInstance* h){ heroToInvite = h; addInvite(); }); }, [this](){ ENGINE->windows().createAndPushWindow<CRClickPopupInt>(std::make_shared<CHeroWindow>(heroToInvite)); });
	}
}

void CTavernWindow::recruitb()
{
	const CGHeroInstance *toBuy = (selected ? h2 : h1)->h;
	const CGObjectInstance *obj = tavernObj;

	GAME->interface()->cb->recruitHero(obj, toBuy, heroToInvite ? heroToInvite->getHeroTypeID() : HeroTypeID::NONE);
	close();
}

void CTavernWindow::thievesguildb()
{
	ENGINE->windows().createAndPushWindow<CThievesGuildWindow>(tavernObj);
}

void CTavernWindow::close()
{
	if (onWindowClosed)
		onWindowClosed();

	CStatusbarWindow::close();
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
				recruit->addHoverText(EButtonState::NORMAL, boost::str(boost::format(LIBRARY->generaltexth->tavernInfo[3]) % sel->h->getNameTranslated() % sel->h->getClassNameTranslated()));

		}

		to.drawBorder(Rect::createAround(sel->pos, 2), Colors::BRIGHT_YELLOW, 2);
	}
}

void CTavernWindow::HeroPortrait::clickPressed(const Point & cursorPosition)
{
	if(h)
		*_sel = _id;
}

void CTavernWindow::HeroPortrait::clickDouble(const Point & cursorPosition)
{
	clickPressed(cursorPosition);
	
	if(onChoose)
		onChoose();
}

void CTavernWindow::HeroPortrait::showPopupWindow(const Point & cursorPosition)
{
	// h3 behavior - right-click also selects hero
	clickPressed(cursorPosition);

	if(h)
		ENGINE->windows().createAndPushWindow<CRClickPopupInt>(std::make_shared<CHeroWindow>(h));
}

CTavernWindow::HeroPortrait::HeroPortrait(int & sel, int id, int x, int y, const CGHeroInstance * H, std::function<void()> OnChoose)
	: CIntObject(LCLICK | DOUBLECLICK | SHOW_POPUP | HOVER),
	h(H), _sel(&sel), _id(id), onChoose(OnChoose)
{
	OBJECT_CONSTRUCTION;
	h = H;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;

	if(H)
	{
		hoverName = LIBRARY->generaltexth->tavernInfo[4];
		boost::algorithm::replace_first(hoverName,"%s",H->getNameTranslated());

		int artifs = (int)h->artifactsWorn.size() + (int)h->artifactsInBackpack.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifactsWorn, ArtifactPosition(i)))
				artifs--;

		description = LIBRARY->generaltexth->allTexts[215];
		boost::algorithm::replace_first(description, "%s", h->getNameTranslated());
		boost::algorithm::replace_first(description, "%d", std::to_string(h->level));
		boost::algorithm::replace_first(description, "%s", h->getClassNameTranslated());
		boost::algorithm::replace_first(description, "%d", std::to_string(artifs));

		portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("portraitsLarge"), h->getIconIndex());
	}
}

void CTavernWindow::HeroPortrait::hover(bool on)
{
	//Hoverable::hover(on);
	if(on)
		ENGINE->statusbar()->write(hoverName);
	else
		ENGINE->statusbar()->clear();
}

CTavernWindow::HeroSelector::HeroSelector(std::map<HeroTypeID, CGHeroInstance*> InviteableHeroes, std::function<void(CGHeroInstance*)> OnChoose)
	: CWindowObject(BORDERED), inviteableHeroes(InviteableHeroes), onChoose(OnChoose)
{
	OBJECT_CONSTRUCTION;

	pos = Rect(
		pos.x,
		pos.y,
		ELEM_PER_LINES * 48,
		std::min((int)(inviteableHeroes.size() / ELEM_PER_LINES + (inviteableHeroes.size() % ELEM_PER_LINES != 0)), MAX_LINES) * 32
	);
	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));

	if(inviteableHeroes.size() / ELEM_PER_LINES > MAX_LINES)
	{
		pos.w += 16;
		slider = std::make_shared<CSlider>(Point(pos.w - 16, 0), pos.h, std::bind(&CTavernWindow::HeroSelector::sliderMove, this, _1), MAX_LINES, std::ceil((double)inviteableHeroes.size() / ELEM_PER_LINES), 0, Orientation::VERTICAL, CSlider::BROWN);
		slider->setPanningStep(32);
		slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, pos.w, pos.h));
	}

	recreate();
	center();
}

void CTavernWindow::HeroSelector::sliderMove(int slidPos)
{
	if(!slider)
		return; // ignore spurious call when slider is being created
	recreate();
	redraw();
}

void CTavernWindow::HeroSelector::recreate()
{
	OBJECT_CONSTRUCTION;

	int sliderLine = slider ? slider->getValue() : 0;
	int x = 0;
	int y = -sliderLine;
	portraits.clear();
	portraitAreas.clear();
	for(auto & h : inviteableHeroes)
	{
		if(y >= 0 && y <= MAX_LINES - 1)
		{
			portraits.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), (*LIBRARY->heroh)[h.first]->imageIndex, 0, x * 48, y * 32));
			portraitAreas.push_back(std::make_shared<LRClickableArea>(Rect(x * 48, y * 32, 48, 32), [this, h](){ close(); onChoose(inviteableHeroes[h.first]); }, [this, h](){ ENGINE->windows().createAndPushWindow<CRClickPopupInt>(std::make_shared<CHeroWindow>(inviteableHeroes[h.first])); }));
		}

		if(x > 0 && x % 15 == 0)
		{
			x = 0;
			y++;
		}
		else
			x++;
	}
}

CShipyardWindow::CShipyardWindow(const TResources & cost, int state, BoatId boatType, const std::function<void()> & onBuy)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("TPSHIP"))
{
	OBJECT_CONSTRUCTION;

	bgWater = std::make_shared<CPicture>(ImagePath::builtin("TPSHIPBK"), 100, 69);

	auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::BOAT, boatType);

	auto boatConstructor = std::dynamic_pointer_cast<const BoatInstanceConstructor>(handler);

	assert(boatConstructor);

	if (boatConstructor)
	{
		AnimationPath boatFilename = boatConstructor->getBoatAnimationName();

		Point waterCenter = Point(bgWater->pos.x+bgWater->pos.w/2, bgWater->pos.y+bgWater->pos.h/2);
		bgShip = std::make_shared<CShowableAnim>(120, 96, boatFilename, CShowableAnim::MAP_OBJECT_MODE, 100, 7);
		bgShip->center(waterCenter);
		bgWater->needRefresh = true;
	}

	// Create resource icons and costs.
	std::string goldValue = std::to_string(cost[EGameResID::GOLD]);
	std::string woodValue = std::to_string(cost[EGameResID::WOOD]);

	goldCost = std::make_shared<CLabel>(118, 294, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, goldValue);
	woodCost = std::make_shared<CLabel>(212, 294, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, woodValue);

	goldPic = std::make_shared<CAnimImage>(AnimationPath::builtin("RESOURCE"), GameResID(EGameResID::GOLD).getNum(), 0, 100, 244);
	woodPic = std::make_shared<CAnimImage>(AnimationPath::builtin("RESOURCE"), GameResID(EGameResID::WOOD).getNum(), 0, 196, 244);

	quit = std::make_shared<CButton>(Point(224, 312), AnimationPath::builtin("ICANCEL"), CButton::tooltip(LIBRARY->generaltexth->allTexts[599]), std::bind(&CShipyardWindow::close, this), EShortcut::GLOBAL_CANCEL);
	build = std::make_shared<CButton>(Point(42, 312), AnimationPath::builtin("IBUY30"), CButton::tooltip(LIBRARY->generaltexth->allTexts[598]), std::bind(&CShipyardWindow::close, this), EShortcut::GLOBAL_ACCEPT);
	build->addCallback(onBuy);

	for(GameResID i = EGameResID::WOOD; i <= EGameResID::GOLD; ++i)
	{
		if(cost[i] > GAME->interface()->cb->getResourceAmount(i))
		{
			build->block(true);
			break;
		}
	}

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	title = std::make_shared<CLabel>(164, 27,  FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[13]);
	costLabel = std::make_shared<CLabel>(164, 220, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->jktexts[14]);
}

void CTransformerWindow::CItem::move()
{
	if(left)
		moveBy(Point(289, 0));
	else
		moveBy(Point(-289, 0));
	left = !left;
}

void CTransformerWindow::CItem::clickPressed(const Point & cursorPosition)
{
	move();
	parent->redraw();
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
	OBJECT_CONSTRUCTION;
	left = true;
	pos.w = 58;
	pos.h = 64;

	pos.x += 45  + (id%3)*83 + id/6*83;
	pos.y += 109 + (id/3)*98;
	icon = std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), parent->army->getCreature(SlotID(id))->getId() + 2);
	count = std::make_shared<CLabel>(28, 76,FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(size));
}

void CTransformerWindow::makeDeal()
{
	for(auto & elem : items)
	{
		if(!elem->left)
			GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::CREATURE_UNDEAD, SlotID(elem->id), {}, {}, hero);
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

bool CTransformerWindow::holdsGarrison(const CArmedInstance * army)
{
	return army == hero;
}

CTransformerWindow::CTransformerWindow(const IMarket * _market, const CGHeroInstance * _hero, const std::function<void()> & onWindowClosed)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("SKTRNBK")),
	hero(_hero),
	onWindowClosed(onWindowClosed),
	market(_market)
{
	OBJECT_CONSTRUCTION;
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

	all = std::make_shared<CButton>(Point(146, 416), AnimationPath::builtin("ALTARMY.DEF"), LIBRARY->generaltexth->zelp[590], [&](){ addAll(); }, EShortcut::RECRUITMENT_UPGRADE_ALL);
	convert = std::make_shared<CButton>(Point(269, 416), AnimationPath::builtin("ALTSACR.DEF"), LIBRARY->generaltexth->zelp[591], [&](){ makeDeal(); }, EShortcut::GLOBAL_ACCEPT);
	cancel = std::make_shared<CButton>(Point(392, 416), AnimationPath::builtin("ICANCEL.DEF"), LIBRARY->generaltexth->zelp[592], [&](){ close(); },EShortcut::GLOBAL_CANCEL);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	titleLeft = std::make_shared<CLabel>(153, 29,FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[485]);//holding area
	titleRight = std::make_shared<CLabel>(153+295, 29, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[486]);//transformer
	helpLeft = std::make_shared<CTextBox>(LIBRARY->generaltexth->allTexts[487], Rect(26,  56, 255, 40), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW);//move creatures to create skeletons
	helpRight = std::make_shared<CTextBox>(LIBRARY->generaltexth->allTexts[488], Rect(320, 56, 255, 40), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW);//creatures here will become skeletons
}

void CTransformerWindow::close()
{
	if (onWindowClosed)
		onWindowClosed();

	CStatusbarWindow::close();
}

CUniversityWindow::CItem::CItem(CUniversityWindow * _parent, int _ID, int X, int Y)
	: CIntObject(LCLICK | SHOW_POPUP | HOVER),
	ID(_ID),
	parent(_parent)
{
	OBJECT_CONSTRUCTION;
	pos.x += X;
	pos.y += Y;

	skill = std::make_shared<CSecSkillPlace>(Point(), CSecSkillPlace::ImageSize::MEDIUM, _ID, 1);
	skill->setClickPressedCallback([this](const CComponentHolder&, const Point& cursorPosition)
		{
			bool skillKnown = parent->hero->getSecSkillLevel(ID);
			bool canLearn = parent->hero->canLearnSkill(ID);

			if(!skillKnown && canLearn)
				ENGINE->windows().createAndPushWindow<CUnivConfirmWindow>(parent, ID, GAME->interface()->cb->getResourceAmount(EGameResID::GOLD) >= 2000);
		});
	update();
}

void CUniversityWindow::CItem::update()
{
	bool skillKnown = parent->hero->getSecSkillLevel(ID);
	bool canLearn =	parent->hero->canLearnSkill(ID);

	ImagePath image;

	if (skillKnown)
		image = ImagePath::builtin("UNIVGOLD");
	else if (canLearn)
		image = ImagePath::builtin("UNIVGREN");
	else
		image = ImagePath::builtin("UNIVRED");

	OBJECT_CONSTRUCTION;
	topBar = std::make_shared<CPicture>(image, Point(-28, -22));
	bottomBar = std::make_shared<CPicture>(image, Point(-28, 48));

	// needs to be on top of background bars
	name = std::make_shared<CLabel>(22, -13, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, ID.toEntity(LIBRARY)->getNameTranslated());
	level = std::make_shared<CLabel>(22, 57, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->levels[0]);
}

CUniversityWindow::CUniversityWindow(const CGHeroInstance * _hero, BuildingID building, const IMarket * _market, const std::function<void()> & onWindowClosed)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("UNIVERS1")),
	hero(_hero),
	onWindowClosed(onWindowClosed),
	market(_market)
{
	OBJECT_CONSTRUCTION;


	std::string titleStr = LIBRARY->generaltexth->allTexts[602];
	std::string speechStr = LIBRARY->generaltexth->allTexts[603];

	if(auto town = dynamic_cast<const CGTownInstance *>(_market))
	{
		auto faction = town->getTown()->faction->getId();
		titlePic = std::make_shared<CAnimImage>(faction.toFaction()->town->clientInfo.buildingsIcons, building.getNum());
	}
	else if(auto uni = dynamic_cast<const CGUniversity *>(_market); uni->appearance)
	{
		titlePic = std::make_shared<CAnimImage>(uni->appearance->animationFile, 0, 0, 0, 0, CShowableAnim::MAP_OBJECT_MODE);
		titleStr = uni->getObjectName();
		speechStr = uni->getSpeechTranslated();
	}
	else
	{
		titlePic = std::make_shared<CPicture>(ImagePath::builtin("UNIVBLDG"));
	}

	titlePic->center(Point(232 + pos.x, 76 + pos.y));

	clerkSpeech = std::make_shared<CTextBox>(speechStr, Rect(24, 129, 413, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	title = std::make_shared<CLabel>(231, 26, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, titleStr);

	std::vector<TradeItemBuy> goods = market->availableItemsIds(EMarketMode::RESOURCE_SKILL);

	for(int i=0; i<goods.size(); i++)//prepare clickable items
		items.push_back(std::make_shared<CItem>(this, goods[i].as<SecondarySkill>().getNum(), 54+i*104, 234));

	cancel = std::make_shared<CButton>(Point(200, 313), AnimationPath::builtin("IOKAY.DEF"), LIBRARY->generaltexth->zelp[632], [&](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUniversityWindow::close()
{
	if (onWindowClosed)
		onWindowClosed();

	CStatusbarWindow::close();
}

void CUniversityWindow::updateSecondarySkills()
{
	for (auto const & item : items)
		item->update();
}

void CUniversityWindow::makeDeal(SecondarySkill skill)
{
	GAME->interface()->cb->trade(market->getObjInstanceID(), EMarketMode::RESOURCE_SKILL, GameResID(GameResID::GOLD), skill, 1, hero);
}

CUnivConfirmWindow::CUnivConfirmWindow(CUniversityWindow * owner_, SecondarySkill SKILL, bool available)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("UNIVERS2.PCX")),
	owner(owner_)
{
	OBJECT_CONSTRUCTION;

	std::string text = LIBRARY->generaltexth->allTexts[608];
	boost::replace_first(text, "%s", LIBRARY->generaltexth->levels[0]);
	boost::replace_first(text, "%s", SKILL.toEntity(LIBRARY)->getNameTranslated());

	boost::replace_first(text, "%d", "2000");

	clerkSpeech = std::make_shared<CTextBox>(text, Rect(24, 129, 413, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	name = std::make_shared<CLabel>(230, 37,  FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, SKILL.toEntity(LIBRARY)->getNameTranslated());
	icon = std::make_shared<CAnimImage>(AnimationPath::builtin("SECSKILL"), SKILL.getNum()*3+3, 0, 211, 51);
	level = std::make_shared<CLabel>(230, 107, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->levels[1]);

	costIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("RESOURCE"), GameResID(EGameResID::GOLD), 0, 210, 210);
	cost = std::make_shared<CLabel>(230, 267, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "2000");

	std::string hoverText = LIBRARY->generaltexth->allTexts[609];
	boost::replace_first(hoverText, "%s", LIBRARY->generaltexth->levels[0]+ " " + SKILL.toEntity(LIBRARY)->getNameTranslated());

	text = LIBRARY->generaltexth->zelp[633].second;
	boost::replace_first(text, "%s", LIBRARY->generaltexth->levels[0]);
	boost::replace_first(text, "%s", SKILL.toEntity(LIBRARY)->getNameTranslated());
	boost::replace_first(text, "%d", "2000");

	confirm = std::make_shared<CButton>(Point(148, 299), AnimationPath::builtin("IBY6432.DEF"), CButton::tooltip(hoverText, text), [this, SKILL](){makeDeal(SKILL);}, EShortcut::GLOBAL_ACCEPT);
	confirm->block(!available);

	cancel = std::make_shared<CButton>(Point(252,299), AnimationPath::builtin("ICANCEL.DEF"), LIBRARY->generaltexth->zelp[631], [&](){ close(); }, EShortcut::GLOBAL_CANCEL);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUnivConfirmWindow::makeDeal(SecondarySkill skill)
{
	owner->makeDeal(skill);
	close();
}

CGarrisonWindow::CGarrisonWindow(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("GARRISON"))
{
	OBJECT_CONSTRUCTION;

	garr = std::make_shared<CGarrisonInt>(Point(92, 127), 4, Point(0,96), up, down, removableUnits);
	{
		auto split = std::make_shared<CButton>(Point(88, 314), AnimationPath::builtin("IDV6432.DEF"), CButton::tooltip(LIBRARY->generaltexth->tcommands[3], ""), [this](){ garr->splitClick(); }, EShortcut::HERO_ARMY_SPLIT );
		garr->addSplitBtn(split);
	}
	quit = std::make_shared<CButton>(Point(399, 314), AnimationPath::builtin("IOK6432.DEF"), CButton::tooltip(LIBRARY->generaltexth->tcommands[8], ""), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT);

	std::string titleText;
	if(down->tempOwner == up->tempOwner)
	{
		titleText = LIBRARY->generaltexth->allTexts[709];
	}
	else
	{
		//assume that this is joining monsters dialog
		if(up->Slots().size() > 0)
		{
			titleText = LIBRARY->generaltexth->allTexts[35];
			boost::algorithm::replace_first(titleText, "%s", up->Slots().begin()->second->getType()->getNamePluralTranslated());
		}
		else
		{
			logGlobal->error("Invalid armed instance for garrison window.");
		}
	}
	title = std::make_shared<CLabel>(275, 30, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, titleText);

	banner = std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), up->getOwner().getNum(), 0, 28, 124);
	portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), down->getIconIndex(), 0, 29, 222);
}

void CGarrisonWindow::updateGarrisons()
{
	garr->recreateSlots();
}

bool CGarrisonWindow::holdsGarrison(const CArmedInstance * army)
{
	return garr->upperArmy() == army || garr->lowerArmy() == army;
}

CHillFortWindow::CHillFortWindow(const CGHeroInstance * visitor, const CGObjectInstance * object)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("APHLFTBK")),
	fort(object),
	hero(visitor)
{
	OBJECT_CONSTRUCTION;

	title = std::make_shared<CLabel>(325, 32, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, fort->getObjectName());

	heroPic = std::make_shared<CHeroArea>(30, 60, hero);

	for(int i=0; i < resCount; i++)
	{
		totalIcons[i] = std::make_shared<CAnimImage>(AnimationPath::builtin("SMALRES"), i, 0, 104 + 76 * i, 237);
		totalLabels[i] = std::make_shared<CLabel>(166 + 76 * i, 253, FONT_SMALL, ETextAlignment::BOTTOMRIGHT);
	}

	for(int i = 0; i < slotsCount; i++)
	{
		upgrade[i] = std::make_shared<CButton>(Point(107 + i * 76, 171), AnimationPath::builtin("APHLF1R"), CButton::tooltip(getTextForSlot(SlotID(i))), [this, i](){ makeDeal(SlotID(i)); }, vstd::next(EShortcut::SELECT_INDEX_1, i));

		for(int j : {0,1})
		{
			slotIcons[i][j] = std::make_shared<CAnimImage>(AnimationPath::builtin("SMALRES"), 0, 0, 104 + 76 * i, 128 + 20 * j);
			slotLabels[i][j] = std::make_shared<CLabel>(168 + 76 * i, 144 + 20 * j, FONT_SMALL, ETextAlignment::BOTTOMRIGHT);
		}
	}

	upgradeAll = std::make_shared<CButton>(Point(30, 231), AnimationPath::builtin("APHLF4R"), CButton::tooltip(LIBRARY->generaltexth->allTexts[432]), [this](){ makeDeal(SlotID(slotsCount));}, EShortcut::RECRUITMENT_UPGRADE_ALL);

	quit = std::make_shared<CButton>(Point(294, 275), AnimationPath::builtin("IOKAY.DEF"), CButton::tooltip(), [this](){close();}, EShortcut::GLOBAL_ACCEPT);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	garr = std::make_shared<CGarrisonInt>(Point(108, 60), 18, Point(), hero, nullptr);

	statusbar->write(LIBRARY->generaltexth->translate(dynamic_cast<const HillFort *>(fort)->getDescriptionToolTip()));

	updateGarrisons();
}

bool CHillFortWindow::holdsGarrison(const CArmedInstance * army)
{
	return hero == army;
}

void CHillFortWindow::updateGarrisons()
{
	constexpr std::array slotImages = { "APHLF1R.DEF", "APHLF1Y.DEF", "APHLF1G.DEF" };
	constexpr std::array allImages =  { "APHLF4R.DEF", "APHLF4Y.DEF", "APHLF4G.DEF" };

	std::array<TResources, slotsCount> costs;// costs [slot ID] [resource ID] = resource count for upgrade

	TResources totalSum; // totalSum[resource ID] = value

	auto getImgIdx = [](CHillFortWindow::State st) -> std::size_t
	{
		switch (st)
		{
		case State::EMPTY:
			return 0;
		case State::UNAVAILABLE:
		case State::ALREADY_UPGRADED:
			return 1;
		default:
			return static_cast<std::size_t>(st);
		}
	};

	for(int i=0; i<slotsCount; i++)
	{
		std::fill(costs[i].begin(), costs[i].end(), 0);
		State newState = getState(SlotID(i));
		if(newState != State::EMPTY)
		{
			if(const CStackInstance * s = hero->getStackPtr(SlotID(i)))
			{
				UpgradeInfo info(s->getCreature()->getId());
				GAME->interface()->cb->fillUpgradeInfo(hero, SlotID(i), info);
				if(info.canUpgrade())	//we have upgrades here - update costs
				{
					costs[i] = info.getUpgradeCosts() * hero->getStackCount(SlotID(i));
					totalSum += costs[i];
				}
			}
		}

		currState[i] = newState;
		upgrade[i]->setImage(AnimationPath::builtin(slotImages[getImgIdx(currState[i])]));
		upgrade[i]->block(currState[i] == State::EMPTY);
		upgrade[i]->addHoverText(EButtonState::NORMAL, getTextForSlot(SlotID(i)));
	}

	//"Upgrade all" slot
	State newState = State::MAKE_UPGRADE;
	{
		TResources myRes = GAME->interface()->cb->getResourceAmount();

		bool allUpgraded = true;//All creatures are upgraded?
		for(int i=0; i<slotsCount; i++)
			allUpgraded &= currState[i] == State::ALREADY_UPGRADED || currState[i] == State::EMPTY || currState[i] == State::UNAVAILABLE;

		if (allUpgraded)
			newState = State::ALREADY_UPGRADED;

		if(!totalSum.canBeAfforded(myRes))
			newState = State::UNAFFORDABLE;
	}

	currState[slotsCount] = newState;
	upgradeAll->setImage(AnimationPath::builtin(allImages[static_cast<std::size_t>(newState)]));

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
		if(currState[i] == State::UNAFFORDABLE || currState[i] == State::MAKE_UPGRADE)
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
				slotIcons[i][0]->setFrame(GameResID(EGameResID::GOLD).getNum());
				slotLabels[i][0]->setText(LIBRARY->generaltexth->allTexts[344]);
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
	assert(slot.getNum() >= 0);
	int offset = (slot.getNum() == slotsCount) ? 2 : 0;
	switch(currState[slot.getNum()])
	{
		case State::ALREADY_UPGRADED:
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[313 + offset], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			break;
		case State::UNAFFORDABLE:
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[314 + offset], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			break;
		case State::UNAVAILABLE:
		{
			std::string message = LIBRARY->generaltexth->translate(dynamic_cast<const HillFort *>(fort)->getUnavailableUpgradeMessage());
			GAME->interface()->showInfoDialog(message, std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			break;
		}
		case State::MAKE_UPGRADE:
			for(int i = 0; i < slotsCount; i++)
			{
				if(slot.getNum() == i || ( slot.getNum() == slotsCount && currState[i] == State::MAKE_UPGRADE ))//this is activated slot or "upgrade all"
				{
					if(const CStackInstance * s = hero->getStackPtr(SlotID(i)))
					{
						UpgradeInfo info(s->getCreatureID());
						GAME->interface()->cb->fillUpgradeInfo(hero, SlotID(i), info);
						GAME->interface()->cb->upgradeCreature(hero, SlotID(i), info.getUpgrade());
					}
				}
			}
			break;
	}
}

std::string CHillFortWindow::getTextForSlot(SlotID slot)
{
	if(!hero->getCreature(slot))//we don`t have creature here
		return "";

	std::string str = LIBRARY->generaltexth->allTexts[318];
	int amount = hero->getStackCount(slot);
	if(amount == 1)
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->getNameSingularTranslated());
	else
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->getNamePluralTranslated());

	return str;
}

CHillFortWindow::State CHillFortWindow::getState(SlotID slot)
{
	TResources myRes = GAME->interface()->cb->getResourceAmount();

	if(hero->slotEmpty(slot))
		return State::EMPTY;

	UpgradeInfo info(hero->getStackPtr(slot)->getCreatureID());
	GAME->interface()->cb->fillUpgradeInfo(hero, slot, info);
	if(info.hasUpgrades() && !info.canUpgrade())
		return State::UNAVAILABLE;  // Hill Fort may limit level of upgradeable creatures, e.g. mini Hill Fort from HOTA

	if(!info.hasUpgrades())
		return State::ALREADY_UPGRADED;

	if(!(info.getUpgradeCosts() * hero->getStackCount(slot)).canBeAfforded(myRes))
		return State::UNAFFORDABLE;

	return State::MAKE_UPGRADE;
}

CThievesGuildWindow::CThievesGuildWindow(const CGObjectInstance * _owner):
	CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin("TpRank")),
	owner(_owner)
{
	OBJECT_CONSTRUCTION;

	SThievesGuildInfo tgi; //info to be displayed
	GAME->interface()->cb->getThievesGuildInfo(tgi, owner);

	exitb = std::make_shared<CButton>(Point(748, 556), AnimationPath::builtin("TPMAGE1"), CButton::tooltip(LIBRARY->generaltexth->allTexts[600]), [&](){ close();}, EShortcut::GLOBAL_RETURN);
	statusbar = CGStatusBar::create(3, 555, ImagePath::builtin("TStatBar.bmp"), 742);

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);

	//data for information table:
	// fields[row][column] = list of id's of players for this box
	constexpr std::vector< std::vector< PlayerColor > > SThievesGuildInfo::* fields[] =
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

		std::string text = LIBRARY->generaltexth->jktexts[24+g];
		boost::algorithm::trim_if(text,boost::algorithm::is_any_of("\""));
		if(settings["general"]["enableUiEnhancements"].Bool() && g >= 2 && g <= 4) // add icons instead of text (text is OH3 behavior)
		{
			auto addicon = [this, y](GameResID res, int x){ columnHeaderIcons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("SMALRES"), res.getNum(), 0, x, y - 10)); };
			if(g == 2) // gold
				addicon(GameResID::GOLD, 125);
			else if(g == 3) // wood, ore
			{
				addicon(GameResID::WOOD, 110);
				addicon(GameResID::ORE, 140);
			}
			else if(g == 4) // mercury, sulfur, crystal, gems
			{
				addicon(GameResID::MERCURY, 80);
				addicon(GameResID::SULFUR, 110);
				addicon(GameResID::CRYSTAL, 140);
				addicon(GameResID::GEMS, 170);
			}
		}
		else
			rowHeaders.push_back(std::make_shared<CLabel>(135, y, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, text, 220));
	}

	for(int g=1; g<tgi.playerColors.size(); ++g)
		columnBackgrounds.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PRSTRIPS"), g-1, 0, 250 + 66*g, 7));

	for(int g=0; g<tgi.playerColors.size(); ++g)
		columnHeaders.push_back(std::make_shared<CLabel>(283 + 66*g, 21, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[16+g]));

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
					cells.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("itgflags"), players[i + j*4].getNum(), 0, rowStartX + (int)i*12, rowStartY));
			}
		}
	}

	static const std::string colorToBox[] = {"PRRED.BMP", "PRBLUE.BMP", "PRTAN.BMP", "PRGREEN.BMP", "PRORANGE.BMP", "PRPURPLE.BMP", "PRTEAL.BMP", "PRROSE.bmp"};

	//printing best hero
	int counter = 0;
	for(auto & iter : tgi.colorToBestHero)
	{
		banners.push_back(std::make_shared<CPicture>(ImagePath::builtin(colorToBox[iter.first.getNum()]), 253 + 66 * counter, 334));
		if(iter.second.portraitSource.isValid())
		{
			bestHeroes.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), iter.second.getIconIndex(), 0, 260 + 66 * counter, 360));
			//TODO: r-click info:
			// - r-click on hero
			if(iter.second.details)
			{
				std::vector<std::string> lines;
				boost::split(lines, LIBRARY->generaltexth->allTexts[184], boost::is_any_of("\n"));
				for(int i=0; i<GameConstants::PRIMARY_SKILLS; ++i)
				{
					primSkillHeaders.push_back(std::make_shared<CLabel>(260 + 66 * counter, 407 + 11 * i, FONT_TINY, ETextAlignment::BOTTOMLEFT, Colors::WHITE, lines[i]));
					primSkillHeadersArea.push_back(std::make_shared<LRClickableArea>(Rect(primSkillHeaders.back()->pos.x - pos.x, primSkillHeaders.back()->pos.y - pos.y - 11, 50, 11), nullptr, [i]{
						CRClickPopup::createAndPush(LIBRARY->generaltexth->arraytxt[2 + i]);
					}));
					primSkillValues.push_back(std::make_shared<CLabel>(310 + 66 * counter, 407 + 11 * i, FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE,
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
		if(it.second != CreatureID::NONE)
			bestCreatures.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), it.second+2, 0, 255 + 66 * counter, 479));
		counter++;
	}

	//printing personality
	counter = 0;
	for(auto & it : tgi.personality)
	{
		std::string text;
		if(it.second == EAiTactic::NONE)
		{
			text = LIBRARY->generaltexth->arraytxt[172];
		}
		else if(it.second != EAiTactic::RANDOM)
		{
			text = LIBRARY->generaltexth->arraytxt[168 + static_cast<int>(it.second)];
		}

		personalities.push_back(std::make_shared<CLabel>(283 + 66*counter, 459, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, text));

		counter++;
	}
}

CObjectListWindow::CItem::CItem(CObjectListWindow * _parent, size_t _id, std::string _text)
	: CIntObject(LCLICK | DOUBLECLICK | RCLICK_POPUP),
	parent(_parent),
	index(_id)
{
	OBJECT_CONSTRUCTION;
	if(parent->images.size() > index)
		icon = std::make_shared<CPicture>(parent->images[index], Point(1, 1));
	border = std::make_shared<CPicture>(ImagePath::builtin("TPGATES"));
	pos = border->pos;

	setRedrawParent(true);

	text = std::make_shared<CLabel>(pos.w/2, pos.h/2, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, _text, 256);
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

void CObjectListWindow::CItem::clickPressed(const Point & cursorPosition)
{
	parent->changeSelection(index);

	if(parent->onClicked)
		parent->onClicked(index);
}

void CObjectListWindow::CItem::clickDouble(const Point & cursorPosition)
{
	if (parent->selected != index)
	{
		clickPressed(cursorPosition);
		return;
	}

	parent->elementSelected();
}

void CObjectListWindow::CItem::showPopupWindow(const Point & cursorPosition)
{
	if(parent->onPopup)
		parent->onPopup(index);
}

CObjectListWindow::CObjectListWindow(const std::vector<int> & _items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection, std::vector<std::shared_ptr<IImage>> images, bool searchBoxEnabled)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("TPGATE")),
	onSelect(Callback),
	selected(initialSelection),
	images(images)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(KEYBOARD);

	items.reserve(_items.size());

	for(int id : _items)
	{
		std::string objectName = GAME->interface()->cb->getObjInstance(ObjectInstanceID(id))->getObjectName();
		trimTextIfTooWide(objectName, id);
		items.emplace_back(id, objectName);
	}
	itemsVisible = items;

	init(titleWidget_, _title, _descr, searchBoxEnabled);
	list->scrollTo(std::min(static_cast<int>(initialSelection + 4), static_cast<int>(items.size() - 1))); // 4 is for centering (list have 9 elements)
}

CObjectListWindow::CObjectListWindow(const std::vector<std::string> & _items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection, std::vector<std::shared_ptr<IImage>> images, bool searchBoxEnabled)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("TPGATE")),
	onSelect(Callback),
	selected(initialSelection),
	images(images)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(KEYBOARD);

	items.reserve(_items.size());

	for(size_t i = 0; i < _items.size(); i++)
	{
		std::string objectName = _items[i];
		trimTextIfTooWide(objectName, static_cast<int>(i));
		items.emplace_back(static_cast<int>(i), objectName);
	}
	itemsVisible = items;

	init(titleWidget_, _title, _descr, searchBoxEnabled);
	list->scrollTo(std::min(static_cast<int>(initialSelection + 4), static_cast<int>(items.size() - 1))); // 4 is for centering (list have 9 elements)
}

void CObjectListWindow::init(std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, bool searchBoxEnabled)
{
	titleWidget = titleWidget_;

	title = std::make_shared<CLabel>(152, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, _title);
	descr = std::make_shared<CLabel>(145, 133, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, _descr);
	exit = std::make_shared<CButton>( Point(228, 402), AnimationPath::builtin("ICANCEL.DEF"), CButton::tooltip(), std::bind(&CObjectListWindow::exitPressed, this), EShortcut::GLOBAL_CANCEL);

	if(titleWidget)
	{
		addChild(titleWidget.get());
		titleWidget->pos.x = pos.w/2 + pos.x - titleWidget->pos.w/2;
		titleWidget->pos.y = 75 + pos.y - titleWidget->pos.h/2;
	}
	list = std::make_shared<CListBox>(std::bind(&CObjectListWindow::genItem, this, _1),
		Point(14, 151), Point(0, 25), 9, itemsVisible.size(), 0, 1, Rect(262, -32, 256, 256) );
	list->setRedrawParent(true);

	ok = std::make_shared<CButton>(Point(15, 402), AnimationPath::builtin("IOKAY.DEF"), CButton::tooltip(), std::bind(&CObjectListWindow::elementSelected, this), EShortcut::GLOBAL_ACCEPT);
	ok->block(!list->size());

	if(!searchBoxEnabled)
		return;

	Rect r(50, 90, pos.w - 100, 16);
	const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
	const ColorRGBA borderColor = ColorRGBA(128, 100, 75);
	const ColorRGBA grayedColor = ColorRGBA(158, 130, 105);
	searchBoxRectangle = std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor);
	searchBoxDescription = std::make_shared<CLabel>(r.center().x, r.center().y, FONT_SMALL, ETextAlignment::CENTER, grayedColor, LIBRARY->generaltexth->translate("vcmi.spellBook.search"));

	searchBox = std::make_shared<CTextInput>(r, FONT_SMALL, ETextAlignment::CENTER, true);
	searchBox->setCallback(std::bind(&CObjectListWindow::itemsSearchCallback, this, std::placeholders::_1));
}

void CObjectListWindow::trimTextIfTooWide(std::string & text, int id) const
{
	int maxWidth = pos.w - 60;	// 60 px for scrollbar and borders
	std::string idStr = '(' + std::to_string(id) + ')';
	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	std::string suffix = " ... " + idStr;

	if(font->getStringWidth(text) >= maxWidth)
	{
		logGlobal->warn("Mapobject name '%s' is too long and probably needs to be fixed! Trimming...", 
			text.substr(0, text.size() - idStr.size() + 1));

		// Trim text until it fits
		while(!text.empty())
		{
			std::string trimmedText = text + suffix;

			if(font->getStringWidth(trimmedText) < maxWidth)
				break;

			TextOperations::trimRightUnicode(text);
		}

		text += suffix;
	}
}

void CObjectListWindow::itemsSearchCallback(const std::string & text)
{
	searchBoxDescription->setEnabled(text.empty());

	itemsVisible.clear();
	std::vector<std::pair<int, decltype(items)::value_type>> rankedItems; // Store (score, item)

	for(const auto & item : items)
	{
		if(auto score = TextOperations::textSearchSimilarityScore(text, item.second)) // Keep only relevant items
			rankedItems.emplace_back(score.value(), item);
	}

	// Sort: Lower score is better match
	std::sort(rankedItems.begin(), rankedItems.end(), [](const auto & a, const auto & b)
	{
		return a.first < b.first;
	});

	for(const auto & rankedItem : rankedItems)
		itemsVisible.push_back(rankedItem.second);

	selected = 0;
	list->resize(itemsVisible.size());
	list->scrollTo(0);
	ok->block(!itemsVisible.size());

	redraw();
}

std::shared_ptr<CIntObject> CObjectListWindow::genItem(size_t index)
{
	if(index < itemsVisible.size())
		return std::make_shared<CItem>(this, index, itemsVisible[index].second);
	return std::shared_ptr<CIntObject>();
}

void CObjectListWindow::elementSelected()
{
	std::function<void(int)> toCall = onSelect;//save
	int where = itemsVisible[selected].first;      //required variables
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

void CObjectListWindow::keyPressed(EShortcut key)
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
		sel = static_cast<int>(itemsVisible.size());

	break; default:
		return;
	}

	vstd::abetween<int>(sel, 0, itemsVisible.size()-1);
	list->scrollTo(sel);
	changeSelection(sel);
}

VideoWindow::VideoWindow(const VideoPath & video, const ImagePath & rim, bool showBackground, float scaleFactor, const std::function<void(bool skipped)> & closeCb)
	: CWindowObject(BORDERED | SHADOW_DISABLED | NEEDS_ANIMATED_BACKGROUND)
	, showBackground(showBackground)
	, closeCb(closeCb)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | KEYBOARD);

	if(!rim.empty())
	{
		setBackground(rim);
		videoPlayer = std::make_shared<VideoWidgetOnce>(Point(80, 186), video, true, this);
		pos = center(Rect(0, 0, 800, 600));
	}
	else
	{
		blackBackground = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, 0, ENGINE->screenDimensions().x, ENGINE->screenDimensions().y));
		videoPlayer = std::make_shared<VideoWidgetOnce>(Point(0, 0), video, true, scaleFactor, this);
		pos = center(Rect(0, 0, videoPlayer->pos.w, videoPlayer->pos.h));
		blackBackground->addBox(Point(0, 0), Point(videoPlayer->pos.w, videoPlayer->pos.h), Colors::BLACK);
	}
}

void VideoWindow::showAll(Canvas & to)
{
	if(showBackground)
		to.fillTexture(ENGINE->renderHandler().loadImage(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE));
	CWindowObject::showAll(to);
}

void VideoWindow::onVideoPlaybackFinished()
{
	exit(false);
}

void VideoWindow::exit(bool skipped)
{
	close();
	if(closeCb)
		closeCb(skipped);
}

void VideoWindow::clickPressed(const Point & cursorPosition)
{
	exit(true);
}

void VideoWindow::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_RETURN)
		exit(true);
}

void VideoWindow::notFocusedClick()
{
	exit(true);
}

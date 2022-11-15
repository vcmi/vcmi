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

#include "CAdvmapInterface.h"
#include "CCastleInterface.h"
#include "CCreatureWindow.h"
#include "CHeroWindow.h"
#include "CreatureCostBox.h"
#include "InfoWindows.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../mapHandler.h"
#include "../CServerHandler.h"

#include "../battle/CBattleInterfaceClasses.h"
#include "../battle/CBattleInterface.h"
#include "../battle/CCreatureAnimation.h"

#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/CCursorHandler.h"

#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"

#include "../lobby/CSavingScreen.h"

#include "../../CCallback.h"

#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/CondSh.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CStopWatch.h"
#include "../lib/CTownHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/HeroBonus.h"
#include "../lib/mapping/CMap.h"
#include "../lib/NetPacksBase.h"
#include "../lib/StartInfo.h"

using namespace CSDL_Ext;

std::list<CFocusable*> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

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
		GH.pushIntT<CStackWindow>(creature, true);
}

void CRecruitmentWindow::CCreatureCard::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if(selected)
		drawBorder(to, pos, int3(248, 0, 0));
	else
		drawBorder(to, pos, int3(232, 212, 120));
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
			slider->moveTo(maxAmount);
		else // if slider already at 0 - emulate call to sliderMoved()
			sliderMoved(maxAmount);

		costPerTroopValue->createItems(card->creature->cost);
		totalCostValue->createItems(card->creature->cost);

		costPerTroopValue->set(card->creature->cost);
		totalCostValue->set(card->creature->cost * maxAmount);

		//Recruit %s
		title->setText(boost::str(boost::format(CGI->generaltexth->tcommands[21]) % card->creature->namePl));

		maxButton->block(maxAmount == 0);
		slider->block(maxAmount == 0);
	}
}

void CRecruitmentWindow::buy()
{
	CreatureID crid =  selected->creature->idNumber;
	SlotID dstslot = dst-> getSlotFor(crid);

	if(!dstslot.validSlot() && (selected->creature->warMachine == ArtifactID::NONE)) //no available slot
	{
		std::string txt;
		if(dst->ID == Obj::HERO)
		{
			txt = CGI->generaltexth->allTexts[425]; //The %s would join your hero, but there aren't enough provisions to support them.
			boost::algorithm::replace_first(txt, "%s", slider->getValue() > 1 ? CGI->creh->objects[crid]->namePl : CGI->creh->objects[crid]->nameSing);
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

void CRecruitmentWindow::showAll(SDL_Surface * to)
{
	CWindowObject::showAll(to);

	// recruit\total values
	drawBorder(to, pos.x + 172, pos.y + 222, 67, 42, int3(239,215,123));
	drawBorder(to, pos.x + 246, pos.y + 222, 67, 42, int3(239,215,123));

	//cost boxes
	drawBorder(to, pos.x + 64,  pos.y + 222, 99, 76, int3(239,215,123));
	drawBorder(to, pos.x + 322, pos.y + 222, 99, 76, int3(239,215,123));

	//buttons borders
	drawBorder(to, pos.x + 133, pos.y + 312, 66, 34, int3(173,142,66));
	drawBorder(to, pos.x + 211, pos.y + 312, 66, 34, int3(173,142,66));
	drawBorder(to, pos.x + 289, pos.y + 312, 66, 34, int3(173,142,66));
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

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	slider = std::make_shared<CSlider>(Point(176,279),135,std::bind(&CRecruitmentWindow::sliderMoved,this, _1),0,0,0,true);

	maxButton = std::make_shared<CButton>(Point(134, 313), "IRCBTNS.DEF", CGI->generaltexth->zelp[553], std::bind(&CSlider::moveToMax, slider), SDLK_m);
	buyButton = std::make_shared<CButton>(Point(212, 313), "IBY6432.DEF", CGI->generaltexth->zelp[554], std::bind(&CRecruitmentWindow::buy, this), SDLK_RETURN);
	cancelButton = std::make_shared<CButton>(Point(290, 313), "ICN6432.DEF", CGI->generaltexth->zelp[555], std::bind(&CRecruitmentWindow::close, this), SDLK_ESCAPE);

	title = std::make_shared<CLabel>(243, 32, FONT_BIG, CENTER, Colors::YELLOW);
	availableValue = std::make_shared<CLabel>(205, 253, FONT_SMALL, CENTER, Colors::WHITE);
	toRecruitValue = std::make_shared<CLabel>(279, 253, FONT_SMALL, CENTER, Colors::WHITE);

	costPerTroopValue = std::make_shared<CreatureCostBox>(Rect(65, 222, 97, 74), CGI->generaltexth->allTexts[346]);
	totalCostValue = std::make_shared<CreatureCostBox>(Rect(323, 222, 97, 74), CGI->generaltexth->allTexts[466]);

	availableTitle = std::make_shared<CLabel>(205, 233, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[465]);
	toRecruitTitle = std::make_shared<CLabel>(279, 233, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[16]);

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
		slider->moveToMax();
	else // if slider already at 0 - emulate call to sliderMoved()
		sliderMoved(slider->getAmount());
}

void CRecruitmentWindow::sliderMoved(int to)
{
	if(!selected)
		return;

	buyButton->block(!to);
	availableValue->setText(boost::lexical_cast<std::string>(selected->amount - to));
	toRecruitValue->setText(boost::lexical_cast<std::string>(to));

	totalCostValue->set(selected->creature->cost * to);
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

	ok = std::make_shared<CButton>(Point(20, 263), "IOK6432", CButton::tooltip(), std::bind(&CSplitWindow::apply, this), SDLK_RETURN);
	cancel = std::make_shared<CButton>(Point(214, 263), "ICN6432", CButton::tooltip(), std::bind(&CSplitWindow::close, this), SDLK_ESCAPE);

	int sliderPosition = total - leftMin - rightMin;

	leftInput = std::make_shared<CTextInput>(Rect(20, 218, 100, 36), FONT_BIG, std::bind(&CSplitWindow::setAmountText, this, _1, true));
	rightInput = std::make_shared<CTextInput>(Rect(176, 218, 100, 36), FONT_BIG, std::bind(&CSplitWindow::setAmountText, this, _1, false));

	//add filters to allow only number input
	leftInput->filters += std::bind(&CTextInput::numberFilter, _1, _2, leftMin, leftMax);
	rightInput->filters += std::bind(&CTextInput::numberFilter, _1, _2, rightMin, rightMax);

	leftInput->setText(boost::lexical_cast<std::string>(leftAmount), false);
	rightInput->setText(boost::lexical_cast<std::string>(rightAmount), false);

	animLeft = std::make_shared<CCreaturePic>(20, 54, creature, true, false);
	animRight = std::make_shared<CCreaturePic>(177, 54,creature, true, false);

	slider = std::make_shared<CSlider>(Point(21, 194), 257, std::bind(&CSplitWindow::sliderMoved, this, _1), 0, sliderPosition, rightAmount - rightMin, true);

	std::string titleStr = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(titleStr,"%s", creature->namePl);
	title = std::make_shared<CLabel>(150, 34, FONT_BIG, CENTER, Colors::YELLOW, titleStr);
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
	slider->moveTo(rightAmount - rightMin);
}

void CSplitWindow::setAmount(int value, bool left)
{
	int total = leftAmount + rightAmount;
	leftAmount  = left ? value : total - value;
	rightAmount = left ? total - value : value;

	leftInput->setText(boost::lexical_cast<std::string>(leftAmount));
	rightInput->setText(boost::lexical_cast<std::string>(rightAmount));
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
	ok = std::make_shared<CButton>(Point(297, 413), "IOKAY", CButton::tooltip(), std::bind(&CLevelWindow::close, this), SDLK_RETURN);

	//%s has gained a level.
	mainTitle = std::make_shared<CLabel>(192, 33, FONT_MEDIUM, CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->allTexts[444]) % hero->name));

	//%s is now a level %d %s.
	levelTitle = std::make_shared<CLabel>(192, 162, FONT_MEDIUM, CENTER, Colors::WHITE,
		boost::str(boost::format(CGI->generaltexth->allTexts[445]) % hero->name % hero->level % hero->type->heroClass->name));

	skillIcon = std::make_shared<CAnimImage>("PSKIL42", pskill, 0, 174, 190);

	skillValue = std::make_shared<CLabel>(192, 253, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->primarySkillNames[pskill] + " +1");
}


CLevelWindow::~CLevelWindow()
{
	//FIXME: call callback if there was nothing to select?
	if (box && box->selectedIndex() != -1)
		cb(box->selectedIndex());

	LOCPLINT->showingDialog->setn(false);
}

static void setIntSetting(std::string group, std::string field, int value)
{
	Settings entry = settings.write[group][field];
	entry->Float() = value;
}

static void setBoolSetting(std::string group, std::string field, bool value)
{
	Settings fullscreen = settings.write[group][field];
	fullscreen->Bool() = value;
}

static std::string resolutionToString(int w, int h)
{
	return std::to_string(w) + 'x' + std::to_string(h);
}

CSystemOptionsWindow::CSystemOptionsWindow()
	: CWindowObject(PLAYER_COLORED, "SysOpBck"),
	onFullscreenChanged(settings.listen["video"]["fullscreen"])
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	title = std::make_shared<CLabel>(242, 32, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[568]);

	const JsonNode & texts = CGI->generaltexth->localizedTexts["systemOptions"];

	//left window section
	leftGroup = std::make_shared<CLabelGroup>(FONT_MEDIUM, CENTER, Colors::YELLOW);
	leftGroup->add(122,  64, CGI->generaltexth->allTexts[569]);
	leftGroup->add(122, 130, CGI->generaltexth->allTexts[570]);
	leftGroup->add(122, 196, CGI->generaltexth->allTexts[571]);
	leftGroup->add(122, 262, texts["resolutionButton"]["label"].String());
	leftGroup->add(122, 347, CGI->generaltexth->allTexts[394]);
	leftGroup->add(122, 412, CGI->generaltexth->allTexts[395]);

	//right section
	rightGroup = std::make_shared<CLabelGroup>(FONT_MEDIUM, TOPLEFT, Colors::WHITE);
	rightGroup->add(282, 57,  CGI->generaltexth->allTexts[572]);
	rightGroup->add(282, 89,  CGI->generaltexth->allTexts[573]);
	rightGroup->add(282, 121, CGI->generaltexth->allTexts[574]);
	rightGroup->add(282, 153, CGI->generaltexth->allTexts[577]);
	rightGroup->add(282, 185, texts["creatureWindowButton"]["label"].String());
	rightGroup->add(282, 217, texts["fullscreenButton"]["label"].String());

	//setting up buttons
	load = std::make_shared<CButton>(Point(246,  298), "SOLOAD.DEF", CGI->generaltexth->zelp[321], [&](){ bloadf(); }, SDLK_l);
	load->setImageOrder(1, 0, 2, 3);

	save = std::make_shared<CButton>(Point(357, 298), "SOSAVE.DEF", CGI->generaltexth->zelp[322], [&](){ bsavef(); }, SDLK_s);
	save->setImageOrder(1, 0, 2, 3);

	restart = std::make_shared<CButton>(Point(246, 357), "SORSTRT", CGI->generaltexth->zelp[323], [&](){ brestartf(); }, SDLK_r);
	restart->setImageOrder(1, 0, 2, 3);

	if(CSH->isGuest())
	{
		load->block(true);
		save->block(true);
		restart->block(true);
	}

	mainMenu = std::make_shared<CButton>(Point(357, 357), "SOMAIN.DEF", CGI->generaltexth->zelp[320], [&](){ bmainmenuf(); }, SDLK_m);
	mainMenu->setImageOrder(1, 0, 2, 3);

	quitGame = std::make_shared<CButton>(Point(246, 415), "soquit.def", CGI->generaltexth->zelp[324], [&](){ bquitf(); }, SDLK_q);
	quitGame->setImageOrder(1, 0, 2, 3);

	backToMap = std::make_shared<CButton>( Point(357, 415), "soretrn.def", CGI->generaltexth->zelp[325], [&](){ breturnf(); }, SDLK_RETURN);
	backToMap->setImageOrder(1, 0, 2, 3);
	backToMap->assignedKeys.insert(SDLK_ESCAPE);

	heroMoveSpeed = std::make_shared<CToggleGroup>(0);
	enemyMoveSpeed = std::make_shared<CToggleGroup>(0);
	mapScrollSpeed = std::make_shared<CToggleGroup>(0);

	heroMoveSpeed->addToggle(1, std::make_shared<CToggleButton>(Point( 28, 77), "sysopb1.def", CGI->generaltexth->zelp[349]));
	heroMoveSpeed->addToggle(2, std::make_shared<CToggleButton>(Point( 76, 77), "sysopb2.def", CGI->generaltexth->zelp[350]));
	heroMoveSpeed->addToggle(4, std::make_shared<CToggleButton>(Point(124, 77), "sysopb3.def", CGI->generaltexth->zelp[351]));
	heroMoveSpeed->addToggle(8, std::make_shared<CToggleButton>(Point(172, 77), "sysopb4.def", CGI->generaltexth->zelp[352]));
	heroMoveSpeed->setSelected((int)settings["adventure"]["heroSpeed"].Float());
	heroMoveSpeed->addCallback(std::bind(&setIntSetting, "adventure", "heroSpeed", _1));

	enemyMoveSpeed->addToggle(2, std::make_shared<CToggleButton>(Point( 28, 144), "sysopb5.def", CGI->generaltexth->zelp[353]));
	enemyMoveSpeed->addToggle(4, std::make_shared<CToggleButton>(Point( 76, 144), "sysopb6.def", CGI->generaltexth->zelp[354]));
	enemyMoveSpeed->addToggle(8, std::make_shared<CToggleButton>(Point(124, 144), "sysopb7.def", CGI->generaltexth->zelp[355]));
	enemyMoveSpeed->addToggle(0, std::make_shared<CToggleButton>(Point(172, 144), "sysopb8.def", CGI->generaltexth->zelp[356]));
	enemyMoveSpeed->setSelected((int)settings["adventure"]["enemySpeed"].Float());
	enemyMoveSpeed->addCallback(std::bind(&setIntSetting, "adventure", "enemySpeed", _1));

	mapScrollSpeed->addToggle(1, std::make_shared<CToggleButton>(Point( 28, 210), "sysopb9.def", CGI->generaltexth->zelp[357]));
	mapScrollSpeed->addToggle(2, std::make_shared<CToggleButton>(Point( 92, 210), "sysob10.def", CGI->generaltexth->zelp[358]));
	mapScrollSpeed->addToggle(4, std::make_shared<CToggleButton>(Point(156, 210), "sysob11.def", CGI->generaltexth->zelp[359]));
	mapScrollSpeed->setSelected((int)settings["adventure"]["scrollSpeed"].Float());
	mapScrollSpeed->addCallback(std::bind(&setIntSetting, "adventure", "scrollSpeed", _1));

	musicVolume = std::make_shared<CVolumeSlider>(Point(29, 359), "syslb.def", CCS->musich->getVolume(), &CGI->generaltexth->zelp[326]);
	musicVolume->addCallback(std::bind(&setIntSetting, "general", "music", _1));

	effectsVolume = std::make_shared<CVolumeSlider>(Point(29, 425), "syslb.def", CCS->soundh->getVolume(), &CGI->generaltexth->zelp[336]);
	effectsVolume->addCallback(std::bind(&setIntSetting, "general", "sound", _1));

	showReminder = std::make_shared<CToggleButton>(Point(246, 87), "sysopchk.def", CGI->generaltexth->zelp[361], [&](bool value)
	{
		setBoolSetting("adventure", "heroReminder", value);
	});
	showReminder->setSelected(settings["adventure"]["heroReminder"].Bool());

	quickCombat = std::make_shared<CToggleButton>(Point(246, 87+32), "sysopchk.def", CGI->generaltexth->zelp[362], [&](bool value)
	{
		setBoolSetting("adventure", "quickCombat", value);
	});
	quickCombat->setSelected(settings["adventure"]["quickCombat"].Bool());

	spellbookAnim = std::make_shared<CToggleButton>(Point(246, 87+64), "sysopchk.def", CGI->generaltexth->zelp[364], [&](bool value)
	{
		setBoolSetting("video", "spellbookAnimation", value);
	});
	spellbookAnim->setSelected(settings["video"]["spellbookAnimation"].Bool());

	fullscreen = std::make_shared<CToggleButton>(Point(246, 215), "sysopchk.def", CButton::tooltip(texts["fullscreenButton"]), [&](bool value)
	{
		setBoolSetting("video", "fullscreen", value);
	});
	fullscreen->setSelected(settings["video"]["fullscreen"].Bool());

	onFullscreenChanged([&](const JsonNode &newState){ fullscreen->setSelected(newState.Bool());});

	gameResButton = std::make_shared<CButton>(Point(28, 275),"buttons/resolution", CButton::tooltip(texts["resolutionButton"]), std::bind(&CSystemOptionsWindow::selectGameRes, this), SDLK_g);

	const auto & screenRes = settings["video"]["screenRes"];
	gameResLabel = std::make_shared<CLabel>(170, 292, FONT_MEDIUM, CENTER, Colors::YELLOW, resolutionToString(screenRes["width"].Integer(), screenRes["height"].Integer()));
}

void CSystemOptionsWindow::selectGameRes()
{
	std::vector<std::string> items;
	const JsonNode & texts = CGI->generaltexth->localizedTexts["systemOptions"]["resolutionMenu"];

#ifndef VCMI_IOS
	SDL_Rect displayBounds;
	SDL_GetDisplayBounds(std::max(0, SDL_GetWindowDisplayIndex(mainWindow)), &displayBounds);
#endif

	size_t currentResolutionIndex = 0;
	size_t i = 0;
	for(const auto & it : conf.guiOptions)
	{
		const auto & resolution = it.first;
#ifndef VCMI_IOS
		if(displayBounds.w < resolution.first || displayBounds.h < resolution.second)
			continue;
#endif

		auto resolutionStr = resolutionToString(resolution.first, resolution.second);
		if(gameResLabel->text == resolutionStr)
			currentResolutionIndex = i;
		items.push_back(std::move(resolutionStr));
		++i;
	}

	GH.pushIntT<CObjectListWindow>(items, nullptr, texts["label"].String(), texts["help"].String(), std::bind(&CSystemOptionsWindow::setGameRes, this, _1), currentResolutionIndex);
}

void CSystemOptionsWindow::setGameRes(int index)
{
	auto iter = conf.guiOptions.begin();
	std::advance(iter, index);

	//do not set resolution to illegal one (0x0)
	assert(iter!=conf.guiOptions.end() && iter->first.first > 0 && iter->first.second > 0);

	Settings gameRes = settings.write["video"]["screenRes"];
	gameRes["width"].Float() = iter->first.first;
	gameRes["height"].Float() = iter->first.second;

	std::string resText;
	resText += boost::lexical_cast<std::string>(iter->first.first);
	resText += "x";
	resText += boost::lexical_cast<std::string>(iter->first.second);
	gameResLabel->setText(resText);
}

void CSystemOptionsWindow::bquitf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this](){ closeAndPushEvent(SDL_USEREVENT, EUserEvent::FORCE_QUIT); }, 0);
}

void CSystemOptionsWindow::breturnf()
{
	close();
}

void CSystemOptionsWindow::bmainmenuf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this](){ closeAndPushEvent(SDL_USEREVENT, EUserEvent::RETURN_TO_MAIN_MENU); }, 0);
}

void CSystemOptionsWindow::bloadf()
{
	close();
	LOCPLINT->proposeLoadingGame();
}

void CSystemOptionsWindow::bsavef()
{
	close();
	GH.pushIntT<CSavingScreen>();
}

void CSystemOptionsWindow::brestartf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [this](){ closeAndPushEvent(SDL_USEREVENT, EUserEvent::RESTART_GAME); }, 0);
}

void CSystemOptionsWindow::closeAndPushEvent(int eventType, int code)
{
	close();
	GH.pushSDLEvent(eventType, code);
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

	title = std::make_shared<CLabel>(200, 35, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[37]);
	cost = std::make_shared<CLabel>(320, 328, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(GameConstants::HERO_GOLD_COST));

	auto rumorText = boost::str(boost::format(CGI->generaltexth->allTexts[216]) % LOCPLINT->cb->getTavernRumor(tavernObj));
	rumor = std::make_shared<CTextBox>(rumorText, Rect(32, 190, 330, 68), 0, FONT_SMALL, CENTER, Colors::WHITE);

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
	cancel = std::make_shared<CButton>(Point(310, 428), "ICANCEL.DEF", CButton::tooltip(CGI->generaltexth->tavernInfo[7]), std::bind(&CTavernWindow::close, this), SDLK_ESCAPE);
	recruit = std::make_shared<CButton>(Point(272, 355), "TPTAV01.DEF", CButton::tooltip(), std::bind(&CTavernWindow::recruitb, this), SDLK_RETURN);
	thiefGuild = std::make_shared<CButton>(Point(22, 428), "TPTAV02.DEF", CButton::tooltip(CGI->generaltexth->tavernInfo[5]), std::bind(&CTavernWindow::thievesguildb, this), SDLK_t);

	if(LOCPLINT->cb->getResourceAmount(Res::GOLD) < GameConstants::HERO_GOLD_COST) //not enough gold
	{
		recruit->addHoverText(CButton::NORMAL, CGI->generaltexth->tavernInfo[0]); //Cannot afford a Hero
		recruit->block(true);
	}
	else if(LOCPLINT->cb->howManyHeroes(true) >= VLC->modh->settings.MAX_HEROES_AVAILABLE_PER_PLAYER)
	{
		//Cannot recruit. You already have %d Heroes.
		recruit->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->tavernInfo[1]) % LOCPLINT->cb->howManyHeroes(true)));
		recruit->block(true);
	}
	else if(LOCPLINT->cb->howManyHeroes(false) >= VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER)
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
	GH.pushIntT<CThievesGuildWindow>(tavernObj);
}

CTavernWindow::~CTavernWindow()
{
	CCS->videoh->close();
}

void CTavernWindow::show(SDL_Surface * to)
{
	CWindowObject::show(to);

	CCS->videoh->update(pos.x+70, pos.y+56, to, true, false);
	if(selected >= 0)
	{
		auto sel = selected ? h2 : h1;

		if (selected != oldSelected  &&  !recruit->isBlocked())
		{
			// Selected hero just changed. Update RECRUIT button hover text if recruitment is allowed.
			oldSelected = selected;

			//Recruit %s the %s
			recruit->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->tavernInfo[3]) % sel->h->name % sel->h->type->heroClass->name));
		}

		printAtMiddleWBLoc(sel->description, 146, 395, FONT_SMALL, 200, Colors::WHITE, to);
		CSDL_Ext::drawBorder(to,sel->pos.x-2,sel->pos.y-2,sel->pos.w+4,sel->pos.h+4,int3(247,223,123));
	}
}

void CTavernWindow::HeroPortrait::clickLeft(tribool down, bool previousState)
{
	if(h && previousState && !down)
		*_sel = _id;
}

void CTavernWindow::HeroPortrait::clickRight(tribool down, bool previousState)
{
	if(h && down)
		GH.pushIntT<CRClickPopupInt>(std::make_shared<CHeroWindow>(h));
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
		boost::algorithm::replace_first(hoverName,"%s",H->name);

		int artifs = (int)h->artifactsWorn.size() + (int)h->artifactsInBackpack.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifactsWorn, ArtifactPosition(i)))
				artifs--;

		description = CGI->generaltexth->allTexts[215];
		boost::algorithm::replace_first(description, "%s", h->name);
		boost::algorithm::replace_first(description, "%d", boost::lexical_cast<std::string>(h->level));
		boost::algorithm::replace_first(description, "%s", h->type->heroClass->name);
		boost::algorithm::replace_first(description, "%d", boost::lexical_cast<std::string>(artifs));

		portrait = std::make_shared<CAnimImage>("portraitsLarge", h->portrait);
	}
}

void CTavernWindow::HeroPortrait::hover(bool on)
{
	//Hoverable::hover(on);
	if(on)
		GH.statusbar->setText(hoverName);
	else
		GH.statusbar->clear();
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

void CExchangeController::swapArtifacts(ArtifactPosition slot)
{
	bool leftHasArt = !left->isPositionFree(slot);
	bool rightHasArt = !right->isPositionFree(slot);

	if(!leftHasArt && !rightHasArt)
		return;

	ArtifactLocation leftLocation = ArtifactLocation(left, slot);
	ArtifactLocation rightLocation = ArtifactLocation(right, slot);

	if(leftHasArt && !left->artifactsWorn.at(slot).artifact->canBePutAt(rightLocation, true))
		return;

	if(rightHasArt && !right->artifactsWorn.at(slot).artifact->canBePutAt(leftLocation, true))
		return;

	if(leftHasArt)
	{
		if(rightHasArt)
		{
			auto art = right->getArt(slot);

			cb->swapArtifacts(leftLocation, rightLocation);
			cb->swapArtifacts(ArtifactLocation(right, right->getArtPos(art)), leftLocation);
		}
		else
			cb->swapArtifacts(leftLocation, rightLocation);
	}
	else
	{
		cb->swapArtifacts(rightLocation, leftLocation);
	}
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

const std::vector<ArtifactPosition> unmovablePositions = {ArtifactPosition::SPELLBOOK, ArtifactPosition::MACH4};

bool isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot)
{
	return slot.second.artifact
		&& !slot.second.locked
		&& !vstd::contains(unmovablePositions, slot.first);
}

// Puts all composite arts to backpack and returns their previous location
std::vector<HeroArtifact> CExchangeController::moveCompositeArtsToBackpack()
{
	std::vector<const CGHeroInstance *> sides = {left, right};
	std::vector<HeroArtifact> artPositions;

	for(auto hero : sides)
	{
		for(int i = ArtifactPosition::HEAD; i < ArtifactPosition::AFTER_LAST; i++)
		{
			auto artPosition = ArtifactPosition(i);
			auto art = hero->getArt(artPosition);

			if(art && art->canBeDisassembled())
			{
				cb->swapArtifacts(
					ArtifactLocation(hero, artPosition),
					ArtifactLocation(hero, ArtifactPosition(GameConstants::BACKPACK_START)));

				artPositions.push_back(HeroArtifact(hero, art, artPosition));
			}
		}
	}

	return artPositions;
}

void CExchangeController::swapArtifacts()
{
	for(int i = ArtifactPosition::HEAD; i < ArtifactPosition::AFTER_LAST; i++)
	{
		if(vstd::contains(unmovablePositions, i))
			continue;

		swapArtifacts(ArtifactPosition(i));
	}

	auto leftHeroBackpack = getBackpackArts(left);
	auto rightHeroBackpack = getBackpackArts(right);

	for(auto leftArt : leftHeroBackpack)
	{
		cb->swapArtifacts(
			ArtifactLocation(left, left->getArtPos(leftArt)),
			ArtifactLocation(right, ArtifactPosition(GameConstants::BACKPACK_START)));
	}

	for(auto rightArt : rightHeroBackpack)
	{
		cb->swapArtifacts(
			ArtifactLocation(right, right->getArtPos(rightArt)),
			ArtifactLocation(left, ArtifactPosition(GameConstants::BACKPACK_START)));
	}
}

std::function<void()> CExchangeController::onSwapArtifacts()
{
	return [&]()
	{
		GsThread::run([=]
		{
			// it is not possible directly exchange composite artifacts like Angelic Alliance and Armor of Damned
			auto compositeArtLocations = moveCompositeArtsToBackpack();

			swapArtifacts();

			for(HeroArtifact artLocation : compositeArtLocations)
			{
				auto target = artLocation.hero == left ? right : left;
				auto currentPos = target->getArtPos(artLocation.artifact);

				cb->swapArtifacts(
					ArtifactLocation(target, currentPos),
					ArtifactLocation(target, artLocation.artPosition));
			}

			view->redraw();
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
				return s.second->getCreatureID().toCreature()->AIValue;
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
		while(vstd::contains_if(source->artifactsWorn, isArtRemovable))
		{
			auto art = std::find_if(source->artifactsWorn.begin(), source->artifactsWorn.end(), isArtRemovable);

			moveArtifact(source, target, art->first);
		}

		while(!source->artifactsInBackpack.empty())
		{
			moveArtifact(source, target, source->getArtPos(source->artifactsInBackpack.begin()->artifact));
		}

		view->redraw();
	});
}

void CExchangeController::moveArtifact(
	const CGHeroInstance * source,
	const CGHeroInstance * target,
	ArtifactPosition srcPosition)
{
	auto artifact = source->getArt(srcPosition);
	auto srcLocation = ArtifactLocation(source, srcPosition);

	for(auto slot : artifact->artType->possibleSlots.at(target->bearerType()))
	{
		auto existingArtifact = target->getArt(slot);
		auto existingArtInfo = target->getSlot(slot);
		ArtifactLocation destLocation(target, slot);

		if(!existingArtifact
			&& (!existingArtInfo || !existingArtInfo->locked)
			&& artifact->canBePutAt(destLocation))
		{
			cb->swapArtifacts(srcLocation, ArtifactLocation(target, slot));
			
			return;
		}
	}

	cb->swapArtifacts(srcLocation, ArtifactLocation(target, ArtifactPosition(GameConstants::BACKPACK_START)));
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
		fmt % h->name % h->level % h->type->heroClass->name;
		return boost::str(fmt);
	};

	titles[0] = std::make_shared<CLabel>(147, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[0]));
	titles[1] = std::make_shared<CLabel>(653, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[1]));

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
			primSkillValues[leftRight].push_back(std::make_shared<CLabel>(352 + (qeLayout ? 96 : 93) * leftRight, (qeLayout ? 22 : 35) + (qeLayout ? 26 : 36) * m, FONT_SMALL, CENTER, Colors::WHITE));


		for(int m=0; m < hero->secSkills.size(); ++m)
			secSkillIcons[leftRight].push_back(std::make_shared<CAnimImage>(SECSK32, 0, 0, 32 + 36 * m + 454 * leftRight, qeLayout ? 83 : 88));

		specImages[leftRight] = std::make_shared<CAnimImage>("UN32", hero->type->imageIndex, 0, 67 + 490 * leftRight, qeLayout ? 41 : 45);

		expImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 4, 0, 103 + 490 * leftRight, qeLayout ? 41 : 45);
		expValues[leftRight] = std::make_shared<CLabel>(119 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, CENTER, Colors::WHITE);

		manaImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 5, 0, 139 + 490 * leftRight, qeLayout ? 41 : 45);
		manaValues[leftRight] = std::make_shared<CLabel>(155 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, CENTER, Colors::WHITE);
	}

	portraits[0] = std::make_shared<CAnimImage>("PortraitsLarge", heroInst[0]->portrait, 0, 257, 13);
	portraits[1] = std::make_shared<CAnimImage>("PortraitsLarge", heroInst[1]->portrait, 0, 485, 13);

	artifs[0] = std::make_shared<CArtifactsOfHero>(Point(-334, 150));
	artifs[0]->commonInfo = std::make_shared<CArtifactsOfHero::SCommonPart>();
	artifs[0]->commonInfo->participants.insert(artifs[0].get());
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = std::make_shared<CArtifactsOfHero>(Point(96, 150));
	artifs[1]->commonInfo = artifs[0]->commonInfo;
	artifs[1]->commonInfo->participants.insert(artifs[1].get());
	artifs[1]->setHero(heroInst[1]);

	addSet(artifs[0]);
	addSet(artifs[1]);

	for(int g=0; g<4; ++g)
	{
		primSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>());
		if (qeLayout)
			primSkillAreas[g]->pos = genRect(22, 152, pos.x + 324, pos.y + 12 + 26 * g);
		else
			primSkillAreas[g]->pos = genRect(32, 140, pos.x + 329, pos.y + 19 + 36 * g);
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->type = g;
		primSkillAreas[g]->bonusValue = -1;
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
			secSkillAreas[b][g]->pos = genRect(32, 32, pos.x + 32 + g*36 + b*454 , pos.y + (qeLayout ? 83 : 88));
			secSkillAreas[b][g]->baseType = 1;

			secSkillAreas[b][g]->type = skill;
			secSkillAreas[b][g]->bonusValue = level;
			secSkillAreas[b][g]->text = CGI->skillh->skillInfo(skill, level);

			secSkillAreas[b][g]->hoverText = CGI->generaltexth->heroscrn[21];
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->generaltexth->levels[level - 1]);
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->skillh->skillName(skill));
		}

		heroAreas[b] = std::make_shared<CHeroArea>(257 + 228*b, 13, hero);

		specialtyAreas[b] = std::make_shared<LRClickableAreaWText>();
		specialtyAreas[b]->pos = genRect(32, 32, pos.x + 69 + 490*b, pos.y + (qeLayout ? 41 : 45));
		specialtyAreas[b]->hoverText = CGI->generaltexth->heroscrn[27];
		specialtyAreas[b]->text = hero->type->specDescr;

		experienceAreas[b] = std::make_shared<LRClickableAreaWText>();
		experienceAreas[b]->pos = genRect(32, 32, pos.x + 105 + 490*b, pos.y + (qeLayout ? 41 : 45));
		experienceAreas[b]->hoverText = CGI->generaltexth->heroscrn[9];
		experienceAreas[b]->text = CGI->generaltexth->allTexts[2];
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->level));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(hero->level+1)));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->exp));

		spellPointsAreas[b] = std::make_shared<LRClickableAreaWText>();
		spellPointsAreas[b]->pos = genRect(32, 32, pos.x + 141 + 490*b, pos.y + (qeLayout ? 41 : 45));
		spellPointsAreas[b]->hoverText = CGI->generaltexth->heroscrn[22];
		spellPointsAreas[b]->text = CGI->generaltexth->allTexts[205];
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%s", hero->name);
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->mana));
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->manaLimit()));

		morale[b] = std::make_shared<MoraleLuckBox>(true, genRect(32, 32, 176 + 490 * b, 39), true);
		luck[b] = std::make_shared<MoraleLuckBox>(false, genRect(32, 32, 212 + 490 * b, 39), true);
	}

	quit = std::make_shared<CButton>(Point(732, 567), "IOKAY.DEF", CGI->generaltexth->zelp[600], std::bind(&CExchangeWindow::close, this), SDLK_RETURN);
	if(queryID.getNum() > 0)
		quit->addCallback([=](){ LOCPLINT->cb->selectionMade(0, queryID); });

	questlogButton[0] = std::make_shared<CButton>(Point( 10, qeLayout ? 39 : 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 0));
	questlogButton[1] = std::make_shared<CButton>(Point(740, qeLayout ? 39 : 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 1));

	Rect barRect(5, 578, 725, 18);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, barRect, 5, 578, false));

	//garrison interface

	garr = std::make_shared<CGarrisonInt>(69, qeLayout ? 122 : 131, 4, Point(418,0), heroInst[0], heroInst[1], true, true);
	auto splitButtonCallback = [&](){ garr->splitClick(); };
	garr->addSplitBtn(std::make_shared<CButton>( Point( 10, qeLayout ? 122 : 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback));
	garr->addSplitBtn(std::make_shared<CButton>( Point(744, qeLayout ? 122 : 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback));

	if (qeLayout && (CGI->generaltexth->qeModCommands.size() >= 5))
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

CExchangeWindow::~CExchangeWindow()
{
	artifs[0]->commonInfo = nullptr;
	artifs[1]->commonInfo = nullptr;
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
			primSkillValues[leftRight][m]->setText(boost::lexical_cast<std::string>(value));
		}

		for(int m=0; m < hero->secSkills.size(); ++m)
		{
			int id = hero->secSkills[m].first;
			int level = hero->secSkills[m].second;

			secSkillIcons[leftRight][m]->setFrame(2 + id * 3 + level);
		}

		expValues[leftRight]->setText(makeNumberShort(hero->exp));
		manaValues[leftRight]->setText(makeNumberShort(hero->mana));

		morale[leftRight]->set(hero);
		luck[leftRight]->set(hero);
	}
}

CShipyardWindow::CShipyardWindow(const std::vector<si32> & cost, int state, int boatType, const std::function<void()> & onBuy)
	: CStatusbarWindow(PLAYER_COLORED, "TPSHIP")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	bgWater = std::make_shared<CPicture>("TPSHIPBK", 100, 69);

	std::string boatFilenames[3] = {"AB01_", "AB02_", "AB03_"};

	Point waterCenter = Point(bgWater->pos.x+bgWater->pos.w/2, bgWater->pos.y+bgWater->pos.h/2);
	bgShip = std::make_shared<CAnimImage>(boatFilenames[boatType], 0, 7, 120, 96, 0);
	bgShip->center(waterCenter);

	// Create resource icons and costs.
	std::string goldValue = boost::lexical_cast<std::string>(cost[Res::GOLD]);
	std::string woodValue = boost::lexical_cast<std::string>(cost[Res::WOOD]);

	goldCost = std::make_shared<CLabel>(118, 294, FONT_SMALL, CENTER, Colors::WHITE, goldValue);
	woodCost = std::make_shared<CLabel>(212, 294, FONT_SMALL, CENTER, Colors::WHITE, woodValue);

	goldPic = std::make_shared<CAnimImage>("RESOURCE", Res::GOLD, 0, 100, 244);
	woodPic = std::make_shared<CAnimImage>("RESOURCE", Res::WOOD, 0, 196, 244);

	quit = std::make_shared<CButton>(Point(224, 312), "ICANCEL", CButton::tooltip(CGI->generaltexth->allTexts[599]), std::bind(&CShipyardWindow::close, this), SDLK_RETURN);
	build = std::make_shared<CButton>(Point(42, 312), "IBUY30", CButton::tooltip(CGI->generaltexth->allTexts[598]), std::bind(&CShipyardWindow::close, this), SDLK_RETURN);
	build->addCallback(onBuy);

	for(Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, 1))
	{
		if(cost[i] > LOCPLINT->cb->getResourceAmount(i))
		{
			build->block(true);
			break;
		}
	}

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	title = std::make_shared<CLabel>(164, 27,  FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[13]);
	costLabel = std::make_shared<CLabel>(164, 220, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->jktexts[14]);
}

CPuzzleWindow::CPuzzleWindow(const int3 & GrailPos, double discoveredRatio)
	: CWindowObject(PLAYER_COLORED | BORDERED, "PUZZLE"),
	grailPos(GrailPos),
	currentAlpha(SDL_ALPHA_OPAQUE)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	CCS->soundh->playSound(soundBase::OBELISK);

	quitb = std::make_shared<CButton>(Point(670, 538), "IOK6432.DEF", CButton::tooltip(CGI->generaltexth->allTexts[599]), std::bind(&CPuzzleWindow::close, this), SDLK_RETURN);
	quitb->assignedKeys.insert(SDLK_ESCAPE);
	quitb->setBorderColor(Colors::METALLIC_GOLD);

	logo = std::make_shared<CPicture>("PUZZLOGO", 607, 3);
	title = std::make_shared<CLabel>(700, 95, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[463]);
	resDataBar = std::make_shared<CResDataBar>("ARESBAR.bmp", 3, 575, 32, 2, 85, 85);

	int faction = LOCPLINT->cb->getStartInfo()->playerInfos.find(LOCPLINT->playerID)->second.castle;

	auto & puzzleMap = (*CGI->townh)[faction]->puzzleMap;

	for(auto & elem : puzzleMap)
	{
		const SPuzzleInfo & info = elem;

		auto piece = std::make_shared<CPicture>(info.filename, info.x, info.y);

		//piece that will slowly disappear
		if(info.whenUncovered <= GameConstants::PUZZLE_MAP_PIECES * discoveredRatio)
		{
			piecesToRemove.push_back(piece);
			piece->needRefresh = true;
			piece->recActions = piece->recActions & ~SHOWALL;
			SDL_SetSurfaceBlendMode(piece->bg,SDL_BLENDMODE_BLEND);
		}
		else
		{
			visiblePieces.push_back(piece);
		}
	}
}

void CPuzzleWindow::showAll(SDL_Surface * to)
{
	int3 moveInt = int3(8, 9, 0);
	Rect mapRect = genRect(544, 591, pos.x + 8, pos.y + 7);
	int3 topTile = grailPos - moveInt;

	MapDrawingInfo info(topTile, LOCPLINT->cb->getVisibilityMap(), &mapRect);
	info.puzzleMode = true;
	info.grailPos = grailPos;
	CGI->mh->drawTerrainRectNew(to, &info);

	CWindowObject::showAll(to);
}

void CPuzzleWindow::show(SDL_Surface * to)
{
	static int animSpeed = 2;

	if(currentAlpha < animSpeed)
	{
		piecesToRemove.clear();
	}
	else
	{
		//update disappearing puzzles
		for(auto & piece : piecesToRemove)
			piece->setAlpha(currentAlpha);
		currentAlpha -= animSpeed;
	}
	CWindowObject::show(to);
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
	icon->setFrame(parent->army->getCreature(SlotID(id))->idNumber + 2);
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
	icon = std::make_shared<CAnimImage>("TWCRPORT", parent->army->getCreature(SlotID(id))->idNumber + 2);
	count = std::make_shared<CLabel>(28, 76,FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(size));
}

void CTransformerWindow::makeDeal()
{
	for(auto & elem : items)
	{
		if(!elem->left)
			LOCPLINT->cb->trade(town, EMarketMode::CREATURE_UNDEAD, elem->id, 0, 0, hero);
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

CTransformerWindow::CTransformerWindow(const CGHeroInstance * _hero, const CGTownInstance * _town)
	: CStatusbarWindow(PLAYER_COLORED, "SKTRNBK"),
	hero(_hero),
	town(_town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	if(hero)
		army = hero;
	else
		army = town;

	for(int i=0; i<GameConstants::ARMY_SIZE; i++)
	{
		if(army->getCreature(SlotID(i)))
			items.push_back(std::make_shared<CItem>(this, army->getStackCount(SlotID(i)), i));
	}

	all = std::make_shared<CButton>(Point(146, 416), "ALTARMY.DEF", CGI->generaltexth->zelp[590], [&](){ addAll(); }, SDLK_a);
	convert = std::make_shared<CButton>(Point(269, 416), "ALTSACR.DEF", CGI->generaltexth->zelp[591], [&](){ makeDeal(); }, SDLK_RETURN);
	cancel = std::make_shared<CButton>(Point(392, 416), "ICANCEL.DEF", CGI->generaltexth->zelp[592], [&](){ close(); },SDLK_ESCAPE);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	titleLeft = std::make_shared<CLabel>(153, 29,FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[485]);//holding area
	titleRight = std::make_shared<CLabel>(153+295, 29, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[486]);//transformer
	helpLeft = std::make_shared<CTextBox>(CGI->generaltexth->allTexts[487], Rect(26,  56, 255, 40), 0, FONT_MEDIUM, CENTER, Colors::YELLOW);//move creatures to create skeletons
	helpRight = std::make_shared<CTextBox>(CGI->generaltexth->allTexts[488], Rect(320, 56, 255, 40), 0, FONT_MEDIUM, CENTER, Colors::YELLOW);//creatures here will become skeletons
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

	name = std::make_shared<CLabel>(22, -13, FONT_SMALL, CENTER, Colors::WHITE, CGI->skillh->skillName(ID));
	level = std::make_shared<CLabel>(22, 57, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->levels[0]);

	pos.h = icon->pos.h;
	pos.w = icon->pos.w;
}

void CUniversityWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		if(state() == 2)
			GH.pushIntT<CUnivConfirmWindow>(parent, ID, LOCPLINT->cb->getResourceAmount(Res::GOLD) >= 2000);
	}
}

void CUniversityWindow::CItem::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CRClickPopup::createAndPush(CGI->skillh->skillInfo(ID, 1), std::make_shared<CComponent>(CComponent::secskill, ID, 1));
	}
}

void CUniversityWindow::CItem::hover(bool on)
{
	if(on)
		GH.statusbar->setText(CGI->skillh->skillName(ID));
	else
		GH.statusbar->clear();
}

int CUniversityWindow::CItem::state()
{
	if(parent->hero->getSecSkillLevel(SecondarySkill(ID)))//hero know this skill
		return 1;
	if(!parent->hero->canLearnSkill())//can't learn more skills
		return 0;
	if(parent->hero->type->heroClass->secSkillProbability[ID]==0)//can't learn this skill (like necromancy for most of non-necros)
		return 0;
	return 2;
}

void CUniversityWindow::CItem::showAll(SDL_Surface * to)
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

	if(market->o->ID == Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance *>(_market);

		if(town)
		{
			auto faction = town->town->faction->index;
			auto bid = town->town->getSpecialBuilding(BuildingSubID::MAGIC_UNIVERSITY)->bid;
			titlePic = std::make_shared<CAnimImage>((*CGI->townh)[faction]->town->clientInfo.buildingsIcons, bid);
		}
		else
			titlePic = std::make_shared<CAnimImage>((*CGI->townh)[ETownType::CONFLUX]->town->clientInfo.buildingsIcons, BuildingID::MAGIC_UNIVERSITY);
	}
	else
		titlePic = std::make_shared<CPicture>("UNIVBLDG");

	titlePic->center(Point(232 + pos.x, 76 + pos.y));

	clerkSpeech = std::make_shared<CTextBox>(CGI->generaltexth->allTexts[603], Rect(24, 129, 413, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);
	title = std::make_shared<CLabel>(231, 26, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[602]);

	std::vector<int> goods = market->availableItemsIds(EMarketMode::RESOURCE_SKILL);
	assert(goods.size() == 4);

	for(int i=0; i<goods.size(); i++)//prepare clickable items
		items.push_back(std::make_shared<CItem>(this, goods[i], 54+i*104, 234));

	cancel = std::make_shared<CButton>(Point(200, 313), "IOKAY.DEF", CGI->generaltexth->zelp[632], [&](){ close(); }, SDLK_RETURN);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUniversityWindow::makeDeal(int skill)
{
	LOCPLINT->cb->trade(market->o, EMarketMode::RESOURCE_SKILL, 6, skill, 1, hero);
}


CUnivConfirmWindow::CUnivConfirmWindow(CUniversityWindow * owner_, int SKILL, bool available)
	: CStatusbarWindow(PLAYER_COLORED, "UNIVERS2.PCX"),
	owner(owner_)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	std::string text = CGI->generaltexth->allTexts[608];
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->skillh->skillName(SKILL));
	boost::replace_first(text, "%d", "2000");

	clerkSpeech = std::make_shared<CTextBox>(text, Rect(24, 129, 413, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);

	name = std::make_shared<CLabel>(230, 37,  FONT_SMALL, CENTER, Colors::WHITE, CGI->skillh->skillName(SKILL));
	icon = std::make_shared<CAnimImage>("SECSKILL", SKILL*3+3, 0, 211, 51);
	level = std::make_shared<CLabel>(230, 107, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->levels[1]);

	costIcon = std::make_shared<CAnimImage>("RESOURCE", Res::GOLD, 0, 210, 210);
	cost = std::make_shared<CLabel>(230, 267, FONT_SMALL, CENTER, Colors::WHITE, "2000");

	std::string hoverText = CGI->generaltexth->allTexts[609];
	boost::replace_first(hoverText, "%s", CGI->generaltexth->levels[0]+ " " + CGI->skillh->skillName(SKILL));

	text = CGI->generaltexth->zelp[633].second;
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->skillh->skillName(SKILL));
	boost::replace_first(text, "%d", "2000");

	confirm = std::make_shared<CButton>(Point(148, 299), "IBY6432.DEF", CButton::tooltip(hoverText, text), [=](){makeDeal(SKILL);}, SDLK_RETURN);
	confirm->block(!available);

	cancel = std::make_shared<CButton>(Point(252,299), "ICANCEL.DEF", CGI->generaltexth->zelp[631], [&](){ close(); }, SDLK_ESCAPE);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
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
	quit = std::make_shared<CButton>(Point(399, 314), "IOK6432.DEF", CButton::tooltip(CGI->generaltexth->tcommands[8], ""), [&](){ close(); }, SDLK_RETURN);

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
			boost::algorithm::replace_first(titleText, "%s", up->Slots().begin()->second->type->namePl);
		}
		else
		{
			logGlobal->error("Invalid armed instance for garrison window.");
		}
	}
	title = std::make_shared<CLabel>(275, 30, FONT_BIG, CENTER, Colors::YELLOW, titleText);

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

	title = std::make_shared<CLabel>(325, 32, FONT_BIG, CENTER, Colors::YELLOW, fort->getObjectName());

	heroPic = std::make_shared<CHeroArea>(30, 60, hero);

	for(int i=0; i < resCount; i++)
	{
		totalIcons[i] = std::make_shared<CAnimImage>("SMALRES", i, 0, 104 + 76 * i, 237);
		totalLabels[i] = std::make_shared<CLabel>(166 + 76 * i, 253, FONT_SMALL, BOTTOMRIGHT);
	}

	for(int i = 0; i < slotsCount; i++)
	{
		upgrade[i] = std::make_shared<CButton>(Point(107 + i * 76, 171), "", CButton::tooltip(getTextForSlot(SlotID(i))), [=](){ makeDeal(SlotID(i)); }, SDLK_1 + i);
		for(auto image : { "APHLF1R.DEF", "APHLF1Y.DEF", "APHLF1G.DEF" })
			upgrade[i]->addImage(image);

		for(int j : {0,1})
		{
			slotIcons[i][j] = std::make_shared<CAnimImage>("SMALRES", 0, 0, 104 + 76 * i, 128 + 20 * j);
			slotLabels[i][j] = std::make_shared<CLabel>(168 + 76 * i, 144 + 20 * j, FONT_SMALL, BOTTOMRIGHT);
		}
	}

	upgradeAll = std::make_shared<CButton>(Point(30, 231), "", CButton::tooltip(CGI->generaltexth->allTexts[432]), [&](){ makeDeal(SlotID(slotsCount));}, SDLK_0);
	for(auto image : { "APHLF4R.DEF", "APHLF4Y.DEF", "APHLF4G.DEF" })
		upgradeAll->addImage(image);

	quit = std::make_shared<CButton>(Point(294, 275), "IOKAY.DEF", CButton::tooltip(), std::bind(&CHillFortWindow::close, this), SDLK_RETURN);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	garr = std::make_shared<CGarrisonInt>(108, 60, 18, Point(), hero, nullptr);
	updateGarrisons();
}

void CHillFortWindow::updateGarrisons()
{
	std::array<TResources, slotsCount> costs;// costs [slot ID] [resource ID] = resource count for upgrade

	TResources totalSumm; // totalSum[resource ID] = value
	totalSumm.resize(GameConstants::RESOURCE_QUANTITY);

	for(int i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		totalSumm[i]=0;

	for(int i=0; i<slotsCount; i++)
	{
		costs[i].clear();
		int newState = getState(SlotID(i));
		if(newState != -1)
		{
			UpgradeInfo info;
			LOCPLINT->cb->getUpgradeInfo(hero, SlotID(i), info);
			if(info.newID.size())//we have upgrades here - update costs
			{
				costs[i] = info.cost[0] * hero->getStackCount(SlotID(i));
				totalSumm += costs[i];
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

		if(!totalSumm.canBeAfforded(myRes))
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

					slotLabels[i][j]->setText(boost::lexical_cast<std::string>(val));
					j++;
				}
			}
			else//free upgrade - print gold image and "Free" text
			{
				slotIcons[i][0]->visible = true;
				slotIcons[i][0]->setFrame(Res::GOLD);
				slotLabels[i][0]->setText(CGI->generaltexth->allTexts[344]);
			}
		}
	}

	for(int i = 0; i < resCount; i++)
	{
		if(totalSumm[i] == 0)
		{
			totalIcons[i]->visible = false;
			totalLabels[i]->setText("");
		}
		else
		{
			totalIcons[i]->visible = true;
			totalLabels[i]->setText(boost::lexical_cast<std::string>(totalSumm[i]));
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
					LOCPLINT->cb->getUpgradeInfo(hero, SlotID(i), info);
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
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->nameSing);
	else
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->namePl);

	return str;
}

int CHillFortWindow::getState(SlotID slot)
{
	TResources myRes = LOCPLINT->cb->getResourceAmount();

	if(hero->slotEmpty(slot))//no creature here
		return -1;

	UpgradeInfo info;
	LOCPLINT->cb->getUpgradeInfo(hero, slot, info);
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
	type |= BLOCK_ADV_HOTKEYS;

	SThievesGuildInfo tgi; //info to be displayed
	LOCPLINT->cb->getThievesGuildInfo(tgi, owner);

	exitb = std::make_shared<CButton>(Point(748, 556), "TPMAGE1", CButton::tooltip(CGI->generaltexth->allTexts[600]), [&](){ close();}, SDLK_RETURN);
	exitb->assignedKeys.insert(SDLK_ESCAPE);
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
		rowHeaders.push_back(std::make_shared<CLabel>(135, y, FONT_MEDIUM, CENTER, Colors::YELLOW, text));
	}

	auto PRSTRIPS = std::make_shared<CAnimation>("PRSTRIPS");
	PRSTRIPS->preload();

	for(int g=1; g<tgi.playerColors.size(); ++g)
		columnBackgrounds.push_back(std::make_shared<CAnimImage>(PRSTRIPS, g-1, 0, 250 + 66*g, 7));

	for(int g=0; g<tgi.playerColors.size(); ++g)
		columnHeaders.push_back(std::make_shared<CLabel>(283 + 66*g, 24, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[16+g]));

	auto itgflags = std::make_shared<CAnimation>("itgflags");
	itgflags->preload();

	//printing flags
	for(int g = 0; g < ARRAY_COUNT(fields); ++g) //by lines
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
					0, FONT_TINY, TOPLEFT, Colors::WHITE));

				for(int i=0; i<iter.second.details->primskills.size(); ++i)
				{
					primSkillValues.push_back(std::make_shared<CLabel>(310 + 66 * counter, 407 + 11*i, FONT_TINY, BOTTOMRIGHT, Colors::WHITE,
							   boost::lexical_cast<std::string>(iter.second.details->primskills[i])));
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

		personalities.push_back(std::make_shared<CLabel>(283 + 66*counter, 459, FONT_SMALL, CENTER, Colors::WHITE, text));

		counter++;
	}
}

CObjectListWindow::CItem::CItem(CObjectListWindow * _parent, size_t _id, std::string _text)
	: CIntObject(LCLICK),
	parent(_parent),
	index(_id)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	border = std::make_shared<CPicture>("TPGATES");
	pos = border->pos;

	type |= REDRAW_PARENT;

	text = std::make_shared<CLabel>(pos.w/2, pos.h/2, FONT_SMALL, CENTER, Colors::WHITE, _text);
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

CObjectListWindow::CObjectListWindow(const std::vector<int> & _items, std::shared_ptr<CIntObject> titleWidget_, std::string _title, std::string _descr, std::function<void(int)> Callback, size_t initialSelection)
	: CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback),
	selected(initialSelection)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	items.reserve(_items.size());
	for(int id : _items)
	{
		items.push_back(std::make_pair(id, CGI->mh->map->objects[id]->getObjectName()));
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

	title = std::make_shared<CLabel>(152, 27, FONT_BIG, CENTER, Colors::YELLOW, _title);
	descr = std::make_shared<CLabel>(145, 133, FONT_SMALL, CENTER, Colors::WHITE, _descr);
	exit = std::make_shared<CButton>( Point(228, 402), "ICANCEL.DEF", CButton::tooltip(), std::bind(&CObjectListWindow::exitPressed, this), SDLK_ESCAPE);

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

	ok = std::make_shared<CButton>(Point(15, 402), "IOKAY.DEF", CButton::tooltip(), std::bind(&CObjectListWindow::elementSelected, this), SDLK_RETURN);
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

void CObjectListWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED)
		return;

	int sel = static_cast<int>(selected);

	switch(key.keysym.sym)
	{
	break; case SDLK_UP:
		sel -=1;

	break; case SDLK_DOWN:
		sel +=1;

	break; case SDLK_PAGEUP:
		sel -=9;

	break; case SDLK_PAGEDOWN:
		sel +=9;

	break; case SDLK_HOME:
		sel = 0;

	break; case SDLK_END:
		sel = static_cast<int>(items.size());

	break; default:
		return;
	}

	vstd::abetween(sel, 0, items.size()-1);
	list->scrollTo(sel);
	changeSelection(sel);
}

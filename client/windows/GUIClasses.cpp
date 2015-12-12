#include "StdInc.h"
#include "GUIClasses.h"

#include "CAdvmapInterface.h"
#include "CCastleInterface.h"
#include "CCreatureWindow.h"
#include "CHeroWindow.h"
#include "CSpellWindow.h"

#include "../CBitmapHandler.h"
#include "../CDefHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CPreGame.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../mapHandler.h"

#include "../battle/CBattleInterfaceClasses.h"
#include "../battle/CBattleInterface.h"
#include "../battle/CCreatureAnimation.h"

#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/CCursorHandler.h"

#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"

#include "../lib/BattleState.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/CondSh.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CStopWatch.h"
#include "../lib/CTownHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/HeroBonus.h"
#include "../lib/mapping/CMap.h"
#include "../lib/NetPacksBase.h"
#include "../lib/StartInfo.h"

/*
 * GUIClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace CSDL_Ext;

std::list<CFocusable*> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

#undef min
#undef max

CRecruitmentWindow::CCreatureCard::CCreatureCard(CRecruitmentWindow *window, const CCreature *crea, int totalAmount):
	CIntObject(LCLICK | RCLICK),
	parent(window),
	selected(false),
	creature(crea),
	amount(totalAmount)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pic = new CCreaturePic(1,1, creature, true, true);
	// 1 + 1 px for borders
	pos.w = pic->pos.w + 2;
	pos.h = pic->pos.h + 2;
}

void CRecruitmentWindow::CCreatureCard::select(bool on)
{
	selected = on;
	redraw();
}

void CRecruitmentWindow::CCreatureCard::clickLeft(tribool down, bool previousState)
{
	if (down)
		parent->select(this);
}

void CRecruitmentWindow::CCreatureCard::clickRight(tribool down, bool previousState)
{
	if (down)
		GH.pushInt(new CStackWindow(creature, true));
}

void CRecruitmentWindow::CCreatureCard::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if (selected)
		drawBorder(to, pos, int3(248, 0, 0));
	else
		drawBorder(to, pos, int3(232, 212, 120));
}

CRecruitmentWindow::CCostBox::CCostBox(Rect position, std::string title)
{
	type |= REDRAW_PARENT;
	pos = position + pos;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CLabel(pos.w/2, 10, FONT_SMALL, CENTER, Colors::WHITE, title);
}

void CRecruitmentWindow::CCostBox::set(TResources res)
{
	//just update values
	for(auto & item : resources)
	{
		item.second.first->setText(boost::lexical_cast<std::string>(res[item.first]));
	}
}

void CRecruitmentWindow::CCostBox::createItems(TResources res)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	for(auto & curr : resources)
	{
		delete curr.second.first;
		delete curr.second.second;
	}
	resources.clear();

	TResources::nziterator iter(res);
	while (iter.valid())
	{
		CAnimImage * image = new CAnimImage("RESOURCE", iter->resType);
		CLabel * text = new CLabel(15, 43, FONT_SMALL, CENTER, Colors::WHITE, "0");

		resources.insert(std::make_pair(iter->resType, std::make_pair(text, image)));
		iter++;
	}

	if (!resources.empty())
	{
		int curx = pos.w / 2 - (16 * resources.size()) - (8 * (resources.size() - 1));
		//reverse to display gold as first resource
		for (auto & res : boost::adaptors::reverse(resources))
		{
			res.second.first->moveBy(Point(curx, 22));
			res.second.second->moveBy(Point(curx, 22));
			curx += 48;
		}
	}
	redraw();
}

void CRecruitmentWindow::select(CCreatureCard *card)
{
	if (card == selected)
		return;

	if (selected)
		selected->select(false);

	selected = card;

	if (selected)
		selected->select(true);

	if (card)
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

	if(!dstslot.validSlot() && !vstd::contains(CGI->arth->bigArtifacts,CGI->arth->creatureToMachineID(crid))) //no available slot
	{
		std::string txt;
		if(dst->ID == Obj::HERO)
		{
			txt = CGI->generaltexth->allTexts[425]; //The %s would join your hero, but there aren't enough provisions to support them.
			boost::algorithm::replace_first(txt, "%s", slider->getValue() > 1 ? CGI->creh->creatures[crid]->namePl : CGI->creh->creatures[crid]->nameSing);
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

CRecruitmentWindow::CRecruitmentWindow(const CGDwelling *Dwelling, int Level, const CArmedInstance *Dst, const std::function<void(CreatureID,int)> &Recruit, int y_offset):
	CWindowObject(PLAYER_COLORED, "TPRCRT"),
	onRecruit(Recruit),
	level(Level),
	dst(Dst),
	selected(nullptr),
	dwelling(Dwelling)
{
	moveBy(Point(0, y_offset));

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	slider = new CSlider(Point(176,279),135,std::bind(&CRecruitmentWindow::sliderMoved,this, _1),0,0,0,true);

	maxButton = new CButton(Point(134, 313), "IRCBTNS.DEF", CGI->generaltexth->zelp[553], std::bind(&CSlider::moveToMax,slider), SDLK_m);
	buyButton = new CButton(Point(212, 313), "IBY6432.DEF", CGI->generaltexth->zelp[554], std::bind(&CRecruitmentWindow::buy,this), SDLK_RETURN);
	cancelButton = new CButton(Point(290, 313), "ICN6432.DEF", CGI->generaltexth->zelp[555], std::bind(&CRecruitmentWindow::close,this), SDLK_ESCAPE);

	title = new CLabel(243, 32, FONT_BIG, CENTER, Colors::YELLOW);
	availableValue = new CLabel(205, 253, FONT_SMALL, CENTER, Colors::WHITE);
	toRecruitValue = new CLabel(279, 253, FONT_SMALL, CENTER, Colors::WHITE);

	costPerTroopValue =  new CCostBox(Rect(65, 222, 97, 74), CGI->generaltexth->allTexts[346]);
	totalCostValue = new CCostBox(Rect(323, 222, 97, 74), CGI->generaltexth->allTexts[466]);

	new CLabel(205, 233, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[465]); //available t
	new CLabel(279, 233, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[16]); //recruit t

	availableCreaturesChanged();
}

void CRecruitmentWindow::availableCreaturesChanged()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	size_t selectedIndex = 0;

	if (!cards.empty() && selected) // find position of selected item
		selectedIndex = std::find(cards.begin(), cards.end(), selected) - cards.begin();

	//deselect card
	select(nullptr);

	//delete old cards
	for(auto & card : cards)
		delete card;
	cards.clear();

	for(int i=0; i<dwelling->creatures.size(); i++)
	{
		//find appropriate level
		if(level >= 0 && i != level)
			continue;

		int amount = dwelling->creatures[i].first;

		//create new cards
		for(auto & creature : boost::adaptors::reverse(dwelling->creatures[i].second))
			cards.push_back(new CCreatureCard(this, CGI->creh->creatures[creature], amount));
	}

	assert(!cards.empty());

	const int creatureWidth = 102;

	//normal distance between cards - 18px
	int requiredSpace = 18;
	//maximum distance we can use without reaching window borders
	int availableSpace = pos.w - 50 - creatureWidth * cards.size();

	if (cards.size() > 1) // avoid division by zero
		availableSpace /= cards.size() - 1;
	else
		availableSpace = 0;

	assert(availableSpace >= 0);

	const int spaceBetween = std::min(requiredSpace, availableSpace);
	const int totalCreatureWidth = spaceBetween + creatureWidth;

	//now we know total amount of cards and can move them to correct position
	int curx = pos.w / 2 - (creatureWidth*cards.size()/2) - (spaceBetween*(cards.size()-1)/2);
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
	if (!selected)
		return;

	buyButton->block(!to);
	availableValue->setText(boost::lexical_cast<std::string>(selected->amount - to));
	toRecruitValue->setText(boost::lexical_cast<std::string>(to));

	totalCostValue->set(selected->creature->cost * to);
}

CSplitWindow::CSplitWindow(const CCreature * creature, std::function<void(int, int)> callback_,
						   int leftMin_, int rightMin_, int leftAmount_, int rightAmount_):
	CWindowObject(PLAYER_COLORED, "GPUCRDIV"),
	callback(callback_),
	leftAmount(leftAmount_),
	rightAmount(rightAmount_),
	leftMin(leftMin_),
	rightMin(rightMin_),
	slider(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	int total = leftAmount + rightAmount;
	int leftMax = total - rightMin;
	int rightMax = total - leftMin;

	ok = new CButton(Point(20, 263), "IOK6432", CButton::tooltip(), std::bind(&CSplitWindow::apply, this), SDLK_RETURN);
	cancel = new CButton(Point(214, 263), "ICN6432", CButton::tooltip(), std::bind(&CSplitWindow::close, this), SDLK_ESCAPE);

	int sliderPositions = total - leftMin - rightMin;

	leftInput = new CTextInput(Rect(20, 218, 100, 36), FONT_BIG, std::bind(&CSplitWindow::setAmountText, this, _1, true));
	rightInput = new CTextInput(Rect(176, 218, 100, 36), FONT_BIG, std::bind(&CSplitWindow::setAmountText, this, _1, false));

	//add filters to allow only number input
	leftInput->filters += std::bind(&CTextInput::numberFilter, _1, _2, leftMin, leftMax);
	rightInput->filters += std::bind(&CTextInput::numberFilter, _1, _2, rightMin, rightMax);

	leftInput->setText(boost::lexical_cast<std::string>(leftAmount), false);
	rightInput->setText(boost::lexical_cast<std::string>(rightAmount), false);

	animLeft = new CCreaturePic(20, 54, creature, true, false);
	animRight = new CCreaturePic(177, 54,creature, true, false);

	slider = new CSlider(Point(21, 194), 257, std::bind(&CSplitWindow::sliderMoved, this, _1), 0, sliderPositions, rightAmount - rightMin, true);

	std::string title = CGI->generaltexth->allTexts[256];
	boost::algorithm::replace_first(title,"%s", creature->namePl);
	new CLabel(150, 34, FONT_BIG, CENTER, Colors::YELLOW, title);
}

void CSplitWindow::setAmountText(std::string text, bool left)
{
	int amount = 0;
	if (text.length())
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
		if (amount > total)
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

CLevelWindow::CLevelWindow(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, std::function<void(ui32)> callback):
	CWindowObject(PLAYER_COLORED, "LVLUPBKG"),
	cb(callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	LOCPLINT->showingDialog->setn(true);

	new CAnimImage("PortraitsLarge", hero->portrait, 0, 170, 66);
	new CButton(Point(297, 413), "IOKAY",  CButton::tooltip(), std::bind(&CLevelWindow::close, this), SDLK_RETURN);

	//%s has gained a level.
	new CLabel(192, 33, FONT_MEDIUM, CENTER, Colors::WHITE,
			   boost::str(boost::format(CGI->generaltexth->allTexts[444]) % hero->name));

	//%s is now a level %d %s.
	new CLabel(192, 162, FONT_MEDIUM, CENTER, Colors::WHITE,
			   boost::str(boost::format(CGI->generaltexth->allTexts[445]) % hero->name % hero->level % hero->type->heroClass->name));

	new CAnimImage("PSKIL42", pskill, 0, 174, 190);

	new CLabel(192, 253, FONT_MEDIUM, CENTER, Colors::WHITE,
			   CGI->generaltexth->primarySkillNames[pskill] + " +1");

	if (!skills.empty())
	{
		std::vector<CSelectableComponent *> comps;

		for(auto & skill : skills)
		{
			comps.push_back(new CSelectableComponent(
								CComponent::secskill,
								skill,
								hero->getSecSkillLevel( SecondarySkill(skill) )+1,
								CComponent::medium));
		}
		box = new CComponentBox(comps, Rect(75, 300, pos.w - 150, 100));
	}
	else
		box = nullptr;
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

CSystemOptionsWindow::CSystemOptionsWindow():
	CWindowObject(PLAYER_COLORED, "SysOpBck"),
	onFullscreenChanged(settings.listen["video"]["fullscreen"])
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	title = new CLabel(242, 32, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[568]);

	const JsonNode & texts = CGI->generaltexth->localizedTexts["systemOptions"];

	//left window section
	leftGroup =  new CLabelGroup(FONT_MEDIUM, CENTER, Colors::YELLOW);
	leftGroup->add(122,  64, CGI->generaltexth->allTexts[569]);
	leftGroup->add(122, 130, CGI->generaltexth->allTexts[570]);
	leftGroup->add(122, 196, CGI->generaltexth->allTexts[571]);
	leftGroup->add(122, 262, texts["resolutionButton"]["label"].String());
	leftGroup->add(122, 347, CGI->generaltexth->allTexts[394]);
	leftGroup->add(122, 412, CGI->generaltexth->allTexts[395]);

	//right section
	rightGroup = new CLabelGroup(FONT_MEDIUM, TOPLEFT, Colors::WHITE);
	rightGroup->add(282, 57,  CGI->generaltexth->allTexts[572]);
	rightGroup->add(282, 89,  CGI->generaltexth->allTexts[573]);
	rightGroup->add(282, 121, CGI->generaltexth->allTexts[574]);
	rightGroup->add(282, 153, CGI->generaltexth->allTexts[577]);
	rightGroup->add(282, 185, texts["creatureWindowButton"]["label"].String());
	rightGroup->add(282, 217, texts["fullscreenButton"]["label"].String());

	//setting up buttons
	load = new CButton (Point(246,  298), "SOLOAD.DEF", CGI->generaltexth->zelp[321], [&] { bloadf(); }, SDLK_l);
	load->setImageOrder(1, 0, 2, 3);

	save = new CButton (Point(357, 298), "SOSAVE.DEF", CGI->generaltexth->zelp[322], [&] { bsavef(); }, SDLK_s);
	save->setImageOrder(1, 0, 2, 3);

	restart = new CButton (Point(246, 357), "SORSTRT", CGI->generaltexth->zelp[323], [&] { brestartf(); }, SDLK_r);
	restart->setImageOrder(1, 0, 2, 3);

	mainMenu = new CButton (Point(357, 357), "SOMAIN.DEF", CGI->generaltexth->zelp[320], [&] { bmainmenuf(); }, SDLK_m);
	mainMenu->setImageOrder(1, 0, 2, 3);

	quitGame = new CButton (Point(246, 415), "soquit.def", CGI->generaltexth->zelp[324], [&] { bquitf(); }, SDLK_q);
	quitGame->setImageOrder(1, 0, 2, 3);

	backToMap = new CButton ( Point(357, 415), "soretrn.def", CGI->generaltexth->zelp[325], [&] { breturnf(); }, SDLK_RETURN);
	backToMap->setImageOrder(1, 0, 2, 3);
	backToMap->assignedKeys.insert(SDLK_ESCAPE);

	heroMoveSpeed = new CToggleGroup(0);
	heroMoveSpeed->addToggle(1, new CToggleButton(Point( 28, 77), "sysopb1.def", CGI->generaltexth->zelp[349]));
	heroMoveSpeed->addToggle(2, new CToggleButton(Point( 76, 77), "sysopb2.def", CGI->generaltexth->zelp[350]));
	heroMoveSpeed->addToggle(4, new CToggleButton(Point(124, 77), "sysopb3.def", CGI->generaltexth->zelp[351]));
	heroMoveSpeed->addToggle(8, new CToggleButton(Point(172, 77), "sysopb4.def", CGI->generaltexth->zelp[352]));
	heroMoveSpeed->setSelected(settings["adventure"]["heroSpeed"].Float());
	heroMoveSpeed->addCallback(std::bind(&setIntSetting, "adventure", "heroSpeed", _1));

	enemyMoveSpeed = new CToggleGroup(0);
	enemyMoveSpeed->addToggle(2, new CToggleButton(Point( 28, 144), "sysopb5.def", CGI->generaltexth->zelp[353]));
	enemyMoveSpeed->addToggle(4, new CToggleButton(Point( 76, 144), "sysopb6.def", CGI->generaltexth->zelp[354]));
	enemyMoveSpeed->addToggle(8, new CToggleButton(Point(124, 144), "sysopb7.def", CGI->generaltexth->zelp[355]));
	enemyMoveSpeed->addToggle(0, new CToggleButton(Point(172, 144), "sysopb8.def", CGI->generaltexth->zelp[356]));
	enemyMoveSpeed->setSelected(settings["adventure"]["enemySpeed"].Float());
	enemyMoveSpeed->addCallback(std::bind(&setIntSetting, "adventure", "enemySpeed", _1));

	mapScrollSpeed = new CToggleGroup(0);
	mapScrollSpeed->addToggle(1, new CToggleButton(Point( 28, 210), "sysopb9.def", CGI->generaltexth->zelp[357]));
	mapScrollSpeed->addToggle(2, new CToggleButton(Point( 92, 210), "sysob10.def", CGI->generaltexth->zelp[358]));
	mapScrollSpeed->addToggle(4, new CToggleButton(Point(156, 210), "sysob11.def", CGI->generaltexth->zelp[359]));
	mapScrollSpeed->setSelected(settings["adventure"]["scrollSpeed"].Float());
	mapScrollSpeed->addCallback(std::bind(&setIntSetting, "adventure", "scrollSpeed", _1));

	musicVolume = new CVolumeSlider(Point(29, 359), "syslb.def", CCS->musich->getVolume(), &CGI->generaltexth->zelp[326]);
	musicVolume->addCallback(std::bind(&setIntSetting, "general", "music", _1));

	effectsVolume = new CVolumeSlider(Point(29, 425), "syslb.def", CCS->soundh->getVolume(), &CGI->generaltexth->zelp[336]);
	effectsVolume->addCallback(std::bind(&setIntSetting, "general", "sound", _1));

	showReminder = new CToggleButton(Point(246, 87), "sysopchk.def", CGI->generaltexth->zelp[361],
			[&] (bool value) { setBoolSetting("adventure", "heroReminder", value); });

	quickCombat = new CToggleButton(Point(246, 87+32), "sysopchk.def", CGI->generaltexth->zelp[362],
			[&] (bool value) { setBoolSetting("adventure", "quickCombat", value); });

	spellbookAnim = new CToggleButton(Point(246, 87+64), "sysopchk.def", CGI->generaltexth->zelp[364],
			[&] (bool value) { setBoolSetting("video", "spellbookAnimation", value); });

	fullscreen = new CToggleButton(Point(246, 215), "sysopchk.def", CButton::tooltip(texts["fullscreenButton"]),
			[&] (bool value) { setBoolSetting("video", "fullscreen", value); });

	showReminder->setSelected(settings["adventure"]["heroReminder"].Bool());
	quickCombat->setSelected(settings["adventure"]["quickCombat"].Bool());
	spellbookAnim->setSelected(settings["video"]["spellbookAnimation"].Bool());
	fullscreen->setSelected(settings["video"]["fullscreen"].Bool());

	onFullscreenChanged([&](const JsonNode &newState){ fullscreen->setSelected(newState.Bool());});

	gameResButton = new CButton(Point(28, 275),"buttons/resolution", CButton::tooltip(texts["resolutionButton"]),
											std::bind(&CSystemOptionsWindow::selectGameRes, this), SDLK_g);

	std::string resText;
	resText += boost::lexical_cast<std::string>(settings["video"]["screenRes"]["width"].Float());
	resText += "x";
	resText += boost::lexical_cast<std::string>(settings["video"]["screenRes"]["height"].Float());
	gameResLabel = new CLabel(170, 292, FONT_MEDIUM, CENTER, Colors::YELLOW, resText);

}

void CSystemOptionsWindow::selectGameRes()
{
	std::vector<std::string> items;
	const JsonNode & texts = CGI->generaltexth->localizedTexts["systemOptions"]["resolutionMenu"];

	for( config::CConfigHandler::GuiOptionsMap::value_type& value : conf.guiOptions)
	{
		std::string resX = boost::lexical_cast<std::string>(value.first.first);
		std::string resY = boost::lexical_cast<std::string>(value.first.second);
		items.push_back(resX + 'x' + resY);
	}

	GH.pushInt(new CObjectListWindow(items, nullptr, texts["label"].String(), texts["help"].String(),
			   std::bind(&CSystemOptionsWindow::setGameRes, this, _1)));
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
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this]{ closeAndPushEvent(SDL_USEREVENT, FORCE_QUIT); }, 0);
}

void CSystemOptionsWindow::breturnf()
{
	GH.popIntTotally(this);
}

void CSystemOptionsWindow::bmainmenuf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this]{ closeAndPushEvent(SDL_USEREVENT, RETURN_TO_MAIN_MENU); }, 0);
}

void CSystemOptionsWindow::bloadf()
{
	GH.popIntTotally(this);
	LOCPLINT->proposeLoadingGame();
}

void CSystemOptionsWindow::bsavef()
{
	GH.popIntTotally(this);
	GH.pushInt(new CSavingScreen(CPlayerInterface::howManyPeople > 1));
}

void CSystemOptionsWindow::brestartf()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [this]{ closeAndPushEvent(SDL_USEREVENT, RESTART_GAME); }, 0);
}

void CSystemOptionsWindow::closeAndPushEvent(int eventType, int code /*= 0*/)
{
	GH.popIntTotally(this);
	GH.pushSDLEvent(eventType, code);
}

CTavernWindow::CTavernWindow(const CGObjectInstance *TavernObj):
	CWindowObject(PLAYER_COLORED, "TPTAVERN"),
	tavernObj(TavernObj)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	std::vector<const CGHeroInstance*> h = LOCPLINT->cb->getAvailableHeroes(TavernObj);
	if(h.size() < 2)
		h.resize(2, nullptr);

	h1 = new HeroPortrait(selected,0,72,299,h[0]);
	h2 = new HeroPortrait(selected,1,162,299,h[1]);

	selected = 0;
	if (!h[0])
		selected = 1;
	if (!h[0] && !h[1])
		selected = -1;
	oldSelected = -1;

	new CLabel(200, 35, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[37]);
	new CLabel(320, 328, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(GameConstants::HERO_GOLD_COST));

	auto rumorText = boost::str(boost::format(CGI->generaltexth->allTexts[216]) % LOCPLINT->cb->getTavernRumor(tavernObj));
	new CTextBox(rumorText, Rect(32, 190, 330, 68), 0, FONT_SMALL, CENTER, Colors::WHITE);

	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
	cancel = new CButton(Point(310, 428), "ICANCEL.DEF", CButton::tooltip(CGI->generaltexth->tavernInfo[7]), std::bind(&CTavernWindow::close, this), SDLK_ESCAPE);
	recruit = new CButton(Point(272, 355), "TPTAV01.DEF", CButton::tooltip(), std::bind(&CTavernWindow::recruitb, this), SDLK_RETURN);
	thiefGuild = new CButton(Point(22, 428), "TPTAV02.DEF", CButton::tooltip(CGI->generaltexth->tavernInfo[5]), std::bind(&CTavernWindow::thievesguildb, this), SDLK_t);

	if(LOCPLINT->cb->getResourceAmount(Res::GOLD) < GameConstants::HERO_GOLD_COST) //not enough gold
	{
		recruit->addHoverText(CButton::NORMAL, CGI->generaltexth->tavernInfo[0]); //Cannot afford a Hero
		recruit->block(true);
	}
	else if(LOCPLINT->castleInt && LOCPLINT->cb->howManyHeroes(true) >= VLC->modh->settings.MAX_HEROES_AVAILABLE_PER_PLAYER)
	{
		//Cannot recruit. You already have %d Heroes.
		recruit->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->tavernInfo[1]) % LOCPLINT->cb->howManyHeroes(true)));
		recruit->block(true);
	}
	else if((!LOCPLINT->castleInt) && LOCPLINT->cb->howManyHeroes(false) >= VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER)
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
	if (LOCPLINT->castleInt)
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
	GH.pushInt( new CThievesGuildWindow(tavernObj) );
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
		HeroPortrait *sel = selected ? h2 : h1;

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
	if(previousState && !down && h)
		*_sel = _id;
}

void CTavernWindow::HeroPortrait::clickRight(tribool down, bool previousState)
{
	if(down && h)
	{
		GH.pushInt(new CRClickPopupInt(new CHeroWindow(h), true));
	}
}

CTavernWindow::HeroPortrait::HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H)
: h(H), _sel(&sel), _id(id)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	h = H;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;

	if(H)
	{
		hoverName = CGI->generaltexth->tavernInfo[4];
		boost::algorithm::replace_first(hoverName,"%s",H->name);

		int artifs = h->artifactsWorn.size() + h->artifactsInBackpack.size();
		for(int i=13; i<=17; i++) //war machines and spellbook don't count
			if(vstd::contains(h->artifactsWorn, ArtifactPosition(i)))
				artifs--;

		description = CGI->generaltexth->allTexts[215];
		boost::algorithm::replace_first(description, "%s", h->name);
		boost::algorithm::replace_first(description, "%d", boost::lexical_cast<std::string>(h->level));
		boost::algorithm::replace_first(description, "%s", h->type->heroClass->name);
		boost::algorithm::replace_first(description, "%d", boost::lexical_cast<std::string>(artifs));

		new CAnimImage("portraitsLarge", h->portrait);
	}
}

void CTavernWindow::HeroPortrait::hover( bool on )
{
	//Hoverable::hover(on);
	if(on)
		GH.statusbar->setText(hoverName);
	else
		GH.statusbar->clear();
}

void CExchangeWindow::questlog(int whichHero)
{
	CCS->curh->dragAndDropCursor(nullptr);
	LOCPLINT->showQuestLog();
}

void CExchangeWindow::prepareBackground()
{
	//printing heroes' names and levels
	auto genTitle = [](const CGHeroInstance *h)
	{
		return boost::str(boost::format(CGI->generaltexth->allTexts[138])
						  % h->name % h->level % h->type->heroClass->name);
	};

	new CLabel(147, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[0]));
	new CLabel(653, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[1]));

	//printing primary skills
	for(int g=0; g<4; ++g)
		new CAnimImage("PSKIL32", g, 0, 385, 19 + 36*g);

	//heroes related thing
	for(int b=0; b<ARRAY_COUNT(heroInst); b++)
	{
		CHeroWithMaybePickedArtifact heroWArt = CHeroWithMaybePickedArtifact(this, heroInst[b]);
		//printing primary skills' amounts
		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
			new CLabel(352 + 93 * b, 35 + 36 * m, FONT_SMALL, CENTER, Colors::WHITE,
					   boost::lexical_cast<std::string>(heroWArt.getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(m))));

		//printing secondary skills
		for(int m=0; m<heroInst[b]->secSkills.size(); ++m)
		{
			int id = heroInst[b]->secSkills[m].first;
			int level = heroInst[b]->secSkills[m].second;
			new CAnimImage("SECSK32", id*3 + level + 2 , 0, 32 + 36 * m + 454 * b, 88);
		}

		//hero's specialty
		new CAnimImage("UN32", heroInst[b]->type->imageIndex, 0, 67 + 490*b, 45);

		//experience
		new CAnimImage("PSKIL32", 4, 0, 103 + 490*b, 45);
		new CLabel(119 + 490*b, 71, FONT_SMALL, CENTER, Colors::WHITE, makeNumberShort(heroInst[b]->exp));

		//mana points
		new CAnimImage("PSKIL32", 5, 0, 139 + 490*b, 45);
		new CLabel(155 + 490*b, 71, FONT_SMALL, CENTER, Colors::WHITE, makeNumberShort(heroInst[b]->mana));
	}

	//printing portraits
	new CAnimImage("PortraitsLarge", heroInst[0]->portrait, 0, 257, 13);
	new CAnimImage("PortraitsLarge", heroInst[1]->portrait, 0, 485, 13);
}

CExchangeWindow::CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID):
	CWindowObject(PLAYER_COLORED | BORDERED, "TRADE2")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	heroInst[0] = LOCPLINT->cb->getHero(hero1);
	heroInst[1] = LOCPLINT->cb->getHero(hero2);

	prepareBackground();

	artifs[0] = new CArtifactsOfHero(Point(-334, 150));
	artifs[0]->commonInfo = new CArtifactsOfHero::SCommonPart;
	artifs[0]->commonInfo->participants.insert(artifs[0]);
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = new CArtifactsOfHero(Point(96, 150));
	artifs[1]->commonInfo = artifs[0]->commonInfo;
	artifs[1]->commonInfo->participants.insert(artifs[1]);
	artifs[1]->setHero(heroInst[1]);

	artSets.push_back(artifs[0]);
	artSets.push_back(artifs[1]);

	//primary skills
	for(int g=0; g<4; ++g)
	{
		//primary skill's clickable areas
		primSkillAreas.push_back(new LRClickableAreaWTextComp());
		primSkillAreas[g]->pos = genRect(32, 140, pos.x + 329, pos.y + 19 + 36 * g);
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->type = g;
		primSkillAreas[g]->bonusValue = -1;
		primSkillAreas[g]->baseType = 0;
		primSkillAreas[g]->hoverText = CGI->generaltexth->heroscrn[1];
		boost::replace_first(primSkillAreas[g]->hoverText, "%s", CGI->generaltexth->primarySkillNames[g]);
	}

	//heroes related thing
	for(int b=0; b<ARRAY_COUNT(heroInst); b++)
	{
		//secondary skill's clickable areas
		for(int g=0; g<heroInst[b]->secSkills.size(); ++g)
		{
			int skill = heroInst[b]->secSkills[g].first,
				level = heroInst[b]->secSkills[g].second; // <1, 3>
			secSkillAreas[b].push_back(new LRClickableAreaWTextComp());
			secSkillAreas[b][g]->pos = genRect(32, 32, pos.x + 32 + g*36 + b*454 , pos.y + 88);
			secSkillAreas[b][g]->baseType = 1;

			secSkillAreas[b][g]->type = skill;
			secSkillAreas[b][g]->bonusValue = level;
			secSkillAreas[b][g]->text = CGI->generaltexth->skillInfoTexts[skill][level-1];

			secSkillAreas[b][g]->hoverText = CGI->generaltexth->heroscrn[21];
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->generaltexth->levels[level - 1]);
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->generaltexth->skillName[skill]);
		}

		portrait[b] = new CHeroArea(257 + 228*b, 13, heroInst[b]);

		specialty[b] = new LRClickableAreaWText();
		specialty[b]->pos = genRect(32, 32, pos.x + 69 + 490*b, pos.y + 45);
		specialty[b]->hoverText = CGI->generaltexth->heroscrn[27];
		specialty[b]->text = heroInst[b]->type->specDescr;

		experience[b] = new LRClickableAreaWText();
		experience[b]->pos = genRect(32, 32, pos.x + 105 + 490*b, pos.y + 45);
		experience[b]->hoverText = CGI->generaltexth->heroscrn[9];
		experience[b]->text = CGI->generaltexth->allTexts[2].c_str();
		boost::algorithm::replace_first(experience[b]->text, "%d", boost::lexical_cast<std::string>(heroInst[b]->level));
		boost::algorithm::replace_first(experience[b]->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(heroInst[b]->level+1)));
		boost::algorithm::replace_first(experience[b]->text, "%d", boost::lexical_cast<std::string>(heroInst[b]->exp));

		spellPoints[b] = new LRClickableAreaWText();
		spellPoints[b]->pos = genRect(32, 32, pos.x + 141 + 490*b, pos.y + 45);
		spellPoints[b]->hoverText = CGI->generaltexth->heroscrn[22];
		spellPoints[b]->text = CGI->generaltexth->allTexts[205];
		boost::algorithm::replace_first(spellPoints[b]->text, "%s", heroInst[b]->name);
		boost::algorithm::replace_first(spellPoints[b]->text, "%d", boost::lexical_cast<std::string>(heroInst[b]->mana));
		boost::algorithm::replace_first(spellPoints[b]->text, "%d", boost::lexical_cast<std::string>(heroInst[b]->manaLimit()));

		//setting morale
		morale[b] = new MoraleLuckBox(true, genRect(32, 32, 176 + 490*b, 39), true);
		morale[b]->set(heroInst[b]);
		//setting luck
		luck[b] = new MoraleLuckBox(false, genRect(32, 32, 212 + 490*b, 39), true);
		luck[b]->set(heroInst[b]);
	}

	//buttons
	quit = new CButton(Point(732, 567), "IOKAY.DEF", CGI->generaltexth->zelp[600], std::bind(&CExchangeWindow::close, this), SDLK_RETURN);
	if(queryID.getNum() > 0)
		quit->addCallback([=]{ LOCPLINT->cb->selectionMade(0, queryID); });

	questlogButton[0] = new CButton(Point( 10, 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog,this, 0));
	questlogButton[1] = new CButton(Point(740, 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog,this, 1));

	Rect barRect(5, 578, 725, 18);
	ourBar = new CGStatusBar(new CPicture(*background, barRect, 5, 578, false));

	//garrison interface
	garr = new CGarrisonInt(69, 131, 4, Point(418,0), *background, Point(69,131), heroInst[0],heroInst[1], true, true);
	garr->addSplitBtn(new CButton( Point( 10, 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), std::bind(&CGarrisonInt::splitClick, garr)));
	garr->addSplitBtn(new CButton( Point(740, 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), std::bind(&CGarrisonInt::splitClick, garr)));
}

CExchangeWindow::~CExchangeWindow() //d-tor
{
	delete artifs[0]->commonInfo;
	artifs[0]->commonInfo = nullptr;
	artifs[1]->commonInfo = nullptr;
}

CShipyardWindow::CShipyardWindow(const std::vector<si32> &cost, int state, int boatType, const std::function<void()> &onBuy):
	CWindowObject(PLAYER_COLORED, "TPSHIP")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	bgWater = new CPicture("TPSHIPBK", 100, 69);

	std::string boatFilenames[3] = {"AB01_", "AB02_", "AB03_"};

	Point waterCenter = Point(bgWater->pos.x+bgWater->pos.w/2, bgWater->pos.y+bgWater->pos.h/2);
	bgShip = new CAnimImage(boatFilenames[boatType], 0, 7, 120, 96, CShowableAnim::USE_RLE);
	bgShip->center(waterCenter);

	// Create resource icons and costs.
	std::string goldValue = boost::lexical_cast<std::string>(cost[Res::GOLD]);
	std::string woodValue = boost::lexical_cast<std::string>(cost[Res::WOOD]);

	goldCost = new CLabel(118, 294, FONT_SMALL, CENTER, Colors::WHITE, goldValue);
	woodCost = new CLabel(212, 294, FONT_SMALL, CENTER, Colors::WHITE, woodValue);

	goldPic = new CAnimImage("RESOURCE", Res::GOLD, 0, 100, 244);
	woodPic = new CAnimImage("RESOURCE", Res::WOOD, 0, 196, 244);

	quit  = new CButton( Point(224, 312), "ICANCEL", CButton::tooltip(CGI->generaltexth->allTexts[599]), std::bind(&CShipyardWindow::close, this), SDLK_RETURN);
	build = new CButton( Point( 42, 312), "IBUY30",  CButton::tooltip(CGI->generaltexth->allTexts[598]), std::bind(&CShipyardWindow::close, this),SDLK_RETURN);
	build->addCallback(onBuy);

	for(Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, 1))
	{
		if(cost[i] > LOCPLINT->cb->getResourceAmount(i))
		{
			build->block(true);
			break;
		}
	}

	statusBar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	title =     new CLabel(164, 27,  FONT_BIG,    CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[13]);
	costLabel = new CLabel(164, 220, FONT_MEDIUM, CENTER, Colors::WHITE,   CGI->generaltexth->jktexts[14]);
}

CPuzzleWindow::CPuzzleWindow(const int3 &GrailPos, double discoveredRatio):
	CWindowObject(PLAYER_COLORED | BORDERED, "PUZZLE"),
	grailPos(GrailPos),
	currentAlpha(SDL_ALPHA_OPAQUE)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CCS->soundh->playSound(soundBase::OBELISK);

	quitb = new CButton(Point(670, 538), "IOK6432.DEF", CButton::tooltip(CGI->generaltexth->allTexts[599]), std::bind(&CPuzzleWindow::close, this), SDLK_RETURN);
	quitb->assignedKeys.insert(SDLK_ESCAPE);
	quitb->borderColor = Colors::METALLIC_GOLD;

	new CPicture("PUZZLOGO", 607, 3);
	new CLabel(700, 95, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[463]);
	new CResDataBar("ARESBAR.bmp", 3, 575, 32, 2, 85, 85);

	int faction = LOCPLINT->cb->getStartInfo()->playerInfos.find(LOCPLINT->playerID)->second.castle;

	auto & puzzleMap = CGI->townh->factions[faction]->puzzleMap;

	for(auto & elem : puzzleMap)
	{
		const SPuzzleInfo & info = elem;

		auto   piece = new CPicture(info.filename, info.x, info.y);

		//piece that will slowly disappear
		if(info.whenUncovered <= GameConstants::PUZZLE_MAP_PIECES * discoveredRatio)
		{
			piecesToRemove.push_back(piece);
			piece->needRefresh = true;
			piece->recActions = piece->recActions & ~SHOWALL;
			SDL_SetSurfaceBlendMode(piece->bg,SDL_BLENDMODE_BLEND);
		}
	}
}

void CPuzzleWindow::showAll(SDL_Surface * to)
{
	int3 moveInt = int3(8, 9, 0);
	Rect mapRect = genRect(544, 591, pos.x + 8, pos.y + 7);
	int3 topTile = grailPos - moveInt;

	MapDrawingInfo info(topTile, &LOCPLINT->cb->getVisibilityMap(), &mapRect);
	info.puzzleMode = true;
	info.grailPos = grailPos;
	CGI->mh->drawTerrainRectNew(to, &info);

	CWindowObject::showAll(to);
}

void CPuzzleWindow::show(SDL_Surface * to)
{
	static int animSpeed = 2;

	if (currentAlpha < animSpeed)
	{
		//animation done
		for(auto & piece : piecesToRemove)
			delete piece;
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
	if (left)
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
		parent->showAll(screen2);
	}
}

void CTransformerWindow::CItem::update()
{
	icon->setFrame(parent->army->getCreature(SlotID(id))->idNumber + 2);
}

CTransformerWindow::CItem::CItem(CTransformerWindow * parent, int size, int id):
	id(id), size(size), parent(parent)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(LCLICK);
	left = true;
	pos.w = 58;
	pos.h = 64;

	pos.x += 45  + (id%3)*83 + id/6*83;
	pos.y += 109 + (id/3)*98;
	icon = new CAnimImage("TWCRPORT", parent->army->getCreature(SlotID(id))->idNumber + 2);
	new CLabel(28, 76,FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(size));//stack size
}

void CTransformerWindow::makeDeal()
{
	for (auto & elem : items)
		if (!elem->left)
			LOCPLINT->cb->trade(town, EMarketMode::CREATURE_UNDEAD, elem->id, 0, 0, hero);
}

void CTransformerWindow::addAll()
{
	for (auto & elem : items)
		if (elem->left)
			elem->move();
	showAll(screen2);
}

void CTransformerWindow::updateGarrisons()
{
	for(auto & item : items)
	{
		item->update();
	}
}

CTransformerWindow::CTransformerWindow(const CGHeroInstance * _hero, const CGTownInstance * _town):
	CWindowObject(PLAYER_COLORED, "SKTRNBK"),
	hero(_hero),
	town(_town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (hero)
		army = hero;
	else
		army = town;

	for (int i=0; i<GameConstants::ARMY_SIZE; i++ )
		if ( army->getCreature(SlotID(i)) )
			items.push_back(new CItem(this, army->getStackCount(SlotID(i)), i));

	all     = new CButton(Point(146, 416), "ALTARMY.DEF", CGI->generaltexth->zelp[590], [&] { addAll(); }, SDLK_a);
	convert = new CButton(Point(269, 416), "ALTSACR.DEF", CGI->generaltexth->zelp[591], [&] { makeDeal(); }, SDLK_RETURN);
	cancel  = new CButton(Point(392, 416), "ICANCEL.DEF", CGI->generaltexth->zelp[592], [&] { close(); },SDLK_ESCAPE);
	bar     = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	new CLabel(153, 29,FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[485]);//holding area
	new CLabel(153+295, 29, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[486]);//transformer
	new CTextBox(CGI->generaltexth->allTexts[487], Rect(26,  56, 255, 40), 0, FONT_MEDIUM, CENTER, Colors::YELLOW);//move creatures to create skeletons
	new CTextBox(CGI->generaltexth->allTexts[488], Rect(320, 56, 255, 40), 0, FONT_MEDIUM, CENTER, Colors::YELLOW);//creatures here will become skeletons

}

void CUniversityWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		if ( state() != 2 )
			return;
		auto  win = new CUnivConfirmWindow(parent, ID, LOCPLINT->cb->getResourceAmount(Res::GOLD) >= 2000);
		GH.pushInt(win);
	}
}

void CUniversityWindow::CItem::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CRClickPopup::createAndPush(CGI->generaltexth->skillInfoTexts[ID][0],
				new CComponent(CComponent::secskill, ID, 1));
	}
}

void CUniversityWindow::CItem::hover(bool on)
{
	if (on)
		GH.statusbar->setText(CGI->generaltexth->skillName[ID]);
	else
		GH.statusbar->clear();
}

int CUniversityWindow::CItem::state()
{
	if (parent->hero->getSecSkillLevel(SecondarySkill(ID)))//hero know this skill
		return 1;
	if (!parent->hero->canLearnSkill())//can't learn more skills
		return 0;
	if (parent->hero->type->heroClass->secSkillProbability[ID]==0)//can't learn this skill (like necromancy for most of non-necros)
		return 0;
	return 2;
}

void CUniversityWindow::CItem::showAll(SDL_Surface * to)
{
	CPicture * bar;
	switch (state())
	{
		case 0: bar = parent->red;
				break;
		case 1: bar = parent->yellow;
				break;
		case 2: bar = parent->green;
				break;
		default:bar = nullptr;
				break;
	}
	assert(bar);

	blitAtLoc(bar->bg, -28, -22, to);
	blitAtLoc(bar->bg, -28,  48, to);
	printAtMiddleLoc  (CGI->generaltexth->skillName[ID], 22, -13, FONT_SMALL, Colors::WHITE,to);//Name
	printAtMiddleLoc  (CGI->generaltexth->levels[0], 22, 57, FONT_SMALL, Colors::WHITE,to);//Level(always basic)

	CAnimImage::showAll(to);
}

CUniversityWindow::CItem::CItem(CUniversityWindow * _parent, int _ID, int X, int Y):
	CAnimImage ("SECSKILL", _ID*3+3, 0, X, Y),
	ID(_ID),
	parent(_parent)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
}

CUniversityWindow::CUniversityWindow(const CGHeroInstance * _hero, const IMarket * _market):
	CWindowObject(PLAYER_COLORED, "UNIVERS1"),
	hero(_hero),
	market(_market)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	green  = new CPicture("UNIVGREN.PCX");
	yellow = new CPicture("UNIVGOLD.PCX");//bars
	red    = new CPicture("UNIVRED.PCX");

	green->recActions  =
	yellow->recActions =
	red->recActions    = DISPOSE;

	CIntObject * titlePic = nullptr;

	if (market->o->ID == Obj::TOWN)
		titlePic = new CAnimImage(CGI->townh->factions[ETownType::CONFLUX]->town->clientInfo.buildingsIcons, BuildingID::MAGIC_UNIVERSITY);
	else
		titlePic = new CPicture("UNIVBLDG");

	titlePic->center(Point(232 + pos.x, 76 + pos.y));

	//Clerk speech
	new CTextBox(CGI->generaltexth->allTexts[603], Rect(24, 129, 413, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);

	//University
	new CLabel(231, 26, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[602]);

	std::vector<int> list = market->availableItemsIds(EMarketMode::RESOURCE_SKILL);

	assert(list.size() == 4);

	for (int i=0; i<list.size(); i++)//prepare clickable items
		items.push_back(new CItem(this, list[i], 54+i*104, 234));

	cancel = new CButton(Point(200, 313), "IOKAY.DEF", CGI->generaltexth->zelp[632], [&]{ close(); }, SDLK_RETURN);

	bar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

CUnivConfirmWindow::CUnivConfirmWindow(CUniversityWindow * PARENT, int SKILL, bool available ):
	CWindowObject(PLAYER_COLORED, "UNIVERS2.PCX"),
	parent(PARENT)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	std::string text = CGI->generaltexth->allTexts[608];
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->generaltexth->skillName[SKILL]);
	boost::replace_first(text, "%d", "2000");

	new CTextBox(text, Rect(24, 129, 413, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);//Clerk speech

	new CLabel(230, 37,  FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth-> skillName[SKILL]);//Skill name
	new CAnimImage("SECSKILL", SKILL*3+3, 0, 211, 51);//skill
	new CLabel(230, 107, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->levels[1]);//Skill level

	new CAnimImage("RESOURCE", Res::GOLD, 0, 210, 210);//gold
	new CLabel(230, 267, FONT_SMALL, CENTER, Colors::WHITE, "2000");//Cost

	std::string hoverText = CGI->generaltexth->allTexts[609];
	boost::replace_first(hoverText, "%s", CGI->generaltexth->levels[0]+ " " + CGI->generaltexth->skillName[SKILL]);

	text = CGI->generaltexth->zelp[633].second;
	boost::replace_first(text, "%s", CGI->generaltexth->levels[0]);
	boost::replace_first(text, "%s", CGI->generaltexth->skillName[SKILL]);
	boost::replace_first(text, "%d", "2000");

	confirm= new CButton(Point(148, 299), "IBY6432.DEF", CButton::tooltip(hoverText, text), [=]{makeDeal(SKILL);}, SDLK_RETURN);
	confirm->block(!available);

	cancel = new CButton(Point(252,299), "ICANCEL.DEF", CGI->generaltexth->zelp[631], [&]{ close(); }, SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CUnivConfirmWindow::makeDeal(int skill)
{
	LOCPLINT->cb->trade(parent->market->o, EMarketMode::RESOURCE_SKILL, 6, skill, 1, parent->hero);
	close();
}

CHillFortWindow::CHillFortWindow(const CGHeroInstance *visitor, const CGObjectInstance *object):
	CWindowObject(PLAYER_COLORED, "APHLFTBK"),
	fort(object),
	hero(visitor)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	slotsCount=7;
	resources =  CDefHandler::giveDefEss("SMALRES.DEF");

	new CLabel(325, 32, FONT_BIG, CENTER, Colors::YELLOW, fort->getObjectName());//Hill Fort

	heroPic = new CHeroArea(30, 60, hero);

	currState.resize(slotsCount+1);
	costs.resize(slotsCount);
	totalSumm.resize(GameConstants::RESOURCE_QUANTITY);

	for (int i = 0; i < slotsCount; i++)
	{
		currState[i] = getState(SlotID(i));
		upgrade[i] = new CButton(Point(107 + i * 76, 171), "", CButton::tooltip(getTextForSlot(SlotID(i))), [=]{ makeDeal(SlotID(i)); }, SDLK_1 + i);
		for (auto image : { "APHLF1R.DEF", "APHLF1Y.DEF", "APHLF1G.DEF" })
			upgrade[i]->addImage(image);
	}

	currState[slotsCount] = getState(SlotID(slotsCount));
	upgradeAll = new CButton(Point(30, 231), "", CButton::tooltip(CGI->generaltexth->allTexts[432]), [&]{ makeDeal(SlotID(slotsCount));}, SDLK_0);
	for (auto image : { "APHLF4R.DEF", "APHLF4Y.DEF", "APHLF4G.DEF" })
		upgradeAll->addImage(image);

	quit = new CButton(Point(294, 275), "IOKAY.DEF", CButton::tooltip(), std::bind(&CHillFortWindow::close, this), SDLK_RETURN);
	bar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	garr = new CGarrisonInt(108, 60, 18, Point(),background->bg,Point(108,60),hero,nullptr);
	updateGarrisons();
}

void CHillFortWindow::updateGarrisons()
{
	for (int i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		totalSumm[i]=0;

	for (int i=0; i<slotsCount; i++)
	{
		costs[i].clear();
		int newState = getState(SlotID(i));
		if (newState != -1)
		{
			UpgradeInfo info;
			LOCPLINT->cb->getUpgradeInfo(hero, SlotID(i), info);
			if (info.newID.size())//we have upgrades here - update costs
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

	int newState = getState(SlotID(slotsCount));
	currState[slotsCount] = newState;
	upgradeAll->setIndex(newState);
	garr->recreateSlots();
}

void CHillFortWindow::makeDeal(SlotID slot)
{
	assert(slot.getNum()>=0);
	int offset = (slot.getNum() == slotsCount)?2:0;
	switch (currState[slot.getNum()])
	{
		case 0:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[314 + offset],
					  std::vector<CComponent*>(), soundBase::sound_todo);
			break;
		case 1:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[313 + offset],
					  std::vector<CComponent*>(), soundBase::sound_todo);
			break;
		case 2:
			for (int i=0; i<slotsCount; i++)
				if ( slot.getNum() ==i || ( slot.getNum() == slotsCount && currState[i] == 2 ) )//this is activated slot or "upgrade all"
				{
					UpgradeInfo info;
					LOCPLINT->cb->getUpgradeInfo(hero, SlotID(i), info);
					LOCPLINT->cb->upgradeCreature(hero, SlotID(i), info.newID[0]);
				}
			break;

	}
}

void CHillFortWindow::showAll (SDL_Surface *to)
{
	CWindowObject::showAll(to);

	for ( int i=0; i<slotsCount; i++)
	{
		if ( currState[i] == 0 || currState[i] == 2 )
		{
			if ( costs[i].size() )//we have several elements
			{
				int curY = 128;//reverse iterator is used to display gold as first element
				for(int j = costs[i].size()-1; j >= 0; j--)
				{
					int val = costs[i][j];
					if(!val) continue;

					blitAtLoc(resources->ourImages[j].bitmap, 104+76*i, curY, to);
					printToLoc(boost::lexical_cast<std::string>(val), 168+76*i, curY+16, FONT_SMALL, Colors::WHITE, to);
					curY += 20;
				}
			}
			else//free upgrade - print gold image and "Free" text
			{
				blitAtLoc(resources->ourImages[6].bitmap, 104+76*i, 128, to);
				printToLoc(CGI->generaltexth->allTexts[344], 168+76*i, 144, FONT_SMALL, Colors::WHITE, to);
			}
		}
	}
	for (int i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		if (totalSumm[i])//this resource is used - display it
		{
			blitAtLoc(resources->ourImages[i].bitmap, 104+76*i, 237, to);
			printToLoc(boost::lexical_cast<std::string>(totalSumm[i]), 166+76*i, 253, FONT_SMALL, Colors::WHITE, to);
		}
	}
}

std::string CHillFortWindow::getTextForSlot(SlotID slot)
{
	if ( !hero->getCreature(slot) )//we don`t have creature here
		return "";

	std::string str = CGI->generaltexth->allTexts[318];
	int amount = hero->getStackCount(slot);
	if ( amount == 1 )
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->nameSing);
	else
		boost::algorithm::replace_first(str,"%s",hero->getCreature(slot)->namePl);

	return str;
}

int CHillFortWindow::getState(SlotID slot)
{
	TResources myRes = LOCPLINT->cb->getResourceAmount();
	if ( slot.getNum() == slotsCount )//"Upgrade all" slot
	{
		bool allUpgraded = true;//All creatures are upgraded?
		for (int i=0; i<slotsCount; i++)
			allUpgraded &=  currState[i] == 1 || currState[i] == -1;

		if (allUpgraded)
			return 1;

		if(!totalSumm.canBeAfforded(myRes))
			return 0;

		return 2;
	}

	if (hero->slotEmpty(slot))//no creature here
		return -1;

	UpgradeInfo info;
	LOCPLINT->cb->getUpgradeInfo(hero, slot, info);
	if (!info.newID.size())//already upgraded
		return 1;

	if(!(info.cost[0] * hero->getStackCount(slot)).canBeAfforded(myRes))
			return 0;

	return 2;//can upgrade
}

CThievesGuildWindow::CThievesGuildWindow(const CGObjectInstance * _owner):
	CWindowObject(PLAYER_COLORED | BORDERED, "TpRank"),
	owner(_owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	type |= BLOCK_ADV_HOTKEYS;

	SThievesGuildInfo tgi; //info to be displayed
	LOCPLINT->cb->getThievesGuildInfo(tgi, owner);

	exitb = new CButton (Point(748, 556), "TPMAGE1", CButton::tooltip(CGI->generaltexth->allTexts[600]), [&]{ close();}, SDLK_RETURN);
	exitb->assignedKeys.insert(SDLK_ESCAPE);
	statusBar = new CGStatusBar(3, 555, "TStatBar.bmp", 742);

	resdatabar = new CMinorResDataBar();
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;

	//data for information table:
	// fields[row][column] = list of id's of players for this box
	static std::vector< std::vector< PlayerColor > > SThievesGuildInfo::* fields[] =
		{ &SThievesGuildInfo::numOfTowns, &SThievesGuildInfo::numOfHeroes,       &SThievesGuildInfo::gold,
		  &SThievesGuildInfo::woodOre,    &SThievesGuildInfo::mercSulfCrystGems, &SThievesGuildInfo::obelisks,
		  &SThievesGuildInfo::artifacts,  &SThievesGuildInfo::army,              &SThievesGuildInfo::income };

	//printing texts & descriptions to background

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
		new CLabel(135, y, FONT_MEDIUM, CENTER, Colors::YELLOW, text);
	}

	for(int g=1; g<tgi.playerColors.size(); ++g)
		new CAnimImage("PRSTRIPS", g-1, 0, 250 + 66*g, 7);

	for(int g=0; g<tgi.playerColors.size(); ++g)
		new CLabel(283 + 66*g, 24, FONT_BIG, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[16+g]);

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

			for (size_t j=0; j< 2; j++)
			{
				// origin of this row | offset for 2nd row| shift right for short rows
				//if we have 2 rows, start either from mid or beginning (depending on count), otherwise center the flags
				int rowStartX = xpos   +   (j ? 6 + (rowLength[j] < 3 ? 12 : 0) : 24 - 6 * rowLength[j]);
				int rowStartY = ypos   +   (j ? 4 : 0);

				for (size_t i=0; i< rowLength[j]; i++)
				{
					new CAnimImage("itgflags", players[i + j*4].getNum(), 0, rowStartX + i*12, rowStartY);
				}
			}
		}
	}

	static const std::string colorToBox[] = {"PRRED.BMP", "PRBLUE.BMP", "PRTAN.BMP", "PRGREEN.BMP", "PRORANGE.BMP", "PRPURPLE.BMP", "PRTEAL.BMP", "PRROSE.bmp"};

	//printing best hero
	int counter = 0;
	for(auto & iter : tgi.colorToBestHero)
	{
		new CPicture(colorToBox[iter.first.getNum()], 253 + 66 * counter, 334);
		if(iter.second.portrait >= 0)
		{
			new CAnimImage("PortraitsSmall", iter.second.portrait, 0, 260 + 66 * counter, 360);
			//TODO: r-click info:
			// - r-click on hero
			// - r-click on primary skill label
			if(iter.second.details)
			{
				new CTextBox(CGI->generaltexth->allTexts[184], Rect(260 + 66*counter, 396, 52, 64),
							 0, FONT_TINY, TOPLEFT, Colors::WHITE);
				for (int i=0; i<iter.second.details->primskills.size(); ++i)
				{
					new CLabel(310 + 66 * counter, 407 + 11*i, FONT_TINY, BOTTOMRIGHT, Colors::WHITE,
							   boost::lexical_cast<std::string>(iter.second.details->primskills[i]));
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
			new CAnimImage("TWCRPORT", it.second+2, 0, 255 + 66 * counter, 479);
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

		new CLabel(283 + 66*counter, 459, FONT_SMALL, CENTER, Colors::WHITE, text);

		counter++;
	}
}

CObjectListWindow::CItem::CItem(CObjectListWindow *_parent, size_t _id, std::string _text):
	parent(_parent),
	index(_id)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	border = new CPicture("TPGATES");
	pos = border->pos;
	addUsedEvents(LCLICK);
	type |= REDRAW_PARENT;

	text = new CLabel(pos.w/2, pos.h/2, FONT_SMALL, CENTER, Colors::WHITE, _text);
	select(index == parent->selected);
}

void CObjectListWindow::CItem::select(bool on)
{
	if (on)
		border->recActions = 255;
	else
		border->recActions = ~(UPDATE | SHOWALL);
	redraw();
}

void CObjectListWindow::CItem::clickLeft(tribool down, bool previousState)
{
	if( previousState && !down)
		parent->changeSelection(index);
}

CObjectListWindow::CObjectListWindow(const std::vector<int> &_items, CIntObject * titlePic, std::string _title, std::string _descr,
				std::function<void(int)> Callback):
	CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback)
{
	items.reserve(_items.size());
	for(int id : _items)
	{
		items.push_back(std::make_pair(id, CGI->mh->map->objects[id]->getObjectName()));
	}

	init(titlePic, _title, _descr);
}

CObjectListWindow::CObjectListWindow(const std::vector<std::string> &_items, CIntObject * titlePic, std::string _title, std::string _descr,
				std::function<void(int)> Callback):
	CWindowObject(PLAYER_COLORED, "TPGATE"),
	onSelect(Callback)
{
	items.reserve(_items.size());

	for (size_t i=0; i<_items.size(); i++)
		items.push_back(std::make_pair(int(i), _items[i]));

	init(titlePic, _title, _descr);
}

void CObjectListWindow::init(CIntObject * titlePic, std::string _title, std::string _descr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	title = new CLabel(152, 27, FONT_BIG, CENTER, Colors::YELLOW, _title);
	descr = new CLabel(145, 133, FONT_SMALL, CENTER, Colors::WHITE, _descr);

	ok = new CButton(Point(15, 402), "IOKAY.DEF", CButton::tooltip(), std::bind(&CObjectListWindow::elementSelected, this), SDLK_RETURN);
	ok->block(true);
	exit = new CButton( Point(228, 402), "ICANCEL.DEF", CButton::tooltip(), std::bind(&CGuiHandler::popIntTotally,&GH, this), SDLK_ESCAPE);

	if (titlePic)
	{
		titleImage = titlePic;
		addChild(titleImage);
		titleImage->recActions = defActions;
		titleImage->pos.x = pos.w/2 + pos.x - titleImage->pos.w/2;
		titleImage->pos.y =75 + pos.y - titleImage->pos.h/2;
	}
	list = new CListBox(std::bind(&CObjectListWindow::genItem, this, _1), CListBox::DestroyFunc(),
		Point(14, 151), Point(0, 25), 9, items.size(), 0, 1, Rect(262, -32, 256, 256) );
	list->type |= REDRAW_PARENT;
}

CIntObject * CObjectListWindow::genItem(size_t index)
{
	if (index < items.size())
		return new CItem(this, index, items[index].second);
	return nullptr;
}

void CObjectListWindow::elementSelected()
{
	std::function<void(int)> toCall = onSelect;//save
	int where = items[selected].first;      //required variables
	GH.popIntTotally(this);//then destroy window
	toCall(where);//and send selected object
}

void CObjectListWindow::changeSelection(size_t which)
{
	ok->block(false);
	if (selected == which)
		return;

	std::list< CIntObject * > elements = list->getItems();
	for(CIntObject * element : elements)
	{
		CItem *item;
		if ( (item = dynamic_cast<CItem*>(element)) )
		{
			if (item->index == selected)
				item->select(false);
			if (item->index == which)
				item->select(true);
		}
	}
	selected = which;
}

void CObjectListWindow::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED)
		return;

	int sel = selected;

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
		sel = items.size();

	break; default:
		return;
	}

	vstd::abetween(sel, 0, items.size()-1);
	list->scrollTo(sel);
	changeSelection(sel);
}

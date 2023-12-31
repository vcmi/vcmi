/*
 * CHeroWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroWindow.h"

#include "CCreatureWindow.h"
#include "CHeroBackpackWindow.h"
#include "CKingdomInterface.h"
#include "GUIClasses.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../gui/CGuiHandler.h"
#include "../gui/TextAlignment.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../render/CAnimation.h"
#include "../render/IRenderHandler.h"

#include "../../CCallback.h"

#include "../lib/ArtifactUtils.h"
#include "../lib/CArtHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CSkillHandler.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

void CHeroSwitcher::clickPressed(const Point & cursorPosition)
{
	//TODO: do not recreate window
	if (false)
	{
		owner->update(hero, true);
	}
	else
	{
		const CGHeroInstance * buf = hero;
		GH.windows().popWindows(1);
		GH.windows().createAndPushWindow<CHeroWindow>(buf);
	}
}

CHeroSwitcher::CHeroSwitcher(CHeroWindow * owner_, Point pos_, const CGHeroInstance * hero_)
	: CIntObject(LCLICK),
	owner(owner_),
	hero(hero_)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos += pos_;

	image = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), hero->getIconIndex());
	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

CHeroWindow::CHeroWindow(const CGHeroInstance * hero)
	: CStatusbarWindow(PLAYER_COLORED, ImagePath::builtin("HeroScr4"))
{
	auto & heroscrn = CGI->generaltexth->heroscrn;

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	curHero = hero;

	banner = std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), LOCPLINT->playerID.getNum(), 0, 606, 8);
	name = std::make_shared<CLabel>(190, 38, EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW);
	title = std::make_shared<CLabel>(190, 65, EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

	statusbar = CGStatusBar::create(7, 559, ImagePath::builtin("ADROLLVR.bmp"), 660);

	quitButton = std::make_shared<CButton>(Point(609, 516), AnimationPath::builtin("hsbtns.def"), CButton::tooltip(heroscrn[17]), [=](){ close(); }, EShortcut::GLOBAL_RETURN);

	if(settings["general"]["enableUiEnhancements"].Bool())
	{
		questlogButton = std::make_shared<CButton>(Point(314, 429), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(heroscrn[0]), [=](){ LOCPLINT->showQuestLog(); }, EShortcut::ADVENTURE_QUEST_LOG);
		backpackButton = std::make_shared<CButton>(Point(424, 429), AnimationPath::builtin("buttons/backpack"), CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"), [=](){ createBackpackWindow(); }, EShortcut::HERO_BACKPACK);
		backpackButton->addOverlay(std::make_shared<CPicture>(ImagePath::builtin("buttons/backpackButtonIcon")));
		dismissButton = std::make_shared<CButton>(Point(534, 429), AnimationPath::builtin("hsbtns2.def"), CButton::tooltip(heroscrn[28]), [=](){ dismissCurrent(); }, EShortcut::HERO_DISMISS);
	}
	else
	{
		dismissLabel = std::make_shared<CTextBox>(CGI->generaltexth->jktexts[8], Rect(370, 430, 65, 35), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		questlogLabel = std::make_shared<CTextBox>(CGI->generaltexth->jktexts[9], Rect(510, 430, 65, 35), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		dismissButton = std::make_shared<CButton>(Point(454, 429), AnimationPath::builtin("hsbtns2.def"), CButton::tooltip(heroscrn[28]), [=](){ dismissCurrent(); }, EShortcut::HERO_DISMISS);
		questlogButton = std::make_shared<CButton>(Point(314, 429), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(heroscrn[0]), [=](){ LOCPLINT->showQuestLog(); }, EShortcut::ADVENTURE_QUEST_LOG);
	}

	formations = std::make_shared<CToggleGroup>(0);
	formations->addToggle(0, std::make_shared<CToggleButton>(Point(481, 483), AnimationPath::builtin("hsbtns6.def"), std::make_pair(heroscrn[23], heroscrn[29]), 0, EShortcut::HERO_TIGHT_FORMATION));
	formations->addToggle(1, std::make_shared<CToggleButton>(Point(481, 519), AnimationPath::builtin("hsbtns7.def"), std::make_pair(heroscrn[24], heroscrn[30]), 0, EShortcut::HERO_LOOSE_FORMATION));

	if(hero->commander)
	{
		commanderButton = std::make_shared<CButton>(Point(317, 18), AnimationPath::builtin("buttons/commander"), CButton::tooltipLocalized("vcmi.heroWindow.openCommander"), [&](){ commanderWindow(); }, EShortcut::HERO_COMMANDER);
	}

	//right list of heroes
	for(int i=0; i < std::min(LOCPLINT->cb->howManyHeroes(false), 8); i++)
		heroList.push_back(std::make_shared<CHeroSwitcher>(this, Point(612, 87 + i * 54), LOCPLINT->cb->getHeroBySerial(i, false)));

	//areas
	portraitArea = std::make_shared<LRClickableAreaWText>(Rect(18, 18, 58, 64));
	portraitImage = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), 0, 0, 19, 19);

	for(int v = 0; v < GameConstants::PRIMARY_SKILLS; ++v)
	{
		auto area = std::make_shared<LRClickableAreaWTextComp>(Rect(30 + 70 * v, 109, 42, 64), ComponentType::PRIM_SKILL);
		area->text = CGI->generaltexth->arraytxt[2+v];
		area->component.subType = PrimarySkill(v);
		area->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % CGI->generaltexth->primarySkillNames[v]);
		primSkillAreas.push_back(area);

		auto value = std::make_shared<CLabel>(53 + 70 * v, 166, FONT_SMALL, ETextAlignment::CENTER);
		primSkillValues.push_back(value);
	}

	auto primSkills = GH.renderHandler().loadAnimation(AnimationPath::builtin("PSKIL42"));
	primSkills->preload();
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 0, 0, 32, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 1, 0, 102, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 2, 0, 172, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 3, 0, 162, 230));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 4, 0, 20, 230));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 5, 0, 242, 111));

	specImage = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), 0, 0, 18, 180);
	specArea = std::make_shared<LRClickableAreaWText>(Rect(18, 180, 136, 42), CGI->generaltexth->heroscrn[27]);
	specName = std::make_shared<CLabel>(69, 205);

	expArea = std::make_shared<LRClickableAreaWText>(Rect(18, 228, 136, 42), CGI->generaltexth->heroscrn[9]);
	morale = std::make_shared<MoraleLuckBox>(true, Rect(175, 179, 53, 45));
	luck = std::make_shared<MoraleLuckBox>(false, Rect(233, 179, 53, 45));
	spellPointsArea = std::make_shared<LRClickableAreaWText>(Rect(162,228, 136, 42), CGI->generaltexth->heroscrn[22]);

	expValue = std::make_shared<CLabel>(68, 252);
	manaValue = std::make_shared<CLabel>(211, 252);

	auto secSkills = GH.renderHandler().loadAnimation(AnimationPath::builtin("SECSKILL"));
	for(int i = 0; i < std::min<size_t>(hero->secSkills.size(), 8u); ++i)
	{
		Rect r = Rect(i%2 == 0  ?  18  :  162,  276 + 48 * (i/2),  136,  42);
		secSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>(r, ComponentType::SEC_SKILL));
		secSkillImages.push_back(std::make_shared<CAnimImage>(secSkills, 0, 0, r.x, r.y));

		int x = (i % 2) ? 212 : 68;
		int y = 280 + 48 * (i/2);

		secSkillValues.push_back(std::make_shared<CLabel>(x, y, FONT_SMALL, ETextAlignment::TOPLEFT));
		secSkillNames.push_back(std::make_shared<CLabel>(x, y+20, FONT_SMALL, ETextAlignment::TOPLEFT));
	}

	// various texts
	labels.push_back(std::make_shared<CLabel>(52, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[1]));
	labels.push_back(std::make_shared<CLabel>(123, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[2]));
	labels.push_back(std::make_shared<CLabel>(193, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[3]));
	labels.push_back(std::make_shared<CLabel>(262, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[4]));

	labels.push_back(std::make_shared<CLabel>(69, 183, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->jktexts[5]));
	labels.push_back(std::make_shared<CLabel>(69, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->jktexts[6]));
	labels.push_back(std::make_shared<CLabel>(213, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->jktexts[7]));

	update(hero);
}

void CHeroWindow::update(const CGHeroInstance * hero, bool redrawNeeded)
{
	auto & heroscrn = CGI->generaltexth->heroscrn;

	if(!hero) //something strange... no hero? it shouldn't happen
	{
		logGlobal->error("Set nullptr hero? no way...");
		return;
	}

	assert(hero == curHero);

	name->setText(curHero->getNameTranslated());
	title->setText((boost::format(CGI->generaltexth->allTexts[342]) % curHero->level % curHero->type->heroClass->getNameTranslated()).str());

	specArea->text = curHero->type->getSpecialtyDescriptionTranslated();
	specImage->setFrame(curHero->type->imageIndex);
	specName->setText(curHero->type->getSpecialtyNameTranslated());

	tacticsButton = std::make_shared<CToggleButton>(Point(539, 483), AnimationPath::builtin("hsbtns8.def"), std::make_pair(heroscrn[26], heroscrn[31]), 0, EShortcut::HERO_TOGGLE_TACTICS);
	tacticsButton->addHoverText(CButton::HIGHLIGHTED, CGI->generaltexth->heroscrn[25]);

	dismissButton->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->heroscrn[16]) % curHero->getNameTranslated() % curHero->type->heroClass->getNameTranslated()));
	portraitArea->hoverText = boost::str(boost::format(CGI->generaltexth->allTexts[15]) % curHero->getNameTranslated() % curHero->type->heroClass->getNameTranslated());
	portraitArea->text = curHero->getBiographyTranslated();
	portraitImage->setFrame(curHero->getIconIndex());

	{
		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
		if(!garr)
		{
			std::string helpBox = heroscrn[32];
			boost::algorithm::replace_first(helpBox, "%s", CGI->generaltexth->allTexts[43]);

			garr = std::make_shared<CGarrisonInt>(Point(15, 485), 8, Point(), curHero);
			auto split = std::make_shared<CButton>(Point(539, 519), AnimationPath::builtin("hsbtns9.def"), CButton::tooltip(CGI->generaltexth->allTexts[256], helpBox), [&](){ garr->splitClick(); });
			garr->addSplitBtn(split);
		}
		if(!arts)
		{
			arts = std::make_shared<CArtifactsOfHeroMain>(Point(-65, -8));
			arts->setHero(curHero);
			addSetAndCallbacks(arts);
		}

		int serial = LOCPLINT->cb->getHeroSerial(curHero, false);

		listSelection.reset();
		if(serial >= 0)
			listSelection = std::make_shared<CPicture>(ImagePath::builtin("HPSYYY"), 612, 33 + serial * 54);
	}

	//primary skills support
	for(size_t g=0; g<primSkillAreas.size(); ++g)
	{
		int value = curHero->getPrimSkillLevel(static_cast<PrimarySkill>(g));
		primSkillAreas[g]->component.value = value;
		primSkillValues[g]->setText(std::to_string(value));
	}

	//secondary skills support
	for(size_t g=0; g< secSkillAreas.size(); ++g)
	{
		SecondarySkill skill = curHero->secSkills[g].first;
		int	level = curHero->getSecSkillLevel(skill);
		std::string skillName = CGI->skillh->getByIndex(skill)->getNameTranslated();
		std::string skillValue = CGI->generaltexth->levels[level-1];

		secSkillAreas[g]->component.subType = skill;
		secSkillAreas[g]->component.value = level;
		secSkillAreas[g]->text = CGI->skillh->getByIndex(skill)->getDescriptionTranslated(level);
		secSkillAreas[g]->hoverText = boost::str(boost::format(heroscrn[21]) % skillValue % skillName);
		secSkillImages[g]->setFrame(skill*3 + level + 2);
		secSkillNames[g]->setText(skillName);
		secSkillValues[g]->setText(skillValue);
	}

	std::ostringstream expstr;
	expstr << curHero->exp;
	expValue->setText(expstr.str());

	std::ostringstream manastr;
	manastr << curHero->mana << '/' << curHero->manaLimit();
	manaValue->setText(manastr.str());

	//printing experience - original format does not support ui64
	expArea->text = CGI->generaltexth->allTexts[2];
	boost::replace_first(expArea->text, "%d", std::to_string(curHero->level));
	boost::replace_first(expArea->text, "%d", std::to_string(CGI->heroh->reqExp(curHero->level+1)));
	boost::replace_first(expArea->text, "%d", std::to_string(curHero->exp));

	//printing spell points, boost::format can't be used due to locale issues
	spellPointsArea->text = CGI->generaltexth->allTexts[205];
	boost::replace_first(spellPointsArea->text, "%s", curHero->getNameTranslated());
	boost::replace_first(spellPointsArea->text, "%d", std::to_string(curHero->mana));
	boost::replace_first(spellPointsArea->text, "%d", std::to_string(curHero->manaLimit()));

	//if we have exchange window with this curHero open
	bool noDismiss=false;

	for(auto cew : GH.windows().findWindows<CExchangeWindow>())
	{
		for(int g=0; g < cew->heroInst.size(); ++g)
			if(cew->heroInst[g] == curHero)
				noDismiss = true;
	}

	//if player only have one hero and no towns
	if(!LOCPLINT->cb->howManyTowns() && LOCPLINT->cb->howManyHeroes() == 1)
		noDismiss = true;

	if(curHero->isMissionCritical())
		noDismiss = true;

	dismissButton->block(noDismiss);

	if(curHero->valOfBonuses(Selector::type()(BonusType::BEFORE_BATTLE_REPOSITION)) == 0)
	{
		tacticsButton->block(true);
	}
	else
	{
		tacticsButton->block(false);
		tacticsButton->addCallback([&](bool on){curHero->tacticFormationEnabled = on;});
	}

	formations->resetCallback();
	//setting formations
	formations->setSelected(curHero->formation == EArmyFormation::TIGHT ? 1 : 0);
	formations->addCallback([=](int value){ LOCPLINT->cb->setFormation(curHero, static_cast<EArmyFormation>(value));});

	morale->set(curHero);
	luck->set(curHero);

	if(redrawNeeded)
		redraw();
}

void CHeroWindow::dismissCurrent()
{
	CFunctionList<void()> ony = [=](){ close(); };
	ony += [=](){ LOCPLINT->cb->dismissHero(curHero); };
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[22], ony, nullptr);
}

void CHeroWindow::createBackpackWindow()
{
	GH.windows().createAndPushWindow<CHeroBackpackWindow>(curHero);
}

void CHeroWindow::commanderWindow()
{
	const auto pickedArtInst = getPickedArtifact();
	const auto hero = getHeroPickedArtifact();

	if(pickedArtInst)
	{
		const auto freeSlot = ArtifactUtils::getArtAnyPosition(curHero->commander, pickedArtInst->getTypeId());
		if(vstd::contains(ArtifactUtils::commanderSlots(), freeSlot)) // We don't want to put it in commander's backpack!
		{
			ArtifactLocation dst(curHero->id, freeSlot);
			dst.creature = SlotID::COMMANDER_SLOT_PLACEHOLDER;
			LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero->id, ArtifactPosition::TRANSITION_POS), dst);
		}
	}
	else
	{
		GH.windows().createAndPushWindow<CStackWindow>(curHero->commander, false);
	}
}

void CHeroWindow::updateGarrisons()
{
	garr->recreateSlots();
	morale->set(curHero);
}

bool CHeroWindow::holdsGarrison(const CArmedInstance * army)
{
	return army == curHero;
}

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
#include "CExchangeWindow.h"

#include "../CPlayerInterface.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/TextAlignment.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../render/IRenderHandler.h"

#include "../../CCallback.h"

#include "../lib/CConfigHandler.h"
#include "../lib/CSkillHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/entities/artifact/ArtifactUtils.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/networkPacks/ArtifactLocation.h"
#include "../lib/texts/CGeneralTextHandler.h"

void CHeroSwitcher::clickPressed(const Point & cursorPosition)
{
	//TODO: do not recreate window
	if (false)
	{
		owner->update();
	}
	else
	{
		const CGHeroInstance * buf = hero;
		ENGINE->windows().popWindows(1);
		ENGINE->windows().createAndPushWindow<CHeroWindow>(buf);
	}
}

CHeroSwitcher::CHeroSwitcher(CHeroWindow * owner_, Point pos_, const CGHeroInstance * hero_)
	: CIntObject(LCLICK),
	owner(owner_),
	hero(hero_)
{
	OBJECT_CONSTRUCTION;
	pos += pos_;

	image = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), hero->getIconIndex());
	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

CHeroWindow::CHeroWindow(const CGHeroInstance * hero)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("HeroScr4"))
{
	auto & heroscrn = LIBRARY->generaltexth->heroscrn;

	OBJECT_CONSTRUCTION;
	curHero = hero;

	banner = std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), GAME->interface()->playerID.getNum(), 0, 606, 8);
	name = std::make_shared<CLabel>(190, 38, EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW);
	title = std::make_shared<CLabel>(190, 65, EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

	statusbar = CGStatusBar::create(7, 559, ImagePath::builtin("ADROLLVR.bmp"), 660);

	quitButton = std::make_shared<CButton>(Point(609, 516), AnimationPath::builtin("hsbtns.def"), CButton::tooltip(heroscrn[17]), [this](){ close(); }, EShortcut::GLOBAL_RETURN);

	if(settings["general"]["enableUiEnhancements"].Bool())
	{
		questlogButton = std::make_shared<CButton>(Point(314, 429), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(heroscrn[0]), [](){ GAME->interface()->showQuestLog(); }, EShortcut::ADVENTURE_QUEST_LOG);
		backpackButton = std::make_shared<CButton>(Point(424, 429), AnimationPath::builtin("heroBackpack"), CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"), [this](){ createBackpackWindow(); }, EShortcut::HERO_BACKPACK);
		backpackButton->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/backpackButtonIcon")));
		dismissButton = std::make_shared<CButton>(Point(534, 429), AnimationPath::builtin("hsbtns2.def"), CButton::tooltip(heroscrn[28]), [this](){ dismissCurrent(); }, EShortcut::HERO_DISMISS);
	}
	else
	{
		dismissLabel = std::make_shared<CTextBox>(LIBRARY->generaltexth->jktexts[8], Rect(370, 430, 65, 35), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		questlogLabel = std::make_shared<CTextBox>(LIBRARY->generaltexth->jktexts[9], Rect(510, 430, 65, 35), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		dismissButton = std::make_shared<CButton>(Point(454, 429), AnimationPath::builtin("hsbtns2.def"), CButton::tooltip(heroscrn[28]), [this](){ dismissCurrent(); }, EShortcut::HERO_DISMISS);
		questlogButton = std::make_shared<CButton>(Point(314, 429), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(heroscrn[0]), [](){ GAME->interface()->showQuestLog(); }, EShortcut::ADVENTURE_QUEST_LOG);
	}

	formations = std::make_shared<CToggleGroup>(0);
	formations->addToggle(0, std::make_shared<CToggleButton>(Point(481, 483), AnimationPath::builtin("hsbtns6.def"), std::make_pair(heroscrn[23], heroscrn[29]), 0, EShortcut::HERO_TIGHT_FORMATION));
	formations->addToggle(1, std::make_shared<CToggleButton>(Point(481, 519), AnimationPath::builtin("hsbtns7.def"), std::make_pair(heroscrn[24], heroscrn[30]), 0, EShortcut::HERO_LOOSE_FORMATION));

	if(hero->getCommander())
	{
		commanderButton = std::make_shared<CButton>(Point(317, 18), AnimationPath::builtin("heroCommander"), CButton::tooltipLocalized("vcmi.heroWindow.openCommander"), [&](){ commanderWindow(); }, EShortcut::HERO_COMMANDER);
		commanderButton->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/commanderButtonIcon")));
	}

	//right list of heroes
	for(int i=0; i < std::min(GAME->interface()->cb->howManyHeroes(false), 8); i++)
		heroList.push_back(std::make_shared<CHeroSwitcher>(this, Point(612, 87 + i * 54), GAME->interface()->cb->getHeroBySerial(i, false)));

	//areas
	portraitArea = std::make_shared<LRClickableAreaWText>(Rect(18, 18, 58, 64));
	portraitImage = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), 0, 0, 19, 19);

	for(int v = 0; v < GameConstants::PRIMARY_SKILLS; ++v)
	{
		auto area = std::make_shared<LRClickableAreaWTextComp>(Rect(30 + 70 * v, 109, 42, 64), ComponentType::PRIM_SKILL);
		area->text = LIBRARY->generaltexth->arraytxt[2+v];
		area->component.subType = PrimarySkill(v);
		area->hoverText = boost::str(boost::format(LIBRARY->generaltexth->heroscrn[1]) % LIBRARY->generaltexth->primarySkillNames[v]);
		primSkillAreas.push_back(area);

		auto value = std::make_shared<CLabel>(53 + 70 * v, 166, FONT_SMALL, ETextAlignment::CENTER);
		primSkillValues.push_back(value);
	}

	primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 0, 0, 32, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 1, 0, 102, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 2, 0, 172, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 3, 0, 162, 230));
	primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 4, 0, 20, 230));
	primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 5, 0, 242, 111));

	specImage = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), 0, 0, 18, 180);
	specArea = std::make_shared<LRClickableAreaWText>(Rect(18, 180, 136, 42), LIBRARY->generaltexth->heroscrn[27]);
	specName = std::make_shared<CLabel>(69, 205);

	expArea = std::make_shared<LRClickableAreaWText>(Rect(18, 228, 136, 42), LIBRARY->generaltexth->heroscrn[9]);
	morale = std::make_shared<MoraleLuckBox>(true, Rect(175, 179, 53, 45));
	luck = std::make_shared<MoraleLuckBox>(false, Rect(233, 179, 53, 45));
	spellPointsArea = std::make_shared<LRClickableAreaWText>(Rect(162,228, 136, 42), LIBRARY->generaltexth->heroscrn[22]);

	expValue = std::make_shared<CLabel>(68, 252);
	manaValue = std::make_shared<CLabel>(211, 252);

	for(int i = 0; i < std::min<size_t>(hero->secSkills.size(), 8u); ++i)
	{
		Rect r = Rect(i%2 == 0  ?  18  :  162,  276 + 48 * (i/2),  136,  42);
		secSkills.emplace_back(std::make_shared<CSecSkillPlace>(r.topLeft(), CSecSkillPlace::ImageSize::MEDIUM));

		int x = (i % 2) ? 212 : 68;
		int y = 280 + 48 * (i/2);

		secSkillValues.push_back(std::make_shared<CLabel>(x, y, FONT_SMALL, ETextAlignment::TOPLEFT));
		secSkillNames.push_back(std::make_shared<CLabel>(x, y+20, FONT_SMALL, ETextAlignment::TOPLEFT));
	}

	// various texts
	labels.push_back(std::make_shared<CLabel>(52, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[1]));
	labels.push_back(std::make_shared<CLabel>(123, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[2]));
	labels.push_back(std::make_shared<CLabel>(193, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[3]));
	labels.push_back(std::make_shared<CLabel>(262, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->jktexts[4]));

	labels.push_back(std::make_shared<CLabel>(69, 183, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->jktexts[5]));
	labels.push_back(std::make_shared<CLabel>(69, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->jktexts[6]));
	labels.push_back(std::make_shared<CLabel>(213, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->jktexts[7]));

	CHeroWindow::update();
}

void CHeroWindow::update()
{
	CWindowWithArtifacts::update();
	auto & heroscrn = LIBRARY->generaltexth->heroscrn;
	assert(curHero);

	name->setText(curHero->getNameTranslated());
	title->setText((boost::format(LIBRARY->generaltexth->allTexts[342]) % curHero->level % curHero->getClassNameTranslated()).str());

	specArea->text = curHero->getHeroType()->getSpecialtyDescriptionTranslated();
	specImage->setFrame(curHero->getHeroType()->imageIndex);
	specName->setText(curHero->getHeroType()->getSpecialtyNameTranslated());

	tacticsButton = std::make_shared<CToggleButton>(Point(539, 483), AnimationPath::builtin("hsbtns8.def"), std::make_pair(heroscrn[26], heroscrn[31]), 0, EShortcut::HERO_TOGGLE_TACTICS);
	tacticsButton->addHoverText(EButtonState::HIGHLIGHTED, LIBRARY->generaltexth->heroscrn[25]);

	dismissButton->addHoverText(EButtonState::NORMAL, boost::str(boost::format(LIBRARY->generaltexth->heroscrn[16]) % curHero->getNameTranslated() % curHero->getClassNameTranslated()));
	portraitArea->hoverText = boost::str(boost::format(LIBRARY->generaltexth->allTexts[15]) % curHero->getNameTranslated() % curHero->getClassNameTranslated());
	portraitArea->text = curHero->getBiographyTranslated();
	portraitImage->setFrame(curHero->getIconIndex());

	{
		OBJECT_CONSTRUCTION;
		if(!garr)
		{
			bool removableTroops = curHero->getOwner() == GAME->interface()->playerID;
			std::string helpBox = heroscrn[32];
			boost::algorithm::replace_first(helpBox, "%s", LIBRARY->generaltexth->allTexts[43]);

			garr = std::make_shared<CGarrisonInt>(Point(15, 485), 8, Point(), curHero, nullptr, removableTroops);
			auto split = std::make_shared<CButton>(Point(539, 519), AnimationPath::builtin("hsbtns9.def"), CButton::tooltip(LIBRARY->generaltexth->allTexts[256], helpBox), [this](){ garr->splitClick(); }, EShortcut::HERO_ARMY_SPLIT);
			garr->addSplitBtn(split);
		}
		if(!arts)
		{
			arts = std::make_shared<CArtifactsOfHeroMain>(Point(-65, -8));
			arts->clickPressedCallback = [this](const CArtPlace & artPlace, const Point & cursorPosition){clickPressedOnArtPlace(curHero, artPlace.slot, true, false, false, cursorPosition);};
			arts->showPopupCallback = [this](CArtPlace & artPlace, const Point & cursorPosition){showArtifactAssembling(*arts, artPlace, cursorPosition);};
			arts->gestureCallback = [this](const CArtPlace & artPlace, const Point & cursorPosition){showQuickBackpackWindow(curHero, artPlace.slot, cursorPosition);};
			arts->setHero(curHero);
			addSet(arts);
			enableKeyboardShortcuts();
		}

		int serial = GAME->interface()->cb->getHeroSerial(curHero, false);

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
	for(size_t g=0; g< secSkills.size(); ++g)
	{
		SecondarySkill skill = curHero->secSkills[g].first;
		int	level = curHero->getSecSkillLevel(skill);
		std::string skillName = skill.toEntity(LIBRARY)->getNameTranslated();
		std::string skillValue = LIBRARY->generaltexth->levels[level-1];

		secSkillNames[g]->setText(skillName);
		secSkillValues[g]->setText(skillValue);
		secSkills[g]->setSkill(skill, level);
	}

	std::ostringstream expstr;
	expstr << curHero->exp;
	expValue->setText(expstr.str());

	std::ostringstream manastr;
	manastr << curHero->mana << '/' << curHero->manaLimit();
	manaValue->setText(manastr.str());

	//printing experience - original format does not support ui64
	expArea->text = LIBRARY->generaltexth->allTexts[2];
	boost::replace_first(expArea->text, "%d", std::to_string(curHero->level));
	boost::replace_first(expArea->text, "%d", std::to_string(LIBRARY->heroh->reqExp(curHero->level+1)));
	boost::replace_first(expArea->text, "%d", std::to_string(curHero->exp));

	//printing spell points, boost::format can't be used due to locale issues
	spellPointsArea->text = LIBRARY->generaltexth->allTexts[205];
	boost::replace_first(spellPointsArea->text, "%s", curHero->getNameTranslated());
	boost::replace_first(spellPointsArea->text, "%d", std::to_string(curHero->mana));
	boost::replace_first(spellPointsArea->text, "%d", std::to_string(curHero->manaLimit()));

	//if we have exchange window with this curHero open
	bool noDismiss=false;

	for(auto cew : ENGINE->windows().findWindows<CExchangeWindow>())
	{
		if (cew->holdsGarrison(curHero))
			noDismiss = true;
	}

	//if player only have one hero and no towns
	if(!GAME->interface()->cb->howManyTowns() && GAME->interface()->cb->howManyHeroes() == 1)
		noDismiss = true;

	if(curHero->isMissionCritical())
		noDismiss = true;

	dismissButton->block(noDismiss);

	if(curHero->valOfBonuses(BonusType::BEFORE_BATTLE_REPOSITION) == 0)
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
	formations->addCallback([this](int value){ GAME->interface()->cb->setFormation(curHero, static_cast<EArmyFormation>(value));});

	morale->set(curHero);
	luck->set(curHero);

	redraw();
}

void CHeroWindow::dismissCurrent()
{
	GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[22], [this]()
		{
			arts->putBackPickedArtifact();
			close();
			GAME->interface()->cb->dismissHero(curHero);
			arts->setHero(nullptr);
		}, nullptr);
}

void CHeroWindow::createBackpackWindow()
{
	ENGINE->windows().createAndPushWindow<CHeroBackpackWindow>(curHero, artSets);
}

void CHeroWindow::commanderWindow()
{
	const auto pickedArtInst = getPickedArtifact();
	const auto hero = getHeroPickedArtifact();

	if(pickedArtInst)
	{
		const auto freeSlot = ArtifactUtils::getArtAnyPosition(curHero->getCommander(), pickedArtInst->getTypeId());
		if(vstd::contains(ArtifactUtils::commanderSlots(), freeSlot)) // We don't want to put it in commander's backpack!
		{
			ArtifactLocation dst(curHero->id, freeSlot);
			dst.creature = SlotID::COMMANDER_SLOT_PLACEHOLDER;
			GAME->interface()->cb->swapArtifacts(ArtifactLocation(hero->id, ArtifactPosition::TRANSITION_POS), dst);
		}
	}
	else
	{
		ENGINE->windows().createAndPushWindow<CStackWindow>(curHero->getCommander(), false);
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

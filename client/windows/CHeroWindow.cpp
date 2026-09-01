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
#include "wiki/WikiWindow.h"

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
#include "../widgets/Slider.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/texts/CGeneralTextHandler.h"

void CHeroSwitcher::clickPressed(const Point & cursorPosition)
{
	//TODO: do not recreate window
	if (false)
	{
		owner->updateArtifacts();
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
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin(ENGINE->isRoeData() ? "HeroScr3" : "HeroScr4"))
{

	OBJECT_CONSTRUCTION;
	curHero = hero;

	banner = std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), GAME->interface()->playerID.getNum(), 0, 606, 8);
	name = std::make_shared<CLabel>(190, 38, EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW);
	title = std::make_shared<CLabel>(190, 65, EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 559, 660, 19), 7, 559));

	quitButton = std::make_shared<CButton>(Point(609, 516), AnimationPath::builtin("hsbtns.def"), CButton::tooltip(LIBRARY->generaltexth->translate("core.heroscrn.17")), [this](){ close(); }, EShortcut::GLOBAL_RETURN);

	if(settings["general"]["enableUiEnhancements"].Bool())
	{
		questlogButton = std::make_shared<CButton>(Point(314, 429), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(LIBRARY->generaltexth->translate("core.heroscrn.0")), [](){ GAME->interface()->showQuestLog(); }, EShortcut::ADVENTURE_QUEST_LOG);
		backpackButton = std::make_shared<CButton>(Point(424, 429), AnimationPath::builtin("heroBackpack"), CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"), [this](){ createBackpackWindow(); }, EShortcut::HERO_BACKPACK);
		backpackButton->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/backpackButtonIcon")));
		dismissButton = std::make_shared<CButton>(Point(534, 429), AnimationPath::builtin("hsbtns2.def"), CButton::tooltip(LIBRARY->generaltexth->translate("core.heroscrn.28")), [this](){ dismissCurrent(); }, EShortcut::HERO_DISMISS);
	}
	else
	{
		dismissLabel = std::make_shared<CTextBox>(LIBRARY->generaltexth->translate("core.jktext.8"), Rect(370, 430, 65, 35), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		questlogLabel = std::make_shared<CTextBox>(LIBRARY->generaltexth->translate("core.jktext.9"), Rect(510, 430, 65, 35), 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		dismissButton = std::make_shared<CButton>(Point(454, 429), AnimationPath::builtin("hsbtns2.def"), CButton::tooltip(LIBRARY->generaltexth->translate("core.heroscrn.28")), [this](){ dismissCurrent(); }, EShortcut::HERO_DISMISS);
		questlogButton = std::make_shared<CButton>(Point(314, 429), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(LIBRARY->generaltexth->translate("core.heroscrn.0")), [](){ GAME->interface()->showQuestLog(); }, EShortcut::ADVENTURE_QUEST_LOG);
	}
	questlogButton->block(!GAME->interface()->hasJournalEntries());

	formations = std::make_shared<CToggleGroup>(0);
	formations->addToggle(0, std::make_shared<CToggleButton>(Point(481, 483), AnimationPath::builtin("hsbtns6.def"), std::make_pair(LIBRARY->generaltexth->translate("core.heroscrn.23"), LIBRARY->generaltexth->translate("core.heroscrn.29")), 0, EShortcut::HERO_TIGHT_FORMATION));
	formations->addToggle(1, std::make_shared<CToggleButton>(Point(481, 519), AnimationPath::builtin("hsbtns7.def"), std::make_pair(LIBRARY->generaltexth->translate("core.heroscrn.24"), LIBRARY->generaltexth->translate("core.heroscrn.30")), 0, EShortcut::HERO_LOOSE_FORMATION));

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

	portraitWikiArea = std::make_shared<LRClickableArea>(Rect(18, 18, 58, 64), [this]()
	{
		ENGINE->windows().createAndPushWindow<WikiWindow>(
			WikiWindow::Style::BROWN,
			WikiEntryKey{WikiCategory::HERO, curHero->getHeroType()->getJsonKey()});
	});

	for(int v = 0; v < GameConstants::PRIMARY_SKILLS; ++v)
	{
		auto area = std::make_shared<LRClickableAreaWTextComp>(Rect(30 + 70 * v, 109, 42, 64), ComponentType::PRIM_SKILL);
		area->text = GAME->translator().translate("core.arraytxt", 2+v);
		area->component.subType = PrimarySkill(v);
		MetaString hoverText;
		hoverText.appendTextID("core.heroscrn.1");
		hoverText.replaceTextID("core.priskill", v);
		area->hoverText = hoverText.toString(&GAME->translator());
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
	specArea = std::make_shared<LRClickableAreaWText>(Rect(18, 180, 136, 42), LIBRARY->generaltexth->translate("core.heroscrn.27"));
	specName = std::make_shared<CLabel>(69, 205);

	expArea = std::make_shared<LRClickableAreaWText>(Rect(18, 228, 136, 42), LIBRARY->generaltexth->translate("core.heroscrn.9"));
	morale = std::make_shared<MoraleLuckBox>(true, Rect(175, 179, 53, 45));
	luck = std::make_shared<MoraleLuckBox>(false, Rect(233, 179, 53, 45));
	spellPointsArea = std::make_shared<LRClickableAreaWText>(Rect(162,228, 136, 42), LIBRARY->generaltexth->translate("core.heroscrn.22"));

	expValue = std::make_shared<CLabel>(68, 252);
	manaValue = std::make_shared<CLabel>(211, 252);

	if(hero->secSkills.size() > 8)
	{
		auto divisionRoundUp = [](int x, int y){ return (x + (y - 1)) / y; };
		int lines = divisionRoundUp(hero->secSkills.size(), 2);
		secSkillSlider = std::make_shared<CSlider>(Point(284, 276), 189, [this](int val){ CHeroWindow::updateArtifacts(); }, 4, lines, 0, Orientation::VERTICAL, CSlider::BROWN);
		secSkillSlider->setPanningStep(48);
		secSkillSlider->setScrollBounds(Rect(-266, 0, secSkillSlider->pos.x - pos.x + secSkillSlider->pos.w, secSkillSlider->pos.h));
	}

	for(int i = 0; i < std::min<size_t>(hero->secSkills.size(), 8u); ++i)
	{
		bool isSmallBox = (secSkillSlider && i%2 == 1);
		Rect r(i%2 == 0  ?  18  :  162,  276 + 48 * (i/2), isSmallBox ? 120 : 136,  42);
		secSkills.emplace_back(std::make_shared<CSecSkillPlace>(r.topLeft(), CSecSkillPlace::ImageSize::MEDIUM));

		int x = (i % 2) ? 212 : 68;
		int y = 280 + 48 * (i/2);
		int width = isSmallBox ? 71 : 87;

		secSkillValues.push_back(std::make_shared<CLabel>(x, y, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, "", width));
		secSkillNames.push_back(std::make_shared<CLabel>(x, y+20, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, "", width));
	}

	// various texts
	labels.push_back(std::make_shared<CLabel>(52, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.1")));
	labels.push_back(std::make_shared<CLabel>(123, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.2")));
	labels.push_back(std::make_shared<CLabel>(193, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.3")));
	labels.push_back(std::make_shared<CLabel>(262, 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.4")));

	labels.push_back(std::make_shared<CLabel>(69, 183, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.5")));
	labels.push_back(std::make_shared<CLabel>(69, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.6")));
	labels.push_back(std::make_shared<CLabel>(213, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->translate("core.jktext.7")));

	addUsedEvents(KEYBOARD);
	CHeroWindow::updateArtifacts();
}

void CHeroWindow::keyPressed(EShortcut key)
{
	if(key == EShortcut::ADVENTURE_OPEN_WIKI)
		ENGINE->windows().createAndPushWindow<WikiWindow>(
			WikiWindow::Style::BROWN,
			WikiEntryKey{WikiCategory::HERO, curHero->getHeroType()->getJsonKey()});
}

void CHeroWindow::updateArtifacts()
{
	OBJECT_CONSTRUCTION;

	CWindowWithArtifacts::updateArtifacts();
	assert(curHero);

	name->setText(GAME->translator().translate(curHero->getNameTextID()));
	MetaString titleText;
	titleText.appendTextID("core.genrltxt.342");
	titleText.replaceNumber(curHero->level);
	titleText.replaceTextID(curHero->getClassNameTextID());
	title->setText(titleText.toString(&GAME->translator()));

	specArea->text = curHero->getHeroType()->getSpecialtyDescriptionTranslated();
	specImage->setFrame(curHero->getHeroType()->imageIndex);
	specName->setText(curHero->getHeroType()->getSpecialtyNameTranslated());

	tacticsButton = std::make_shared<CToggleButton>(Point(539, 483), AnimationPath::builtin("hsbtns8.def"), std::make_pair(LIBRARY->generaltexth->translate("core.heroscrn.26"), LIBRARY->generaltexth->translate("core.heroscrn.31")), 0, EShortcut::HERO_TOGGLE_TACTICS);
	tacticsButton->addHoverText(EButtonState::HIGHLIGHTED, LIBRARY->generaltexth->translate("core.heroscrn.25"));
	tacticsButton->setSelectedSilent(curHero->tacticFormationEnabled);

	MetaString dismissText;
	dismissText.appendTextID("core.heroscrn.16");
	dismissText.replaceTextID(curHero->getNameTextID());
	dismissText.replaceTextID(curHero->getClassNameTextID());

	dismissButton->addHoverText(EButtonState::NORMAL, dismissText.toString(&GAME->translator()));
	portraitArea->hoverText = curHero->getObjectName().toString(&GAME->translator());
	portraitArea->text = GAME->translator().translate(curHero->getBiographyTextID());
	portraitImage->setFrame(curHero->getIconIndex());

	{
		if(!garr)
		{
			bool removableTroops = curHero->getOwner() == GAME->interface()->playerID;
			MetaString helpBoxText = MetaString::createFromTextID("core.heroscrn.32");
			helpBoxText.replaceTextID("core.genrltxt.43");
			std::string helpBox = helpBoxText.toString(&GAME->translator());

			garr = std::make_shared<CGarrisonInt>(Point(15, 485), 8, Point(), curHero, nullptr, removableTroops);
			auto split = std::make_shared<CButton>(Point(539, 519), AnimationPath::builtin("hsbtns9.def"), CButton::tooltip(LIBRARY->generaltexth->allTexts[256], helpBox), [this](){ garr->splitClick(); }, EShortcut::HERO_ARMY_SPLIT);
			garr->addSplitBtn(split);
		}
		if(!arts)
		{
			arts = std::make_shared<CArtifactsOfHeroMain>(Point(-65, -8));
			arts->clickPressedCallback = [this](const CArtPlace & artPlace, const Point & cursorPosition){clickPressedOnArtPlace(curHero, artPlace.slot, true, false, false, cursorPosition);};
			arts->showPopupCallback = [this](CArtPlace & artPlace, const Point & cursorPosition){showArtifactPopup(*arts, artPlace, cursorPosition);};
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
	for(size_t g=0; g < secSkills.size(); ++g)
	{
		int offset = secSkillSlider ? secSkillSlider->getValue() * 2 : 0;
		if(curHero->secSkills.size() < g + offset + 1)
		{
			secSkillNames[g]->setText("");
			secSkillValues[g]->setText("");
			secSkills[g]->setSkill(SecondarySkill::NONE);
			break;
		}
		SecondarySkill skill = curHero->secSkills[g + offset].first;
		int	level = curHero->getSecSkillLevel(skill);
		std::string skillName = skill.toEntity(LIBRARY)->getNameTranslated();
		std::string skillValue = GAME->translator().translate("core.skilllev", level-1);

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

	MetaString expText;
	expText.appendTextID("core.genrltxt.2");
	expText.replaceNumber(curHero->level);
	expText.replaceNumber(LIBRARY->heroh->reqExp(curHero->level + 1));
	expText.replaceNumber(curHero->exp);
	expArea->text = expText.toString(&GAME->translator());

	MetaString spellPointsText;
	spellPointsText.appendTextID("core.genrltxt.205");
	spellPointsText.replaceTextID(curHero->getNameTextID());
	spellPointsText.replaceNumber(curHero->mana);
	spellPointsText.replaceNumber(curHero->manaLimit());
	spellPointsArea->text = spellPointsText.toString(&GAME->translator());

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
		tacticsButton->addCallback([this](bool on){ GAME->interface()->cb->setTactics(curHero, on); });
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

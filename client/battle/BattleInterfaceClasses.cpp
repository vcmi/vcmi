/*
 * BattleInterfaceClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleInterfaceClasses.h"

#include "BattleInterface.h"
#include "BattleActionsController.h"
#include "BattleRenderer.h"
#include "BattleSiegeController.h"
#include "BattleFieldController.h"
#include "BattleStacksController.h"
#include "BattleWindow.h"

#include "../CPlayerInterface.h"
#include "../gui/CursorHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/MouseButton.h"
#include "../gui/WindowHandler.h"
#include "../media/IMusicPlayer.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IFont.h"
#include "../render/Graphics.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../windows/CMessage.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/CSpellWindow.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../render/IRenderHandler.h"
#include "../adventureMap/CInGameConsole.h"
#include "../eventsSDL/InputHandler.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/json/JsonUtils.h"


void BattleConsole::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	Point line1 (pos.x + pos.w/2, pos.y +  8);
	Point line2 (pos.x + pos.w/2, pos.y + 24);

	auto visibleText = getVisibleText();

	if(visibleText.size() > 0)
		to.drawText(line1, FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, visibleText[0]);

	if(visibleText.size() > 1)
		to.drawText(line2, FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, visibleText[1]);
}

std::vector<std::string> BattleConsole::getVisibleText() const
{
	// high priority texts that hide battle log entries
	for(const auto & text : {consoleText, hoverText})
	{
		if (text.empty())
			continue;

		auto result = CMessage::breakText(text, pos.w, FONT_SMALL);

		if(result.size() > 2 && text.find('\n') != std::string::npos)
		{
			// Text has too many lines to fit into console, but has line breaks. Try ignore them and fit text that way
			std::string cleanText = boost::algorithm::replace_all_copy(text, "\n", " ");
			result = CMessage::breakText(cleanText, pos.w, FONT_SMALL);
		}

		if(result.size() > 2)
			result.resize(2);
		return result;
	}

	// log is small enough to fit entirely - display it as such
	if (logEntries.size() < 3)
		return logEntries;

	return { logEntries[scrollPosition - 1], logEntries[scrollPosition] };
}

std::vector<std::string> BattleConsole::splitText(const std::string &text)
{
	std::vector<std::string> lines;
	std::vector<std::string> output;

	boost::split(lines, text, boost::is_any_of("\n"));

	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	for(const auto & line : lines)
	{
		if (font->getStringWidth(text) < pos.w)
		{
			output.push_back(line);
		}
		else
		{
			std::vector<std::string> substrings = CMessage::breakText(line, pos.w, FONT_SMALL);
			output.insert(output.end(), substrings.begin(), substrings.end());
		}
	}
	return output;
}

bool BattleConsole::addText(const std::string & text)
{
	logGlobal->trace("CBattleConsole message: %s", text);

	auto newLines = splitText(text);

	logEntries.insert(logEntries.end(), newLines.begin(), newLines.end());
	scrollPosition = (int)logEntries.size()-1;
	redraw();
	return true;
}
void BattleConsole::scrollUp(ui32 by)
{
	if(scrollPosition > static_cast<int>(by))
		scrollPosition -= by;
	redraw();
}

void BattleConsole::scrollDown(ui32 by)
{
	if(scrollPosition + by < logEntries.size())
		scrollPosition += by;
	redraw();
}

BattleConsole::BattleConsole(const BattleInterface & owner, std::shared_ptr<CPicture> backgroundSource, const Point & objectPos, const Point & imagePos, const Point &size)
	: CIntObject(LCLICK)
	, owner(owner)
	, scrollPosition(-1)
	, enteringText(false)
{
	OBJECT_CONSTRUCTION;
	pos += objectPos;
	pos.w = size.x;
	pos.h = size.y;

	background = std::make_shared<CPicture>(backgroundSource->getSurface(), Rect(imagePos, size), 0, 0 );
}

void BattleConsole::deactivate()
{
	if (enteringText)
		GAME->interface()->cingconsole->endEnteringText(false);

	CIntObject::deactivate();
}

void BattleConsole::clickPressed(const Point & cursorPosition)
{
	if(owner.makingTurn() && !owner.openingPlaying())
	{
		ENGINE->windows().createAndPushWindow<BattleConsoleWindow>(boost::algorithm::join(logEntries, "\n"));
	}
}

void BattleConsole::setEnteringMode(bool on)
{
	consoleText.clear();

	if (on)
	{
		assert(enteringText == false);
		ENGINE->input().startTextInput(pos);
	}
	else
	{
		assert(enteringText == true);
		ENGINE->input().stopTextInput();
	}
	enteringText = on;
	redraw();
}

void BattleConsole::setEnteredText(const std::string & text)
{
	assert(enteringText == true);
	consoleText = text;
	redraw();
}

void BattleConsole::write(const std::string & Text)
{
	hoverText = Text;
	redraw();
}

void BattleConsole::clearIfMatching(const std::string & Text)
{
	if (hoverText == Text)
		clear();
}

void BattleConsole::clear()
{
	write({});
}

BattleConsoleWindow::BattleConsoleWindow(const std::string & text)
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 429;
	pos.h = 434;

	updateShadow();
	center();

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	buttonOk = std::make_shared<CButton>(Point(183, 388), AnimationPath::builtin("IOKAY"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	Rect textArea(18, 17, 393, 354);
	textBoxBackgroundBorder = std::make_shared<TransparentFilledRectangle>(textArea, ColorRGBA(0, 0, 0, 75), ColorRGBA(128, 100, 75));
	textBox = std::make_shared<CTextBox>(text, textArea.resize(-5), CSlider::BROWN);
	if(textBox->slider)
		textBox->slider->scrollToMax();
}

const CGHeroInstance * BattleHero::instance()
{
	return hero;
}

void BattleHero::tick(uint32_t msPassed)
{
	size_t groupIndex = static_cast<size_t>(phase);

	float timePassed = msPassed / 1000.f;

	flagCurrentFrame += currentSpeed * timePassed;
	currentFrame += currentSpeed * timePassed;

	if(flagCurrentFrame >= flagAnimation->size(0))
		flagCurrentFrame -= flagAnimation->size(0);

	if(currentFrame >= animation->size(groupIndex))
	{
		currentFrame -= animation->size(groupIndex);
		switchToNextPhase();
	}
}

void BattleHero::render(Canvas & canvas)
{
	size_t groupIndex = static_cast<size_t>(phase);

	auto flagFrame = flagAnimation->getImage(flagCurrentFrame, 0, true);
	auto heroFrame = animation->getImage(currentFrame, groupIndex, true);

	Point heroPosition = pos.center() - parent->pos.topLeft() - heroFrame->dimensions() / 2;
	Point flagPosition = pos.center() - parent->pos.topLeft() - flagFrame->dimensions() / 2;

	if(defender)
		flagPosition += Point(-4, -41);
	else
		flagPosition += Point(4, -41);

	canvas.draw(flagFrame, flagPosition);
	canvas.draw(heroFrame, heroPosition);
}

void BattleHero::pause()
{
	currentSpeed = 0.f;
}

void BattleHero::play()
{
	//H3 speed: 10 fps ( 100 ms per frame)
	currentSpeed = 10.f;
}

float BattleHero::getFrame() const
{
	return currentFrame;
}

void BattleHero::collectRenderableObjects(BattleRenderer & renderer)
{
	auto hex = defender ? BattleHex(GameConstants::BFIELD_WIDTH-1) : BattleHex(0);

	renderer.insert(EBattleFieldLayer::HEROES, hex, [this](BattleRenderer::RendererRef canvas)
	{
		render(canvas);
	});
}

void BattleHero::onPhaseFinished(const std::function<void()> & callback)
{
	phaseFinishedCallback = callback;
}

void BattleHero::setPhase(EHeroAnimType newPhase)
{
	nextPhase = newPhase;
	switchToNextPhase(); //immediately switch to next phase and then restore idling phase
	nextPhase = EHeroAnimType::HOLDING;
}

void BattleHero::heroLeftClicked()
{
	if(owner.actionsController->heroSpellcastingModeActive()) //we are casting a spell
		return;

	if(!hero || !owner.makingTurn())
		return;

	if(owner.getBattle()->battleCanCastSpell(hero, spells::Mode::HERO) == ESpellCastProblem::OK) //check conditions
	{
		ENGINE->cursor().set(Cursor::Map::POINTER);
		ENGINE->windows().createAndPushWindow<CSpellWindow>(hero, owner.getCurrentPlayerInterface());
	}
}

void BattleHero::heroRightClicked()
{
	if(settings["battle"]["stickyHeroInfoWindows"].Bool())
		return;

	Point windowPosition;
	if(ENGINE->screenDimensions().x < 1000)
	{
		windowPosition.x = (!defender) ? owner.fieldController->pos.left() + 1 : owner.fieldController->pos.right() - 79;
		windowPosition.y = owner.fieldController->pos.y + 135;
	}
	else
	{
		windowPosition.x = (!defender) ? owner.fieldController->pos.left() - 93 : owner.fieldController->pos.right() + 15;
		windowPosition.y = owner.fieldController->pos.y;
	}

	InfoAboutHero targetHero;
	if(owner.makingTurn() || settings["session"]["spectate"].Bool())
	{
		auto h = defender ? owner.defendingHeroInstance : owner.attackingHeroInstance;
		targetHero.initFromHero(h, InfoAboutHero::EInfoLevel::INBATTLE);
		ENGINE->windows().createAndPushWindow<HeroInfoWindow>(targetHero, &windowPosition);
	}
}

void BattleHero::switchToNextPhase()
{
	phase = nextPhase;
	currentFrame = 0.f;

	auto copy = phaseFinishedCallback;
	phaseFinishedCallback.clear();
	copy();
}

BattleHero::BattleHero(const BattleInterface & owner, const CGHeroInstance * hero, bool defender):
	defender(defender),
	hero(hero),
	owner(owner),
	phase(EHeroAnimType::HOLDING),
	nextPhase(EHeroAnimType::HOLDING),
	currentSpeed(0.f),
	currentFrame(0.f),
	flagCurrentFrame(0.f)
{
	AnimationPath animationPath;

	if(!hero->getHeroType()->battleImage.empty())
		animationPath = hero->getHeroType()->battleImage;
	else
	if(hero->gender == EHeroGender::FEMALE)
		animationPath = hero->getHeroClass()->imageBattleFemale;
	else
		animationPath = hero->getHeroClass()->imageBattleMale;

	animation = ENGINE->renderHandler().loadAnimation(animationPath, EImageBlitMode::WITH_SHADOW);

	pos.w = 64;
	pos.h = 136;
	pos.x = owner.fieldController->pos.x + (defender ? (owner.fieldController->pos.w - pos.w) : 0);
	pos.y = owner.fieldController->pos.y;

	if(defender)
		animation->verticalFlip();

	if(defender)
		flagAnimation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CMFLAGR"), EImageBlitMode::COLORKEY);
	else
		flagAnimation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CMFLAGL"), EImageBlitMode::COLORKEY);

	flagAnimation->playerColored(hero->tempOwner);

	switchToNextPhase();
	play();

	addUsedEvents(TIME);
}

QuickSpellPanel::QuickSpellPanel(BattleInterface & owner)
	: CIntObject(0), owner(owner)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | SHOW_POPUP | MOVE | INPUT_MODE_CHANGE);

	pos = Rect(0, 0, 52, 600);
	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	rect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w + 1, pos.h + 1), ColorRGBA(0, 0, 0, 0), ColorRGBA(241, 216, 120, 255));

	create();
}

std::vector<std::tuple<SpellID, bool>> QuickSpellPanel::getSpells() const
{
	std::vector<SpellID> spellIds;
	std::vector<bool> spellIdsFromSetting;
	for(int i = 0; i < QUICKSPELL_SLOTS; i++)
	{
		std::string spellIdentifier = persistentStorage["quickSpell"][std::to_string(i)].String();
		SpellID id;
		try
		{
			id = SpellID::decode(spellIdentifier);
		}
		catch(const IdentifierResolutionException& e)
		{
			id = SpellID::NONE;
		}	
		spellIds.push_back(id);	
		spellIdsFromSetting.push_back(id != SpellID::NONE);	
	}

	// autofill empty slots with spells if possible
	auto hero = owner.getBattle()->battleGetMyHero();
	for(int i = 0; i < QUICKSPELL_SLOTS; i++)
	{
		if(spellIds[i] != SpellID::NONE)
			continue;

		for(const auto & availableSpellID : LIBRARY->spellh->getDefaultAllowed())
		{
			const auto * availableSpell = availableSpellID.toSpell();
			if(!availableSpell->isAdventure() && !availableSpell->isCreatureAbility() && hero->canCastThisSpell(availableSpell) && !vstd::contains(spellIds, availableSpell->getId()))
			{
				spellIds[i] = availableSpell->getId();
				break;
			}	
		}
	}

	std::vector<std::tuple<SpellID, bool>> ret;
	for(int i = 0; i < QUICKSPELL_SLOTS; i++)
		ret.push_back(std::make_tuple(spellIds[i], spellIdsFromSetting[i]));
	return ret;
}

void QuickSpellPanel::create()
{
	OBJECT_CONSTRUCTION;

	const JsonNode config = JsonUtils::assembleFromFiles("config/shortcutsConfig");

	labels.clear();
	buttons.clear();
	buttonsDisabled.clear();

	auto hero = owner.getBattle()->battleGetMyHero();
	if(!hero)
		return;

	auto spells = getSpells();
	for(int i = 0; i < QUICKSPELL_SLOTS; i++) {
		SpellID id;
		bool fromSettings;
		std::tie(id, fromSettings) = spells[i];

		auto button = std::make_shared<CButton>(Point(2, 7 + 50 * i), AnimationPath::builtin("spellint"), CButton::tooltip(), [this, id, hero](){
			if(id.hasValue() && id.toSpell()->canBeCast(owner.getBattle().get(), spells::Mode::HERO, hero))
			{
				owner.castThisSpell(id);
			}
		});
		button->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("spellint"), id != SpellID::NONE ? id.num + 1 : 0));
		button->addPopupCallback([this, i, hero](){
			ENGINE->input().hapticFeedback();
			ENGINE->windows().createAndPushWindow<CSpellWindow>(hero, owner.curInt.get(), true, [this, i](SpellID spell){
				Settings configID = persistentStorage.write["quickSpell"][std::to_string(i)];
				configID->String() = spell == SpellID::NONE ? "" : spell.toSpell()->identifier;
				create();
			});
		});

		if(fromSettings)
			buttonsIsAutoGenerated.push_back(std::make_shared<TransparentFilledRectangle>(Rect(45, 37 + 50 * i, 5, 5), Colors::ORANGE));

		if(!id.hasValue() || !id.toSpell()->canBeCast(owner.getBattle().get(), spells::Mode::HERO, hero))
		{
			buttonsDisabled.push_back(std::make_shared<TransparentFilledRectangle>(Rect(2, 7 + 50 * i, 48, 36), ColorRGBA(0, 0, 0, 172)));
		}
		if(ENGINE->input().getCurrentInputMode() == InputMode::KEYBOARD_AND_MOUSE)
			labels.push_back(std::make_shared<CLabel>(7, 10 + 50 * i, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, config["keyboard"]["battleSpellShortcut" + std::to_string(i)].String()));

		buttons.push_back(button);
	}
}

void QuickSpellPanel::show(Canvas & to)
{
	showAll(to);
	CIntObject::show(to);
}

void QuickSpellPanel::inputModeChanged(InputMode modi)
{
	create();
	redraw();
}

HeroInfoBasicPanel::HeroInfoBasicPanel(const InfoAboutHero & hero, Point * position, bool initializeBackground)
	: CIntObject(0)
{
	OBJECT_CONSTRUCTION;
	if (position != nullptr)
		moveTo(*position);

	if(initializeBackground)
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("CHRPOP"));
		background->setPlayerColor(hero.owner);
	}

	initializeData(hero);
}

void HeroInfoBasicPanel::initializeData(const InfoAboutHero & hero)
{
	OBJECT_CONSTRUCTION;
	auto attack = hero.details->primskills[0];
	auto defense = hero.details->primskills[1];
	auto power = hero.details->primskills[2];
	auto knowledge = hero.details->primskills[3];
	auto morale = hero.details->morale;
	auto luck = hero.details->luck;
	auto currentSpellPoints = hero.details->mana;
	auto maxSpellPoints = hero.details->manaLimit;

	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), hero.getIconIndex(), 0, 10, 6));

	//primary stats
	labels.push_back(std::make_shared<CLabel>(9, 75, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[380] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 87, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[381] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 99, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[382] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 111, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[383] + ":"));

	labels.push_back(std::make_shared<CLabel>(69, 87, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(attack)));
	labels.push_back(std::make_shared<CLabel>(69, 99, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(defense)));
	labels.push_back(std::make_shared<CLabel>(69, 111, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(power)));
	labels.push_back(std::make_shared<CLabel>(69, 123, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(knowledge)));

	//morale+luck
	labels.push_back(std::make_shared<CLabel>(9, 131, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[384] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 143, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[385] + ":"));

	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("IMRL22"), std::clamp(morale + 3, 0, 6), 0, 47, 131));
	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ILCK22"), std::clamp(luck + 3, 0, 6), 0, 47, 143));

	//spell points
	labels.push_back(std::make_shared<CLabel>(39, 174, EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[387]));
	labels.push_back(std::make_shared<CLabel>(39, 186, EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, std::to_string(currentSpellPoints) + "/" + std::to_string(maxSpellPoints)));
}

void HeroInfoBasicPanel::update(const InfoAboutHero & updatedInfo)
{
	icons.clear();
	labels.clear();

	initializeData(updatedInfo);
}

void HeroInfoBasicPanel::show(Canvas & to)
{
	showAll(to);
	CIntObject::show(to);
}


StackInfoBasicPanel::StackInfoBasicPanel(const CStack * stack, bool initializeBackground)
	: CIntObject(0)
{
	OBJECT_CONSTRUCTION;

	if(initializeBackground)
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("CCRPOP"));
		background->pos.y += 37;
		background->setPlayerColor(stack->getOwner());
		background2 = std::make_shared<CPicture>(ImagePath::builtin("CHRPOP"));
		background2->setPlayerColor(stack->getOwner());
	}

	initializeData(stack);
}

void StackInfoBasicPanel::initializeData(const CStack * stack)
{
	OBJECT_CONSTRUCTION;

	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), stack->creatureId().getNum() + 2, 0, 10, 6));
	labels.push_back(std::make_shared<CLabel>(10 + 58, 6 + 64, FONT_MEDIUM, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, TextOperations::formatMetric(stack->getCount(), 4)));

	int damageMultiplier = 1;
	if (stack->hasBonusOfType(BonusType::SIEGE_WEAPON))
	{
		static const auto bonusSelector =
			Selector::sourceTypeSel(BonusSource::ARTIFACT).Or(
			Selector::sourceTypeSel(BonusSource::HERO_BASE_SKILL)).And(
			Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK)));

		damageMultiplier += stack->valOfBonuses(bonusSelector);
	}

	auto attack = std::to_string(LIBRARY->creatures()->getByIndex(stack->creatureIndex())->getAttack(stack->isShooter())) + "(" + std::to_string(stack->getAttack(stack->isShooter())) + ")";
	auto defense = std::to_string(LIBRARY->creatures()->getByIndex(stack->creatureIndex())->getDefense(stack->isShooter())) + "(" + std::to_string(stack->getDefense(stack->isShooter())) + ")";
	auto damage = std::to_string(damageMultiplier * stack->getMinDamage(stack->isShooter())) + "-" + std::to_string(damageMultiplier * stack->getMaxDamage(stack->isShooter()));
	auto health = stack->getMaxHealth();
	auto morale = stack->moraleVal();
	auto luck = stack->luckVal();

	auto killed = stack->getKilled();
	auto healthRemaining = TextOperations::formatMetric(std::max(stack->getAvailableHealth() - (stack->getCount() - 1) * health, (si64)0), 4);

	//primary stats*/
	labels.push_back(std::make_shared<CLabel>(9, 75, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[380] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 87, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[381] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 99, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[386] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 111, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[389] + ":"));

	labels.push_back(std::make_shared<CLabel>(69, 87, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, attack));
	labels.push_back(std::make_shared<CLabel>(69, 99, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, defense));
	labels.push_back(std::make_shared<CLabel>(69, 111, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, damage));
	labels.push_back(std::make_shared<CLabel>(69, 123, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(health)));

	//morale+luck
	labels.push_back(std::make_shared<CLabel>(9, 131, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[384] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 143, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[385] + ":"));

	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("IMRL22"), std::clamp(morale + 3, 0, 6), 0, 47, 131));
	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ILCK22"), std::clamp(luck + 3, 0, 6), 0, 47, 143));

	//extra information
	labels.push_back(std::make_shared<CLabel>(9, 168, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.battleWindow.killed") + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 180, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[389] + ":"));

	labels.push_back(std::make_shared<CLabel>(69, 180, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(killed)));
	labels.push_back(std::make_shared<CLabel>(69, 192, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, healthRemaining));

	//spells
	static const Point firstPos(15, 206); // position of 1st spell box
	static const Point offset(0, 38);  // offset of each spell box from previous

	for(int i = 0; i < 3; i++)
		icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("SpellInt"), 78, 0, firstPos.x + offset.x * i, firstPos.y + offset.y * i));

	int printed=0; //how many effect pics have been printed
	std::vector<SpellID> spells = stack->activeSpells();
	for(SpellID effect : spells)
	{
		//not all effects have graphics (for eg. Acid Breath)
		//for modded spells iconEffect is added to SpellInt.def
		const bool hasGraphics = (effect < SpellID::THUNDERBOLT) || (effect >= SpellID::AFTER_LAST);

		if (hasGraphics)
		{
			//FIXME: support permanent duration
			auto spellBonuses = stack->getBonuses(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(effect)));

			if (spellBonuses->empty())
				throw std::runtime_error("Failed to find effects for spell " + effect.toSpell()->getJsonKey());

			int duration = spellBonuses->front()->turnsRemain;

			icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("SpellInt"), effect.getNum() + 1, 0, firstPos.x + offset.x * printed, firstPos.y + offset.y * printed));
			if(settings["general"]["enableUiEnhancements"].Bool())
				labels.push_back(std::make_shared<CLabel>(firstPos.x + offset.x * printed + 46, firstPos.y + offset.y * printed + 36, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(duration)));
			if(++printed >= 3 || (printed == 2 && spells.size() > 3)) // interface limit reached
				break;
		}
	}

	if(spells.size() == 0)
		labelsMultiline.push_back(std::make_shared<CMultiLineLabel>(Rect(firstPos.x, firstPos.y, 48, 36), EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[674]));
	if(spells.size() > 3)
		labelsMultiline.push_back(std::make_shared<CMultiLineLabel>(Rect(firstPos.x + offset.x * 2, firstPos.y + offset.y * 2 - 4, 48, 36), EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, "..."));
}

void StackInfoBasicPanel::update(const CStack * updatedInfo)
{
	icons.clear();
	labels.clear();
	labelsMultiline.clear();

	initializeData(updatedInfo);
}

void StackInfoBasicPanel::show(Canvas & to)
{
	showAll(to);
	CIntObject::show(to);
}

HeroInfoWindow::HeroInfoWindow(const InfoAboutHero & hero, Point * position)
	: CWindowObject(RCLICK_POPUP | SHADOW_DISABLED, ImagePath::builtin("CHRPOP"))
{
	OBJECT_CONSTRUCTION;
	if (position != nullptr)
		moveTo(*position);

	background->setPlayerColor(hero.owner); //maybe add this functionality to base class?

	content = std::make_shared<HeroInfoBasicPanel>(hero, nullptr, false);
}

BattleResultWindow::BattleResultWindow(const BattleResult & br, CPlayerInterface & _owner, bool allowReplay)
	: owner(_owner)
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(ImagePath::builtin("CPRESULT"));
	background->setPlayerColor(owner.playerID);
	pos = center(background->pos);

	exit = std::make_shared<CButton>(Point(384, 505), AnimationPath::builtin("iok6432.def"), std::make_pair("", ""), [&](){ bExitf();}, EShortcut::GLOBAL_ACCEPT);
	exit->setBorderColor(Colors::METALLIC_GOLD);
	
	if(allowReplay || owner.cb->getStartInfo()->extraOptionsInfo.unlimitedReplay)
	{
		repeat = std::make_shared<CButton>(Point(24, 505), AnimationPath::builtin("icn6432.def"), std::make_pair("", ""), [&](){ bRepeatf();}, EShortcut::GLOBAL_CANCEL);
		repeat->setBorderColor(Colors::METALLIC_GOLD);
		labels.push_back(std::make_shared<CLabel>(232, 520, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.battleResultsWindow.applyResultsLabel")));
	}

	if(br.winner == BattleSide::ATTACKER)
	{
		labels.push_back(std::make_shared<CLabel>(59, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[410]));
	}
	else
	{
		labels.push_back(std::make_shared<CLabel>(59, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[411]));
	}
	
	if(br.winner == BattleSide::DEFENDER)
	{
		labels.push_back(std::make_shared<CLabel>(412, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[410]));
	}
	else
	{
		labels.push_back(std::make_shared<CLabel>(408, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[411]));
	}

	labels.push_back(std::make_shared<CLabel>(232, 302, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,  LIBRARY->generaltexth->allTexts[407]));
	labels.push_back(std::make_shared<CLabel>(232, 332, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[408]));
	labels.push_back(std::make_shared<CLabel>(232, 428, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[409]));

	std::string sideNames[2] = {"N/A", "N/A"};

	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		auto heroInfo = owner.cb->getBattle(br.battleID)->battleGetHeroInfo(i);
		const int xs[] = {21, 392};

		if(heroInfo.portraitSource.isValid()) //attacking hero
		{
			icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), heroInfo.getIconIndex(), 0, xs[static_cast<int>(i)], 38));
			sideNames[static_cast<int>(i)] = heroInfo.name;
		}
		else
		{
			auto stacks = owner.cb->getBattle(br.battleID)->battleGetAllStacks();
			vstd::erase_if(stacks, [i](const CStack * stack) //erase stack of other side and not coming from garrison
			{
				return stack->unitSide() != i || !stack->base;
			});

			auto best = vstd::maxElementByFun(stacks, [](const CStack * stack)
			{
				return stack->unitType()->getAIValue();
			});

			if(best != stacks.end()) //should be always but to be safe...
			{
				icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), (*best)->unitType()->getIconIndex(), 0, xs[static_cast<int>(i)], 38));
				sideNames[static_cast<int>(i)] = (*best)->unitType()->getNamePluralTranslated();
			}
		}
	}

	//printing attacker and defender's names
	labels.push_back(std::make_shared<CLabel>(89, 37, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, sideNames[0]));
	labels.push_back(std::make_shared<CLabel>(381, 53, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, sideNames[1]));

	//printing casualties
	for(auto step : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if(br.casualties[step].size()==0)
		{
			labels.push_back(std::make_shared<CLabel>(235, 360 + 97 * static_cast<int>(step), FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[523]));
		}
		else
		{
			int xPos = 235 - ((int)br.casualties[step].size()*32 + ((int)br.casualties[step].size() - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + static_cast<int>(step) * 97;
			for(auto & elem : br.casualties[step])
			{
				auto creature = elem.first.toEntity(LIBRARY);
				if (creature->getId() == CreatureID::ARROW_TOWERS )
					continue; // do not show destroyed towers in battle results

				icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), creature->getIconIndex(), 0, xPos, yPos));
				std::ostringstream amount;
				amount<<elem.second;
				labels.push_back(std::make_shared<CLabel>(xPos + 16, yPos + 42, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, amount.str()));
				xPos += 42;
			}
		}
	}

	auto resources = getResources(br);

	description = std::make_shared<CTextBox>(resources.resultText.toString(), Rect(69, 203, 330, 68), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	videoPlayer = std::make_shared<VideoWidget>(Point(107, 70), resources.prologueVideo, resources.loopedVideo, false);

	ENGINE->music().playMusic(resources.musicName, false, true);
}

BattleResultResources BattleResultWindow::getResources(const BattleResult & br)
{
	//printing result description
	bool weAreAttacker = owner.cb->getBattle(br.battleID)->battleGetMySide() == BattleSide::ATTACKER;
	bool weAreDefender = !weAreAttacker;
	bool weWon = (br.winner == BattleSide::ATTACKER && weAreAttacker) || (br.winner == BattleSide::DEFENDER && !weAreAttacker);
	bool isSiege = owner.cb->getBattle(br.battleID)->battleGetDefendedTown() != nullptr;

	BattleResultResources resources;

	if(weWon)
	{
		if(isSiege && weAreDefender)
		{
			resources.musicName = AudioPath::builtin("Music/Defend Castle");
			resources.prologueVideo = VideoPath::builtin("DEFENDALL.BIK");
			resources.loopedVideo = VideoPath::builtin("defendloop.bik");
		}
		else
		{
			resources.musicName = AudioPath::builtin("Music/Win Battle");
			resources.prologueVideo = VideoPath::builtin("WIN3.BIK");
			resources.loopedVideo = VideoPath::builtin("WIN3.BIK");
		}

		switch(br.result)
		{
		case EBattleResult::NORMAL:
			resources.resultText.appendTextID("core.genrltxt.304");
			break;
		case EBattleResult::ESCAPE:
			resources.resultText.appendTextID("core.genrltxt.303");
			break;
		case EBattleResult::SURRENDER:
			resources.resultText.appendTextID("core.genrltxt.302");
			break;
		default:
			throw std::runtime_error("Invalid battle result!");
		}

		const CGHeroInstance * ourHero = owner.cb->getBattle(br.battleID)->battleGetMyHero();
		if (ourHero)
		{
			resources.resultText.appendTextID("core.genrltxt.305");
			resources.resultText.replaceTextID(ourHero->getNameTextID());
			resources.resultText.replaceNumber(br.exp[weAreAttacker ? BattleSide::ATTACKER : BattleSide::DEFENDER]);
		}
	}
	else // we lose
	{
		switch(br.result)
		{
		case EBattleResult::NORMAL:
			resources.resultText.appendTextID("core.genrltxt.311");
			resources.musicName = AudioPath::builtin("Music/LoseCombat");
			resources.prologueVideo = VideoPath::builtin("LBSTART.BIK");
			resources.loopedVideo = VideoPath::builtin("LBLOOP.BIK");
			break;
		case EBattleResult::ESCAPE:
			resources.resultText.appendTextID("core.genrltxt.310");
			resources.musicName = AudioPath::builtin("Music/Retreat Battle");
			resources.prologueVideo = VideoPath::builtin("RTSTART.BIK");
			resources.loopedVideo = VideoPath::builtin("RTLOOP.BIK");
			break;
		case EBattleResult::SURRENDER:
			resources.resultText.appendTextID("core.genrltxt.309");
			resources.musicName = AudioPath::builtin("Music/Surrender Battle");
			resources.prologueVideo = VideoPath::builtin("SURRENDER.BIK");
			resources.loopedVideo = VideoPath::builtin("SURRENDER.BIK");
			break;
		default:
				throw std::runtime_error("Invalid battle result!");
		}

		if(isSiege && weAreDefender)
		{
			resources.musicName = AudioPath::builtin("Music/LoseCastle");
			resources.prologueVideo = VideoPath::builtin("LOSECSTL.BIK");
			resources.loopedVideo = VideoPath::builtin("LOSECSLP.BIK");
		}
	}

	return resources;
}

void BattleResultWindow::activate()
{
	owner.showingDialog->setBusy();
	CIntObject::activate();
}

void BattleResultWindow::buttonPressed(int button)
{
	if (resultCallback)
		resultCallback(button);

	CPlayerInterface &intTmp = owner; //copy reference because "this" will be destructed soon

	close();

	if(ENGINE->windows().topWindow<BattleWindow>())
		ENGINE->windows().popWindows(1); //pop battle interface if present

	//Result window and battle interface are gone. We requested all dialogs to be closed before opening the battle,
	//so we can be sure that there is no dialogs left on GUI stack.
	intTmp.showingDialog->setFree();
}

void BattleResultWindow::bExitf()
{
	buttonPressed(0);
}

void BattleResultWindow::bRepeatf()
{
	buttonPressed(1);
}

StackQueue::StackQueue(bool Embedded, BattleInterface & owner)
	: embedded(Embedded),
	owner(owner)
{
	OBJECT_CONSTRUCTION;

	uint32_t queueSize = QUEUE_SIZE_BIG;

	if(embedded)
	{
		int32_t queueSmallOutsideYOffset = 65;
		bool queueSmallOutside = settings["battle"]["queueSmallOutside"].Bool() && (pos.y - queueSmallOutsideYOffset) >= 0;
		queueSize = std::clamp(static_cast<int>(settings["battle"]["queueSmallSlots"].Float()), 1, queueSmallOutside ? ENGINE->screenDimensions().x / 41 : 19);

		pos.w = queueSize * 41;
		pos.h = 49;
		pos.x += parent->pos.w/2 - pos.w/2;
		pos.y += queueSmallOutside ? -queueSmallOutsideYOffset : 10;
	}
	else
	{
		pos.w = 800;
		pos.h = 85;
		pos.x += 0;
		pos.y -= pos.h;

		background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));
	}

	stackBoxes.resize(queueSize);
	for (int i = 0; i < stackBoxes.size(); i++)
	{
		stackBoxes[i] = std::make_shared<StackBox>(this);
		stackBoxes[i]->moveBy(Point(1 + (embedded ? 41 : 80) * i, 0));
	}
}

void StackQueue::show(Canvas & to)
{
	if (embedded)
		showAll(to);
	CIntObject::show(to);
}

void StackQueue::update()
{
	std::vector<battle::Units> queueData;

	owner.getBattle()->battleGetTurnOrder(queueData, stackBoxes.size(), 0);

	size_t boxIndex = 0;
	ui32 tmpTurn = -1;

	for(size_t turn = 0; turn < queueData.size() && boxIndex < stackBoxes.size(); turn++)
	{
		for(size_t unitIndex = 0; unitIndex < queueData[turn].size() && boxIndex < stackBoxes.size(); boxIndex++, unitIndex++)
		{
			ui32 currentTurn = owner.round + turn;
			stackBoxes[boxIndex]->setUnit(queueData[turn][unitIndex], turn, tmpTurn != currentTurn && owner.round != 0 && (!embedded || tmpTurn != -1) ? (std::optional<ui32>)currentTurn : std::nullopt);
			tmpTurn = currentTurn;
		}
	}

	for(; boxIndex < stackBoxes.size(); boxIndex++)
		stackBoxes[boxIndex]->setUnit(nullptr);
}

int32_t StackQueue::getSiegeShooterIconID()
{
	return owner.siegeController->getSiegedTown()->getFactionID().getNum();
}

std::optional<uint32_t> StackQueue::getHoveredUnitIdIfAny() const
{
	for(const auto & stackBox : stackBoxes)
	{
		if(stackBox->isHovered())
		{
			return stackBox->getBoundUnitID();
		}
	}

	return std::nullopt;
}

StackQueue::StackBox::StackBox(StackQueue * owner):
	CIntObject(SHOW_POPUP | HOVER), owner(owner)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin(owner->embedded ? "StackQueueSmall" : "StackQueueLarge"));

	pos.w = background->pos.w;
	pos.h = background->pos.h;

	if(owner->embedded)
	{
		icon = std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), 0, 0, 5, 2);
		amount = std::make_shared<CLabel>(pos.w/2, pos.h - 7, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
		roundRect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, 2, 48), ColorRGBA(0, 0, 0, 255), ColorRGBA(0, 255, 0, 255));
	}
	else
	{
		icon = std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), 0, 0, 9, 1);
		amount = std::make_shared<CLabel>(pos.w/2, pos.h - 8, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);
		roundRect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, 15, 18), ColorRGBA(0, 0, 0, 255), ColorRGBA(241, 216, 120, 255));
		round = std::make_shared<CLabel>(6, 9, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		Point iconPos(pos.w - 16, pos.h - 16);

		defendIcon = std::make_shared<CPicture>(ImagePath::builtin("battle/QueueDefend"), iconPos);
		waitIcon = std::make_shared<CPicture>(ImagePath::builtin("battle/QueueWait"), iconPos);

		defendIcon->setEnabled(false);
		waitIcon->setEnabled(false);
	}
	roundRect->disable();
}

void StackQueue::StackBox::setUnit(const battle::Unit * unit, size_t turn, std::optional<ui32> currentTurn)
{
	if(unit)
	{
		boundUnitID = unit->unitId();
		background->setPlayerColor(unit->unitOwner());
		icon->visible = true;

		// temporary code for mod compatibility:
		// first, set icon that should definitely exist (arrow tower icon in base vcmi mod)
		// second, try to switch to icon that should be provided by mod
		// if mod is not up to date and does have arrow tower icon yet - second setFrame call will fail and retain previously set image
		// for 1.2 release & later next line should be moved into 'else' block
		icon->setFrame(unit->creatureIconIndex(), 0);
		if (unit->unitType()->getId() == CreatureID::ARROW_TOWERS)
			icon->setFrame(owner->getSiegeShooterIconID(), 1);

		roundRect->setEnabled(currentTurn.has_value());
		if(!owner->embedded)
			round->setEnabled(currentTurn.has_value());

		amount->setText(TextOperations::formatMetric(unit->getCount(), 4));
		if(currentTurn && !owner->embedded)
		{
			std::string tmp = std::to_string(*currentTurn);
			const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
			int len = font->getStringWidth(tmp);
			roundRect->pos.w = len + 6;
			round->pos = Rect(roundRect->pos.center().x, roundRect->pos.center().y, 0, 0);
			round->setText(tmp);
		}

		if(!owner->embedded)
		{
			bool defended = unit->defended(turn) || (turn > 0 && unit->defended(turn - 1));
			bool waited = unit->waited(turn) && !defended;

			defendIcon->setEnabled(defended);
			waitIcon->setEnabled(waited);
		}
	}
	else
	{
		boundUnitID = std::nullopt;
		background->setPlayerColor(PlayerColor::NEUTRAL);
		icon->visible = false;
		icon->setFrame(0);
		amount->setText("");
		if(!owner->embedded)
		{
			defendIcon->setEnabled(false);
			waitIcon->setEnabled(false);
		}
	}
}

std::optional<uint32_t> StackQueue::StackBox::getBoundUnitID() const
{
	return boundUnitID;
}

bool StackQueue::StackBox::isBoundUnitHighlighted() const
{
	auto unitIdsToHighlight = owner->owner.stacksController->getHoveredStacksUnitIds();
	return vstd::contains(unitIdsToHighlight, getBoundUnitID());
}

void StackQueue::StackBox::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	if(isBoundUnitHighlighted())
		to.drawBorder(background->pos, Colors::CYAN, 2);
}

void StackQueue::StackBox::show(Canvas & to)
{
	CIntObject::show(to);

	if(isBoundUnitHighlighted())
		to.drawBorder(background->pos, Colors::CYAN, 2);
}

void StackQueue::StackBox::showPopupWindow(const Point & cursorPosition)
{
	auto stacks = owner->owner.getBattle()->battleGetAllStacks();
	for(const CStack * stack : stacks)
		if(boundUnitID.has_value() && stack->unitId() == *boundUnitID)
			ENGINE->windows().createAndPushWindow<CStackWindow>(stack, true);
}

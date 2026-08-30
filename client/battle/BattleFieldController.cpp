/*
 * BattleFieldController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleFieldController.h"

#include "BattleActionsController.h"
#include "BattleEffectsController.h"
#include "BattleInterface.h"
#include "BattleHero.h"
#include "BattleObstacleController.h"
#include "BattleProjectileController.h"
#include "CreatureAnimation.h"
#include "BattleRenderer.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"
#include "BattleWindow.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../adventureMap/CInGameConsole.h"
#include "../eventsSDL/InputHandler.h"
#include "../eventsSDL/ControllerPromptFamily.h"
#include "../gui/CursorHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/ShortcutHandler.h"
#include "../gui/TextAlignment.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../render/IFont.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../../lib/BattleFieldHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"

namespace
{
constexpr int CONTROLLER_PROMPT_WIDTH = 196;
constexpr int CONTROLLER_PROMPT_HEIGHT = 27;
constexpr int CONTROLLER_PROMPT_GAP = 3;
constexpr int CONTROLLER_FACE_GLYPH_SIZE = 24;
constexpr int CONTROLLER_SHOULDER_GLYPH_WIDTH = 72;
constexpr int CONTROLLER_SHOULDER_GLYPH_HEIGHT = 20;
constexpr int CONTROLLER_GLYPH_TEXT_SPACING = 4;
constexpr int CONTROLLER_TEXT_OUTLINE_WIDTH = 1;
constexpr double CONTROLLER_UNIT_NAVIGATION_CONE_ALIGNMENT = 0.5;
constexpr ColorRGBA CONTROLLER_GENERIC_LABEL_COLOR(58, 40, 20, 255);

Rect controllerPromptBounds(const Point & battlefieldOrigin)
{
	return Rect(battlefieldOrigin.x + 79, battlefieldOrigin.y + 86, 642, 469);
}

std::string fitControllerPromptText(const std::string & text, const IFont & font, int maxWidth)
{
	if(maxWidth <= 0)
		return {};
	if(font.getStringWidth(text) <= maxWidth)
		return text;

	const std::string ellipsis = "...";
	if(font.getStringWidth(ellipsis) > maxWidth)
		return {};

	std::string result;
	for(size_t index = 0; index < text.size();)
	{
		const size_t characterSize = TextOperations::getUnicodeCharacterSize(text[index]);
		const std::string candidate = result + text.substr(index, characterSize) + ellipsis;
		if(font.getStringWidth(candidate) > maxWidth)
			break;
		result += text.substr(index, characterSize);
		index += characterSize;
	}
	return result + ellipsis;
}

int controllerOutlinedTextWidth(const IFont & font, const std::string & text)
{
	return text.empty() ? 0 : static_cast<int>(font.getStringWidth(text)) + CONTROLLER_TEXT_OUTLINE_WIDTH * 2;
}

void drawControllerOutlinedText(Canvas & to, const Point & center, const std::string & text)
{
	to.drawText(center + Point(-CONTROLLER_TEXT_OUTLINE_WIDTH, 0), FONT_MEDIUM, Colors::BLACK, ETextAlignment::CENTER, text);
	to.drawText(center + Point(CONTROLLER_TEXT_OUTLINE_WIDTH, 0), FONT_MEDIUM, Colors::BLACK, ETextAlignment::CENTER, text);
	to.drawText(center + Point(0, -CONTROLLER_TEXT_OUTLINE_WIDTH), FONT_MEDIUM, Colors::BLACK, ETextAlignment::CENTER, text);
	to.drawText(center + Point(0, CONTROLLER_TEXT_OUTLINE_WIDTH), FONT_MEDIUM, Colors::BLACK, ETextAlignment::CENTER, text);
	to.drawText(center, FONT_MEDIUM, Colors::WHITE, ETextAlignment::CENTER, text);
}

std::optional<std::string> battleFaceButtonSprite(
	ControllerPrompt::Family family, const std::string & binding, bool pressed)
{
	if(binding != "a" && binding != "b" && binding != "x" && binding != "y")
		return std::nullopt;
	const auto state = pressed ? ControllerPrompt::State::PRESSED : ControllerPrompt::State::NORMAL;
	return ControllerPrompt::faceButtonSprite(family, binding, state);
}

std::optional<std::string> battleShoulderSprite(
	ControllerPrompt::Family family,
	const std::vector<std::string> & previousBindings,
	const std::vector<std::string> & nextBindings,
	bool pressed = false)
{
	if(previousBindings.size() != 1 || nextBindings.size() != 1
		|| previousBindings.front() != "leftshoulder" || nextBindings.front() != "rightshoulder")
		return std::nullopt;
	const std::string state = pressed ? "pressed" : "normal";
	if(family == ControllerPrompt::Family::PLAYSTATION)
		return "controllerActionBar/playstation-shoulders-" + state + ".png";
	if(family == ControllerPrompt::Family::GENERIC || family == ControllerPrompt::Family::XBOX)
		return "controllerActionBar/generic-shoulders-" + state + ".png";
	return std::nullopt;
}
}

namespace HexMasks
{
	// mask definitions that has set to 1 the edges present in the hex edges highlight image
	/*
	    /\
	   0  1
	  /    \
	 |      |
	 5      2
	 |      |
	  \    /
	   4  3
	    \/
	*/
	enum HexEdgeMasks {
		empty                 = 0b000000, // empty used when wanting to keep indexes the same but no highlight should be displayed
		topLeft               = 0b000001,
		topRight              = 0b000010,
		right                 = 0b000100,
		bottomRight           = 0b001000,
		bottomLeft            = 0b010000,
		left                  = 0b100000,
						  
		top                   = 0b000011,
		bottom                = 0b011000,
		topRightHalfCorner    = 0b000110,
		bottomRightHalfCorner = 0b001100,
		bottomLeftHalfCorner  = 0b110000,
		topLeftHalfCorner     = 0b100001,

		rightTopAndBottom     = 0b001010, // special case, right half can be drawn instead of only top and bottom
		leftTopAndBottom      = 0b010001, // special case, left half can be drawn instead of only top and bottom
						  
		rightHalf             = 0b001110,
		leftHalf              = 0b110001,
						  
		topRightCorner        = 0b000111,
		bottomRightCorner     = 0b011100,
		bottomLeftCorner      = 0b111000,
		topLeftCorner         = 0b100011
	};
}

/// predefined offsets for earthquake screen shake, matching H3 behavior
static const std::array<Point, 17> earthquakeShakeOffsets = {{
	{ 0,  0},
	{ 2,  2},
	{ 4,  1},
	{ 3, -2},
	{ 0, -6},
	{ 2, -2},
	{-1,  3},
	{-5,  4},
	{-8,  6},
	{-5,  4},
	{-8,  6},
	{-4,  2},
	{-1,  1},
	{-3, -3},
	{-5, -7},
	{-7, -5},
	{-2, -3},
}};

static const std::map<int, int> hexEdgeMaskToFrameIndex =
{
    { HexMasks::empty, 0 },
    { HexMasks::topLeft, 1 },
    { HexMasks::topRight, 2 },
    { HexMasks::right, 3 },
    { HexMasks::bottomRight, 4 },
    { HexMasks::bottomLeft, 5 },
    { HexMasks::left, 6 },
    { HexMasks::top, 7 },
    { HexMasks::bottom, 8 },
    { HexMasks::topRightHalfCorner, 9 },
    { HexMasks::bottomRightHalfCorner, 10 },
    { HexMasks::bottomLeftHalfCorner, 11 },
    { HexMasks::topLeftHalfCorner, 12 },
    { HexMasks::rightTopAndBottom, 13 },
    { HexMasks::leftTopAndBottom, 14 },
    { HexMasks::rightHalf, 13 },
    { HexMasks::leftHalf, 14 },
    { HexMasks::topRightCorner, 15 },
    { HexMasks::bottomRightCorner, 16 },
    { HexMasks::bottomLeftCorner, 17 },
    { HexMasks::topLeftCorner, 18 }
};

namespace
{
constexpr uint32_t NAVIGATION_SETTLE_DELAY_MS = 16;
constexpr uint32_t NAVIGATION_INITIAL_REPEAT_MS = 320;
constexpr uint32_t NAVIGATION_REPEAT_MS = 110;
constexpr double NAVIGATION_DIRECTION_CHANGE_DEGREES = 30.0;

double angularDistance(double x1, double y1, double x2, double y2)
{
	const double first = std::atan2(y1, x1) * 180.0 / M_PI;
	const double second = std::atan2(y2, x2) * 180.0 / M_PI;
	double result = std::fmod(std::abs(first - second), 360.0);
	return result > 180.0 ? 360.0 - result : result;
}

bool isMeleeAction(PossiblePlayerBattleAction::Actions action)
{
	return action == PossiblePlayerBattleAction::ATTACK
		|| action == PossiblePlayerBattleAction::LONG_WEAPON_ATTACK
		|| action == PossiblePlayerBattleAction::WALK_AND_ATTACK
		|| action == PossiblePlayerBattleAction::ATTACK_AND_RETURN;
}
}

void BattleFieldController::RepeatState::start(bool settleFirst)
{
	elapsed = 0;
	initialPending = settleFirst;
	repeating = false;
}

bool BattleFieldController::RepeatState::ready(uint32_t msPassed)
{
	elapsed += msPassed;
	if(initialPending)
	{
		if(elapsed < NAVIGATION_SETTLE_DELAY_MS)
			return false;
		initialPending = false;
		elapsed = 0;
		return true;
	}

	const uint32_t threshold = repeating ? NAVIGATION_REPEAT_MS : NAVIGATION_INITIAL_REPEAT_MS;
	if(elapsed < threshold)
		return false;
	elapsed -= threshold;
	repeating = true;
	return true;
}

void BattleFieldController::RepeatState::reset()
{
	elapsed = 0;
	initialPending = false;
	repeating = false;
}

void BattleFieldController::NavigationState::update(bool horizontal, double value)
{
	(horizontal ? x : y) = value;
	const bool nextActive = !vstd::isAlmostZero(x) || !vstd::isAlmostZero(y);
	if(!nextActive)
	{
		active = false;
		directionX = directionY = 0.0;
		repeat.reset();
		return;
	}

	if(!active || angularDistance(directionX, directionY, x, y) > NAVIGATION_DIRECTION_CHANGE_DEGREES)
		repeat.start(true);
	active = true;
	directionX = x;
	directionY = y;
}

bool BattleFieldController::NavigationState::ready(uint32_t msPassed)
{
	if(!active)
		return false;
	return repeat.ready(msPassed);
}

void BattleFieldController::NavigationState::reset()
{
	x = y = directionX = directionY = 0.0;
	active = false;
	repeat.reset();
}

BattleFieldController::BattleFieldController(BattleInterface & owner):
	owner(owner)
{
	OBJECT_CONSTRUCTION;

	//preparing cells and hexes
	cellBorder = ENGINE->renderHandler().loadImage(ImagePath::builtin("CCELLGRD.BMP"), EImageBlitMode::COLORKEY);
	cellShade = ENGINE->renderHandler().loadImage(ImagePath::builtin("CCELLSHD.BMP"), EImageBlitMode::SIMPLE);
	cellUnitMovementHighlight = ENGINE->renderHandler().loadImage(ImagePath::builtin("UnitMovementHighlight.PNG"), EImageBlitMode::COLORKEY);
	cellUnitMaxMovementHighlight = ENGINE->renderHandler().loadImage(ImagePath::builtin("UnitMaxMovementHighlight.PNG"), EImageBlitMode::COLORKEY);

	rangedFullDamageLimitImages = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("battle/rangeHighlights/rangeHighlightsGreen.json"), EImageBlitMode::COLORKEY);
	shootingRangeLimitImages = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("battle/rangeHighlights/rangeHighlightsRed.json"), EImageBlitMode::COLORKEY);

	if(!owner.siegeController)
	{
		auto bfieldType = owner.getBattle()->battleGetBattlefieldType();

		if(bfieldType == BattleField::NONE)
			logGlobal->error("Invalid battlefield returned for current battle");
		else
			background = ENGINE->renderHandler().loadImage(bfieldType.getInfo()->graphics, EImageBlitMode::OPAQUE);
	}
	else
	{
		auto backgroundName = owner.siegeController->getBattleBackgroundName();
		background = ENGINE->renderHandler().loadImage(backgroundName, EImageBlitMode::OPAQUE);
	}

	pos.w = background->width();
	pos.h = background->height();

	backgroundWithHexes = std::make_unique<Canvas>(Point(background->width(), background->height()), CanvasScalingPolicy::AUTO);

	updateAccessibleHexes();
	addUsedEvents(LCLICK | SHOW_POPUP | MOVE | TIME | GESTURE | INPUT_MODE_CHANGE);
}

BattleFieldController::~BattleFieldController()
{
	ENGINE->cursor().setControllerNativeHidden(false);
}

void BattleFieldController::startShakeAnimation()
{
	shakeFrameTotal = 17 * std::clamp(static_cast<int>(4.0f - AnimationControls::getAnimationSpeedFactor()), 1, 3);
	shakeFrameCounter = 0;
	shakeOffset = earthquakeShakeOffsets[0];
}

void BattleFieldController::updateShake()
{
	if (shakeFrameCounter >= shakeFrameTotal)
	{
		shakeOffset = Point(0, 0);
		return;
	}

	shakeFrameCounter++;
	if (shakeFrameCounter < shakeFrameTotal)
		shakeOffset = earthquakeShakeOffsets[shakeFrameCounter % earthquakeShakeOffsets.size()];
	else
		shakeOffset = Point(0, 0);
}

void BattleFieldController::activate()
{
	GAME->interface()->cingconsole->pos = this->pos;
	CIntObject::activate();
	if(isControllerNativeMode())
	{
		ENGINE->cursor().setControllerNativeHidden(true);
		ensureControllerFocus();
	}
}

void BattleFieldController::deactivate()
{
	resetControllerInput();
	CIntObject::deactivate();
}

void BattleFieldController::inputModeChanged(InputMode inputMode)
{
	if(inputMode == InputMode::CONTROLLER && !controllerCursorMode)
	{
		ENGINE->cursor().setControllerNativeHidden(true);
		if(isActive())
		{
			if(controllerRestoreHex.isValid())
				focusHex(controllerRestoreHex);
			else
				focusActiveStack();
		}
		refreshControllerPresentation();
	}
	else
	{
		ENGINE->cursor().setControllerNativeHidden(false);
	}
}

void BattleFieldController::createHeroes()
{
	OBJECT_CONSTRUCTION;

	// create heroes as part of our constructor for correct positioning inside battlefield
	if(owner.attackingHeroInstance)
		owner.attackingHero = std::make_shared<BattleHero>(owner, owner.attackingHeroInstance, false);

	if(owner.defendingHeroInstance)
		owner.defendingHero = std::make_shared<BattleHero>(owner, owner.defendingHeroInstance, true);
}

void BattleFieldController::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if (!on && pos.isInside(finalPosition))
		clickPressed(finalPosition);
}

void BattleFieldController::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	Point distance = currentPosition - initialPosition;

	if (distance.length() < settings["battle"]["swipeAttackDistance"].Float())
		hoveredHex = getHexAtPosition(initialPosition);
	else
		hoveredHex = BattleHex::INVALID;

	currentAttackOriginPoint = currentPosition;

	if (pos.isInside(initialPosition))
		owner.actionsController->onHexHovered(getHoveredHex());
}

void BattleFieldController::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	if(isControllerNativeMode())
		return;

	currentAttackOriginPoint = cursorPosition;

	// hex rects of the bottom rows extend under the command panel, so only treat the cursor as hovering a hex
	// when it is actually over the battlefield - otherwise hovering the panel leaks a unit range highlight.
	// This handler is also invoked when the cursor is over the battle queue: in that case keep the cursor and
	// status bar in sync with the queue-hovered stack, so that pointing at the queue is equivalent to pointing
	// at the stack on the battlefield.
	if (pos.isInside(cursorPosition))
	{
		hoveredHex = getHexAtPosition(cursorPosition);
		owner.actionsController->onHexHovered(getHoveredHex());
	}
	else if (const CStack * queueStack = getQueueHoveredStack())
	{
		hoveredHex = BattleHex::INVALID;
		owner.actionsController->onHexHovered(queueStack->getPosition());
	}
	else
	{
		hoveredHex = BattleHex::INVALID;
		owner.actionsController->onHoverEnded();
	}
}

bool BattleFieldController::isControllerNativeMode() const
{
	return ENGINE->input().getCurrentInputMode() == InputMode::CONTROLLER && !controllerCursorMode;
}

bool BattleFieldController::isControllerCursorMode() const
{
	return controllerCursorMode;
}

void BattleFieldController::focusHex(const BattleHex & hex, std::optional<uint32_t> unitId)
{
	if(!hex.isValid())
		return;

	hoveredHex = hex;
	controllerRestoreHex = hex;
	const CStack * stack = owner.getBattle()->battleGetStackByPos(hex, true);
	controllerFocusedUnitId = unitId ? unitId
		: stack != nullptr ? std::optional<uint32_t>(stack->unitId()) : std::nullopt;
	currentAttackOriginPoint = hexPositionAbsolute(hex).center();
	const auto meleeDirections = controllerMeleeDirections();
	if(!meleeDirections.empty())
		currentAttackOriginPoint = attackDirectionPoint(hex, meleeDirections.front());
}

void BattleFieldController::ensureControllerFocus()
{
	if(hoveredHex.isValid())
		return;
	focusActiveStack();
}

void BattleFieldController::focusActiveStack()
{
	const CStack * activeStack = owner.stacksController->getActiveStack();
	if(activeStack != nullptr)
	{
		focusHex(activeStack->getPosition(), activeStack->unitId());
		refreshControllerPresentation();
	}
}

void BattleFieldController::restoreControllerFocus(const BattleHex & hex)
{
	if(!isControllerNativeMode() || !hex.isValid())
		return;
	focusHex(hex);
	refreshControllerPresentation();
}

void BattleFieldController::controllerStackMoved(const CStack * stack)
{
	if(isControllerNativeMode() && stack != nullptr && controllerFocusedUnitId == stack->unitId())
	{
		focusHex(stack->getPosition(), stack->unitId());
		refreshControllerPresentation();
	}
}

void BattleFieldController::controllerStackRemoved(uint32_t stackId)
{
	if(controllerFocusedUnitId == stackId)
	{
		controllerFocusedUnitId.reset();
		focusActiveStack();
	}
}

BattleHex BattleFieldController::getControllerFocusedHex() const
{
	return controllerRestoreHex.isValid() ? controllerRestoreHex : hoveredHex;
}

void BattleFieldController::updateNavigationOwner(NavigationOwner changedOwner)
{
	NavigationState & changed = changedOwner == NavigationOwner::HEX ? hexNavigation : unitNavigation;
	NavigationState & other = changedOwner == NavigationOwner::HEX ? unitNavigation : hexNavigation;

	if(navigationOwner == NavigationOwner::NONE && changed.active)
		navigationOwner = changedOwner;
	else if(navigationOwner == changedOwner && !changed.active)
		navigationOwner = other.active
			? (changedOwner == NavigationOwner::HEX ? NavigationOwner::UNIT : NavigationOwner::HEX)
			: NavigationOwner::NONE;
}

bool BattleFieldController::controllerAxisMoved(int instanceId, const std::vector<EShortcut> & actions, double value)
{
	bool handled = false;
	if(controllerInstance != -1 && controllerInstance != instanceId)
		resetControllerInput();
	controllerInstance = instanceId;

	for(const auto action : actions)
	{
		switch(action)
		{
		case EShortcut::MOUSE_CURSOR_X:
			hexNavigation.update(true, value);
			updateNavigationOwner(NavigationOwner::HEX);
			handled = true;
			break;
		case EShortcut::MOUSE_CURSOR_Y:
			hexNavigation.update(false, value);
			updateNavigationOwner(NavigationOwner::HEX);
			handled = true;
			break;
		case EShortcut::MOUSE_SWIPE_X:
			unitNavigation.update(true, value);
			updateNavigationOwner(NavigationOwner::UNIT);
			handled = true;
			break;
		case EShortcut::MOUSE_SWIPE_Y:
			unitNavigation.update(false, value);
			updateNavigationOwner(NavigationOwner::UNIT);
			handled = true;
			break;
		default:
			break;
		}
	}
	return handled;
}

void BattleFieldController::resetControllerInput()
{
	const bool presentationChanged = controllerPressedHex.isValid() || controllerMeleeRepeatDirection.has_value();
	hexNavigation.reset();
	unitNavigation.reset();
	navigationOwner = NavigationOwner::NONE;
	controllerInstance = -1;
	controllerPressedHex = BattleHex::INVALID;
	controllerPressedAction = PossiblePlayerBattleAction::INVALID;
	controllerMeleeRepeatDirection.reset();
	controllerMeleeRepeat.reset();
	if(presentationChanged)
		redraw();
}

BattleHex::EDir BattleFieldController::controllerHexDirection() const
{
	const double angle = std::atan2(hexNavigation.directionY, hexNavigation.directionX) * 180.0 / M_PI;
	if(angle < -150.0 || angle >= 150.0) return BattleHex::LEFT;
	if(angle < -90.0) return BattleHex::TOP_LEFT;
	if(angle < -30.0) return BattleHex::TOP_RIGHT;
	if(angle < 30.0) return BattleHex::RIGHT;
	if(angle < 90.0) return BattleHex::BOTTOM_RIGHT;
	return BattleHex::BOTTOM_LEFT;
}

bool BattleFieldController::moveControllerHex()
{
	ensureControllerFocus();
	if(!hoveredHex.isValid())
		return false;

	auto tryDirection = [this](BattleHex::EDir direction)
	{
		try
		{
			focusHex(hoveredHex.cloneInDirection(direction, true));
			return true;
		}
		catch(const std::out_of_range &)
		{
			return false;
		}
	};

	const auto direction = controllerHexDirection();
	if(tryDirection(direction))
		return true;

	const double angle = std::atan2(hexNavigation.directionY, hexNavigation.directionX) * 180.0 / M_PI;
	const double verticalAngle = hexNavigation.directionY < 0.0 ? -90.0 : 90.0;
	if(std::abs(angle - verticalAngle) > 10.0)
		return false;
	if(direction == BattleHex::TOP_LEFT) return tryDirection(BattleHex::TOP_RIGHT);
	if(direction == BattleHex::TOP_RIGHT) return tryDirection(BattleHex::TOP_LEFT);
	if(direction == BattleHex::BOTTOM_LEFT) return tryDirection(BattleHex::BOTTOM_RIGHT);
	if(direction == BattleHex::BOTTOM_RIGHT) return tryDirection(BattleHex::BOTTOM_LEFT);
	return false;
}

bool BattleFieldController::browseControllerUnit()
{
	ensureControllerFocus();
	if(!hoveredHex.isValid())
		return false;

	const auto stackCenter = [this](const CStack & stack)
	{
		Point result = hexPositionAbsolute(stack.getPosition()).center();
		if(stack.doubleWide())
			result = (result + hexPositionAbsolute(stack.occupiedHex()).center()) / 2;
		return result;
	};

	const CStack * focusedStack = controllerFocusedUnitId
		? owner.getBattle()->battleGetStackByID(*controllerFocusedUnitId, false)
		: nullptr;
	const CStack * originStack = focusedStack != nullptr && focusedStack->getPosition() == hoveredHex
		? focusedStack : nullptr;
	const Point origin = originStack ? stackCenter(*originStack) : hexPositionAbsolute(hoveredHex).center();
	const double directionMagnitudeSquared = unitNavigation.directionX * unitNavigation.directionX
		+ unitNavigation.directionY * unitNavigation.directionY;
	const CStack * bestStack = nullptr;
	bool bestInsideCone = false;
	double bestAlignment = -1.0;
	si64 bestDistance = 0;

	for(const CStack * stack : owner.getBattle()->battleGetAllStacks())
	{
		if(!stack->isValidTarget(false) || !stack->getPosition().isValid())
			continue;
		if(originStack && stack->unitId() == originStack->unitId())
			continue;

		const Point candidate = stackCenter(*stack);
		const double deltaX = candidate.x - origin.x;
		const double deltaY = candidate.y - origin.y;
		const double dot = deltaX * unitNavigation.directionX + deltaY * unitNavigation.directionY;
		if(dot <= 0.0)
			continue;
		const si64 distance = static_cast<si64>(deltaX * deltaX + deltaY * deltaY);
		const double alignment = dot * dot / (static_cast<double>(distance) * directionMagnitudeSquared);
		const bool insideCone = alignment >= CONTROLLER_UNIT_NAVIGATION_CONE_ALIGNMENT;
		const bool betterInsideCone = insideCone && (!bestInsideCone || distance < bestDistance
			|| (distance == bestDistance && alignment > bestAlignment));
		const bool betterOutsideCone = !insideCone && !bestInsideCone && (alignment > bestAlignment
			|| (vstd::isAlmostEqual(alignment, bestAlignment) && distance < bestDistance));
		if(!bestStack || betterInsideCone || betterOutsideCone
			|| (insideCone == bestInsideCone && vstd::isAlmostEqual(alignment, bestAlignment)
				&& distance == bestDistance && stack->unitId() < bestStack->unitId()))
		{
			bestStack = stack;
			bestInsideCone = insideCone;
			bestAlignment = alignment;
			bestDistance = distance;
		}
	}

	if(bestStack == nullptr)
		return false;
	focusHex(bestStack->getPosition(), bestStack->unitId());
	return true;
}

void BattleFieldController::refreshControllerPresentation()
{
	if(!isControllerNativeMode() || !hoveredHex.isValid())
		return;
	const auto action = controllerActionAt(hoveredHex);
	if(action)
		owner.actionsController->onHexHovered(hoveredHex, *action);
	else
		owner.actionsController->onHexHovered(hoveredHex);
	redraw();
}

std::optional<PossiblePlayerBattleAction> BattleFieldController::controllerActionAt(const BattleHex & hex) const
{
	if(!isControllerNativeMode() || !hex.isValid() || owner.stacksController->getActiveStack() == nullptr)
		return std::nullopt;

	const CStack * activeStack = owner.stacksController->getActiveStack();
	if(owner.isInTacticsMode() || activeStack->hasBonusOfType(BonusType::SIEGE_WEAPON))
	{
		if(owner.getBattle()->battleGetStackByPos(hex, true) != nullptr)
			return PossiblePlayerBattleAction(PossiblePlayerBattleAction::CREATURE_INFO);
		return std::nullopt;
	}
	return owner.actionsController->legalActionAt(hex);
}

bool BattleFieldController::controllerPrimaryPressed()
{
	const auto action = controllerActionAt(hoveredHex);
	if(!action)
		return false;
	controllerPressedHex = hoveredHex;
	controllerPressedAction = action->get();
	redraw();
	return true;
}

bool BattleFieldController::controllerPrimaryReleased()
{
	const BattleHex pressedHex = controllerPressedHex;
	const auto pressedAction = controllerPressedAction;
	controllerPressedHex = BattleHex::INVALID;
	controllerPressedAction = PossiblePlayerBattleAction::INVALID;
	const auto action = controllerActionAt(hoveredHex);
	redraw();
	if(!action || hoveredHex != pressedHex || action->get() != pressedAction)
		return false;

	if(pressedAction == PossiblePlayerBattleAction::CREATURE_INFO)
	{
		const CStack * stack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);
		if(stack == nullptr)
			return false;
		owner.windowObject->openControllerInspect();
		return true;
	}

	owner.actionsController->onHexLeftClicked(hoveredHex);
	return true;
}

bool BattleFieldController::controllerInspectAvailable() const
{
	return ENGINE->input().getCurrentInputMode() == InputMode::CONTROLLER && hoveredHex.isValid()
		&& owner.stacksController->getActiveStack() != nullptr
		&& !owner.actionsController->heroSpellcastingModeActive()
		&& !owner.actionsController->creatureSpellcastingModeActive()
		&& owner.getBattle()->battleGetStackByPos(hoveredHex, true) != nullptr;
}

Point BattleFieldController::attackDirectionPoint(const BattleHex & target, BattleHex::EDir direction) const
{
	const BattleHexArray & neighbours = target.getAllNeighbouringTiles();
	if(direction >= BattleHex::TOP_LEFT && direction <= BattleHex::LEFT)
		return hexPositionAbsolute(neighbours[direction]).center();
	if(direction == BattleHex::TOP)
		return (hexPositionAbsolute(neighbours[0]).center() + hexPositionAbsolute(neighbours[1]).center()) / 2 + Point(0, -5);
	if(direction == BattleHex::BOTTOM)
		return (hexPositionAbsolute(neighbours[3]).center() + hexPositionAbsolute(neighbours[4]).center()) / 2 + Point(0, 5);
	return Point::makeInvalid();
}

std::vector<BattleHex::EDir> BattleFieldController::controllerMeleeDirections() const
{
	std::vector<BattleHex::EDir> result;
	const auto action = controllerActionAt(hoveredHex);
	const CStack * attacker = owner.stacksController->getActiveStack();
	if(!action || attacker == nullptr || !isMeleeAction(action->get()))
		return result;

	const auto available = owner.getBattle()->battleGetAvailableHexes(attacker, false);
	const bool allowLongWeapon = action->get() == PossiblePlayerBattleAction::LONG_WEAPON_ATTACK;
	for(int index = 0; index < 8; ++index)
	{
		const auto direction = static_cast<BattleHex::EDir>(index);
		if(!owner.getBattle()->battleCanAttackHex(available, attacker, hoveredHex, direction))
			continue;
		if(owner.getBattle()->fromWhichHexAttack(attacker, hoveredHex, direction, allowLongWeapon).isValid())
			result.push_back(direction);
	}
	return result;
}

bool BattleFieldController::controllerMeleeDirectionAvailable() const
{
	return controllerMeleeDirections().size() > 1;
}

bool BattleFieldController::cycleControllerMeleeDirection(bool forward)
{
	const auto directions = controllerMeleeDirections();
	if(directions.size() <= 1)
		return false;

	const auto current = selectAttackDirection(hoveredHex);
	auto iterator = std::find(directions.begin(), directions.end(), current);
	size_t index = iterator == directions.end() ? 0 : std::distance(directions.begin(), iterator);
	index = forward ? (index + 1) % directions.size() : (index + directions.size() - 1) % directions.size();
	currentAttackOriginPoint = attackDirectionPoint(hoveredHex, directions[index]);
	refreshControllerPresentation();
	return true;
}

bool BattleFieldController::controllerMeleeDirectionPressed(bool forward)
{
	if(!cycleControllerMeleeDirection(forward))
		return false;
	controllerMeleeRepeatDirection = forward;
	controllerMeleeRepeat.start(false);
	return true;
}

bool BattleFieldController::controllerMeleeDirectionReleased(bool forward)
{
	if(controllerMeleeRepeatDirection != forward)
		return false;
	controllerMeleeRepeatDirection.reset();
	controllerMeleeRepeat.reset();
	redraw();
	return true;
}

void BattleFieldController::toggleControllerCursorMode()
{
	if(ENGINE->input().getCurrentInputMode() != InputMode::CONTROLLER)
		return;

	ENGINE->input().clearControllerAxisMotion();
	resetControllerInput();
	if(!controllerCursorMode)
	{
		controllerRestoreHex = hoveredHex;
		controllerCursorMode = true;
		ENGINE->cursor().setControllerNativeHidden(false);
	}
	else
	{
		controllerCursorMode = false;
		ENGINE->cursor().setControllerNativeHidden(true);
		if(controllerRestoreHex.isValid())
			focusHex(controllerRestoreHex);
		else
			focusActiveStack();
		refreshControllerPresentation();
	}
	redraw();
}

std::string BattleFieldController::getControllerPrimaryActionName() const
{
	const auto action = controllerActionAt(hoveredHex);
	if(!action)
		return "none";
	switch(action->get())
	{
	case PossiblePlayerBattleAction::MOVE_STACK: return "move";
	case PossiblePlayerBattleAction::ATTACK:
	case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
	case PossiblePlayerBattleAction::WALK_AND_ATTACK:
	case PossiblePlayerBattleAction::ATTACK_AND_RETURN: return "attack";
	case PossiblePlayerBattleAction::SHOOT: return "shoot";
	case PossiblePlayerBattleAction::CREATURE_INFO: return "inspect";
	default: return "none";
	}
}

bool BattleFieldController::drawControllerPrompts(Canvas & to)
{
	const auto actionName = getControllerPrimaryActionName();
	const auto family = ENGINE->input().getActiveControllerPromptFamily();
	if(family == ControllerPrompt::Family::UNKNOWN)
		return false;

	const auto acceptBindings = ENGINE->shortcuts().getJoystickButtonBindings(EShortcut::GLOBAL_ACCEPT);
	const auto inspectBindings = ENGINE->shortcuts().getJoystickButtonBindings(EShortcut::GLOBAL_CANCEL);
	const auto previousBindings = ENGINE->shortcuts().getJoystickButtonBindings(EShortcut::BATTLE_DEFEND);
	const auto nextBindings = ENGINE->shortcuts().getJoystickButtonBindings(EShortcut::BATTLE_WAIT);
	const auto directionSprite = battleShoulderSprite(family, previousBindings, nextBindings);
	const bool directionBindingsAvailable = previousBindings.size() == 1 && nextBindings.size() == 1;
	const bool drawPrimary = actionName != "none" && acceptBindings.size() == 1;
	const bool drawInspect = controllerInspectAvailable() && inspectBindings.size() == 1;
	const bool drawDirection = drawPrimary && controllerMeleeDirectionAvailable()
		&& (directionSprite || directionBindingsAvailable);
	const int rowCount = static_cast<int>(drawPrimary) + static_cast<int>(drawInspect) + static_cast<int>(drawDirection);
	if(rowCount == 0)
		return false;

	const Rect bounds = controllerPromptBounds(pos.topLeft());
	const Rect anchor = hexPositionAbsolute(hoveredHex);
	const int rowWidth = std::min(CONTROLLER_PROMPT_WIDTH, bounds.w);
	const int groupHeight = rowCount * CONTROLLER_PROMPT_HEIGHT + (rowCount - 1) * CONTROLLER_PROMPT_GAP;
	const int x = std::clamp(anchor.center().x - rowWidth / 2, bounds.x, bounds.x + bounds.w - rowWidth);
	const bool preferBelow = actionName != "inspect";
	int y = preferBelow
		? anchor.y + anchor.h + CONTROLLER_PROMPT_GAP
		: anchor.y - groupHeight - CONTROLLER_PROMPT_GAP;
	if(y < bounds.y || y + groupHeight > bounds.y + bounds.h)
	{
		y = preferBelow
			? anchor.y - groupHeight - CONTROLLER_PROMPT_GAP
			: anchor.y + anchor.h + CONTROLLER_PROMPT_GAP;
	}
	y = std::clamp(y, bounds.y, bounds.y + bounds.h - groupHeight);

	std::optional<Rect> directionRow;
	std::optional<Rect> primaryRow;
	std::optional<Rect> inspectRow;
	auto addRow = [&](std::optional<Rect> & row)
	{
		row = Rect(x, y, rowWidth, CONTROLLER_PROMPT_HEIGHT);
		y += CONTROLLER_PROMPT_HEIGHT + CONTROLLER_PROMPT_GAP;
	};
	if(drawDirection) addRow(directionRow);
	if(drawPrimary) addRow(primaryRow);
	if(drawInspect) addRow(inspectRow);

	const auto & font = ENGINE->renderHandler().loadFont(FONT_MEDIUM);
	auto contentLayout = [&](const Rect & row, int textWidth, int glyphWidth, int glyphHeight)
	{
		const int spacing = glyphWidth > 0 ? CONTROLLER_GLYPH_TEXT_SPACING : 0;
		const int contentWidth = glyphWidth + spacing + textWidth;
		const int rightmostX = bounds.x + std::max(0, bounds.w - contentWidth);
		const int glyphX = std::clamp(row.center().x - contentWidth / 2, bounds.x, rightmostX);
		return std::pair(
			Point(glyphX, row.center().y - glyphHeight / 2),
			Point(glyphX + glyphWidth + spacing + textWidth / 2, row.center().y));
	};
	auto drawSprite = [&](const std::string & spritePath, const Point & topLeft)
	{
		auto [iterator, inserted] = controllerPromptSprites.try_emplace(spritePath);
		if(inserted)
			iterator->second = ENGINE->renderHandler().loadImage(ImagePath::builtin(spritePath), EImageBlitMode::COLORKEY);
		to.draw(iterator->second, topLeft);
	};
	auto drawFaceRow = [&](const Rect & row, const std::vector<std::string> & bindings, const std::string & text, bool pressed)
	{
		const std::string fittedText = fitControllerPromptText(text, *font,
			std::max(0, bounds.w - CONTROLLER_FACE_GLYPH_SIZE - CONTROLLER_GLYPH_TEXT_SPACING
				- CONTROLLER_TEXT_OUTLINE_WIDTH * 2));
		const auto [glyph, textCenter] = contentLayout(row,
			controllerOutlinedTextWidth(*font, fittedText),
			CONTROLLER_FACE_GLYPH_SIZE, CONTROLLER_FACE_GLYPH_SIZE);
		const auto spritePath = battleFaceButtonSprite(family, bindings.front(), pressed);
		if(spritePath)
		{
			drawSprite(*spritePath, glyph);
			if(ControllerPrompt::usesRuntimeFaceLabel(family))
			{
				to.drawText(glyph + Point(CONTROLLER_FACE_GLYPH_SIZE / 2, CONTROLLER_FACE_GLYPH_SIZE / 2),
					FONT_SMALL, CONTROLLER_GENERIC_LABEL_COLOR, ETextAlignment::CENTER,
					ControllerPrompt::buttonLabel(family, bindings.front()));
			}
		}
		else
		{
			to.drawText(glyph + Point(CONTROLLER_FACE_GLYPH_SIZE / 2, CONTROLLER_FACE_GLYPH_SIZE / 2),
				FONT_SMALL, Colors::WHITE,
				ETextAlignment::CENTER, ControllerPrompt::buttonLabel(family, bindings.front()));
		}
		drawControllerOutlinedText(to, textCenter, fittedText);
	};
	auto drawDirectionRow = [&](const Rect & row)
	{
		const std::string directionText = LIBRARY->generaltexth->translate("vcmi.battleWindow.controller.attackDirection");
		if(directionSprite)
		{
			const std::string fittedText = fitControllerPromptText(directionText, *font,
				std::max(0, bounds.w - CONTROLLER_SHOULDER_GLYPH_WIDTH - CONTROLLER_GLYPH_TEXT_SPACING
					- CONTROLLER_TEXT_OUTLINE_WIDTH * 2));
			const auto [glyph, textCenter] = contentLayout(row,
				controllerOutlinedTextWidth(*font, fittedText),
				CONTROLLER_SHOULDER_GLYPH_WIDTH, CONTROLLER_SHOULDER_GLYPH_HEIGHT);
			drawSprite(*directionSprite, glyph);
			if(controllerMeleeRepeatDirection)
			{
				const bool forward = *controllerMeleeRepeatDirection;
				const auto pressedSprite = battleShoulderSprite(family, previousBindings, nextBindings, true);
				if(pressedSprite)
				{
					auto [iterator, inserted] = controllerPromptSprites.try_emplace(*pressedSprite);
					if(inserted)
						iterator->second = ENGINE->renderHandler().loadImage(
							ImagePath::builtin(*pressedSprite), EImageBlitMode::COLORKEY);
					const int halfWidth = CONTROLLER_SHOULDER_GLYPH_WIDTH / 2;
					const int sourceX = forward ? halfWidth : 0;
					to.draw(iterator->second, glyph + Point(sourceX, 0),
						Rect(sourceX, 0, halfWidth, CONTROLLER_SHOULDER_GLYPH_HEIGHT));
				}
			}
			drawControllerOutlinedText(to, textCenter, fittedText);
			return;
		}

		const std::string bindingText = ControllerPrompt::buttonLabel(family, previousBindings.front())
			+ "/" + ControllerPrompt::buttonLabel(family, nextBindings.front()) + " " + directionText;
		const std::string fittedText = fitControllerPromptText(bindingText, *font,
			std::max(0, bounds.w - CONTROLLER_TEXT_OUTLINE_WIDTH * 2));
		const auto [glyph, textCenter] = contentLayout(row,
			controllerOutlinedTextWidth(*font, fittedText), 0, 0);
		static_cast<void>(glyph);
		drawControllerOutlinedText(to, textCenter, fittedText);
	};

	if(directionRow)
		drawDirectionRow(*directionRow);
	if(primaryRow)
	{
		const std::string textKey = "vcmi.battleWindow.controller." + actionName;
		drawFaceRow(*primaryRow, acceptBindings,
			LIBRARY->generaltexth->translate(textKey), controllerPressedHex == hoveredHex);
	}
	if(inspectRow)
		drawFaceRow(*inspectRow, inspectBindings,
			LIBRARY->generaltexth->translate("vcmi.battleWindow.controller.holdInspect"), false);
	return true;
}

void BattleFieldController::clickPressed(const Point & cursorPosition)
{
	if(ENGINE->input().getCurrentInputMode() != InputMode::CONTROLLER)
		mouseMoved(cursorPosition, Point());

	// a click on the battlefield cancels ongoing auto-combat (H3 behavior)
	if(owner.curInt->isAutoFightOn)
	{
		owner.curInt->isAutoFightOn = false;
		return;
	}

	BattleHex selectedHex = getHoveredHex();

	if (selectedHex != BattleHex::INVALID)
		owner.actionsController->onHexLeftClicked(selectedHex);
}

void BattleFieldController::showPopupWindow(const Point & cursorPosition)
{
	if(ENGINE->input().getCurrentInputMode() != InputMode::CONTROLLER)
		mouseMoved(cursorPosition, Point());

	BattleHex selectedHex = getHoveredHex();

	if (selectedHex != BattleHex::INVALID)
		owner.actionsController->onHexRightClicked(selectedHex);
}

void BattleFieldController::renderBattlefield(Canvas & canvas)
{
	Rect renderPos = pos;
	renderPos.x += shakeOffset.x;
	renderPos.y += shakeOffset.y;
	Canvas clippedCanvas(canvas, renderPos);

	showBackground(clippedCanvas);

	BattleRenderer renderer(owner);

	renderer.execute(clippedCanvas);

	owner.projectilesController->render(clippedCanvas);
}

void BattleFieldController::showBackground(Canvas & canvas)
{
	if (owner.stacksController->getActiveStack() != nullptr )
		showBackgroundImageWithHexes(canvas);
	else
		showBackgroundImage(canvas);

	showHighlightedHexes(canvas);
}

void BattleFieldController::showBackgroundImage(Canvas & canvas)
{
	canvas.draw(background, Point(0, 0));

	owner.obstacleController->showAbsoluteObstacles(canvas);
	if ( owner.siegeController )
		owner.siegeController->showAbsoluteObstacles(canvas);

	if (settings["battle"]["cellBorders"].Bool())
	{
		for (int i=0; i<GameConstants::BFIELD_SIZE; ++i)
		{
			if ( i % GameConstants::BFIELD_WIDTH == 0)
				continue;
			if ( i % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1)
				continue;

			canvas.draw(cellBorder, hexPositionLocal(i).topLeft());
		}
	}
}

void BattleFieldController::showBackgroundImageWithHexes(Canvas & canvas)
{
	canvas.draw(*backgroundWithHexes, Point(0, 0));
}

void BattleFieldController::redrawBackgroundWithHexes()
{
	const CStack *activeStack = owner.stacksController->getActiveStack();
	if(activeStack)
		availableHexes = owner.getBattle()->battleGetAvailableHexes(activeStack, false);
	else
		availableHexes.clear();

	// prepare background graphic with hexes and shaded hexes
	backgroundWithHexes->draw(background, Point(0,0));
	owner.obstacleController->showAbsoluteObstacles(*backgroundWithHexes);
	if(owner.siegeController)
		owner.siegeController->showAbsoluteObstacles(*backgroundWithHexes);

	// show shaded hexes for active's stack valid movement and the hexes that it can attack
	if(activeStack && settings["battle"]["stackRange"].Bool())
	{
		auto occupiableHexes = owner.getBattle()->battleGetOccupiableHexes(availableHexes, activeStack);
		for(si16 hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
		{
			//shade occupiable and attackable hexes
			if (occupiableHexes.contains(hex) ||
				(owner.getBattle()->battleCanAttackUnit(activeStack, owner.getBattle()->battleGetStackByPos(hex, true)) &&
					owner.getBattle()->battleCanAttackHex(availableHexes, activeStack, hex)) ||
				(owner.getBattle()->battleGetStackByPos(hex, true) &&
					owner.getBattle()->battleCanShoot(activeStack, hex)))
				showHighlightedHex(*backgroundWithHexes, cellShade, hex, false);
		}
	}

	// draw cell borders
	if(settings["battle"]["cellBorders"].Bool())
	{
		for(int i=0; i<GameConstants::BFIELD_SIZE; ++i)
		{
			if(i % GameConstants::BFIELD_WIDTH == 0)
				continue;
			if(i % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1)
				continue;

			backgroundWithHexes->draw(cellBorder, hexPositionLocal(i).topLeft());
		}
	}
}

void BattleFieldController::showHighlightedHex(Canvas & canvas, std::shared_ptr<IImage> highlight, const BattleHex & hex, bool darkBorder)
{
	Point hexPos = hexPositionLocal(hex).topLeft();

	canvas.draw(highlight, hexPos);
	if(!darkBorder && settings["battle"]["cellBorders"].Bool())
		canvas.draw(cellBorder, hexPos);
}

BattleHexArray BattleFieldController::getHighlightedHexesForActiveStack()
{
	if(!owner.stacksController->getActiveStack())
		return BattleHexArray();

	if(!settings["battle"]["stackRange"].Bool())
		return BattleHexArray();

	auto hoveredHex = getHoveredHex();

	return owner.getBattle()->battleGetAttackedHexes(owner.stacksController->getActiveStack(), hoveredHex);
}

BattleHexArray BattleFieldController::getMovementRangeForHoveredStack()
{
	if (!owner.stacksController->getActiveStack())
		return BattleHexArray();

	if (!settings["battle"]["movementHighlightOnHover"].Bool() && !ENGINE->isKeyboardShiftDown())
		return BattleHexArray();

	auto hoveredStack = getHoveredStack();
	return hoveredStack ? owner.getBattle()->battleGetOccupiableHexes(hoveredStack, true) : BattleHexArray();
}

BattleHexArray BattleFieldController::getHighlightedHexesForSpellRange()
{
	BattleHexArray result;
	auto hoveredHex = getHoveredHex();

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = owner.actionsController->getCurrentCastMode();
	spell = owner.actionsController->getCurrentSpell(hoveredHex);
	caster = owner.actionsController->getCurrentSpellcaster();

	if(caster && spell) //when casting spell
	{
		// printing shaded hex(es)
		spells::BattleCast event(owner.getBattle().get(), caster, mode, spell);
		auto shadedHexes = spell->battleMechanics(&event)->rangeInHexes(hoveredHex);

		for(const BattleHex & shadedHex : shadedHexes)
		{
			if((shadedHex.getX() != 0) && (shadedHex.getX() != GameConstants::BFIELD_WIDTH - 1))
				result.insert(shadedHex);
		}
	}
	return result;
}

BattleHexArray BattleFieldController::getHighlightedHexesForMovementTarget()
{
	const CStack * stack = owner.stacksController->getActiveStack();
	auto hoveredHex = getHoveredHex();

	if(!stack)
		return {};

	auto hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, false);

	bool canReach = owner.getBattle()->battleCanAttackHex(availableHexes, stack, hoveredHex);
	bool canAttack = canReach && (owner.getBattle()->battleCanAttackUnit(stack, hoveredStack));
	bool adjacentSpellCaster = stack->hasBonusOfType(BonusType::ADJACENT_SPELLCASTER) && stack->canCast();
	bool canCastAdjacentSpell = false;
	if (canReach && adjacentSpellCaster && hoveredStack)
	{
		spells::Mode mode = owner.actionsController->getCurrentCastMode();
		auto * spell = owner.actionsController->getCurrentSpell(hoveredHex);
		auto * caster = owner.actionsController->getCurrentSpellcaster();
		if(caster && spell)
		{
			spells::Target target;
			target.emplace_back(hoveredStack);
			target.emplace_back(hoveredHex);

			spells::BattleCast event(owner.getBattle().get(), caster, mode, spell);
			canCastAdjacentSpell = spell->battleMechanics(&event)->canBeCastAt(target);
		}
	}

	if(canAttack || canCastAdjacentSpell)
	{
		const bool allowLongWeapon = owner.actionsController->currentActionUsesLongWeapon(hoveredHex);
		BattleHex fromHex = owner.getBattle()->fromWhichHexAttack(stack, hoveredHex, selectAttackDirection(hoveredHex), allowLongWeapon);
		assert(fromHex.isValid());
		if(stack->doubleWide())
			return {fromHex, stack->occupiedHex(fromHex)};

		return {fromHex};
	}

	auto toHex = owner.getBattle()->toWhichHexMove(availableHexes, stack, hoveredHex);
	if (!toHex.isValid())
		return {};
	
	if (stack->doubleWide())
		return {toHex, stack->occupiedHex(toHex)};
	else
		return {toHex};
}

// Range limit highlight helpers

BattleHexArray BattleFieldController::getRangeHexes(const BattleHex & sourceHex, uint8_t distance) const
{
	BattleHexArray rangeHexes;

	if (!settings["battle"]["rangeLimitHighlightOnHover"].Bool() && !ENGINE->isKeyboardShiftDown())
		return rangeHexes;

	// get only battlefield hexes that are within the given distance
	for(auto i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		BattleHex hex(i);
		if(hex.isAvailable() && BattleHex::getDistance(sourceHex, hex) <= distance)
			rangeHexes.insert(hex);
	}

	return rangeHexes;
}

BattleHexArray BattleFieldController::getRangeLimitHexes(const BattleHex & sourceHex, const BattleHexArray & rangeHexes, uint8_t distanceToLimit) const
{
	BattleHexArray rangeLimitHexes;

	// from range hexes get only the ones at the limit
	for(auto & hex : rangeHexes)
	{
		if(BattleHex::getDistance(sourceHex, hex) == distanceToLimit)
			rangeLimitHexes.insert(hex);
	}

	return rangeLimitHexes;
}

bool BattleFieldController::isHexInRangeLimit(const BattleHex & hex, const BattleHexArray & rangeLimitHexes, int * hexIndexInRangeLimit) const
{
	bool  hexInRangeLimit = false;

	if(!rangeLimitHexes.empty())
	{
		auto pos = std::find(rangeLimitHexes.begin(), rangeLimitHexes.end(), hex);
		*hexIndexInRangeLimit = std::distance(rangeLimitHexes.begin(), pos);
		hexInRangeLimit = pos != rangeLimitHexes.end();
	}

	return hexInRangeLimit;
}

std::vector<std::vector<BattleHex::EDir>> BattleFieldController::getOutsideNeighbourDirectionsForLimitHexes(
	const BattleHexArray & wholeRangeHexes, const BattleHexArray & rangeLimitHexes) const
{
	std::vector<std::vector<BattleHex::EDir>> output;

	if(wholeRangeHexes.empty())
		return output;

	for(const auto & hex : rangeLimitHexes)
	{
		// get all neighbours and their directions
		
		const BattleHexArray & neighbouringTiles = hex.getAllNeighbouringTiles();

		std::vector<BattleHex::EDir> outsideNeighbourDirections;

		// for each neighbour add to output only the valid ones and only that are not found in range Hexes
		for(auto direction = 0; direction < 6; direction++)
		{
			if(!neighbouringTiles[direction].isAvailable())
				continue;

			if(!wholeRangeHexes.contains(neighbouringTiles[direction]))
				outsideNeighbourDirections.push_back(BattleHex::EDir(direction)); // push direction
		}

		output.push_back(outsideNeighbourDirections);
	}

	return output;
}

std::vector<std::shared_ptr<IImage>> BattleFieldController::calculateRangeLimitHighlightImages(std::vector<std::vector<BattleHex::EDir>> hexesNeighbourDirections, std::shared_ptr<CAnimation> limitImages)
{
	std::vector<std::shared_ptr<IImage>> output; // if no image is to be shown an empty image is still added to help with traverssing the range

	if(hexesNeighbourDirections.empty())
		return output;

	for(auto & directions : hexesNeighbourDirections)
	{
		std::bitset<6> mask;
		
		// convert directions to mask
		for(auto direction : directions)
			mask.set(direction);

		uint8_t imageKey = static_cast<uint8_t>(mask.to_ulong());
		output.push_back(limitImages->getImage(hexEdgeMaskToFrameIndex.at(imageKey)));
	}

	return output;
}

void BattleFieldController::calculateRangeLimitAndHighlightImages(uint8_t distance, std::shared_ptr<CAnimation> rangeLimitImages, BattleHexArray & rangeLimitHexes, std::vector<std::shared_ptr<IImage>> & rangeLimitHexesHighlights)
{
		BattleHexArray rangeHexes = getRangeHexes(hoveredHex, distance);
		rangeLimitHexes = getRangeLimitHexes(hoveredHex, rangeHexes, distance);
		std::vector<std::vector<BattleHex::EDir>> rangeLimitNeighbourDirections = getOutsideNeighbourDirectionsForLimitHexes(rangeHexes, rangeLimitHexes);
		rangeLimitHexesHighlights = calculateRangeLimitHighlightImages(rangeLimitNeighbourDirections, rangeLimitImages);
}

void BattleFieldController::showHighlightedHexes(Canvas & canvas)
{
	BattleHexArray rangedFullDamageLimitHexes;
	BattleHexArray shootingRangeLimitHexes;

	std::vector<std::shared_ptr<IImage>> rangedFullDamageLimitHexesHighlights;
	std::vector<std::shared_ptr<IImage>> shootingRangeLimitHexesHighlights;

	BattleHexArray hoveredStackMovementRangeHexes = getMovementRangeForHoveredStack();
	BattleHexArray hoveredSpellHexes = getHighlightedHexesForSpellRange();
	BattleHexArray hoveredMoveHexes  = getHighlightedHexesForMovementTarget();

	BattleHex hoveredHex = getHoveredHex();
	BattleHexArray hoveredMouseHex = hoveredHex.isAvailable() ? BattleHexArray({ hoveredHex }) : BattleHexArray();

	const CStack * hoveredStack = getHoveredStack();
	if(!hoveredStack && hoveredHex == BattleHex::INVALID)
		return;

	// skip range limit calculations if unit hovered is not a shooter
	if(hoveredStack && hoveredStack->isShooter())
	{
		// calculate array with highlight images for ranged full damage limit
		auto rangedFullDamageDistance = hoveredStack->getRangedFullDamageDistance();
		calculateRangeLimitAndHighlightImages(rangedFullDamageDistance, rangedFullDamageLimitImages, rangedFullDamageLimitHexes, rangedFullDamageLimitHexesHighlights);

		// calculate array with highlight images for shooting range limit
		auto shootingRangeDistance = hoveredStack->getShootingRangeDistance();
		calculateRangeLimitAndHighlightImages(shootingRangeDistance, shootingRangeLimitImages, shootingRangeLimitHexes, shootingRangeLimitHexesHighlights);
	}

	bool useSpellRangeForMouse = hoveredHex != BattleHex::INVALID
		&& (owner.actionsController->currentActionSpellcasting(getHoveredHex())
			|| owner.actionsController->creatureSpellcastingModeActive()); //at least shooting with SPELL_LIKE_ATTACK can operate in spellcasting mode without being actual spellcast
	bool useMoveRangeForMouse = !hoveredMoveHexes.empty() || !settings["battle"]["mouseShadow"].Bool();


	BattleHexArray hoveredMouseHexes;
	if(hoveredHex != BattleHex::INVALID && owner.actionsController->currentActionWalkAndCast(getHoveredHex()))
	{
		hoveredMouseHexes = hoveredSpellHexes;
		for(const auto & hex : useMoveRangeForMouse ? hoveredMoveHexes : hoveredMouseHex)
		{
			hoveredMouseHexes.insert(hex);
		}
	}
	else
	{
		hoveredMouseHexes = useSpellRangeForMouse
			? hoveredSpellHexes
			: ( useMoveRangeForMouse ? hoveredMoveHexes : hoveredMouseHex);
	}

	for(int hex = 0; hex < GameConstants::BFIELD_SIZE; ++hex)
	{
		bool stackMovement = hoveredStackMovementRangeHexes.contains(hex);
		bool mouse = hoveredMouseHexes.contains(hex);

		// calculate if hex is Ranged Full Damage Limit and its position in highlight array
		int hexIndexInRangedFullDamageLimit = 0;
		bool hexInRangedFullDamageLimit = isHexInRangeLimit(hex, rangedFullDamageLimitHexes, &hexIndexInRangedFullDamageLimit);

		// calculate if hex is Shooting Range Limit and its position in highlight array
		int hexIndexInShootingRangeLimit = 0;
		bool hexInShootingRangeLimit = isHexInRangeLimit(hex, shootingRangeLimitHexes, &hexIndexInShootingRangeLimit);

		if(stackMovement && mouse) // area where hovered stackMovement can move shown with highlight. Because also affected by mouse cursor, shade as well
		{
			showHighlightedHex(canvas, cellUnitMovementHighlight, hex, false);
			showHighlightedHex(canvas, cellShade, hex, true);
		}
		if(!stackMovement && mouse) // hexes affected only at mouse cursor shown as shaded
		{
			showHighlightedHex(canvas, cellShade, hex, true);
		}
		if(stackMovement && !mouse) // hexes where hovered stackMovement can move shown with highlight
		{
			showHighlightedHex(canvas, cellUnitMovementHighlight, hex, false);
		}
		if(hexInRangedFullDamageLimit)
		{
			showHighlightedHex(canvas, rangedFullDamageLimitHexesHighlights[hexIndexInRangedFullDamageLimit], hex, false);
		}
		if(hexInShootingRangeLimit)
		{
			showHighlightedHex(canvas, shootingRangeLimitHexesHighlights[hexIndexInShootingRangeLimit], hex, false);
		}
	}
}

Rect BattleFieldController::hexPositionLocal(const BattleHex & hex) const
{
	int x = 14 + ((hex.getY())%2==0 ? 22 : 0) + 44*hex.getX();
	int y = 86 + 42 *hex.getY();
	int w = cellShade->width();
	int h = cellShade->height();
	return Rect(x, y, w, h);
}

Rect BattleFieldController::hexPositionAbsolute(const BattleHex & hex) const
{
	return hexPositionLocal(hex) + pos.topLeft();
}

bool BattleFieldController::isPixelInHex(Point const & position)
{
	return !cellShade->isTransparent(position);
}

BattleHex BattleFieldController::getHoveredHex()
{
	// if mouse is not over the battlefield itself but over a stack in the battle queue,
	// treat the position of that stack as the hovered hex so that pointing at the queue
	// is equivalent to pointing at the stack on the battlefield
	if(hoveredHex == BattleHex::INVALID)
	{
		if(const CStack * queueStack = getQueueHoveredStack())
			return queueStack->getPosition();
	}

	return hoveredHex;
}

const CStack* BattleFieldController::getQueueHoveredStack() const
{
	if(!owner.windowObject->getQueueHoveredUnitId().has_value())
		return nullptr;

	for(const CStack * stack : owner.getBattle()->battleGetAllStacks())
		if(stack->unitId() == *owner.windowObject->getQueueHoveredUnitId())
			return stack;

	return nullptr;
}

const CStack* BattleFieldController::getHoveredStack()
{
	const CStack* hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);

	if(const CStack * queueStack = getQueueHoveredStack())
		hoveredStack = queueStack;

	return hoveredStack;
}

BattleHex BattleFieldController::getHexAtPosition(Point hoverPos)
{
	if (owner.attackingHero)
	{
		if (owner.attackingHero->pos.isInside(hoverPos))
			return BattleHex::HERO_ATTACKER;
	}

	if (owner.defendingHero)
	{
		if (owner.defendingHero->pos.isInside(hoverPos))
			return BattleHex::HERO_DEFENDER;
	}

	for (int h = 0; h < GameConstants::BFIELD_SIZE; ++h)
	{
		Rect hexPosition = hexPositionAbsolute(h);

		if (!hexPosition.isInside(hoverPos))
			continue;

		if (isPixelInHex(hoverPos - hexPosition.topLeft()))
			return h;
	}

	return BattleHex::INVALID;
}

BattleHex::EDir BattleFieldController::selectAttackDirection(const BattleHex & myNumber) const
{
	auto attacker = owner.stacksController->getActiveStack();
	assert(attacker);

	// When the target is pointed at through the battle queue there is no meaningful mouse
	// position on the battlefield to derive an approach direction from, so the raw cursor
	// position would always yield the same corner. Instead, pretend the cursor sits on the
	// attacker: the nearest-test-point logic below then selects the attack-from hex closest
	// to the attacker, which is what a player usually wants when simply saying "attack that
	// stack".
	Point originPoint = currentAttackOriginPoint;
	if(!pos.isInside(originPoint) && getQueueHoveredStack() != nullptr)
		originPoint = hexPositionAbsolute(attacker->getPosition()).center();

	// For each valid direction, select position to test against
	std::array<Point, 8> testPoint;
	testPoint.fill(Point::makeInvalid());

	for (size_t i = 0; i < 6; ++i)
		if (owner.getBattle()->battleCanAttackHex(availableHexes, attacker, myNumber, BattleHex::EDir(i)))
			testPoint[i] = attackDirectionPoint(myNumber, BattleHex::EDir(i));

	// For bottom/top directions select central point, but move it a bit away from true center to reduce zones allocated to them
	if (owner.getBattle()->battleCanAttackHex(availableHexes, attacker, myNumber, BattleHex::EDir(6)))
		testPoint[6] = attackDirectionPoint(myNumber, BattleHex::TOP);

	if (owner.getBattle()->battleCanAttackHex(availableHexes, attacker, myNumber, BattleHex::EDir(7)))
		testPoint[7] = attackDirectionPoint(myNumber, BattleHex::BOTTOM);

	// Compute distance between tested position & cursor position and pick nearest
	int nearestDistance = std::numeric_limits<int>::max();
	size_t nearest = -1;

	for (size_t i = 0; i < 8; ++i)
	{
		if (testPoint[i].isValid())
		{
			int distance = (testPoint[i].y - originPoint.y)*(testPoint[i].y - originPoint.y) + (testPoint[i].x - originPoint.x)*(testPoint[i].x - originPoint.x);
			if (nearest == -1 || distance < nearestDistance)
			{
				nearestDistance = distance;
				nearest = i;
			}
		}
	}

	if (nearest == -1)
		// Zero available tiles to attack from
		logGlobal->error("Error: cannot find a hex to attack hex %d from!", myNumber);
	return BattleHex::EDir(nearest);
}

void BattleFieldController::updateAccessibleHexes()
{
	auto accessibility = owner.getBattle()->getAccessibility();

	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE || (accessibility[i] == EAccessibility::SIDE_COLUMN));
}

bool BattleFieldController::stackCountOutsideHex(const BattleHex & number) const
{
	return stackCountOutsideHexes[number.toInt()];
}

void BattleFieldController::showAll(Canvas & to)
{
	show(to);
}

void BattleFieldController::tick(uint32_t msPassed)
{
	updateShake();
	updateAccessibleHexes();
	owner.stacksController->tick(msPassed);
	owner.obstacleController->tick(msPassed);
	owner.projectilesController->tick(msPassed);

	if(!isControllerNativeMode() || owner.stacksController->getActiveStack() == nullptr)
	{
		resetControllerInput();
		return;
	}

	ensureControllerFocus();
	bool focusMoved = false;
	if(navigationOwner == NavigationOwner::HEX && hexNavigation.ready(msPassed))
		focusMoved = moveControllerHex();
	else if(navigationOwner == NavigationOwner::UNIT && unitNavigation.ready(msPassed))
		focusMoved = browseControllerUnit();
	if(focusMoved)
	{
		controllerPressedHex = BattleHex::INVALID;
		controllerPressedAction = PossiblePlayerBattleAction::INVALID;
		controllerMeleeRepeatDirection.reset();
		refreshControllerPresentation();
	}

	if(controllerMeleeRepeatDirection)
	{
		if(controllerMeleeRepeat.ready(msPassed))
		{
			if(!cycleControllerMeleeDirection(*controllerMeleeRepeatDirection))
			{
				controllerMeleeRepeatDirection.reset();
				controllerMeleeRepeat.reset();
				redraw();
			}
		}
	}
}

void BattleFieldController::show(Canvas & to)
{
	CanvasClipRectGuard guard(to, pos);

	renderBattlefield(to);

	if(isActive() && isControllerNativeMode() && getHoveredHex().isValid()
		&& owner.stacksController->getActiveStack() != nullptr)
	{
		to.draw(ENGINE->cursor().getCurrentImage(), hexPositionAbsolute(getHoveredHex()).center() - ENGINE->cursor().getPivotOffset());
		drawControllerPrompts(to);
	}
	else if (isActive() && isGesturing() && getHoveredHex() != BattleHex::INVALID)
		to.draw(ENGINE->cursor().getCurrentImage(), hexPositionAbsolute(getHoveredHex()).center() - ENGINE->cursor().getPivotOffset());

	if(isActive() && ENGINE->input().getCurrentInputMode() == InputMode::CONTROLLER && controllerCursorMode)
	{
		const Rect bounds = controllerPromptBounds(pos.topLeft());
		const Rect indicator(bounds.center().x - 60, bounds.y + 8, 120, 22);
		to.drawColorBlended(indicator, ColorRGBA(45, 28, 16, 190));
		to.drawBorder(indicator, ColorRGBA(198, 164, 104), 1);
		to.drawText(indicator.center(), FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER,
			LIBRARY->generaltexth->translate("vcmi.battleWindow.controller.cursorMode"));
	}
}

bool BattleFieldController::receiveEvent(const Point & position, int eventType) const
{
	if (eventType == HOVER)
		return true;
	if(eventType == GESTURE && ENGINE->input().getCurrentInputMode() == InputMode::CONTROLLER && controllerCursorMode)
		return true;
	return CIntObject::receiveEvent(position, eventType);
}

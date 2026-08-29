/*
 * ControllerActionButton.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ControllerActionButton.h"

#include "../GameEngine.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/ShortcutHandler.h"
#include "../gui/TextAlignment.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../render/IFont.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

namespace
{

const Point promptSpritePosition(12, 4);
constexpr int promptTextLeft = 44;
constexpr int promptTextWidth = 68;
constexpr ColorRGBA genericFaceLabelColor(58, 40, 20, 255);
constexpr ColorRGBA disabledGenericFaceLabelColor(115, 105, 92, 110);

std::optional<std::string> resolvePromptSprite(ControllerPrompt::Family family,
	const std::vector<std::string> & bindings, ControllerPrompt::State state)
{
	if(bindings.size() != 1)
		return std::nullopt;
	return ControllerPrompt::faceButtonSprite(family, bindings.front(), state);
}

std::string resolvePromptLabel(ControllerPrompt::Family family, const std::vector<std::string> & bindings)
{
	if(!ControllerPrompt::usesRuntimeFaceLabel(family)
		|| bindings.size() != 1 || !ControllerPrompt::isFaceButtonBinding(bindings.front()))
		return "";
	return ControllerPrompt::buttonLabel(family, bindings.front());
}

}

class CControllerActionButton::PromptOverlay final : public CIntObject
{
	std::shared_ptr<IImage> sprite;
	std::optional<std::string> spriteName;
	std::string spriteLabel;
	std::string text;
	ControllerPrompt::State state = ControllerPrompt::State::NORMAL;

public:
	PromptOverlay(std::string text)
		: CIntObject(0)
		, text(std::move(text))
	{
	}

	void setText(const std::string & newText)
	{
		text = newText;
	}

	void setPresentation(const std::optional<std::string> & newSpriteName, const std::string & newSpriteLabel,
		ControllerPrompt::State newState)
	{
		if(spriteName != newSpriteName)
		{
			spriteName = newSpriteName;
			sprite = spriteName
				? ENGINE->renderHandler().loadImage(ImagePath::builtin(*spriteName), EImageBlitMode::COLORKEY)
				: nullptr;
		}
		spriteLabel = newSpriteLabel;
		state = newState;
	}

	void showAll(Canvas & to) override
	{
		if(!sprite)
			return;

		to.draw(sprite, pos.topLeft() + promptSpritePosition);
		if(!spriteLabel.empty())
		{
			to.drawText(pos.topLeft() + promptSpritePosition + Point(12, 12), EFonts::FONT_SMALL,
				state == ControllerPrompt::State::DISABLED ? disabledGenericFaceLabelColor : genericFaceLabelColor,
				ETextAlignment::CENTER, spriteLabel);
		}

		const auto & font = ENGINE->renderHandler().loadFont(EFonts::FONT_SMALL);
		const int textTop = pos.y + (pos.h - static_cast<int>(font->getLineHeight())) / 2;
		CanvasClipRectGuard clipGuard(to, Rect(pos.x + promptTextLeft, pos.y, promptTextWidth, pos.h));
		to.drawText(Point(pos.x + promptTextLeft, textTop), EFonts::FONT_SMALL, Colors::BLACK,
			ETextAlignment::TOPLEFT, text);
	}
};

CControllerActionButton::CControllerActionButton(Point position, const AnimationPath & image,
	const std::pair<std::string, std::string> & help, CFunctionList<void()> callback, EShortcut key,
	bool playerColoredButton)
	: CButton(position, image, help, callback, key, playerColoredButton)
	, mousePosition(position)
	, mouseImage(image)
	, mouseImagePlayerColored(playerColoredButton)
{
}

Point CControllerActionButton::absolutePosition(const Point & relativePosition) const
{
	return parent ? parent->pos.topLeft() + relativePosition : relativePosition;
}

void CControllerActionButton::setControllerPrompt(const JsonPath & buttonImage, Point position,
	const std::string & actionText, std::function<void()> visibilityChanged)
{
	setRedrawParent(true);
	promptImage = buttonImage;
	promptPosition = position;
	controllerPromptVisibilityChanged = std::move(visibilityChanged);
	if(!promptOverlay)
	{
		promptOverlay = std::make_shared<PromptOverlay>(actionText);
		setOverlay(promptOverlay);
		addUsedEvents(INPUT_MODE_CHANGE | TIME);
	}
	else
		promptOverlay->setText(actionText);

	refreshPresentation(ENGINE->input().getCurrentInputMode());
}

void CControllerActionButton::refreshPresentation(InputMode inputMode)
{
	if(!promptPosition)
		return;

	const auto bindings = ENGINE->shortcuts().getJoystickButtonBindings(assignedKey);
	promptBindings = bindings;
	ControllerPrompt::State state = ControllerPrompt::State::NORMAL;
	if(isBlocked())
		state = ControllerPrompt::State::DISABLED;
	else if(isPressed())
		state = ControllerPrompt::State::PRESSED;
	promptFamily = inputMode == InputMode::CONTROLLER
		? ENGINE->input().getActiveControllerPromptFamily()
		: ControllerPrompt::Family::UNKNOWN;
	const auto spriteName = inputMode == InputMode::CONTROLLER
		? resolvePromptSprite(promptFamily, bindings, state)
		: std::nullopt;
	const bool showControllerPrompt = spriteName.has_value();

	if(showControllerPrompt != controllerPromptVisible)
	{
		controllerPromptVisible = showControllerPrompt;
		if(controllerPromptVisible)
		{
			setConfigurable(*promptImage);
			moveTo(absolutePosition(*promptPosition));
			promptOverlay->pos.w = pos.w;
			promptOverlay->pos.h = pos.h;
			promptOverlay->moveTo(pos.topLeft());
			moveChildForeground(promptOverlay.get());
		}
		else
		{
			setImage(mouseImage, mouseImagePlayerColored);
			moveTo(absolutePosition(mousePosition));
			promptOverlay->moveTo(Rect::createCentered(pos, promptOverlay->pos.dimensions()).topLeft());
			moveChildForeground(promptOverlay.get());
		}

		if(controllerPromptVisibilityChanged)
			controllerPromptVisibilityChanged();
	}

	promptOverlay->setPresentation(spriteName, resolvePromptLabel(promptFamily, bindings), state);
	redraw();
}

void CControllerActionButton::refreshPromptState()
{
	refreshControllerPrompt();
	if(!controllerPromptVisible)
		return;

	ControllerPrompt::State state = ControllerPrompt::State::NORMAL;
	if(isBlocked())
		state = ControllerPrompt::State::DISABLED;
	else if(isPressed())
		state = ControllerPrompt::State::PRESSED;

	promptOverlay->moveTo(pos.topLeft());
	promptOverlay->setPresentation(
		resolvePromptSprite(promptFamily, promptBindings, state),
		resolvePromptLabel(promptFamily, promptBindings), state);
	redraw();
}

void CControllerActionButton::refreshControllerPrompt()
{
	if(!promptPosition || ENGINE->input().getCurrentInputMode() != InputMode::CONTROLLER)
		return;

	const auto currentFamily = ENGINE->input().getActiveControllerPromptFamily();
	const auto currentBindings = ENGINE->shortcuts().getJoystickButtonBindings(assignedKey);
	if(currentFamily != promptFamily || currentBindings != promptBindings)
		refreshPresentation(InputMode::CONTROLLER);
}

bool CControllerActionButton::isControllerPromptVisible() const
{
	return controllerPromptVisible;
}

void CControllerActionButton::block(bool on)
{
	CButton::block(on);
	refreshPromptState();
}

void CControllerActionButton::activate()
{
	if(promptPosition)
		refreshPresentation(ENGINE->input().getCurrentInputMode());
	CButton::activate();
}

void CControllerActionButton::clickPressed(const Point & cursorPosition)
{
	CButton::clickPressed(cursorPosition);
	refreshPromptState();
}

void CControllerActionButton::clickReleased(const Point & cursorPosition)
{
	if(controllerPromptVisible && isPressed())
	{
		promptOverlay->setPresentation(
			resolvePromptSprite(promptFamily, promptBindings, ControllerPrompt::State::NORMAL),
			resolvePromptLabel(promptFamily, promptBindings), ControllerPrompt::State::NORMAL);
	}

	// CButton::clickReleased invokes the callback, which may destroy this button.
	CButton::clickReleased(cursorPosition);
}

void CControllerActionButton::clickCancel(const Point & cursorPosition)
{
	if(controllerPromptVisible && isPressed())
	{
		promptOverlay->setPresentation(
			resolvePromptSprite(promptFamily, promptBindings, ControllerPrompt::State::NORMAL),
			resolvePromptLabel(promptFamily, promptBindings), ControllerPrompt::State::NORMAL);
	}
	CButton::clickCancel(cursorPosition);
}

void CControllerActionButton::hover(bool on)
{
	CButton::hover(on);
	refreshPromptState();
}

void CControllerActionButton::inputModeChanged(InputMode inputMode)
{
	refreshPresentation(inputMode);
}

void CControllerActionButton::tick(uint32_t)
{
	refreshControllerPrompt();
}

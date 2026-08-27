/*
 * ControllerActionButton.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Buttons.h"
#include "../eventsSDL/ControllerPromptFamily.h"

class CControllerActionButton final : public CButton
{
	class PromptOverlay;

	const Point mousePosition;
	const AnimationPath mouseImage;
	const bool mouseImagePlayerColored;
	std::optional<JsonPath> promptImage;
	std::optional<Point> promptPosition;
	std::shared_ptr<PromptOverlay> promptOverlay;
	std::function<void()> controllerPromptVisibilityChanged;
	bool controllerPromptVisible = false;
	ControllerPrompt::Family promptFamily = ControllerPrompt::Family::UNKNOWN;
	std::vector<std::string> promptBindings;

	Point absolutePosition(const Point & relativePosition) const;
	void refreshPresentation(InputMode inputMode);
	void refreshPromptState();
	void refreshControllerPrompt();

public:
	CControllerActionButton(Point position, const AnimationPath & image, const std::pair<std::string, std::string> & help,
		CFunctionList<void()> callback = nullptr, EShortcut key = {}, bool playerColoredButton = false);

	void setControllerPrompt(const JsonPath & buttonImage, Point position, const std::string & actionText,
		std::function<void()> visibilityChanged);
	bool isControllerPromptVisible() const;
	void block(bool on);

	void activate() override;
	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void hover(bool on) override;
	void inputModeChanged(InputMode inputMode) override;
	void tick(uint32_t msPassed) override;
};

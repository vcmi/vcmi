/*
 * EditorProgressWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

namespace Load
{
class Progress;
}

class CLabel;
class CPanel;

/// Window that shows how a computation on another thread advances. It closes when the computation is finished
class EditorProgressWindow : public WindowBase
{
	static constexpr int WIDTH = 360;
	static constexpr int HEIGHT = 90;
	static constexpr int BAR_HEIGHT = 18;

	Load::Progress & source;
	std::function<void()> onFinished;
	std::shared_ptr<CPanel> panel;
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CLabel> percentLabel;
	int shownPercent = -1;

public:
	/// The source has to live until the window is closed
	EditorProgressWindow(const std::string & title, Load::Progress & source, const std::function<void()> & onFinished);

	/// Opens the window, onFinished is called after it is closed
	static void show(const std::string & title, Load::Progress & source, const std::function<void()> & onFinished);

	void tick(uint32_t msPassed) override;
	void showAll(Canvas & to) override;
};

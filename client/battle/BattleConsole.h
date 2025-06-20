/*
 * BattleConsole.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class BattleInterface;
class CFilledTexture;
class CButton;
class TransparentFilledRectangle;
class CTextBox;

class BattleConsoleWindow : public CWindowObject
{
private:
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<TransparentFilledRectangle> textBoxBackgroundBorder;
	std::shared_ptr<CTextBox> textBox;

public:
	BattleConsoleWindow(const std::string & text);
};

/// Class which shows the console at the bottom of the battle screen and manages the text of the console
class BattleConsole : public CIntObject, public IStatusBar
{
private:
	const BattleInterface & owner;

	std::shared_ptr<CPicture> background;

	/// List of all texts added during battle, essentially - log of entire battle
	std::vector<std::string> logEntries;

	/// Current scrolling position, to allow showing older entries via scroll buttons
	int scrollPosition;

	/// current hover text set on mouse move, takes priority over log entries
	std::string hoverText;

	/// current text entered via in-game console, takes priority over both log entries and hover text
	std::string consoleText;

	/// if true then we are currently entering console text
	bool enteringText;

	/// splits text into individual strings for battle log
	std::vector<std::string> splitText(const std::string & text) const;

	/// select line(s) that will be visible in UI
	std::vector<std::string> getVisibleText() const;

public:
	BattleConsole(const BattleInterface & owner, const std::shared_ptr<CPicture> & backgroundSource, const Point & objectPos, const Point & imagePos, const Point & size);

	void showAll(Canvas & to) override;
	void deactivate() override;

	void clickPressed(const Point & cursorPosition) override;

	bool addText(const std::string & text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void scrollUp(ui32 by = 1); //scrolls console up by 'by' positions
	void scrollDown(ui32 by = 1); //scrolls console up by 'by' positions

	// IStatusBar interface
	void write(const std::string & Text) override;
	void clearIfMatching(const std::string & Text) override;
	void clear() override;
	void setEnteringMode(bool on) override;
	void setEnteredText(const std::string & text) override;
};

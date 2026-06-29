/*
 * CAdventureOptions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../gui/Shortcut.h"
#include "../../lib/filesystem/ResourcePath.h"

class CButton;
class CLabel;
class CMultiLineLabel;
class CSlider;

/// Adventure options dialog where you can view the world, dig, play the replay of the last turn,...
class AdventureOptions : public CWindowObject
{
	struct ButtonEntry
	{
		AnimationPath animPath;
		EShortcut     shortcut;
		bool          isBlocked;
		std::function<void()> callback;
		std::string   labelKey; ///< empty when no label
	};

	std::shared_ptr<CButton> exit;

	/// Pre-parsed data for every button (all entries, not just visible ones)
	std::vector<ButtonEntry> allEntries;

	/// Currently visible buttons – at most MAX_VISIBLE objects alive at once
	std::vector<std::shared_ptr<CButton>> buttons;

	/// Labels for the currently visible buttons (enhanced mode only)
	std::vector<std::shared_ptr<CMultiLineLabel>> buttonLabels;

	/// Scrollbar – only created when allEntries.size() > MAX_VISIBLE
	std::shared_ptr<CSlider> scrollBar;

	static constexpr int BUTTON_X       = 24;
	static constexpr int BUTTON_START_Y = 23;
	static constexpr int BUTTON_STEP    = 58;
	static constexpr int MAX_VISIBLE    = 5;

	/// (Re)creates the visible buttons for the given scroll position.
	void rebuildVisibleButtons(int scrollPos, bool enhanced);

public:
	AdventureOptions();

	static void showScenarioInfo();
};


/*
 * WikiModContent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../Global.h"
#include "WikiWindow.h"

class CIntObject;
class CViewport;

/// Callback for wiki internal navigation from the mod page.
using WikiModNavigateCallback = std::function<void(WikiCategory cat, const std::string & key)>;

/**
 * Builds the rich scrollable content for a "Mod" wiki entry.
 *
 * @param viewport         The viewport whose content() node the widgets are added to.
 * @param modId            The top-level mod ID string (e.g. "hota").
 * @param viewportWidth    Usable pixel width (excluding the vertical scrollbar).
 * @param blueStyle        true = blue colour theme, false = brown/default.
 * @param navigateCallback Called when the user left-clicks an entity row.
 */
std::vector<std::shared_ptr<CIntObject>> buildModContent(
	CViewport & viewport,
	const std::string & modId,
	int viewportWidth,
	bool blueStyle,
	WikiModNavigateCallback navigateCallback);

/**
 * Collects all top-level active mods that have at least one game entity
 * and returns them as a sorted list of WikiEntry objects ready to be used
 * as category entries in the Wiki element list.
 */
std::vector<WikiEntry> collectModEntries();

/*
 * WikiCreatureContent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../Global.h"

VCMI_LIB_NAMESPACE_BEGIN
class CCreature;
VCMI_LIB_NAMESPACE_END

class CIntObject;
class CViewport;

/// Callback type for wiki internal navigation (same as in WikiTownContent)
using WikiCreatureNavigateCallback = std::function<void(const std::string & creatureName)>;

/**
 * Builds the rich scrollable content for the "Creature" wiki entry.
 *
 * The returned widgets are owned by the caller and should be kept alive
 * as long as the viewport is visible (same pattern as buildTownContent).
 *
 * @param viewport         The viewport whose content() node the widgets are added to.
 * @param creature         The creature to display.  Must not be null.
 * @param viewportWidth    Usable pixel width (excluding the vertical scrollbar).
 * @param blueStyle        true = blue colour theme, false = brown/default.
 * @param navigateCallback Called when the user left-clicks a related creature
 *                         (e.g. an upgrade).  May be empty.
 */
std::vector<std::shared_ptr<CIntObject>> buildCreatureContent(
	CViewport & viewport,
	const CCreature * creature,
	int viewportWidth,
	bool blueStyle,
	WikiCreatureNavigateCallback navigateCallback = {});

/*
 * WikiTownContent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../gui/CIntObject.h"

enum class WikiCategory : int;

VCMI_LIB_NAMESPACE_BEGIN
class CFaction;
VCMI_LIB_NAMESPACE_END

class CViewport;

/// Callback type for navigating to another wiki entry from within the town content.
using WikiNavigateCallback = std::function<void(WikiCategory, const std::string &)>;

/// Populates a CViewport with real town data for the Wiki window.
///
/// Call buildTownContent() to fill the viewport with:
///   1. Town title
///   2. Town background with all built structures overlaid
///   3. Table of buildings (icon, name, cost)
///   4. Table of creatures per tier (icon, name, recruit cost + stats)
///
/// The returned vector holds shared_ptrs that keep viewport children alive.
std::vector<std::shared_ptr<CIntObject>> buildTownContent(
	CViewport & viewport,
	const CFaction * faction,
	int viewportWidth,
	bool blueStyle = false,
	WikiNavigateCallback navigateCallback = nullptr);

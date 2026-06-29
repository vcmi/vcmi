/*
 * WikiArtifactContent.h, part of VCMI engine
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
class CArtifact;
VCMI_LIB_NAMESPACE_END

class CIntObject;
class CViewport;

enum class WikiCategory : int;  // forward declaration

/// Callback for wiki internal navigation from the artifact page.
using WikiArtifactNavigateCallback = std::function<void(WikiCategory cat, const std::string & name)>;

/**
 * Builds the rich scrollable content for an "Artifact" wiki entry.
 *
 * @param viewport         The viewport whose content() node the widgets are added to.
 * @param art              The artifact to display. Must not be null.
 * @param viewportWidth    Usable pixel width (excluding the vertical scrollbar).
 * @param blueStyle        true = blue colour theme, false = brown/default.
 * @param navigateCallback Called when the user left-clicks a related artifact
 *                         (constituent or combined set). May be empty.
 */
std::vector<std::shared_ptr<CIntObject>> buildArtifactContent(
	CViewport & viewport,
	const CArtifact * art,
	int viewportWidth,
	bool blueStyle,
	WikiArtifactNavigateCallback navigateCallback = {});

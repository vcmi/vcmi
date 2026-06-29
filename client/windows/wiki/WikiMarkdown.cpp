/*
 * WikiMarkdown.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiMarkdown.h"

#include "markdown/MDRenderer.h"

#include "../../widgets/CViewport.h"
#include "../../gui/CIntObject.h"

std::vector<std::shared_ptr<CIntObject>> buildMarkdownContent(
	const CViewport & viewport,
	const std::string & markdownText,
	int viewportWidth,
	bool blueStyle,
	const std::function<void(const std::string &)> & onWikiLink,
	std::map<std::string, int> * anchors)
{
	std::vector<std::shared_ptr<CIntObject>> widgets;
	OBJECT_CONSTRUCTION_TARGETED(viewport.content());
	MDRenderer renderer(
		widgets,
		viewportWidth,
		viewportWidth - MD_MARGIN * 2,
		blueStyle,
		onWikiLink,
		anchors);
	renderer.run(markdownText);
	return widgets;
}

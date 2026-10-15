/*
 * EditorSettingsPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorSettingsPage.h"

#include "../EditorMap.h"

EditorSettingsPage::EditorSettingsPage(EditorMap & editor, const Rect & area)
	: editor(editor)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;
}

CMap & EditorSettingsPage::map() const
{
	return *editor.getMap();
}

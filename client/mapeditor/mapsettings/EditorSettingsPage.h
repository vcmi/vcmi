/*
 * EditorSettingsPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../gui/CIntObject.h"

class CMap;
class CGObjectInstance;
class EditorMap;

/// Base of the tabs of the map settings window. A page keeps its changes until apply() is called
class EditorSettingsPage : public CIntObject
{
protected:
	EditorMap & editor;

	CMap & map() const;

public:
	EditorSettingsPage(EditorMap & editor, const Rect & area);

	/// Writes the settings of the page into the map
	virtual void apply() = 0;
};

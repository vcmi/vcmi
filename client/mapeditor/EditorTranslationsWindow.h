/*
 * EditorTranslationsWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CDialogWindow.h"

class CMap;
class CMapHeader;
class EditorMap;
class CCheckBox;
class CDropDown;
class CFormView;
class CTextField;

namespace EditorTranslations
{
	/// Drops the texts of the objects that are not on the map anymore
	void cleanupRemovedItems(CMap & map);
	/// Drops the texts which identifier contains the pattern
	void cleanupRemovedItems(CMap & map, const std::string & pattern);
}

/// Window that edits the texts of the map in every language it supports
class EditorTranslationsWindow : public CDialogWindow
{
	static constexpr size_t MAX_SHOWN = 150;

	CMapHeader & header;
	std::string mapPreferredLanguage;
	std::vector<std::string> languages;

	std::shared_ptr<CDropDown> languageChoice;
	std::shared_ptr<CCheckBox> supportedCheck;
	std::shared_ptr<CTextField> filterField;
	std::shared_ptr<CFormView> table;

	std::string currentLanguage() const;
	void languageChanged();
	void supportedChanged(bool supported);
	void fillTable();

public:
	explicit EditorTranslationsWindow(EditorMap & editor);
	~EditorTranslationsWindow() override;

	static void show(EditorMap & editor);
};

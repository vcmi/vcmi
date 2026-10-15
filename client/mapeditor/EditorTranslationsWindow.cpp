/*
 * EditorTranslationsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorTranslationsWindow.h"

#include "EditorMap.h"
#include "EditorTexts.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CCheckBox.h"
#include "../widgets/CDropDown.h"
#include "../widgets/CFormView.h"
#include "../widgets/CTextField.h"
#include "../widgets/UiTheme.h"
#include "../windows/CPromptWindows.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace EditorTranslations
{

void cleanupRemovedItems(CMap & map)
{
	std::set<std::string> existingObjects{"map", "header"};
	for(const auto & object : map.objects)
	{
		if(object)
			existingObjects.insert(object->instanceName);
	}

	for(auto & translations : map.translations.Struct())
	{
		JsonNode updateTranslations;
		for(auto & s : translations.second.Struct())
		{
			std::vector<std::string> parts;
			boost::split(parts, s.first, boost::is_any_of("."));

			for(const auto & part : parts)
			{
				if(existingObjects.count(part))
				{
					updateTranslations.Struct()[s.first] = s.second;
					break;
				}
			}
		}
		translations.second = updateTranslations;
	}
}

void cleanupRemovedItems(CMap & map, const std::string & pattern)
{
	for(auto & translations : map.translations.Struct())
	{
		JsonNode updateTranslations;
		for(auto & s : translations.second.Struct())
		{
			if(s.first.find(pattern) == std::string::npos)
				updateTranslations.Struct()[s.first] = s.second;
		}
		translations.second = updateTranslations;
	}
}

}

EditorTranslationsWindow::EditorTranslationsWindow(EditorMap & editor)
	: CDialogWindow(Point(760, 600), EditorTexts::get("translations.title"), false)
	, header(*editor.getMap())
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	// Remove "map.<mapname>." prefix from all translation keys for editor display
	// Internal VMAP translations are saved WITHOUT this prefix
	for(auto & langPair : header.translations.Struct())
	{
		JsonNode cleanedTranslations;
		for(auto & entry : langPair.second.Struct())
			cleanedTranslations.Struct()[CMapFormatJson::removeMapNamePrefix(entry.first)] = entry.second;

		langPair.second = cleanedTranslations;
	}

	//fill languages list
	std::vector<std::string> languageNames;
	std::set<int> foundLanguages;
	int preferred = -1;
	for(auto & language : Languages::getLanguageList())
	{
		languageNames.push_back(language.nameEnglish + " (" + language.nameNative + ")");
		languages.push_back(language.identifier);

		if(header.translations.Struct().count(language.identifier) && !header.translations[language.identifier].Struct().empty())
			foundLanguages.insert(static_cast<int>(languages.size()) - 1);
		if(language.identifier == LIBRARY->generaltexth->getPreferredLanguage())
			preferred = static_cast<int>(languages.size()) - 1;
	}

	if(preferred >= 0 && !foundLanguages.empty() && !foundLanguages.count(preferred))
	{
		preferred = *foundLanguages.begin();
		mapPreferredLanguage = languages.at(preferred);
	}

	if(mapPreferredLanguage.empty())
		mapPreferredLanguage = LIBRARY->generaltexth->getPreferredLanguage();

	const int rowHeight = UiTheme::CONTROL_HEIGHT;
	languageChoice = std::make_shared<CDropDown>(Rect(0, 0, size.x / 2, rowHeight), languageNames, std::max(preferred, 0), [this](int) { languageChanged(); }, EditorTexts::get("translations.language"));
	supportedCheck = std::make_shared<CCheckBox>(Rect(size.x / 2 + UiTheme::SPACING, 0, size.x / 2 - UiTheme::SPACING, rowHeight), EditorTexts::get("translations.supported"), false, [this](bool on) { supportedChanged(on); });
	filterField = std::make_shared<CTextField>(Rect(0, rowHeight + UiTheme::SPACING, size.x, rowHeight), "", [this](const std::string &) { fillTable(); });

	const int tableTop = 2 * (rowHeight + UiTheme::SPACING);
	table = std::make_shared<CFormView>(Rect(0, tableTop, size.x, size.y - tableTop), 280);

	onCancel = [this]() { header.registerMapStrings(); };
	onAccept = [this]()
	{
		header.registerMapStrings();
		return true;
	};

	languageChanged();
}

EditorTranslationsWindow::~EditorTranslationsWindow() = default;

std::string EditorTranslationsWindow::currentLanguage() const
{
	return languages.at(languageChoice->getSelected());
}

void EditorTranslationsWindow::languageChanged()
{
	const std::string language = currentLanguage();
	const bool hasLanguage = header.translations.Struct().count(language) != 0;

	supportedCheck->setChecked(hasLanguage);
	fillTable();
}

void EditorTranslationsWindow::supportedChanged(bool supported)
{
	const std::string language = currentLanguage();

	if(supported)
	{
		//copy from default language
		header.translations[language] = header.translations[mapPreferredLanguage];
		fillTable();
		return;
	}

	if(language == mapPreferredLanguage)
	{
		CMessageDialog::showMessage(EditorTexts::get("translations.removeTitle"), EditorTexts::get("translations.defaultRemoval"));
		supportedCheck->setChecked(true);
		return;
	}

	auto remove = [this, language]()
	{
		header.translations.Struct().erase(language);
		fillTable();
	};

	if(header.translations[language].Struct().empty())
	{
		remove();
		return;
	}

	CMessageDialog::askConfirmation(EditorTexts::get("translations.removeTitle"), EditorTexts::get("translations.removeConfirm"), remove, [this]()
	{
		supportedCheck->setChecked(true);
	});
}

void EditorTranslationsWindow::fillTable()
{
	table->clear();

	const std::string language = currentLanguage();
	if(!header.translations.Struct().count(language))
		return;

	if(auto * map = dynamic_cast<CMap *>(&header))
		EditorTranslations::cleanupRemovedItems(*map);

	const std::string filter = filterField->getText();
	auto & translation = header.translations[language];

	size_t shown = 0;
	for(auto & entry : translation.Struct())
	{
		if(!filter.empty() && !boost::algorithm::icontains(entry.first, filter) && !boost::algorithm::icontains(entry.second.String(), filter))
			continue;

		if(++shown > MAX_SHOWN)
		{
			table->addParagraph(EditorTexts::get("translations.truncated"));
			break;
		}

		std::string text = entry.second.String();
		boost::replace_all(text, "\n", "\\n");

		const std::string key = entry.first;
		table->addText(key, text, [&translation, key](const std::string & edited)
		{
			std::string restored = edited;
			boost::replace_all(restored, "\\n", "\n");
			translation[key].String() = restored;
		});
	}

	table->finish();
}

void EditorTranslationsWindow::show(EditorMap & editor)
{
	ENGINE->windows().createAndPushWindow<EditorTranslationsWindow>(editor);
}

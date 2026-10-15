/*
 * EditorGeneralPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorGeneralPage.h"

#include "../EditorMap.h"
#include "../EditorTexts.h"

#include "../../widgets/CCheckBox.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapping/CMapEditManager.h"
#include "../../../lib/texts/MetaString.h"

#include <vstd/DateUtils.h>

namespace
{
constexpr int LABEL_WIDTH = 190;
constexpr int DESCRIPTION_HEIGHT = 110;
constexpr const char * DATE_FORMAT = "%Y-%m-%d %H:%M";

const std::array<EMapDifficulty, 5> DIFFICULTIES = {
	EMapDifficulty::EASY,
	EMapDifficulty::NORMAL,
	EMapDifficulty::HARD,
	EMapDifficulty::EXPERT,
	EMapDifficulty::IMPOSSIBLE
};

std::time_t parseDateTime(const std::string & text, std::time_t fallback)
{
	std::tm parsed{};
	if(std::sscanf(text.c_str(), "%d-%d-%d %d:%d", &parsed.tm_year, &parsed.tm_mon, &parsed.tm_mday, &parsed.tm_hour, &parsed.tm_min) < 3)
		return fallback;

	parsed.tm_year -= 1900;
	parsed.tm_mon -= 1;
	parsed.tm_isdst = -1;

	const std::time_t result = std::mktime(&parsed);
	return result == static_cast<std::time_t>(-1) ? fallback : result;
}
}

EditorGeneralPage::EditorGeneralPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	const CMap & m = map();
	form = std::make_shared<CFormView>(Rect(0, 0, area.w, area.h), LABEL_WIDTH);

	nameField = form->addText(EditorTexts::get("general.name"), EditorTexts::translate(m.name), nullptr);

	descriptionField = std::make_shared<CMultiLineEdit>(Rect(0, 0, form->contentWidth() - LABEL_WIDTH, DESCRIPTION_HEIGHT), EditorTexts::translate(m.description));
	form->addWidget(EditorTexts::get("general.description"), descriptionField);

	authorField = form->addText(EditorTexts::get("general.author"), EditorTexts::translate(m.author), nullptr);
	contactField = form->addText(EditorTexts::get("general.contact"), EditorTexts::translate(m.authorContact), nullptr);
	dateField = form->addText(EditorTexts::get("general.creationTime"), vstd::getFormattedDateTime(m.creationDateTime, DATE_FORMAT), nullptr);
	versionField = form->addText(EditorTexts::get("general.version"), EditorTexts::translate(m.mapVersion), nullptr);

	levelLimitInput = std::make_shared<CNumberInput>(Rect(0, 0, form->contentWidth() - LABEL_WIDTH, UiTheme::CONTROL_HEIGHT), std::max<int>(1, m.levelLimit), 1, 99);
	levelLimitCheck = form->addCheck(EditorTexts::get("general.limitLevel"), m.levelLimit != 0, [this](bool on)
	{
		levelLimitInput->block(!on);
	});
	form->addWidget("", levelLimitInput);
	levelLimitInput->block(m.levelLimit == 0);

	int difficulty = 0;
	for(size_t i = 0; i < DIFFICULTIES.size(); ++i)
		if(DIFFICULTIES[i] == m.difficulty)
			difficulty = static_cast<int>(i);

	difficultyChoice = form->addChoice(EditorTexts::get("general.difficulty"),
		{EditorTexts::get("difficulty.easy"), EditorTexts::get("difficulty.normal"), EditorTexts::get("difficulty.hard"), EditorTexts::get("difficulty.expert"), EditorTexts::get("difficulty.impossible")},
		difficulty, nullptr);

	form->finish();
}

void EditorGeneralPage::apply()
{
	CMap & m = map();

	m.name = MetaString::createFromTextID(mapRegisterLocalizedString("map", m, TextIdentifier("header", "name"), nameField->getText()));
	m.description = MetaString::createFromTextID(mapRegisterLocalizedString("map", m, TextIdentifier("header", "description"), descriptionField->getText()));
	m.author = MetaString::createFromRawString(authorField->getText());
	m.authorContact = MetaString::createFromRawString(contactField->getText());
	m.creationDateTime = parseDateTime(dateField->getText(), m.creationDateTime);
	m.mapVersion = MetaString::createFromRawString(versionField->getText());
	m.levelLimit = levelLimitCheck->isChecked() ? levelLimitInput->getValue() : 0;
	m.difficulty = DIFFICULTIES.at(difficultyChoice->getSelected());
}

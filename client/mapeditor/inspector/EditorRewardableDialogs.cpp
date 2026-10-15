/*
 * EditorRewardableDialogs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorRewardableDialogs.h"

#include "../EditorMap.h"
#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CSelectList.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"
#include "../../windows/CPromptWindows.h"

#include "../../../lib/CSkillHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/json/JsonKeyExtractor.h"
#include "../../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../../lib/mapObjectConstructors/IObjectInfo.h"
#include "../../../lib/mapObjects/CRewardableObject.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/rewardable/Info.h"
#include "../../../lib/spells/CSpellHandler.h"

#include <boost/algorithm/string/predicate.hpp>

namespace
{
const std::string ABILITIES_CATEGORY = "secondarySkill";
const std::string ABILITIES_NAME = "gainedSkill";
const std::string SHRINE_CATEGORY = "spell";
const std::string SHRINE_NAME = "gainedSpell";
const std::string SCHOLAR_DICE_CATEGORY = "dice";
const std::string SCHOLAR_DICE_NAME = "map";

const Rewardable::Info & objectInfo(const CRewardableObject & object, std::unique_ptr<IObjectInfo> & holder)
{
	holder = LIBRARY->objtypeh->getHandlerFor(object.ID, object.subID)->getObjectInfo();
	return dynamic_cast<const Rewardable::Info &>(*holder);
}

JsonNode stringVariable(const std::string & value)
{
	JsonNode variable;
	variable.String() = value;
	variable.setModScope(ModScope::scopeGame());
	return variable;
}
}

// ---------------------------------------------------------------------------------------------

EditorAbilitiesDialog::EditorAbilitiesDialog(EditorMap & editor, CRewardableObject & hut, const std::function<void()> & onChanged)
	: CDialogWindow(Point(460, 520), EditorTexts::get("abilities.title"))
	, editor(editor)
	, hut(hut)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int rowHeight = UiTheme::CONTROL_HEIGHT;

	const bool customized = !presetAbilities().empty();
	std::set<SecondarySkill> checked = customized ? presetAbilities() : defaultAbilities();

	for(const auto & skill : LIBRARY->skillh->objects)
	{
		const bool allowed = vstd::contains(editor.getMap()->allowedAbilities, skill->getId());
		entries.push_back({skill->getId(), skill->getNameTranslated(), allowed, vstd::contains(checked, skill->getId()) && (allowed || !customized)});
	}

	customizeCheck = std::make_shared<CCheckBox>(Rect(0, 0, size.x, rowHeight), EditorTexts::get("abilities.customize"), customized, [this](bool)
	{
		refreshList();
	});
	filterField = std::make_shared<CTextField>(Rect(0, rowHeight + UiTheme::SPACING, size.x, rowHeight), "", [this](const std::string &) { refreshList(); });

	const int listTop = 2 * (rowHeight + UiTheme::SPACING);
	list = std::make_shared<CSelectList>(Rect(0, listTop, size.x, size.y - listTop), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	list->onCheck = [this](int row, bool on)
	{
		entries.at(shown.at(row)).checked = on;
	};

	addButton(EditorTexts::get("abilities.selectAll"), [this]() { setAllChecked(true); });
	addButton(EditorTexts::get("abilities.deselectAll"), [this]() { setAllChecked(false); });

	refreshList();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

std::set<SecondarySkill> EditorAbilitiesDialog::presetAbilities() const
{
	JsonKeyExtractor extractor(editor.getMap()->cb);
	const JsonNode preset = hut.configuration.getPresetVariable(ABILITIES_CATEGORY, ABILITIES_NAME);

	if(preset.isNull())
		return {};

	return extractor.filterKeys(preset, LIBRARY->skillh->getDefaultAllowed());
}

std::set<SecondarySkill> EditorAbilitiesDialog::defaultAbilities() const
{
	JsonKeyExtractor extractor(editor.getMap()->cb);
	std::unique_ptr<IObjectInfo> holder;
	const JsonNode & defaults = objectInfo(hut, holder).getParameters()["variables"][ABILITIES_CATEGORY][ABILITIES_NAME];

	if(defaults.isNull())
		return editor.getMap()->allowedAbilities;

	return extractor.filterKeys(defaults, editor.getMap()->allowedAbilities);
}

void EditorAbilitiesDialog::refreshList()
{
	const std::string filter = filterField->getText();
	const bool customized = customizeCheck->isChecked();

	shown.clear();
	std::vector<CSelectList::Row> rows;
	for(size_t i = 0; i < entries.size(); ++i)
	{
		if(!filter.empty() && !boost::algorithm::icontains(entries[i].name, filter))
			continue;

		CSelectList::Row row;
		row.text = entries[i].name;
		row.checked = entries[i].checked;
		row.enabled = customized && entries[i].allowed;
		rows.push_back(std::move(row));
		shown.push_back(static_cast<int>(i));
	}
	list->setRows(std::move(rows), true);
}

void EditorAbilitiesDialog::setAllChecked(bool on)
{
	if(!customizeCheck->isChecked())
		return;

	for(auto & entry : entries)
		if(entry.allowed)
			entry.checked = on;

	refreshList();
}

void EditorAbilitiesDialog::commit()
{
	if(!customizeCheck->isChecked())
	{
		hut.configuration.variables.preset.clear();
		return;
	}

	JsonNode preset;
	for(const auto & entry : entries)
	{
		if(entry.checked)
		{
			JsonNode key;
			key.String() = LIBRARY->skills()->getById(entry.id)->getJsonKey();
			preset["anyOf"].Vector().push_back(key);
		}
	}

	// the list can include skills of any mod
	preset.setModScope(ModScope::scopeGame());
	hut.configuration.presetVariable(ABILITIES_CATEGORY, ABILITIES_NAME, preset);
}

// ---------------------------------------------------------------------------------------------

EditorScholarDialog::EditorScholarDialog(CRewardableObject & scholar, const std::function<void()> & onChanged)
	: CDialogWindow(Point(460, 190), EditorTexts::get("scholar.title"))
	, scholar(scholar)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	kinds[0] = {"primarySkill", "gainedStat", 2, {}, {}};
	for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
	{
		kinds[0].keys.push_back(NPrimarySkill::names[i]);
		kinds[0].names.push_back(EditorTexts::translate(TextIdentifier("core.priskill", i)));
	}

	kinds[1] = {"secondarySkill", "gainedSkill", 1, {}, {}};
	for(const auto & skill : LIBRARY->skillh->objects)
	{
		kinds[1].keys.push_back(skill->getJsonKey());
		kinds[1].names.push_back(skill->getNameTranslated());
	}

	kinds[2] = {"spell", "gainedSpell", 0, {}, {}};
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(spell->isCommonHeroSpell())
		{
			kinds[2].keys.push_back(spell->getJsonKey());
			kinds[2].names.push_back(spell->getNameTranslated());
		}
	}

	// kind 0 is random, the others are the kinds above shifted by one
	int kind = 0;
	int value = 0;
	for(int i = 0; i < static_cast<int>(kinds.size()) && kind == 0; ++i)
	{
		const JsonNode variable = scholar.configuration.getPresetVariable(kinds[i].variableCategory, kinds[i].variableName);
		if(variable.isNull())
			continue;

		const auto found = std::ranges::find(kinds[i].keys, variable.String());
		if(found != kinds[i].keys.end())
		{
			kind = i + 1;
			value = static_cast<int>(found - kinds[i].keys.begin());
		}
	}

	const int rowHeight = UiTheme::CONTROL_HEIGHT;
	kindChoice = std::make_shared<CDropDown>(Rect(0, 0, size.x, rowHeight),
		std::vector<std::string>{EditorTexts::get("scholar.random"), EditorTexts::get("scholar.primary"), EditorTexts::get("scholar.secondary"), EditorTexts::get("scholar.spell")},
		kind, [this](int index) { refreshValues(index, 0); }, EditorTexts::get("scholar.title"));
	valueChoice = std::make_shared<CDropDown>(Rect(0, rowHeight + UiTheme::SPACING, size.x, rowHeight), std::vector<std::string>{}, -1, nullptr, EditorTexts::get("scholar.title"));

	refreshValues(kind, value);

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorScholarDialog::refreshValues(int kind, int value)
{
	if(kind == 0)
	{
		valueChoice->setOptions({}, -1);
		valueChoice->block(true);
		return;
	}

	valueChoice->setOptions(kinds.at(kind - 1).names, value);
	valueChoice->block(false);
}

void EditorScholarDialog::commit()
{
	scholar.configuration.variables.preset.clear();

	const int kind = kindChoice->getSelected();
	if(kind == 0)
		return;

	const Kind & chosen = kinds.at(kind - 1);
	scholar.configuration.presetVariable(chosen.variableCategory, chosen.variableName, stringVariable(chosen.keys.at(valueChoice->getSelected())));
	scholar.configuration.presetVariable(SCHOLAR_DICE_CATEGORY, SCHOLAR_DICE_NAME, JsonNode(chosen.dice));
}

// ---------------------------------------------------------------------------------------------

void EditorShrineDialog::show(CRewardableObject & shrine, const std::function<void()> & onChanged)
{
	std::unique_ptr<IObjectInfo> holder;
	JsonNode levelNode;
	try
	{
		levelNode = objectInfo(shrine, holder).getParameters()["variables"][SHRINE_CATEGORY][SHRINE_NAME]["level"];
	}
	catch(const std::runtime_error &)
	{
		levelNode = JsonNode();
	}

	const std::string title = EditorTexts::get("shrine.cannotOpen");
	if(!levelNode.isNumber())
	{
		CMessageDialog::showMessage(title, EditorTexts::get("shrine.levelUnknown"));
		return;
	}

	const int level = static_cast<int>(levelNode.Integer());
	if(level < 1 || level > GameConstants::SPELL_LEVELS)
	{
		CMessageDialog::showMessage(title, EditorTexts::format("shrine.levelInvalid", {std::to_string(level)}));
		return;
	}

	ENGINE->windows().createAndPushWindow<EditorShrineDialog>(shrine, level, onChanged);
}

EditorShrineDialog::EditorShrineDialog(CRewardableObject & shrine, int spellLevel, const std::function<void()> & onChanged)
	: CDialogWindow(Point(460, 170), EditorTexts::get("shrine.title"))
	, shrine(shrine)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int rowHeight = UiTheme::CONTROL_HEIGHT;

	std::vector<std::string> names;
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(spell->isCommonHeroSpell() && spell->getLevel() == spellLevel)
		{
			keys.push_back(spell->getJsonKey());
			names.push_back(spell->getNameTranslated());
		}
	}

	int selectedSpell = 0;
	bool custom = false;
	const JsonNode variable = shrine.configuration.getPresetVariable(SHRINE_CATEGORY, SHRINE_NAME);
	if(!variable.isNull())
	{
		const auto found = std::ranges::find(keys, variable.String());
		if(found != keys.end())
		{
			custom = true;
			selectedSpell = static_cast<int>(found - keys.begin());
		}
	}

	customCheck = std::make_shared<CCheckBox>(Rect(0, 0, size.x, rowHeight), EditorTexts::get("shrine.custom"), custom, [this](bool on)
	{
		spellChoice->block(!on);
	});
	spellChoice = std::make_shared<CDropDown>(Rect(0, rowHeight + UiTheme::SPACING, size.x, rowHeight), names, selectedSpell, nullptr, EditorTexts::get("shrine.title"));
	spellChoice->block(!custom);

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorShrineDialog::commit()
{
	shrine.configuration.variables.preset.clear();

	if(customCheck->isChecked() && !keys.empty())
		shrine.configuration.presetVariable(SHRINE_CATEGORY, SHRINE_NAME, stringVariable(keys.at(spellChoice->getSelected())));
}

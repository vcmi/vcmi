/*
 * EditorLimiterEditor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../widgets/CItemValuePicker.h"

#include "../../../lib/rewardable/Limiter.h"

class CDropDown;
class CFormView;
class CMap;
class CNumberInput;
class CPlayerSetEditor;
class CResourceEditor;
class CSetPicker;

/// Picker of a set of entities. The picker items are numbered, ids translates the numbers to the identifiers of the entities
struct EditorIdPicker
{
	std::shared_ptr<CSetPicker> picker;
	std::vector<int> ids;

	std::vector<int> getIds() const;
};

namespace EditorPickers
{
	/// Entities that the map allows are listed, the selected ones are listed always
	EditorIdPicker artifacts(const CMap & map, const Rect & area, const std::vector<ArtifactID> & selected);
	EditorIdPicker spells(const CMap & map, const Rect & area, const std::vector<SpellID> & selected);
	EditorIdPicker heroes(const Rect & area, const std::vector<HeroTypeID> & selected);
	EditorIdPicker heroClasses(const Rect & area, const std::vector<HeroClassID> & selected);

	/// Secondary skills with levels 1 - 3
	std::shared_ptr<CItemValuePicker> skills(const CMap & map, const Rect & area, const std::map<SecondarySkill, si32> & selected);
	std::shared_ptr<CItemValuePicker> creatures(const Rect & area, const std::vector<CStackBasicDescriptor> & selected);

	std::map<SecondarySkill, si32> skillsValue(const std::shared_ptr<CItemValuePicker> & picker);
	std::vector<CStackBasicDescriptor> creaturesValue(const std::shared_ptr<CItemValuePicker> & picker);

	std::vector<std::string> skillLevelNames();
	std::vector<std::string> dayOfWeekNames();
}

/// Controls of the fields of a limiter, added as rows of a form
class EditorLimiterEditor
{
	std::shared_ptr<CDropDown> dayOfWeek;
	std::shared_ptr<CNumberInput> daysPassed;
	std::shared_ptr<CNumberInput> heroLevel;
	std::shared_ptr<CNumberInput> heroExperience;
	std::shared_ptr<CNumberInput> manaPoints;
	std::shared_ptr<CNumberInput> manaPercentage;
	std::array<std::shared_ptr<CNumberInput>, 4> primary;
	std::shared_ptr<CResourceEditor> resources;
	EditorIdPicker artifacts;
	EditorIdPicker spells;
	std::shared_ptr<CItemValuePicker> skills;
	std::shared_ptr<CItemValuePicker> creatures;
	EditorIdPicker heroes;
	EditorIdPicker heroClasses;
	std::shared_ptr<CPlayerSetEditor> players;

public:
	/// controlWidth is the width of the controls that are placed beside the labels of the form
	EditorLimiterEditor(CFormView & form, int controlWidth, const CMap & map, const Rewardable::Limiter & limiter);

	/// Writes the values into the limiter, the fields that the editor does not show are not changed
	void apply(Rewardable::Limiter & limiter) const;
};

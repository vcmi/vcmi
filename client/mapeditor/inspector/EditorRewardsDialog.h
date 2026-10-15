/*
 * EditorRewardsDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

#include "../../../lib/rewardable/Configuration.h"

class CCheckBox;
class CDropDown;
class CFormView;
class CListEditor;
class CMap;
class CMultiLineEdit;
class CNumberInput;
class CResourceEditor;
class CRewardableObject;
class CTextField;
class EditorLimiterEditor;
class EditorMap;

/// Window that edits one bonus granted to the hero
class EditorBonusDialog : public CDialogWindow
{
	std::vector<BonusDuration::Type> durations;
	std::vector<BonusType> types;
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CDropDown> durationChoice;
	std::shared_ptr<CDropDown> typeChoice;
	std::shared_ptr<CNumberInput> valueInput;

public:
	EditorBonusDialog(const std::shared_ptr<Bonus> & bonus, const std::function<void(BonusDuration::Type, BonusType, int)> & onSaved);
};

/// Window that edits one reward of an object with its conditions
class EditorRewardInfoDialog : public CDialogWindow
{
public:
	struct Draft
	{
		Rewardable::VisitInfo info;
		std::string messageText;
	};

private:
	Draft draft;
	std::vector<std::shared_ptr<Bonus>> bonuses;
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CTextField> messageField;
	std::shared_ptr<CNumberInput> heroLevel;
	std::shared_ptr<CNumberInput> heroExperience;
	std::shared_ptr<CNumberInput> manaDiff;
	std::shared_ptr<CNumberInput> manaPercentage;
	std::shared_ptr<CNumberInput> overflowFactor;
	std::shared_ptr<CNumberInput> movePoints;
	std::shared_ptr<CNumberInput> movePercentage;
	std::shared_ptr<CCheckBox> removeObject;
	std::array<std::shared_ptr<CNumberInput>, 4> primary;
	std::shared_ptr<CResourceEditor> resources;
	struct Pickers;
	std::shared_ptr<Pickers> pickers;
	std::shared_ptr<CCheckBox> castCheck;
	std::shared_ptr<CDropDown> castSpell;
	std::shared_ptr<CDropDown> castLevel;
	std::vector<int> castSpellIds;
	std::shared_ptr<CListEditor> bonusList;
	std::shared_ptr<EditorLimiterEditor> limiter;

	void refreshBonuses(int selected);
	void editBonus(int index);

public:
	EditorRewardInfoDialog(const CMap & map, CRewardableObject & object, const Draft & draft, const std::function<void(const Draft &)> & onSaved);
	~EditorRewardInfoDialog() override;
};

/// Window that edits the configuration of a rewardable object
class EditorRewardsDialog : public CDialogWindow
{
	using Draft = EditorRewardInfoDialog::Draft;

	EditorMap & editor;
	CRewardableObject & object;
	std::vector<Draft> rewards;
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CDropDown> visitMode;
	std::shared_ptr<CDropDown> selectMode;
	std::shared_ptr<CDropDown> windowMode;
	std::shared_ptr<CTextField> onSelectText;
	std::shared_ptr<CCheckBox> canRefuse;
	std::shared_ptr<CNumberInput> resetPeriod;
	std::shared_ptr<CNumberInput> resetWeeks;
	std::shared_ptr<CNumberInput> resetMonths;
	std::shared_ptr<CCheckBox> resetVisitors;
	std::shared_ptr<CCheckBox> resetRewards;
	std::shared_ptr<CListEditor> rewardList;

	void updateEnabled();
	void refreshList(int selected);
	void edit(int index);
	void commit();

public:
	EditorRewardsDialog(EditorMap & editor, CRewardableObject & object, const std::function<void()> & onChanged);
};

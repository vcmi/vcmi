/*
 * EditorStartingBonusDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

#include "../../../lib/campaign/CampaignBonus.h"

class CDropDown;
class CFormView;
class CMap;

/// Window that edits a bonus that the player can choose at the start of a scenario
class EditorStartingBonusDialog : public CDialogWindow
{
	struct Values
	{
		int hero = 0;
		int spell = 0;
		int creature = 0;
		int amount = 1;
		int building = 0;
		int artifact = 0;
		std::array<int, 4> primary = {};
		int skill = 0;
		int mastery = 0;
		int resource = 0;
	};

	PlayerColor color;
	const CMap & map;
	Values values;
	int type = 0;
	bool buildingAvailable = false;
	std::vector<int> heroIds;
	std::vector<std::string> heroNames;
	std::shared_ptr<CDropDown> typeChoice;
	std::shared_ptr<CFormView> form;

	void buildForm();
	void load(const CampaignBonus & bonus);
	CampaignBonus save() const;
	void addIdChoice(const std::string & label, const std::vector<int> & ids, const std::vector<std::string> & names, int & value);

public:
	EditorStartingBonusDialog(PlayerColor color, const CMap & map, const CampaignBonus & bonus, const std::function<void(const CampaignBonus &)> & onSaved);

	/// Name of the hero in the bonus, which can also be one of the special selections
	static std::string heroName(const CMap & map, int heroId);
	static std::string describe(const CampaignBonus & bonus, const CMap & map);
};

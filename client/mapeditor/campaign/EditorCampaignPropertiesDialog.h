/*
 * EditorCampaignPropertiesDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

#include "../../../lib/campaign/CampaignRegions.h"

class CampaignState;
class CCheckBox;
class CDropDown;
class CFormView;
class CListEditor;
class CMultiLineEdit;
class CNumberInput;
class CTextField;

/// Window that edits the header and the regions of a campaign
class EditorCampaignPropertiesDialog : public CDialogWindow
{
	std::shared_ptr<CampaignState> campaign;
	CampaignRegions regions;
	bool shrinkConfirmed = false;

	std::shared_ptr<CFormView> form;
	std::shared_ptr<CTextField> nameField;
	std::shared_ptr<CMultiLineEdit> descriptionField;
	std::shared_ptr<CTextField> authorField;
	std::shared_ptr<CTextField> contactField;
	std::shared_ptr<CTextField> dateField;
	std::shared_ptr<CTextField> versionField;
	std::shared_ptr<CTextField> musicField;
	std::shared_ptr<CCheckBox> difficultyCheck;
	std::shared_ptr<CTextField> loadingBackgroundField;
	std::shared_ptr<CCheckBox> videoRimCheck;
	std::shared_ptr<CTextField> videoRimField;
	std::shared_ptr<CTextField> introField;
	std::shared_ptr<CTextField> outroField;
	std::shared_ptr<CDropDown> presetChoice;
	std::shared_ptr<CTextField> backgroundField;
	std::array<std::shared_ptr<CTextField>, 3> suffixFields;
	std::shared_ptr<CTextField> prefixField;
	std::shared_ptr<CNumberInput> colorSuffixInput;
	std::shared_ptr<CListEditor> regionList;

	void loadRegions();
	void readRegionFields();
	void refreshRegionList(int selected);
	void editRegion(int index);
	bool commit();

public:
	EditorCampaignPropertiesDialog(const std::shared_ptr<CampaignState> & campaign, const std::function<void()> & onChanged);
};

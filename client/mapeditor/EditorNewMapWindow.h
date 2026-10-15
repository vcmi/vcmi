/*
 * EditorNewMapWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CDialogWindow.h"

#include "../../lib/rmg/CMapGenOptions.h"

class CMap;
class CFormView;
class CCheckBox;
class CDropDown;
class CNumberInput;
class CTextButton;
class IGameInfoCallback;

/// Window that sets the parameters of a new map: an empty one or a generated one
class EditorNewMapWindow : public CDialogWindow
{
	std::shared_ptr<CMapGenOptions> options;
	IGameInfoCallback * callback;
	std::function<void(std::unique_ptr<CMap>)> onCreated;

	std::shared_ptr<CFormView> form;
	std::shared_ptr<CDropDown> sizeChoice;
	std::shared_ptr<CNumberInput> widthInput;
	std::shared_ptr<CNumberInput> heightInput;
	std::shared_ptr<CNumberInput> levelsInput;
	std::shared_ptr<CCheckBox> randomCheck;
	std::shared_ptr<CDropDown> humanChoice;
	std::shared_ptr<CDropDown> cpuChoice;
	std::shared_ptr<CDropDown> humanTeamsChoice;
	std::shared_ptr<CDropDown> cpuTeamsChoice;
	std::shared_ptr<CDropDown> waterChoice;
	std::shared_ptr<CDropDown> monsterChoice;
	std::shared_ptr<CCheckBox> dirtRoad;
	std::shared_ptr<CCheckBox> gravelRoad;
	std::shared_ptr<CCheckBox> cobblestoneRoad;
	std::shared_ptr<CDropDown> templateChoice;
	std::shared_ptr<CCheckBox> seedCheck;
	std::shared_ptr<CNumberInput> seedInput;

	std::vector<const CRmgTemplate *> templates;
	bool randomMap = false;
	bool customSize = false;
	bool layerWarningShown = false;

	void build();
	void loadSettings();
	void saveSettings();
	void updateAvailability();
	void updateTemplateList();
	void applyRandomOptions();
	void showLayerDialog();
	void create();

	void onSizeChosen(int index);
	void onLevelsChanged(int levels);
	void onHumansChosen(int index);
	void onComputersChosen(int index);
	void onHumanTeamsChosen(int index);
	void onComputerTeamsChosen(int index);

public:
	EditorNewMapWindow(IGameInfoCallback * callback, const std::function<void(std::unique_ptr<CMap>)> & onCreated);

	static void show(IGameInfoCallback * callback, const std::function<void(std::unique_ptr<CMap>)> & onCreated);
};

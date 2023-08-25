/*
 * RandomMapTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"

#include "../../lib/FunctionList.h"
#include "../../lib/GameConstants.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../gui/InterfaceObjectConfigurable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapGenOptions;

VCMI_LIB_NAMESPACE_END

class CToggleButton;
class CLabel;
class CLabelGroup;
class CSlider;
class CPicture;

class RandomMapTab : public InterfaceObjectConfigurable
{
public:
	RandomMapTab();

	void updateMapInfoByHost();
	void setMapGenOptions(std::shared_ptr<CMapGenOptions> opts);
	void setTemplate(const CRmgTemplate *);
	CMapGenOptions & obtainMapGenOptions() {return *mapGenOptions;}

	CFunctionList<void(std::shared_ptr<CMapInfo>, std::shared_ptr<CMapGenOptions>)> mapInfoChanged;

private:
	void deactivateButtonsFrom(CToggleGroup & group, const std::set<int> & allowed);
	std::vector<int> getPossibleMapSizes();

	std::shared_ptr<CMapGenOptions> mapGenOptions;
	std::shared_ptr<CMapInfo> mapInfo;
	
	//options allowed - need to store as impact each other
	std::set<int> playerCountAllowed, playerTeamsAllowed, compCountAllowed, compTeamsAllowed;
};

class TeamAlignmentsWidget: public InterfaceObjectConfigurable
{
public:
	TeamAlignmentsWidget(RandomMapTab & randomMapTab);
	
private:
	std::shared_ptr<CFilledTexture> background;
	std::shared_ptr<CLabelGroup> labels;
	std::shared_ptr<CButton> buttonOk, buttonCancel;
	std::vector<std::shared_ptr<CToggleGroup>> players;
	std::vector<std::shared_ptr<CIntObject>> placeholders;
};

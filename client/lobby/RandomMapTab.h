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
#include "../gui/InterfaceObjectConfigurable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapGenOptions;

VCMI_LIB_NAMESPACE_END

class CToggleButton;
class CLabel;
class CLabelGroup;
class CSlider;

class RandomMapTab : public InterfaceObjectConfigurable
{
public:
	RandomMapTab();

	void updateMapInfoByHost();
	void setMapGenOptions(std::shared_ptr<CMapGenOptions> opts);

	CFunctionList<void(std::shared_ptr<CMapInfo>, std::shared_ptr<CMapGenOptions>)> mapInfoChanged;

private:
	void deactivateButtonsFrom(CToggleGroup * group, int startId);
	void validatePlayersCnt(int playersCnt);
	void validateCompOnlyPlayersCnt(int compOnlyPlayersCnt);
	std::vector<int> getPossibleMapSizes();

	std::shared_ptr<CMapGenOptions> mapGenOptions;
	std::shared_ptr<CMapInfo> mapInfo;
};

class TemplatesDropBox : public CIntObject
{
	struct ListItem : public CIntObject
	{
		std::shared_ptr<CLabel> labelName;
		std::shared_ptr<CPicture> hoverImage;
		ListItem(Point position, const std::string & text);
		void updateItem(int info = 0, bool selected = false);
		
		void hover(bool on) override;
	};
	
public:
	TemplatesDropBox(std::shared_ptr<CMapGenOptions> options);
	
	void hover(bool on) override;
	void clickLeft(tribool down, bool previousState) override;
	
private:
	
	void sliderMove(int slidPos);
	
	std::shared_ptr<CMapGenOptions> mapGenOptions;
	
	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<ListItem>> listItems;
	std::shared_ptr<CSlider> slider;
	
};

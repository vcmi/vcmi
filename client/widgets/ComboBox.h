/*
 * ComboBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/InterfaceObjectConfigurable.h"
#include "Buttons.h"

class ComboBox : public CButton
{
	class DropDown : public InterfaceObjectConfigurable
	{
		struct Item : public InterfaceObjectConfigurable
		{
			DropDown & dropDown;
			const void * item = nullptr;
			
			Item(const JsonNode &, ComboBox::DropDown &, Point position);
			void updateItem(int index, const void * item = nullptr);
			
			void hover(bool on) override;
			void clickPressed(const Point & cursorPosition) override;
			void clickReleased(const Point & cursorPosition) override;
		};
		
		friend struct Item;
		
	public:
		DropDown(const JsonNode &, ComboBox &);
		
		bool receiveEvent(const Point & position, int eventType) const override;
		void clickPressed(const Point & cursorPosition) override;
		void setItem(const void *);
			
	private:
		std::shared_ptr<DropDown::Item> buildItem(const JsonNode & config);
		
		void sliderMove(int slidPos);
		void updateListItems();
		
		ComboBox & comboBox;
		std::vector<std::shared_ptr<Item>> items;
		std::vector<const void *> curItems;
	};
	
	friend class DropDown;
	
	void setItem(const void *);

public:
	ComboBox(Point position, const std::string & defName, const std::pair<std::string, std::string> & help, const JsonNode & dropDownDescriptor, EShortcut key = {}, bool playerColoredButton = false);
	
	//define this callback to fill input vector with data for the combo box
	std::function<void(std::vector<const void *> &)> onConstructItems;
	
	//callback is called when item is selected and its value can be used
	std::function<void(const void *)> onSetItem;
	
	//return text value from item data
	std::function<std::string(int, const void *)> getItemText;
	
	void setItem(int id);
};

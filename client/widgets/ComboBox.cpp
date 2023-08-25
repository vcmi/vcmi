/*
 * ComboBox.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ComboBox.h"

#include "Slider.h"
#include "Images.h"
#include "TextControls.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"

ComboBox::DropDown::Item::Item(const JsonNode & config, ComboBox::DropDown & _dropDown, Point position)
	: InterfaceObjectConfigurable(LCLICK | HOVER, position),
	dropDown(_dropDown)
{
	build(config);
	
	if(auto w = widget<CPicture>("hoverImage"))
	{
		pos.w = w->pos.w;
		pos.h = w->pos.h;
	}
	setRedrawParent(true);
}

void ComboBox::DropDown::Item::updateItem(int idx, const void * _item)
{
	if(auto w = widget<CLabel>("labelName"))
	{
		item = _item;
		w->setText(dropDown.comboBox.getItemText(idx, item));
	}
}

void ComboBox::DropDown::Item::hover(bool on)
{
	auto h = widget<CPicture>("hoverImage");
	auto w = widget<CLabel>("labelName");
	if(h && w)
	{
		if(w->getText().empty())
			h->visible = false;
		else
			h->visible = on;
	}
	redraw();
}

void ComboBox::DropDown::Item::clickPressed(const Point & cursorPosition)
{
	if(isHovered())
		dropDown.setItem(item);
}

void ComboBox::DropDown::Item::clickReleased(const Point & cursorPosition)
{
	dropDown.clickPressed(cursorPosition);
	dropDown.clickReleased(cursorPosition);
}

ComboBox::DropDown::DropDown(const JsonNode & config, ComboBox & _comboBox):
	InterfaceObjectConfigurable(LCLICK | HOVER),
	comboBox(_comboBox)
{
	REGISTER_BUILDER("item", &ComboBox::DropDown::buildItem);
	
	comboBox.onConstructItems(curItems);
	
	addCallback("sliderMove", std::bind(&ComboBox::DropDown::sliderMove, this, std::placeholders::_1));
	
	pos = comboBox.pos;
	
	build(config);
	
	if(auto w = widget<CSlider>("slider"))
	{
		w->setAmount(curItems.size());
	}

	//FIXME: this should be done by InterfaceObjectConfigurable, but might have side-effects
	pos = children.front()->pos;
	for (auto const & child : children)
		pos = pos.include(child->pos);
	
	updateListItems();
}

std::shared_ptr<ComboBox::DropDown::Item> ComboBox::DropDown::buildItem(const JsonNode & config)
{
	auto position = readPosition(config["position"]);
	items.push_back(std::make_shared<Item>(config, *this, position));
	return items.back();
}

void ComboBox::DropDown::sliderMove(int slidPos)
{
	auto w = widget<CSlider>("slider");
	if(!w)
		return; // ignore spurious call when slider is being created
	updateListItems();
	redraw();
}

bool ComboBox::DropDown::receiveEvent(const Point & position, int eventType) const
{
	if (eventType == LCLICK)
		return true; // we want drop box to close when clicking outside drop box borders

	return CIntObject::receiveEvent(position, eventType);
}

void ComboBox::DropDown::clickPressed(const Point & cursorPosition)
{
	if (!pos.isInside(cursorPosition))
	{
		assert(GH.windows().isTopWindow(this));
		GH.windows().popWindows(1);
	}
}

void ComboBox::DropDown::updateListItems()
{
	if(auto w = widget<CSlider>("slider"))
	{
		int elemIdx = w->getValue();
		for(auto item : items)
		{
			if(elemIdx < curItems.size())
			{
				item->updateItem(elemIdx, curItems[elemIdx]);
				elemIdx++;
			}
			else
			{
				item->updateItem(elemIdx);
			}
		}
	}
}

void ComboBox::DropDown::setItem(const void * item)
{
	comboBox.setItem(item);
	
	assert(GH.windows().isTopWindow(this));
	GH.windows().popWindows(1);
}

void ComboBox::DropDown::constructItems()
{
	comboBox.onConstructItems(curItems);
}

ComboBox::ComboBox(Point position, const std::string & defName, const std::pair<std::string, std::string> & help, const JsonNode & dropDownDescriptor, EShortcut key, bool playerColoredButton):
	CButton(position, defName, help, 0, key, playerColoredButton)
{
	addCallback([&, dropDownDescriptor]()
	{
		GH.windows().createAndPushWindow<ComboBox::DropDown>(dropDownDescriptor, *this);
	});
}

void ComboBox::setItem(const void * item)
{
	if(auto w = std::dynamic_pointer_cast<CLabel>(overlay))
		addTextOverlay(getItemText(0, item), w->font, w->color);
	
	onSetItem(item);
}

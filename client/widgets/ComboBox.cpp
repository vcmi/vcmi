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
#include "../GameEngine.h"
#include "../gui/WindowHandler.h"

ComboBox::DropDown::Item::Item(const JsonNode & config, ComboBox::DropDown & _dropDown, Point position)
	: InterfaceObjectConfigurable(LCLICK | HOVER, position),
	dropDown(_dropDown)
{
	build(config);
	
	if(auto w = widget<CIntObject>("hoverImage"))
	{
		pos.w = w->pos.w;
		pos.h = w->pos.h;
		w->disable();
	}
	setRedrawParent(true);
}

void ComboBox::DropDown::Item::updateItem(int idx, const void * _item)
{
	item = _item;
	if(auto w = widget<CLabel>("labelName"))
	{
		if(dropDown.comboBox.getItemText)
			w->setText(dropDown.comboBox.getItemText(idx, item));
	}
}

void ComboBox::DropDown::Item::hover(bool on)
{
	auto h = widget<CIntObject>("hoverImage");
	auto w = widget<CLabel>("labelName");
	if(h && w)
	{
		if(w->getText().empty() || on == false)
			h->disable();
		else
			h->enable();
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

ComboBox::DropDown::DropDown(const JsonNode & config, ComboBox & _comboBox, Point dropDownPosition):
	InterfaceObjectConfigurable(LCLICK | HOVER),
	comboBox(_comboBox)
{
	REGISTER_BUILDER("item", &ComboBox::DropDown::buildItem);
	
	if(comboBox.onConstructItems)
		comboBox.onConstructItems(curItems);
	
	addCallback("sliderMove", std::bind(&ComboBox::DropDown::sliderMove, this, std::placeholders::_1));
	
	pos = comboBox.pos + dropDownPosition;
	
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
		assert(ENGINE->windows().isTopWindow(this));
		ENGINE->windows().popWindows(1);
	}
}

void ComboBox::DropDown::updateListItems()
{
	int elemIdx = 0;

	if(auto w = widget<CSlider>("slider"))
		elemIdx = w->getValue();

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

void ComboBox::DropDown::setItem(const void * item)
{
	comboBox.setItem(item);
	
	assert(ENGINE->windows().isTopWindow(this));
	ENGINE->windows().popWindows(1);
}

ComboBox::ComboBox(Point position, const AnimationPath & defName, const std::pair<std::string, std::string> & help, const JsonNode & dropDownDescriptor, Point dropDownPosition, EShortcut key, bool playerColoredButton):
	CButton(position, defName, help, 0, key, playerColoredButton)
{
	addCallback([this, dropDownDescriptor, dropDownPosition]()
	{
		ENGINE->windows().createAndPushWindow<ComboBox::DropDown>(dropDownDescriptor, *this, dropDownPosition);
	});
}

void ComboBox::setItem(const void * item)
{
	auto w = std::dynamic_pointer_cast<CLabel>(getOverlay());
	if( w && getItemText)
		setTextOverlay(getItemText(0, item), w->font, w->color);
	
	if(onSetItem)
		onSetItem(item);
}

void ComboBox::setItem(int id)
{
	std::vector<const void *> tempItems;
	onConstructItems(tempItems);
	setItem(tempItems.at(id));
}

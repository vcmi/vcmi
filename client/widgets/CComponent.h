/*
 * CComponent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Component;

VCMI_LIB_NAMESPACE_END

class CAnimImage;
class CLabel;

/// common popup window component
class CComponent : public virtual CIntObject
{
public:
	enum Etype
	{
		primskill, secskill, resource, creature, artifact, experience, spell, morale, luck, building, hero, flag, typeInvalid
	};

	//NOTE: not all types have exact these sizes or have less than 4 of them. In such cases closest one will be used
	enum ESize
	{
		tiny,  // ~22-24px
		small, // ~30px
		medium,// ~42px
		large,  // ~82px
		sizeInvalid
	};

private:
	std::vector<std::shared_ptr<CLabel>> lines;

	size_t getIndex();
	const std::vector<std::string> getFileName();
	void setSurface(std::string defName, int imgPos);
	std::string getSubtitleInternal();

	void init(Etype Type, int Subtype, int Val, ESize imageSize);

public:
	std::shared_ptr<CAnimImage> image;

	Etype compType; //component type
	ESize size; //component size.
	int subtype{}; //type-dependant subtype. See getSomething methods for details
	int val{}; // value \ strength \ amount of component. See getSomething methods for details
	bool perDay; // add "per day" text to subtitle

	std::string getDescription();
	std::string getSubtitle();

	CComponent(Etype Type, int Subtype, int Val = 0, ESize imageSize=large);
	CComponent(const Component &c, ESize imageSize=large);

	void clickRight(tribool down, bool previousState) override; //call-in
};

/// component that can be selected or deselected
class CSelectableComponent : public CComponent, public CKeyShortcut
{
	void init();
public:
	bool selected{}; //if true, this component is selected
	std::function<void()> onSelect; //function called on selection change

	void showAll(SDL_Surface * to) override;
	void select(bool on);

	void clickLeft(tribool down, bool previousState) override; //call-in
	CSelectableComponent(Etype Type, int Sub, int Val, ESize imageSize=large, std::function<void()> OnSelect = nullptr);
	CSelectableComponent(const Component & c, std::function<void()> OnSelect = nullptr);
};

/// box with multiple components (up to 8?)
/// will take ownership on components and delete them afterwards
class CComponentBox : public CIntObject
{
	std::vector<std::shared_ptr<CComponent>> components;

	std::vector<std::shared_ptr<CLabel>> orLabels;

	std::shared_ptr<CSelectableComponent> selected;
	std::function<void(int newID)> onSelect;

	void selectionChanged(std::shared_ptr<CSelectableComponent> newSelection);

	//get position of "or" text between these comps
	//it will place "or" equidistant to both images
	Point getOrTextPos(CComponent *left, CComponent * right);

	//get distance between these copmonents
	int getDistance(CComponent *left, CComponent * right);
	void placeComponents(bool selectable);

public:
	/// return index of selected item
	int selectedIndex();

	/// constructor for non-selectable components
	CComponentBox(std::vector<std::shared_ptr<CComponent>> components, Rect position);

	/// constructor for selectable components
	/// will also create "or" labels between components
	/// onSelect - optional function that will be called every time on selection change
	CComponentBox(std::vector<std::shared_ptr<CSelectableComponent>> components, Rect position, std::function<void(int newID)> onSelect = nullptr);
};

#pragma once

#include "../global.h"

#include <list>

#include "GUIBase.h"
#include "GUIClasses.h"

class AdventureMapButton;
class CAnimImage;
class CHighlightableButtonsGroup;
class CResDataBar;
class CSlider;
class CTownInfo;
class CCreaInfo;
class HeroSlots;

/*
 * CKingdomInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CKingdHeroList;
class CKingdTownList;
class IInfoBoxData;

/* 
 * Several classes to display basically any data.
 * Main part - class InfoBox which controls how data will be formatted\positioned
 * InfoBox have image and 0-2 labels
 * In constructor it should receive object that implements IInfoBoxData interface
 * 
 * interface IInfoBoxData defines way to get data for use in InfoBox
 * have several implementations:
 * InfoBoxHeroData - to display one of fields from hero (e.g. absolute value of primary skills)
 * InfoBoxCustomHeroData - to display one of hero fields without hero (e.g. bonuses from objects)
 * InfoBoxTownData - data from town
 * InfoBoxCustom - user-defined data
 */

/// Displays one of object propertries with image and optional labels
class InfoBox : public CIntObject
{
public:
	enum InfoPos
	{
		POS_UP_DOWN, POS_DOWN, POS_RIGHT, POS_INSIDE, POS_CORNER, POS_NONE
	};
	enum InfoSize
	{
		SIZE_TINY, SIZE_SMALL, SIZE_MEDIUM, SIZE_BIG, SIZE_HUGE
	};

private:
	InfoSize size;
	InfoPos  infoPos;
	IInfoBoxData *data;

	CLabel * value;
	CLabel * name;
	CAnimImage * image;
	HoverableArea *hover;

public:
	InfoBox(Point position, InfoPos Pos, InfoSize Size, IInfoBoxData *Data);
	~InfoBox();

	void clickRight(tribool down, bool previousState);
	void clickLeft(tribool down, bool previousState);

	//Update object if data may have changed
	//void update();
};

class IInfoBoxData
{
public:
	enum InfoType
	{
		HERO_PRIMARY_SKILL, HERO_MANA, HERO_EXPERIENCE, HERO_SPECIAL, HERO_SECONDARY_SKILL,
		//TODO: Luck? Morale? Artifact?
		ARMY_SLOT,//TODO
		TOWN_GROWTH, TOWN_AVAILABLE, TOWN_BUILDING,//TODO
		CUSTOM
	};

protected:
	InfoType type;

	IInfoBoxData(InfoType Type);

public:
	//methods that generate values for displaying
	virtual std::string getValueText()=0;
	virtual std::string getNameText()=0;
	virtual std::string getImageName(InfoBox::InfoSize size)=0;
	virtual std::string getHoverText()=0;
	virtual size_t getImageIndex()=0;

	//TODO: replace with something better
	virtual bool prepareMessage(std::string &text, SComponent **comp)=0;
};

class InfoBoxAbstractHeroData : public IInfoBoxData
{
protected:
	virtual int  getSubID()=0;
	virtual si64 getValue()=0;

public:
	InfoBoxAbstractHeroData(InfoType Type);

	std::string getValueText();
	std::string getNameText();
	std::string getImageName(InfoBox::InfoSize size);
	std::string getHoverText();
	size_t getImageIndex();

	bool prepareMessage(std::string &text, SComponent **comp);
};

class InfoBoxHeroData : public InfoBoxAbstractHeroData
{
	const CGHeroInstance * hero;
	int index;//index of data in hero (0-7 for sec. skill, 0-3 for pr. skill)

	int  getSubID();
	si64 getValue();

public:
	InfoBoxHeroData(InfoType Type, const CGHeroInstance *Hero, int Index=0);

	//To get a bit different texts for hero window
	std::string getHoverText();
	std::string getValueText();

	bool prepareMessage(std::string &text, SComponent **comp);
};

class InfoBoxCustomHeroData : public InfoBoxAbstractHeroData
{
	int subID;//subID of data (0=attack...)
	si64 value;//actual value of data, 64-bit to fit experience and negative values

	int  getSubID();
	si64 getValue();

public:
	InfoBoxCustomHeroData(InfoType Type, int subID, si64 value);
};

class InfoBoxCustom : public IInfoBoxData
{
public:
	std::string valueText;
	std::string nameText;
	std::string imageName;
	std::string hoverText;
	size_t imageIndex;

	InfoBoxCustom(std::string ValueText, std::string NameText, std::string ImageName, size_t ImageIndex, std::string HoverText="");

	std::string getValueText();
	std::string getNameText();
	std::string getImageName(InfoBox::InfoSize size);
	std::string getHoverText();
	size_t getImageIndex();

	bool prepareMessage(std::string &text, SComponent **comp);
};

//TODO!!!
class InfoBoxTownData : public IInfoBoxData
{
	const CGTownInstance * town;
	int index;//index of data in town
	int value;//actual value of data

public:
	InfoBoxTownData(InfoType Type, const CGTownInstance * Town, int Index);
	InfoBoxTownData(InfoType Type, int SubID, int Value);

	std::string getValueText();
	std::string getNameText();
	std::string getImageName(InfoBox::InfoSize size);
	std::string getHoverText();
	size_t getImageIndex();
};

////////////////////////////////////////////////////////////////////////////////

/// Interface used in CTabbedInt and CListBox
class IGuiObjectListManager
{
public:
	//Create object for specified position, may return NULL if no object is needed at this position
	//NOTE: position may be greater then size (empty rows in ListBox)
	virtual CIntObject * getObject(size_t position)=0;

	//Called when object needs to be removed
	virtual void removeObject(CIntObject * object)
	{
		delete object;
	};
	virtual ~IGuiObjectListManager(){};
};

/// Used as base for Tabs and List classes
class CObjectList : public CIntObject
{
	IGuiObjectListManager *manager;

protected:
	//Internal methods for safe creation of items (Children capturing and activation/deactivation if needed)
	void deleteItem(CIntObject* item);
	CIntObject* createItem(size_t index);

public:
	CObjectList(IGuiObjectListManager *Manager);
	~CObjectList();
};

/// Window element with multiple tabs
class CTabbedInt : public CObjectList
{
private:
	CIntObject * activeTab;
	size_t activeID;

public:
	//Manager - object which implements this interface, will be destroyed by TabbedInt
	//Pos - position of object, all tabs will be moved here
	//ActiveID - ID of initially active tab
	CTabbedInt(IGuiObjectListManager *Manager, Point position=Point(), size_t ActiveID=0);

	void setActive(size_t which);
	//recreate active tab
	void reset();

	//return currently active item
	CIntObject * getItem();
};

/// List of IntObjects with optional slider
class CListBox : public CObjectList
{
private:
	std::list< CIntObject* > items;
	size_t first;
	size_t totalSize;

	Point itemOffset;
	CSlider * slider;

	void updatePositions();
public:
	//Manager - object which implements this interface, will be destroyed by ListBox
	//Pos - position of first item
	//ItemOffset - distance between items in the list
	//VisibleSize - maximal number of displayable at once items
	//TotalSize 
	//Slider - slider style, bit field: 1 = present(disabled), 2=horisontal(vertical), 4=blue(brown)
	//SliderPos - position of slider, if present
	CListBox(IGuiObjectListManager *Manager, Point Pos, Point ItemOffset, size_t VisibleSize,
	         size_t TotalSize, size_t InitialPos=0, int Slider=0, Rect SliderPos=Rect() );

	//recreate all visible items
	void reset();

	//return currently active items
	std::list< CIntObject * > getItems();

	//scroll list
	void moveToPos(size_t which);
	void moveToNext();
	void moveToPrev();
};

////////////////////////////////////////////////////////////////////////////////

/// Class which holds all parts of kingdom overview window
class CKingdomInterface : public CGarrisonHolder, public CArtifactHolder
{
private:
	CListBox * dwellingsList;
	CTabbedInt * tabArea;
	CPicture * background;

	//Main buttons
	AdventureMapButton *btnTowns;
	AdventureMapButton *btnHeroes;
	AdventureMapButton *btnExit;

	//Buttons for scrolling dwellings list
	AdventureMapButton *dwellUp, *dwellDown;
	AdventureMapButton *dwellTop, *dwellBottom;

	InfoBox * minesBox[7];

	HoverableArea * incomeArea;
	CLabel * incomeAmount;

	CGStatusBar * statusbar;
	CResDataBar *resdatabar;

	void activateTab(size_t which);

	//Internal functions used during construction
	void generateButtons();
	void generateObjectsList(const std::vector<const CGObjectInstance * > &ownedObjects);
	void generateMinesList(const std::vector<const CGObjectInstance * > &ownedObjects);

public:
	CKingdomInterface();

	void townChanged(const CGTownInstance *town);
	void updateGarrisons();
	void artifactRemoved(const ArtifactLocation &artLoc);
	void artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc);
	void artifactDisassembled(const ArtifactLocation &artLoc);
	void artifactAssembled(const ArtifactLocation &artLoc);
};

/// List item with town
class CTownItem : public CGarrisonHolder
{
	CAnimImage *background;
	CAnimImage *picture;
	CLabel *name;
	CLabel *income;
	CGarrisonInt *garr;
	LRClickableAreaOpenTown *townArea;

	HeroSlots *heroes;
	CTownInfo *hall, *fort;
	std::vector<CCreaInfo*> available;
	std::vector<CCreaInfo*> growth;

public:
	const CGTownInstance * town;

	CTownItem(const CGTownInstance* town);

	void updateGarrisons();
	void update();
};

/// List item with hero
class CHeroItem : public CWindowWithGarrison
{
	const CGHeroInstance * hero;

	CAnimImage *background;
	CAnimImage *portrait;
	CLabel *name;
	CHeroArea *heroArea;

	CLabel *artsText;
	CTabbedInt *artsTabs;

	CHighlightableButtonsGroup *artButtons;
	std::vector<InfoBox*> heroInfo;
	MoraleLuckBox * morale, * luck;

	void onArtChange(int tabIndex);

public:
	CArtifactsOfHero *heroArts;

	CHeroItem(const CGHeroInstance* hero, CArtifactsOfHero::SCommonPart * artsCommonPart);
};

/// Tab with all hero-specific data
class CKingdHeroList : public CGarrisonHolder, public CWindowWithArtifacts
{
private:
	std::vector<CHeroItem*> heroItems;
	CListBox * heroes;
	CPicture * title;
	CLabel * heroLabel;
	CLabel * skillsLabel;

public:
	CKingdHeroList(size_t maxSize);

	void updateGarrisons();
};

/// Tab with all town-specific data
class CKingdTownList : public CGarrisonHolder
{
private:
	std::vector<CTownItem*> townItems;
	CListBox * towns;
	CPicture * title;
	CLabel * townLabel;
	CLabel * garrHeroLabel;
	CLabel * visitHeroLabel;
	
public:
	CKingdTownList(size_t maxSize);
	
	void townChanged(const CGTownInstance *town);
	void updateGarrisons();
};

#pragma once

#include "../widgets/CArtifactHolder.h"
#include "../widgets/CGarrisonInt.h"

class CButton;
class CAnimImage;
class CToggleGroup;
class CResDataBar;
class CSlider;
class CTownInfo;
class CCreaInfo;
class HeroSlots;
class LRClickableAreaOpenTown;
class CComponent;
class CHeroArea;
class MoraleLuckBox;
class CListBox;
class CTabbedInt;
class CGStatusBar;

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
	CHoverableArea *hover;

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
	virtual bool prepareMessage(std::string &text, CComponent **comp)=0;

	virtual ~IInfoBoxData(){};
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

	bool prepareMessage(std::string &text, CComponent **comp);
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

	bool prepareMessage(std::string &text, CComponent **comp);
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

	bool prepareMessage(std::string &text, CComponent **comp);
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

/// Class which holds all parts of kingdom overview window
class CKingdomInterface : public CWindowObject, public CGarrisonHolder, public CArtifactHolder
{
private:
	struct OwnedObjectInfo
	{
		int imageID;
		ui32 count;
		std::string hoverText;
		OwnedObjectInfo():
		    imageID(0),
		    count(0)
		{}
	};
	std::vector<OwnedObjectInfo> objects;

	CListBox * dwellingsList;
	CTabbedInt * tabArea;

	//Main buttons
	CButton *btnTowns;
	CButton *btnHeroes;
	CButton *btnExit;

	//Buttons for scrolling dwellings list
	CButton *dwellUp, *dwellDown;
	CButton *dwellTop, *dwellBottom;

	InfoBox * minesBox[7];

	CHoverableArea * incomeArea;
	CLabel * incomeAmount;

	CGStatusBar * statusbar;
	CResDataBar *resdatabar;

	void activateTab(size_t which);

	//Internal functions used during construction
	void generateButtons();
	void generateObjectsList(const std::vector<const CGObjectInstance * > &ownedObjects);
	void generateMinesList(const std::vector<const CGObjectInstance * > &ownedObjects);

	CIntObject* createOwnedObject(size_t index);
	CIntObject* createMainTab(size_t index);

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
class CTownItem : public CIntObject, public CGarrisonHolder
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
class CHeroItem : public CIntObject, public CWindowWithGarrison
{
	const CGHeroInstance * hero;

	std::vector<CIntObject *> artTabs;

	CAnimImage *portrait;
	CLabel *name;
	CHeroArea *heroArea;

	CLabel *artsText;
	CTabbedInt *artsTabs;

	CToggleGroup *artButtons;
	std::vector<InfoBox*> heroInfo;
	MoraleLuckBox * morale, * luck;

	void onArtChange(int tabIndex);

	CIntObject * onTabSelected(size_t index);
	void onTabDeselected(CIntObject *object);

public:
	CArtifactsOfHero *heroArts;

	CHeroItem(const CGHeroInstance* hero, CArtifactsOfHero::SCommonPart * artsCommonPart);
};

/// Tab with all hero-specific data
class CKingdHeroList : public CIntObject, public CGarrisonHolder, public CWindowWithArtifacts
{
private:
	CArtifactsOfHero::SCommonPart artsCommonPart;

	std::vector<CHeroItem*> heroItems;
	CListBox * heroes;
	CPicture * title;
	CLabel * heroLabel;
	CLabel * skillsLabel;

	CIntObject* createHeroItem(size_t index);
	void destroyHeroItem(CIntObject *item);
public:
	CKingdHeroList(size_t maxSize);

	void updateGarrisons();
};

/// Tab with all town-specific data
class CKingdTownList : public CIntObject, public CGarrisonHolder
{
private:
	std::vector<CTownItem*> townItems;
	CListBox * towns;
	CPicture * title;
	CLabel * townLabel;
	CLabel * garrHeroLabel;
	CLabel * visitHeroLabel;
	
	CIntObject* createTownItem(size_t index);
public:
	CKingdTownList(size_t maxSize);
	
	void townChanged(const CGTownInstance *town);
	void updateGarrisons();
};

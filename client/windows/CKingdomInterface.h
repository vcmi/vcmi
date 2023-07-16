/*
 * CKingdomInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CWindowWithArtifacts.h"
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
	InfoPos infoPos;
	std::shared_ptr<IInfoBoxData> data;

	std::shared_ptr<CLabel> value;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CAnimImage> image;
	std::shared_ptr<CHoverableArea> hover;

public:
	InfoBox(Point position, InfoPos Pos, InfoSize Size, std::shared_ptr<IInfoBoxData> Data);
	~InfoBox();

	void showPopupWindow(const Point & cursorPosition) override;
	void clickPressed(const Point & cursorPosition) override;
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
	virtual void prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp)=0;

	virtual ~IInfoBoxData(){};
};

class InfoBoxAbstractHeroData : public IInfoBoxData
{
protected:
	virtual int  getSubID()=0;
	virtual si64 getValue()=0;

public:
	InfoBoxAbstractHeroData(InfoType Type);

	std::string getValueText() override;
	std::string getNameText() override;
	std::string getImageName(InfoBox::InfoSize size) override;
	std::string getHoverText() override;
	size_t getImageIndex() override;

	void prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp) override;
};

class InfoBoxHeroData : public InfoBoxAbstractHeroData
{
	const CGHeroInstance * hero;
	int index;//index of data in hero (0-7 for sec. skill, 0-3 for pr. skill)

	int  getSubID() override;
	si64 getValue() override;

public:
	InfoBoxHeroData(InfoType Type, const CGHeroInstance *Hero, int Index=0);

	//To get a bit different texts for hero window
	std::string getHoverText() override;
	std::string getValueText() override;

	void prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp) override;
};

class InfoBoxCustomHeroData : public InfoBoxAbstractHeroData
{
	int subID;//subID of data (0=attack...)
	si64 value;//actual value of data, 64-bit to fit experience and negative values

	int  getSubID() override;
	si64 getValue() override;

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

	std::string getValueText() override;
	std::string getNameText() override;
	std::string getImageName(InfoBox::InfoSize size) override;
	std::string getHoverText() override;
	size_t getImageIndex() override;

	void prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp) override;
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

	std::string getValueText() override;
	std::string getNameText() override;
	std::string getImageName(InfoBox::InfoSize size) override;
	std::string getHoverText() override;
	size_t getImageIndex() override;
};

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

	std::shared_ptr<CListBox> dwellingsList;
	std::shared_ptr<CTabbedInt> tabArea;

	//Main buttons
	std::shared_ptr<CButton> btnTowns;
	std::shared_ptr<CButton> btnHeroes;
	std::shared_ptr<CButton> btnExit;

	//Buttons for scrolling dwellings list
	std::shared_ptr<CButton> dwellUp;
	std::shared_ptr<CButton> dwellDown;
	std::shared_ptr<CButton> dwellTop;
	std::shared_ptr<CButton> dwellBottom;

	std::array<std::shared_ptr<InfoBox>, 7> minesBox;

	std::shared_ptr<CHoverableArea> incomeArea;
	std::shared_ptr<CLabel> incomeAmount;

	std::shared_ptr<CGStatusBar> statusbar;
	std::shared_ptr<CResDataBar> resdatabar;

	void activateTab(size_t which);

	//Internal functions used during construction
	void generateButtons();
	void generateObjectsList(const std::vector<const CGObjectInstance * > &ownedObjects);
	void generateMinesList(const std::vector<const CGObjectInstance * > &ownedObjects);

	std::shared_ptr<CIntObject> createOwnedObject(size_t index);
	std::shared_ptr<CIntObject> createMainTab(size_t index);

public:
	CKingdomInterface();

	void townChanged(const CGTownInstance *town);
	void heroRemoved();
	void updateGarrisons() override;
	void artifactRemoved(const ArtifactLocation &artLoc) override;
	void artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc, bool withRedraw) override;
	void artifactDisassembled(const ArtifactLocation &artLoc) override;
	void artifactAssembled(const ArtifactLocation &artLoc) override;
};

/// List item with town
class CTownItem : public CIntObject, public CGarrisonHolder
{
	std::shared_ptr<CAnimImage> background;
	std::shared_ptr<CAnimImage> picture;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CLabel> income;
	std::shared_ptr<CGarrisonInt> garr;

	std::shared_ptr<HeroSlots> heroes;
	std::shared_ptr<CTownInfo> hall;
	std::shared_ptr<CTownInfo> fort;

	std::vector<std::shared_ptr<CCreaInfo>> available;
	std::vector<std::shared_ptr<CCreaInfo>> growth;

	std::shared_ptr<LRClickableAreaOpenTown> openTown;

public:
	const CGTownInstance * town;

	CTownItem(const CGTownInstance * Town);

	void updateGarrisons() override;
	void update();
};

/// List item with hero
class CHeroItem : public CIntObject, public CGarrisonHolder
{
	const CGHeroInstance * hero;

	std::vector<std::shared_ptr<CIntObject>> artTabs;

	std::shared_ptr<CAnimImage> portrait;
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CHeroArea> heroArea;

	std::shared_ptr<CLabel> artsText;
	std::shared_ptr<CTabbedInt> artsTabs;

	std::shared_ptr<CToggleGroup> artButtons;
	std::vector<std::shared_ptr<InfoBox>> heroInfo;
	std::shared_ptr<MoraleLuckBox> morale;
	std::shared_ptr<MoraleLuckBox> luck;

	std::shared_ptr<CGarrisonInt> garr;

	void onArtChange(int tabIndex);

	std::shared_ptr<CIntObject> onTabSelected(size_t index);

public:
	std::shared_ptr<CArtifactsOfHeroKingdom> heroArts;

	void updateGarrisons() override;

	CHeroItem(const CGHeroInstance * hero);
};

/// Tab with all hero-specific data
class CKingdHeroList : public CIntObject, public CGarrisonHolder, public CWindowWithArtifacts
{
private:
	std::shared_ptr<CListBox> heroes;
	std::shared_ptr<CPicture> title;
	std::shared_ptr<CLabel> heroLabel;
	std::shared_ptr<CLabel> skillsLabel;

	std::shared_ptr<CIntObject> createHeroItem(size_t index);
public:
	CKingdHeroList(size_t maxSize);

	void updateGarrisons() override;
};

/// Tab with all town-specific data
class CKingdTownList : public CIntObject, public CGarrisonHolder
{
private:
	std::shared_ptr<CListBox> towns;
	std::shared_ptr<CPicture> title;
	std::shared_ptr<CLabel> townLabel;
	std::shared_ptr<CLabel> garrHeroLabel;
	std::shared_ptr<CLabel> visitHeroLabel;

	std::shared_ptr<CIntObject> createTownItem(size_t index);
public:
	CKingdTownList(size_t maxSize);

	void townChanged(const CGTownInstance * town);
	void updateGarrisons() override;
};

/*
 * MiscWidgets.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../../lib/networkPacks/Component.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGGarrison;
class CGCreature;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;
class CArmedInstance;
class CGTownInstance;
class CGHeroInstance;
class AFactionMember;

VCMI_LIB_NAMESPACE_END

class CLabel;
class CTextBox;
class CGarrisonInt;
class CCreatureAnim;
class CComponent;
class CAnimImage;
class LRClickableArea;
class TransparentFilledRectangle;

/// Shows a text by moving the mouse cursor over the object
class CHoverableArea: public virtual CIntObject
{
public:
	std::string hoverText;

	void hover (bool on) override;

	CHoverableArea();
	virtual ~CHoverableArea();
};

/// Can interact on left and right mouse clicks, plus it shows a text when by hovering over it
class LRClickableAreaWText: public CHoverableArea
{
public:
	std::string text;

	LRClickableAreaWText();
	LRClickableAreaWText(const Rect & Pos, const std::string & HoverText = "", const std::string & ClickText = "");
	virtual ~LRClickableAreaWText();
	void init();

	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
};

/// base class for hero/town tooltips
class CArmyTooltip : public CIntObject
{
	std::shared_ptr<CLabel> title;
	std::vector<std::shared_ptr<CAnimImage>> icons;
	std::vector<std::shared_ptr<CLabel>> subtitles;
	void init(const InfoAboutArmy & army);
public:
	CArmyTooltip(Point pos, const InfoAboutArmy & army);
	CArmyTooltip(Point pos, const CArmedInstance * army);
};

/// base class garrison tooltips
class CGarrisonTooltip : public CIntObject
{
	std::shared_ptr<CLabel> title;
	std::vector<std::shared_ptr<CAnimImage>> icons;
	std::vector<std::shared_ptr<CLabel>> subtitles;
	void init(const InfoAboutArmy& army);
public:
	CGarrisonTooltip(Point pos, const InfoAboutArmy& army);
};

/// Class for hero tooltip. Does not have any background!
/// background for infoBox: ADSTATHR
/// background for tooltip: HEROQVBK
class CHeroTooltip : public CArmyTooltip
{
	std::shared_ptr<CAnimImage> portrait;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::shared_ptr<CAnimImage> morale;
	std::shared_ptr<CAnimImage> luck;

	void init(const InfoAboutHero & hero);
public:
	CHeroTooltip(Point pos, const InfoAboutHero & hero);
	CHeroTooltip(Point pos, const CGHeroInstance * hero);
};

/// Class for HD mod-like interactable infobox tooltip. Does not have any background!
class CInteractableHeroTooltip : public CIntObject
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CAnimImage> portrait;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::shared_ptr<CAnimImage> morale;
	std::shared_ptr<CAnimImage> luck;
	std::shared_ptr<CGarrisonInt> garrison;

	void init(const InfoAboutHero & hero);
public:
	CInteractableHeroTooltip(Point pos, const CGHeroInstance * hero);
};

/// Class for town tooltip. Does not have any background!
/// background for infoBox: ADSTATCS
/// background for tooltip: TOWNQVBK
class CTownTooltip : public CArmyTooltip
{
	std::shared_ptr<CAnimImage> fort;
	std::shared_ptr<CAnimImage> hall;
	std::shared_ptr<CAnimImage> build;
	std::shared_ptr<CLabel> income;
	std::shared_ptr<CPicture> garrisonedHero;
	std::shared_ptr<CAnimImage> res1;
	std::shared_ptr<CAnimImage> res2;

	void init(const InfoAboutTown & town);
public:
	CTownTooltip(Point pos, const InfoAboutTown & town);
	CTownTooltip(Point pos, const CGTownInstance * town);
};

/// Class for HD mod-like interactable infobox tooltip. Does not have any background!
class CInteractableTownTooltip : public CIntObject
{
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CAnimImage> fort;
	std::shared_ptr<CAnimImage> hall;
	std::shared_ptr<CAnimImage> build;
	std::shared_ptr<CLabel> income;
	std::shared_ptr<CPicture> garrisonedHero;
	std::shared_ptr<CAnimImage> res1;
	std::shared_ptr<CAnimImage> res2;
	std::shared_ptr<CGarrisonInt> garrison;
	
	std::shared_ptr<LRClickableArea> fastTavern;
	std::shared_ptr<LRClickableArea> fastMarket;
	std::shared_ptr<LRClickableArea> fastTownHall;
	std::shared_ptr<LRClickableArea> fastArmyPurchase;

	void init(const CGTownInstance * town);
public:
	CInteractableTownTooltip(Point pos, const CGTownInstance * town);
};

/// draws picture with creature on background, use Animated=true to get animation
class CCreaturePic : public CIntObject
{
private:
	std::shared_ptr<CPicture> bg;
	std::shared_ptr<CCreatureAnim> anim; //displayed animation
	std::shared_ptr<CLabel> amount;

	void show(Canvas & to) override;
public:
	CCreaturePic(int x, int y, const CCreature * cre, bool Big=true, bool Animated=true);
	void setAmount(int newAmount);
};

class CreatureTooltip : public CIntObject
{
	std::shared_ptr<CAnimImage> creatureImage;
	std::shared_ptr<CTextBox> tooltipTextbox;

	void show(Canvas & to) override;
public:
	CreatureTooltip(Point pos, const CGCreature * creature);
};

/// Resource bar like that at the bottom of the adventure map screen
class CMinorResDataBar : public CIntObject
{
	std::shared_ptr<CPicture> background;

	std::string buildDateString();
public:
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
	CMinorResDataBar();
	~CMinorResDataBar();
};

/// Performs an action by left-clicking on it. Opens hero window by default
class CHeroArea: public CIntObject
{
public:
	using ClickFunctor = std::function<void()>;

	CHeroArea(int x, int y, const CGHeroInstance * hero);
	void addClickCallback(ClickFunctor callback);
	void addRClickCallback(ClickFunctor callback);
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;
private:
	const CGHeroInstance * hero;
	std::shared_ptr<CAnimImage> portrait;
	ClickFunctor clickFunctor;
	ClickFunctor clickRFunctor;
	ClickFunctor showPopupHandler;
};

/// Can interact on left and right mouse clicks
class LRClickableAreaWTextComp: public LRClickableAreaWText
{
public:
	Component component;

	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;

	LRClickableAreaWTextComp(const Rect &Pos = Rect(0,0,0,0), ComponentType baseType = ComponentType::NONE);
	std::shared_ptr<CComponent> createComponent() const;
};

/// Opens town screen by left-clicking on it
class LRClickableAreaOpenTown: public LRClickableAreaWTextComp
{
public:
	const CGTownInstance * town;
	void clickPressed(const Point & cursorPosition) override;
	LRClickableAreaOpenTown(const Rect & Pos, const CGTownInstance * Town);
};

/// Can do action on click
class LRClickableArea: public CIntObject
{
	std::function<void()> onClick;
	std::function<void()> onPopup;
public:
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	LRClickableArea(const Rect & Pos, std::function<void()> onClick = nullptr, std::function<void()> onPopup = nullptr);
};

class MoraleLuckBox : public LRClickableAreaWTextComp
{
	std::shared_ptr<CAnimImage> image;
	std::shared_ptr<CLabel> label;
public:
	bool morale; //true if morale, false if luck
	bool small;

	void set(const AFactionMember *node);

	MoraleLuckBox(bool Morale, const Rect &r, bool Small=false);
};

class SelectableSlot : public LRClickableAreaWTextComp
{
	std::shared_ptr<TransparentFilledRectangle> selection;
	bool selected;

public:
	SelectableSlot(Rect area, Point oversize, const int width);
	SelectableSlot(Rect area, Point oversize);
	SelectableSlot(Rect area, const int width = 1);
	void selectSlot(bool on);
	bool isSelected() const;
	void setSelectionWidth(int width);
	void moveSelectionForeground();
};

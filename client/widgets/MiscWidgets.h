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

VCMI_LIB_NAMESPACE_BEGIN

class CGGarrison;
struct InfoAboutArmy;
class CArmedInstance;
class IBonusBearer;

VCMI_LIB_NAMESPACE_END

class CLabel;
class CCreatureAnim;
class CComponent;
class CAnimImage;

/// Shows a text by moving the mouse cursor over the object
class CHoverableArea: public virtual CIntObject
{
public:
	std::string hoverText;

	virtual void hover (bool on) override;

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

	virtual void clickLeft(tribool down, bool previousState) override;
	virtual void clickRight(tribool down, bool previousState) override;
};

/// base class for hero/town/garrison tooltips
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

/// draws picture with creature on background, use Animated=true to get animation
class CCreaturePic : public CIntObject
{
private:
	std::shared_ptr<CPicture> bg;
	std::shared_ptr<CCreatureAnim> anim; //displayed animation
	std::shared_ptr<CLabel> amount;

	void show(SDL_Surface * to) override;
public:
	CCreaturePic(int x, int y, const CCreature * cre, bool Big=true, bool Animated=true);
	void setAmount(int newAmount);
};

/// Resource bar like that at the bottom of the adventure map screen
class CMinorResDataBar : public CIntObject
{
	std::shared_ptr<CPicture> background;

	std::string buildDateString();
public:
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	CMinorResDataBar();
	~CMinorResDataBar();
};

/// Opens hero window by left-clicking on it
class CHeroArea: public CIntObject
{
	const CGHeroInstance * hero;
	std::shared_ptr<CAnimImage> portrait;

public:
	CHeroArea(int x, int y, const CGHeroInstance * _hero);

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void hover(bool on) override;
};

/// Can interact on left and right mouse clicks
class LRClickableAreaWTextComp: public LRClickableAreaWText
{
public:
	int baseType;
	int bonusValue;
	virtual void clickLeft(tribool down, bool previousState) override;
	virtual void clickRight(tribool down, bool previousState) override;

	LRClickableAreaWTextComp(const Rect &Pos = Rect(0,0,0,0), int BaseType = -1);
	std::shared_ptr<CComponent> createComponent() const;
};

/// Opens town screen by left-clicking on it
class LRClickableAreaOpenTown: public LRClickableAreaWTextComp
{
public:
	const CGTownInstance * town;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	LRClickableAreaOpenTown(const Rect & Pos, const CGTownInstance * Town);
};

class MoraleLuckBox : public LRClickableAreaWTextComp
{
	std::shared_ptr<CAnimImage> image;
public:
	bool morale; //true if morale, false if luck
	bool small;

	void set(const IBonusBearer *node);

	MoraleLuckBox(bool Morale, const Rect &r, bool Small=false);
};

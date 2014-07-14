#pragma once

#include "CComponent.h"

/*
 * MiscWidgets.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreatureAnim;
class CComponent;
class CGGarrison;
class CSelectableComponent;
class InfoAboutArmy;
class CArmedInstance;
class IBonusBearer;

/// text + comp. + ok button
class CInfoWindow : public CSimpleWindow
{ //window able to delete its components when closed
	bool delComps; //whether comps will be deleted

public:
	typedef std::vector<std::pair<std::string,CFunctionList<void()> > > TButtonsInfo;
	typedef std::vector<CComponent*> TCompsInfo;
	QueryID ID; //for identification
	CTextBox *text;
	std::vector<CAdventureMapButton *> buttons;
	std::vector<CComponent*> components;
	CSlider *slider;

	void setDelComps(bool DelComps);
	virtual void close();

	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void sliderMoved(int to);

	CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo &comps = TCompsInfo(), const TButtonsInfo &Buttons = TButtonsInfo(), bool delComps = true); //c-tor
	CInfoWindow(); //c-tor
	~CInfoWindow(); //d-tor

	//use only before the game starts! (showYesNoDialog in LOCPLINT must be used then)
	static void showInfoDialog( const std::string & text, const std::vector<CComponent*> *components, bool DelComps = true, PlayerColor player = PlayerColor(1));
	static void showOkDialog(const std::string & text, const std::vector<CComponent*> *components, const boost::function<void()> & onOk, bool delComps = true, PlayerColor player = PlayerColor(1));
	static void showYesNoDialog( const std::string & text, const std::vector<CComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps = true, PlayerColor player = PlayerColor(1));
	static CInfoWindow *create(const std::string &text, PlayerColor playerID = PlayerColor(1), const std::vector<CComponent*> *components = nullptr, bool DelComps = false);

	/// create text from title and description: {title}\n\n description
	static std::string genText(std::string title, std::string description);
};

/// popup displayed on R-click
class CRClickPopup : public CIntObject
{
public:
	virtual void close();
	void clickRight(tribool down, bool previousState);

	CRClickPopup();
	virtual ~CRClickPopup(); //d-tor

	static CIntObject* createInfoWin(Point position, const CGObjectInstance * specific);
	static void createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps = CInfoWindow::TCompsInfo());
	static void createAndPush(const std::string &txt, CComponent * component);
	static void createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment = BOTTOMRIGHT);
};

/// popup displayed on R-click
class CRClickPopupInt : public CRClickPopup
{
public:
	IShowActivatable *inner;
	bool delInner;

	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	CRClickPopupInt(IShowActivatable *our, bool deleteInt); //c-tor
	virtual ~CRClickPopupInt(); //d-tor
};

class CInfoPopup : public CRClickPopup
{
public:
	bool free; //TODO: comment me
	SDL_Surface * bitmap; //popup background
	void close();
	void show(SDL_Surface * to);
	CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free=false); //c-tor
	CInfoPopup(SDL_Surface * Bitmap, const Point &p, EAlignment alignment, bool Free=false); //c-tor
	CInfoPopup(SDL_Surface * Bitmap = nullptr, bool Free = false); //default c-tor

	void init(int x, int y);
	~CInfoPopup(); //d-tor
};

/// popup on adventure map for town\hero objects
class CInfoBoxPopup : public CWindowObject
{
	Point toScreen(Point pos);
public:
	CInfoBoxPopup(Point position, const CGTownInstance * town);
	CInfoBoxPopup(Point position, const CGHeroInstance * hero);
	CInfoBoxPopup(Point position, const CGGarrison * garr);
};

/// component selection window
class CSelWindow : public CInfoWindow
{ //warning - this window deletes its components by closing!
public:
	void selectionChange(unsigned to);
	void madeChoice(); //looks for selected component and calls callback
	CSelWindow(const std::string& text, PlayerColor player, int charperline ,const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, QueryID askID); //c-tor
	CSelWindow(){}; //c-tor
	//notification - this class inherits important destructor from CInfoWindow
};

/// base class for hero/town/garrison tooltips
class CArmyTooltip : public CIntObject
{
	void init(const InfoAboutArmy &army);
public:
	CArmyTooltip(Point pos, const InfoAboutArmy &army);
	CArmyTooltip(Point pos, const CArmedInstance * army);
};

/// Class for hero tooltip. Does not have any background!
/// background for infoBox: ADSTATHR
/// background for tooltip: HEROQVBK
class CHeroTooltip : public CArmyTooltip
{
	void init(const InfoAboutHero &hero);
public:
	CHeroTooltip(Point pos, const InfoAboutHero &hero);
	CHeroTooltip(Point pos, const CGHeroInstance * hero);
};

/// Class for town tooltip. Does not have any background!
/// background for infoBox: ADSTATCS
/// background for tooltip: TOWNQVBK
class CTownTooltip : public CArmyTooltip
{
	void init(const InfoAboutTown &town);
public:
	CTownTooltip(Point pos, const InfoAboutTown &town);
	CTownTooltip(Point pos, const CGTownInstance * town);
};

/// draws picture with creature on background, use Animated=true to get animation
class CCreaturePic : public CIntObject
{
private:
	CPicture *bg;
	CCreatureAnim *anim; //displayed animation

public:
	CCreaturePic(int x, int y, const CCreature *cre, bool Big=true, bool Animated=true); //c-tor
};

/// Resource bar like that at the bottom of the adventure map screen
class CMinorResDataBar : public CIntObject
{
public:
	SDL_Surface *bg; //background bitmap
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	CMinorResDataBar(); //c-tor
	~CMinorResDataBar(); //d-tor
};

class MoraleLuckBox : public LRClickableAreaWTextComp
{
	CAnimImage *image;
public:
	bool morale; //true if morale, false if luck
	bool small;

	void set(const IBonusBearer *node);

	MoraleLuckBox(bool Morale, const Rect &r, bool Small=false);
};

/// Opens hero window by left-clicking on it
class CHeroArea: public CIntObject
{
	const CGHeroInstance * hero;
public:

	CHeroArea(int x, int y, const CGHeroInstance * _hero);

	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover(bool on);
};

/// Opens town screen by left-clicking on it
class LRClickableAreaOpenTown: public LRClickableAreaWTextComp
{
public:
	const CGTownInstance * town;
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	LRClickableAreaOpenTown();
};

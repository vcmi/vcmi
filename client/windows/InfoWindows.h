/*
 * InfoWindows.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowObject.h"
#include "../../lib/FunctionList.h"

struct SDL_Surface;
struct Rect;
class CAnimImage;
class CLabel;
class CAnimation;
class CComponent;
class CSelectableComponent;
class CGGarrison;
class CTextBox;
class CButton;
class CSlider;
class CArmyTooltip;

// Window GUI class
class CSimpleWindow : public CIntObject
{
public:
	SDL_Surface * bitmap; //background
	virtual void show(SDL_Surface * to) override;
	CSimpleWindow():bitmap(nullptr){};
	virtual ~CSimpleWindow();
};

/// text + comp. + ok button
class CInfoWindow : public CSimpleWindow
{
public:
	typedef std::vector<std::pair<std::string, CFunctionList<void()> > > TButtonsInfo;
	typedef std::vector<std::shared_ptr<CComponent>> TCompsInfo;
	QueryID ID; //for identification
	std::shared_ptr<CTextBox> text;
	std::vector<std::shared_ptr<CButton>> buttons;
	TCompsInfo components;

	virtual void close();

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	void sliderMoved(int to);

	CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo & comps = TCompsInfo(), const TButtonsInfo & Buttons = TButtonsInfo());
	CInfoWindow();
	~CInfoWindow();

	//use only before the game starts! (showYesNoDialog in LOCPLINT must be used then)
	static void showInfoDialog( const std::string & text, const TCompsInfo & components, PlayerColor player = PlayerColor(1));
	static void showOkDialog(const std::string & text, const TCompsInfo & components, const std::function<void()> & onOk, PlayerColor player = PlayerColor(1));
	static void showYesNoDialog( const std::string & text, const TCompsInfo & components, const CFunctionList<void()> & onYes, const CFunctionList<void()> & onNo, PlayerColor player = PlayerColor(1));
	static CInfoWindow * create(const std::string & text, PlayerColor playerID = PlayerColor(1), const TCompsInfo & components = TCompsInfo());

	/// create text from title and description: {title}\n\n description
	static std::string genText(std::string title, std::string description);
};

/// popup displayed on R-click
class CRClickPopup : public CIntObject
{
public:
	virtual void close();
	void clickRight(tribool down, bool previousState) override;

	CRClickPopup();
	virtual ~CRClickPopup();

	static CIntObject* createInfoWin(Point position, const CGObjectInstance * specific);
	static void createAndPush(const std::string & txt, const CInfoWindow::TCompsInfo &comps = CInfoWindow::TCompsInfo());
	static void createAndPush(const std::string & txt, std::shared_ptr<CComponent> component);
	static void createAndPush(const CGObjectInstance * obj, const Point & p, EAlignment alignment = BOTTOMRIGHT);
};

/// popup displayed on R-click
class CRClickPopupInt : public CRClickPopup
{
public:
	IShowActivatable *inner;
	bool delInner;

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	CRClickPopupInt(IShowActivatable *our, bool deleteInt);
	virtual ~CRClickPopupInt();
};

class CInfoPopup : public CRClickPopup
{
public:
	bool free; //TODO: comment me
	SDL_Surface * bitmap; //popup background
	void close() override;
	void show(SDL_Surface * to) override;
	CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free=false);
	CInfoPopup(SDL_Surface * Bitmap, const Point &p, EAlignment alignment, bool Free=false);
	CInfoPopup(SDL_Surface * Bitmap = nullptr, bool Free = false);

	void init(int x, int y);
	~CInfoPopup();
};

/// popup on adventure map for town\hero objects
class CInfoBoxPopup : public CWindowObject
{
	std::shared_ptr<CArmyTooltip> tooltip;
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
	CSelWindow(const std::string & text, PlayerColor player, int charperline ,const std::vector<std::shared_ptr<CSelectableComponent>> & comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, QueryID askID);
	CSelWindow(){};
	//notification - this class inherits important destructor from CInfoWindow
};

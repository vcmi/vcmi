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
#include "../gui/TextAlignment.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
class CGGarrison;
class CGCreature;

VCMI_LIB_NAMESPACE_END

class CComponent;
class CComponentBox;
class CSelectableComponent;
class CTextBox;
class CButton;
class CFilledTexture;

/// text + comp. + ok button
class CInfoWindow : public WindowBase
{
public:
	using TButtonsInfo = std::vector<std::pair<AnimationPath, CFunctionList<void()>>>;
	using TCompsInfo = std::vector<std::shared_ptr<CComponent>>;
	QueryID ID; //for identification
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CTextBox> text;
	std::shared_ptr<CComponentBox> components;
	std::vector<std::shared_ptr<CButton>> buttons;

	void close() override;
	void showAll(Canvas & to) override;

	void sliderMoved(int to);

	CInfoWindow(const std::string & Text, PlayerColor player, const TCompsInfo & comps = TCompsInfo(), const TButtonsInfo & Buttons = TButtonsInfo());
	CInfoWindow();
	~CInfoWindow();

	//use only before the game starts! (showYesNoDialog in LOCPLINT must be used then)
	static void showInfoDialog(const std::string & text, const TCompsInfo & components, PlayerColor player = PlayerColor(1));
	static void showYesNoDialog(const std::string & text, const TCompsInfo & components, const CFunctionList<void()> & onYes, const CFunctionList<void()> & onNo, PlayerColor player = PlayerColor(1));
	static std::shared_ptr<CInfoWindow> create(const std::string & text, PlayerColor playerID = PlayerColor(1), const TCompsInfo & components = TCompsInfo());

	/// create text from title and description: {title}\n\n description
	static std::string genText(const std::string & title, const std::string & description);
};

/// popup displayed on R-click
class CRClickPopup : public WindowBase
{
public:
	bool isPopupWindow() const override;

	static std::shared_ptr<WindowBase> createCustomInfoWindow(Point position, const CGObjectInstance * specific);
	static void createAndPush(const std::string & txt, const CInfoWindow::TCompsInfo & comps = CInfoWindow::TCompsInfo());
	static void createAndPush(const std::string & txt, const std::shared_ptr<CComponent> & component);
	static void createAndPush(const CGObjectInstance * obj, const Point & p, ETextAlignment alignment = ETextAlignment::BOTTOMRIGHT);
};

/// popup displayed on R-click
class CRClickPopupInt : public CRClickPopup
{
	std::shared_ptr<CIntObject> inner;

	Point dragDistance;

public:
	CRClickPopupInt(const std::shared_ptr<CIntObject> & our);
	~CRClickPopupInt();

	void mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance) override;
};

/// popup on adventure map for town\hero and other objects with customized popup content
class CInfoBoxPopup : public CWindowObject
{
	std::shared_ptr<CIntObject> tooltip;
	Point toScreen(Point pos);

	Point dragDistance;

public:
	CInfoBoxPopup(Point position, const CGTownInstance * town);
	CInfoBoxPopup(Point position, const CGHeroInstance * hero);
	CInfoBoxPopup(Point position, const CGGarrison * garr);
	CInfoBoxPopup(Point position, const CGCreature * creature);

	void mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance) override;
};

/// component selection window
class CSelWindow : public CInfoWindow
{
public:
	void madeChoice(); //looks for selected component and calls callback
	void madeChoiceAndClose();
	CSelWindow(const std::string & text, PlayerColor player, int charperline, const std::vector<std::shared_ptr<CSelectableComponent>> & comps, const std::vector<std::pair<AnimationPath,CFunctionList<void()> > > &Buttons, QueryID askID);
};

#ifndef __ADVENTUREMAPBUTTON_H__
#define __ADVENTUREMAPBUTTON_H__

#include "FunctionList.h"
#include <boost/bind.hpp>
#include "GUIBase.h"

/*
 * AdventureMapButton.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CDefEssential;

namespace config{struct ButtonInfo;}


class CButtonBase : public KeyShortcut//basic buttton class
{
public:
	struct TextOverlay
	{
		EFonts font;
		std::string text;
		int x, y;
	} *text;
	void addTextOverlay(const std::string Text, EFonts font);

	int bitmapOffset; //TODO: comment me
	int type; //advmapbutton=2 //TODO: comment me
	bool abs;//TODO: comment me
	//bool active; //if true, this button is active and can be pressed
	bool notFreeButton; //TODO: comment me
	CIntObject * ourObj; // "owner"
	int state; //TODO: comment me
	std::vector< std::vector<SDL_Surface*> > imgs; //images for this button
	int curimg; //curently displayed image from imgs
	virtual void show(SDL_Surface * to);
	virtual void showAll(SDL_Surface * to);
	//virtual void activate()=0;
	//virtual void deactivate()=0;
	CButtonBase(); //c-tor
	virtual ~CButtonBase(); //d-tor
};


class AdventureMapButton : public CButtonBase
{
public:
	std::map<int,std::string> hoverTexts; //state -> text for statusbar
	std::string helpBox; //for right-click help
	CFunctionList<void()> callback;
	bool actOnDown, //runs when mouse is pressed down over it, not when up
		hoverable; //if true, button will be highlighted when hovered
	ui8 blocked;

	void clickRight(tribool down, bool previousState);
	virtual void clickLeft(tribool down, bool previousState);
	void hover (bool on);
	void block(ui8 on); //if button is blocked then it'll change it's graphic to inactive (offset==2) and won't react on l-clicks
	//void activate(); // makes button active
	//void deactivate(); // makes button inactive (but doesn't delete)

	AdventureMapButton(); //c-tor
	AdventureMapButton( const std::map<int,std::string> &, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	AdventureMapButton( const std::pair<std::string, std::string> help, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, config::ButtonInfo *info, int key=0);//c-tor
	//AdventureMapButton( std::string Name, std::string HelpBox, boost::function<void()> Callback, int x, int y, std::string defName, bool activ=false,  std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor

	void init(const CFunctionList<void()> &Callback, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key );
	void setDef(const std::string & defName, bool playerColoredButton);
};

class CHighlightableButton 
	: public AdventureMapButton
{
public:
	CHighlightableButton(const CFunctionList<void()> &onSelect, const CFunctionList<void()> &onDeselect, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key=0);
	CHighlightableButton(const std::pair<std::string, std::string> help, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	CHighlightableButton(const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	bool onlyOn, selected;
	CFunctionList<void()> callback2; //when de-selecting
	void select(bool on);
	void clickLeft(tribool down, bool previousState);
};

class CHighlightableButtonsGroup : public CIntObject
{
public:
	CFunctionList2<void(int)> onChange; //called when changing selected button with new button's id
	std::vector<CHighlightableButton*> buttons;
	bool musicLike; //determines the behaviour of this group

	//void addButton(const std::map<int,std::string> &tooltip, const std::string &HelpBox, const std::string &defName, int x, int y, int uid);
	void addButton(CHighlightableButton* bt);//add existing button, it'll be deleted by CHighlightableButtonsGroup destructor
	void addButton(const std::map<int,std::string> &tooltip, const std::string &HelpBox, const std::string &defName, int x, int y, int uid, const CFunctionList<void()> &OnSelect=0, int key=0); //creates new button
	CHighlightableButtonsGroup(const CFunctionList2<void(int)> &OnChange, bool musicLikeButtons = false);
	~CHighlightableButtonsGroup();
	void activate();
	void deactivate();
	void select(int id, bool mode); //mode==0: id is serial; mode==1: id is unique button id
	void selectionChanged(int to);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void block(ui8 on);
};


class CSlider : public CIntObject
{
public:
	AdventureMapButton *left, *right, *slider; //if vertical then left=up
	int capacity,//how many elements can be active at same time
		amount, //how many elements
		positions, //number of highest position (0 if there is only one)
		value; //first active element
	bool horizontal;
	CDefEssential *imgs ;

	boost::function<void(int)> moved;
	//void(T::*moved)(int to);
	//T* owner;

	void redrawSlider(); 

	void sliderClicked();
	void moveLeft();
	void clickLeft(tribool down, bool previousState);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void moveRight();
	void moveTo(int to);
	void block(bool on);
	void setAmount(int to);
	//void activate(); // makes button active
	//void deactivate(); // makes button inactive (but doesn't delete)
	//void show(SDL_Surface * to);
	CSlider(int x, int y, int totalw, boost::function<void(int)> Moved, int Capacity, int Amount, 
		int Value=0, bool Horizontal=true, int style = 0); //style 0 - brown, 1 - blue
	~CSlider();
};	

#endif // __ADVENTUREMAPBUTTON_H__

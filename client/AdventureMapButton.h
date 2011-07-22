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

extern SDL_Color tytulowy, tlo, zwykly ;

class CAnimation;
class CAnimImage;
class CLabel;

namespace config{struct ButtonInfo;}

/// Base class for buttons.
class CButtonBase : public KeyShortcut
{
public:
	enum ButtonState
	{
		NORMAL=0,
		PRESSED=1,
		BLOCKED=2,
		HIGHLIGHTED=3
	};
private:
	int bitmapOffset; // base offset of visible bitmap from animation
	ButtonState state;//current state of button from enum

public:
	bool swappedImages,//fix for some buttons: normal and pressed image are swapped
		keepFrame; // don't change visual representation

	void addTextOverlay(const std::string &Text, EFonts font, SDL_Color color = zwykly);
	void update();//to refresh button after image or text change

	void setOffset(int newOffset);
	void setState(ButtonState newState);
	ButtonState getState();

	//just to make code clearer
	void block(bool on);
	bool isBlocked();
	bool isHighlighted();

	CAnimImage * image; //image for this button
	CLabel * text;//text overlay

	CButtonBase(); //c-tor
	virtual ~CButtonBase(); //d-tor
};

/// Typical Heroes 3 button which can be inactive or active and can 
/// hold further information if you right-click it
class AdventureMapButton : public CButtonBase
{
	std::vector<std::string> imageNames;//store list of images that can be used by this button
	size_t currentImage;
public:
	std::map<int, std::string> hoverTexts; //text for statusbar
	std::string helpBox; //for right-click help
	CFunctionList<void()> callback;
	bool actOnDown,//runs when mouse is pressed down over it, not when up
	     hoverable,//if true, button will be highlighted when hovered
		 borderEnabled,
		 soundDisabled;
	SDL_Color borderColor;

	void clickRight(tribool down, bool previousState);
	virtual void clickLeft(tribool down, bool previousState);
	void hover (bool on);

	AdventureMapButton(); //c-tor
	AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	AdventureMapButton( const std::pair<std::string, std::string> &help, const CFunctionList<void()> &Callback, int x, int y, const std::string &defName, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	AdventureMapButton( const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &Callback, config::ButtonInfo *info, int key=0);//c-tor

	void init(const CFunctionList<void()> &Callback, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key );

	void setIndex(size_t index, bool playerColoredButton=false);
	void setImage(CAnimation* anim, bool playerColoredButton=false);
	void setPlayerColor(int player);
	void showAll(SDL_Surface *to);
};

/// A button which can be selected/deselected
class CHighlightableButton 
	: public AdventureMapButton
{
public:
	CHighlightableButton(const CFunctionList<void()> &onSelect, const CFunctionList<void()> &onDeselect, const std::map<int,std::string> &Name, const std::string &HelpBox, bool playerColoredButton, const std::string &defName, std::vector<std::string> * add, int x, int y, int key=0);
	CHighlightableButton(const std::pair<std::string, std::string> &help, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	CHighlightableButton(const std::string &Name, const std::string &HelpBox, const CFunctionList<void()> &onSelect, int x, int y, const std::string &defName, int myid, int key=0, std::vector<std::string> * add = NULL, bool playerColoredButton = false );//c-tor
	bool onlyOn;//button can not be de-selected
	bool selected;//state of highlightable button
	int ID; //for identification
	CFunctionList<void()> callback2; //when de-selecting
	void select(bool on);
	void clickLeft(tribool down, bool previousState);
};

/// A group of buttons where one button can be selected
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
	void select(int id, bool mode); //mode==0: id is serial; mode==1: id is unique button id
	void selectionChanged(int to);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void block(ui8 on);
};

/// A typical slider which can be orientated horizontally/vertically.
class CSlider : public CIntObject
{
public:
	AdventureMapButton *left, *right, *slider; //if vertical then left=up
	int capacity,//how many elements can be active at same time
		amount, //how many elements
		positions, //number of highest position (0 if there is only one)
		value; //first active element
	bool horizontal;
	bool wheelScrolling;
	bool keyScrolling;

	boost::function<void(int)> moved;

	void redrawSlider(); 
	void sliderClicked();
	void moveLeft();
	void moveRight();
	void moveTo(int to);
	void block(bool on);
	void setAmount(int to);

	void keyPressed(const SDL_KeyboardEvent & key);
	void wheelScrolled(bool down, bool in);
	void clickLeft(tribool down, bool previousState);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void showAll(SDL_Surface * to);

	CSlider(int x, int y, int totalw, boost::function<void(int)> Moved, int Capacity, int Amount, 
		int Value=0, bool Horizontal=true, int style = 0); //style 0 - brown, 1 - blue
	~CSlider();
	void moveToMax();
};	

#endif // __ADVENTUREMAPBUTTON_H__

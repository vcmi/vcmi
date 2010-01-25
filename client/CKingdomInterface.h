#ifndef __CKINGDOMINTERFACE_H__
#define __CKINGDOMINTERFACE_H__



#include "../global.h"
#include <SDL.h>
#include "GUIBase.h"
#include "../hch/CMusicBase.h"
class AdventureMapButton;
class CHighlightableButtonsGroup;
class CResDataBar;
class CStatusBar;
class CSlider;
class CMinorResDataBar;

/*
 * CKingdomInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CKingdomInterface : public CIntObject
{
/*	class CDwellingList : public
	{
	public:
		void mouseMoved (const SDL_MouseMotionEvent & sEvent);
		void genList();
		void select(int which);
		void draw(SDL_Surface * to);
		int size(); //how many elements do we have
	}*/
	class CResIncomePic : public CIntObject
	{
	public:
		int resID,value;//resource ID
		CResIncomePic(int RID, CDefEssential * Mines);//c-tor
		~CResIncomePic();//d-tor
		void hover(bool on);
		void show(SDL_Surface * to);
		CDefEssential * mines;//pointer to mines pictures;
	};
	class CTownItem : public CIntObject
	{
	public:
		int numb;//position on screen (1..4)
		const CGTownInstance * town;
		void show(SDL_Surface * to);
		void activate();
		void deactivate();
		CTownItem (int num, const CGTownInstance * Town);//c-tor
		~CTownItem();//d-tor
	};
	class CHeroItem : public CIntObject
	{
	public:
		const CGHeroInstance * hero;
		int artGroup,numb;//current art group (0 = equiped, 1 = misc, 2 = backpack)
		void onArtChange(int newstate);//changes artgroup
		void show(SDL_Surface * to);
		void activate();
		void deactivate();
		CHeroItem (int num, const CGHeroInstance * Hero);//c-tor
		~CHeroItem();//d-tor
	};
public:
	//common data
	int state;//0 = initialisation 1 = towns showed, 2 = heroes;
	SDL_Surface * bg;//background
	CStatusBar * statusbar;//statusbar
	CResDataBar *resdatabar;//resources

	AdventureMapButton *exit;//exit button
	AdventureMapButton *toTowns;//town button
	AdventureMapButton *toHeroes;//hero button
	CDefEssential * title; //title bar
	//hero/town lists
	bool showHarrisoned;//show harrisoned hero in heroes list or not
	CSlider * slider;//slider
	int heroPos,townPos,size;//position of lists; size of list
	std::vector<CHeroItem *> heroes;//heroes list
	std::vector<CTownItem *> towns;//towns list
	static CDefEssential * slots, *fort, *hall;

	//income pics
	std::vector<CResIncomePic *> incomes;//mines + incomes
	CDefEssential * mines;//picture of mines

	CKingdomInterface(); //c-tor
	~CKingdomInterface(); //d-tor
	void recreateHeroList(int pos);//recreating heroes list (on slider move)
	void recreateTownList(int pos);//same for town list
	void keyPressed(const SDL_KeyboardEvent & key);
	void listToTowns();//changing list to town view
	void listToHeroes();//changing list to heroes view
	void sliderMoved(int newpos);//when we move a slider...
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void close();
	void activate();
	void deactivate();
};

#endif // __CCASTLEINTERFACE_H__

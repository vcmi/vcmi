#ifndef __CKINGDOMINTERFACE_H__
#define __CKINGDOMINTERFACE_H__



#include "../global.h"
#include <SDL.h>
#include "GUIBase.h"
#include "GUIClasses.h"
#include "../hch/CMusicBase.h"
class AdventureMapButton;
class CHighlightableButtonsGroup;
class CResDataBar;
class CStatusBar;
class CSlider;
class CMinorResDataBar;
class HoverableArea;
/*class LRClickableAreaWText
class LRClickableAreaWTextComp*/

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
	class CTownItem : public CWindowWithGarrison
	{
		class CCreaPlace: public LRClickableAreaWTextComp
		{
		public:
			const CGTownInstance * town;
			CCreaPlace(); //c-tor
			void clickLeft(tribool down, bool previousState);
			void clickRight(tribool down, bool previousState);
			void activate();
			void deactivate();
		};
	public:
		const CGTownInstance * town;
		CKingdomInterface * owner;
		int numb;//position on screen (1..size)
		HoverableArea *hallArea, *fortArea, *incomeArea;//hoverable text for town hall, fort, income
		LRClickableAreaOpenHero * garrHero, *visitHero;//portraits of heroes
		LRClickableAreaOpenTown * townImage;//town image
		std::vector < HoverableArea * > creaGrowth;
		std::vector < CCreaPlace * > creaCount;
		void setTown(const CGTownInstance * newTown);//change town and update info
		void showAll(SDL_Surface * to);
		void activate();
		void deactivate();
		CTownItem (int num, CKingdomInterface * Owner);//c-tor
		~CTownItem();//d-tor
	};
	class CHeroItem : public CWindowWithGarrison
	{
		class CArtPlace: public LRClickableAreaWTextComp
		{
		public:
			CHeroItem * hero;
			CArtPlace(CHeroItem * owner); //c-tor
			void clickLeft(tribool down, bool previousState);
			void clickRight(tribool down, bool previousState);
			void activate();
			void deactivate();
		};

		public:
		const CGHeroInstance * hero;
		CKingdomInterface * owner;
		int artGroup,numb;//current art group (0 = equiped, 1 = misc, 2 = backpack)
		int backpackPos;//first visible artifact in backpack
		AdventureMapButton * artLeft, * artRight;//buttons for backpack
		LRClickableAreaOpenHero * portrait;
		LRClickableAreaWText * experience;
		LRClickableAreaWTextComp * morale, * luck;
		LRClickableAreaWText * spellPoints;
		LRClickableAreaWText * speciality;
		std::vector<LRClickableAreaWTextComp *> primarySkills;
		std::vector<LRClickableAreaWTextComp *> secondarySkills;
		std::vector<LRClickableAreaWTextComp *> artifacts;
		std::vector<LRClickableAreaWTextComp *> backpack;
		CHighlightableButtonsGroup * artButtons;
		void setHero(const CGHeroInstance * newHero);//change hero and update info
		void scrollArts(int move);//moving backpack, receiving distance
		void onArtChange(int newstate);//changes artgroup
		void showAll(SDL_Surface * to);
		void activate();
		void deactivate();
		CHeroItem (int num, CKingdomInterface * Owner);//c-tor
		~CHeroItem();//d-tor
	};
public:
	//common data
	int state;//1 = towns showed, 2 = heroes;
	SDL_Surface * bg;//background
	CStatusBar * statusbar;//statusbar
	CResDataBar *resdatabar;//resources
	int size,PicCount;

	//buttons
	AdventureMapButton *exit;//exit button
	AdventureMapButton *toTowns;//town button
	AdventureMapButton *toHeroes;//hero button
	CDefEssential * title; //title bar

	//hero/town lists
	CSlider * slider;//slider
	bool showHarrisoned;//show harrisoned hero in heroes list or not, disabled by default
	int heroPos,townPos;//position of lists
	std::vector<CHeroItem *> heroes;//heroes list
	std::vector<CTownItem *> towns;//towns list
	static CDefEssential * slots, *fort, *hall;

	//objects list
	int objSize, objPos;
	CDefEssential *objPics;
	std::map<int,std::pair<int, std::string*> > objList; //dwelling ID, count, hover text
	std::vector <HoverableArea* > ObjList;//list of dwellings
	AdventureMapButton* ObjUp, *ObjDown, *ObjTop, *ObjBottom;//buttons for dwellings list

	//income pics
	std::vector<HoverableArea*> incomes;//mines + incomes
	std::vector<int> incomesVal;//values to print
	CDefEssential * mines;

	CKingdomInterface(); //c-tor
	~CKingdomInterface(); //d-tor
	void moveObjectList(int newPos);
	void recreateHeroList(int pos);//recreating heroes list (on slider move)
	void recreateTownList(int pos);//same for town list
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

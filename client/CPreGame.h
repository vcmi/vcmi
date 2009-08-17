#ifndef __CPREGAME_H__
#define __CPREGAME_H__
#include "../global.h"
#include <set>
#include <SDL.h>
#include "../StartInfo.h"
#include "CMessage.h"
#include "../lib/map.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include "GUIBase.h"

/*
 * CPreGame.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct CMusicHandler;
using boost::bind;
using boost::ref;

enum EState { //where are we?
	mainMenu, newGame, loadGame, ScenarioList, saveGame
};


class CMenuScreen : public CIntObject
{
public:
	CPicture *bgAd;
	AdventureMapButton *buttons[5];

	CMenuScreen(EState which);
	~CMenuScreen();
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void moveTo(CMenuScreen *next);
};


struct FileInfo
{
	std::string name;
	std::time_t date;
};

class InfoCard : public CIntObject
{
public:
	CPicture *bg; 
	EState type;

	CHighlightableButtonsGroup *difficulty;
	CDefHandler *sizes, *sFlags;;

	void changeSelection(const CMapInfo *to);
	void showAll(SDL_Surface * to);
	InfoCard(EState Type);
	~InfoCard();
};

class SelectionTab : public CIntObject
{
public:
	int positions; //how many entries (games/maps) can be shown
	CPicture *bg; //general bg image
	CSlider *slider;
	std::vector<CMapInfo> allItems;
	std::vector<CMapInfo*> curItems;
	size_t selectionPos;
	boost::function<void(CMapInfo *)> onSelect;

	ESortBy sortingBy;
	bool ascending;

	CDefHandler *format;

	void getFiles(std::vector<FileInfo> &out, const std::string &dirname, const std::string &ext);
	void parseMaps(std::vector<FileInfo> &files, int start = 0, int threads = 1);
	void parseGames(std::vector<FileInfo> &files);
	void filter(int size); //0 - all
	void select(int position); //position: <0 - positions>  position on the screen
	int getPosition(int x, int y); //convert mouse coords to entry position; -1 means none
	void sliderMove(int slidPos);
	void sortBy(int criteria);
	void sort();
	void printMaps(SDL_Surface *to);

	void showAll(SDL_Surface * to);
	void clickLeft(tribool down, bool previousState);
	SelectionTab(EState Type, const boost::function<void(CMapInfo *)> &OnSelect);
	~SelectionTab();
};

class OptionsTab : public CIntObject
{
public:
	enum SelType {TOWN, HERO, BONUS};
	struct SelectedBox : public CIntObject //img with current town/hero/bonus
	{
		SelType which;
		ui8 player; //serial nr
		void showAll(SDL_Surface * to);
		SelectedBox(SelType Which, ui8 Player);
	};

	struct PlayerOptionsEntry : public CIntObject
	{
		PlayerSettings &s;
		CPicture *bg;
		AdventureMapButton *btns[6]; //left and right for town, hero, bonus
		AdventureMapButton *flag;
		SelectedBox *town;
		SelectedBox *hero;
		SelectedBox *bonus;
		
		PlayerOptionsEntry(OptionsTab *owner, PlayerSettings &S);
		void showAll(SDL_Surface * to);
	};
	CPicture *bg;
	CSlider *turnDuration;

	std::set<int> usedHeroes;

	std::vector<PlayerOptionsEntry *> entries;

	void nextCastle(int player, int dir); //dir == -1 or +1
	void nextHero(int player, int dir); //dir == -1 or +1
	void nextBonus(int player, int dir); //dir == -1 or +1
	void setTurnLength(int npos);
	void flagPressed(int player);

	void changeSelection(const CMapInfo *to);
	OptionsTab(EState Type/*, StartInfo &Opts*/);
	~OptionsTab();
	void showAll(SDL_Surface * to);

	int nextAllowedHero( int min, int max, int incl, int dir );

	bool canUseThisHero( int ID );
};

class CSelectionScreen : public CIntObject
{
public:
	CPicture *bg; //general bg image
	AdventureMapButton *start, *back;
	InfoCard *card;
	SelectionTab *sel;
	OptionsTab *opt;

	EState type; //new/save/load#Game
	const CMapInfo *current;
	StartInfo sInfo;
	CIntObject *curTab;

	CSelectionScreen(EState Type);
	~CSelectionScreen();
	void toggleTab(CIntObject *tab);
	void changeSelection(const CMapInfo *to);
	void updateStartInfo(const CMapInfo * to);
	void startGame();
	void difficultyChange(int to);
};

class CGPreGame : public CIntObject
{
public:
	SDL_Surface *mainbg;
	CMenuScreen *scrs[3];

	SDL_Surface *nHero, *rHero, *nTown, *rTown; // none/random hero/town imgs
	CDefHandler *bonuses;
	CDefHandler *victory, *loss;

	CGPreGame();
	~CGPreGame();
	void run();
	void openSel(EState type);
	void loadGraphics();
	void disposeGraphics();
};

extern CGPreGame *CGP;

#endif // __CPREGAME_H__

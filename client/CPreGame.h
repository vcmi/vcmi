#ifndef __CPREGAME_H__
#define __CPREGAME_H__
#include "../global.h"
#include <set>
#include <SDL.h>
#include "../StartInfo.h"
#include "GUIBase.h"
#include "FunctionList.h"

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
class CMapHeader;
class CCampaignHeader;
class CTextInput;
class CGStatusBar;

class CMapInfo
{
public:
	CMapHeader * mapHeader; //may be NULL if campaign
	CCampaignHeader * campaignHeader; //may be NULL if scenario
	ui8 seldiff; //selected difficulty (only in saved games)
	std::string filename;
	std::string date;
	int playerAmnt, humenPlayers;
	CMapInfo(bool map = true);
	~CMapInfo();
	//CMapInfo(const std::string &fname, const unsigned char *map);
	void mapInit(const std::string &fname, const unsigned char *map);
	void campaignInit();
	void countPlayers();
};

enum ESortBy{_playerAm, _size, _format, _name, _viccon, _loscon};

class mapSorter
{
public:
	ESortBy sortBy;
	bool operator()(const CMapInfo *aaa, const CMapInfo *bbb);
	mapSorter(ESortBy es):sortBy(es){};
};

class CMenuScreen : public CIntObject
{
public:
	enum EState { //where are we?
		mainMenu, newGame, loadGame, campaignMain, ScenarioList, saveGame, scenarioInfo, campaignList
	};

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
	std::string name; // file name with full path and extension
	std::time_t date;
};

class InfoCard : public CIntObject
{
public:
	CPicture *bg; 
	CMenuScreen::EState type;

	CHighlightableButtonsGroup *difficulty;
	CDefHandler *sizes, *sFlags;;

	void changeSelection(const CMapInfo *to);
	void showAll(SDL_Surface * to);
	void clickRight(tribool down, bool previousState);
	void showTeamsPopup();
	InfoCard(CMenuScreen::EState Type);
	~InfoCard();
};

class SelectionTab : public CIntObject
{
private:
	CDefHandler *format; //map size

	void parseMaps(std::vector<FileInfo> &files, int start = 0, int threads = 1);
	void parseGames(std::vector<FileInfo> &files);
	void parseCampaigns( std::vector<FileInfo> & files );
	void getFiles(std::vector<FileInfo> &out, const std::string &dirname, const std::string &ext);
	CMenuScreen::EState tabType; 
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

	CTextInput *txt;

	
	void filter(int size, bool selectFirst = false); //0 - all
	void select(int position); //position: <0 - positions>  position on the screen
	void selectAbs(int position); //position: absolute position in curItems vector
	int getPosition(int x, int y); //convert mouse coords to entry position; -1 means none
	void sliderMove(int slidPos);
	void sortBy(int criteria);
	void sort();
	void printMaps(SDL_Surface *to);
	int getLine();
	void selectFName(const std::string &fname);

	void showAll(SDL_Surface * to);
	void clickLeft(tribool down, bool previousState);
	void wheelScrolled(bool down, bool in);
	void keyPressed(const SDL_KeyboardEvent & key);
	void onDoubleClick();
	SelectionTab(CMenuScreen::EState Type, const boost::function<void(CMapInfo *)> &OnSelect);
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

		SDL_Surface *getImg() const;
		const std::string *getText() const;

		SelectedBox(SelType Which, ui8 Player);
		void showAll(SDL_Surface * to);
		void clickRight(tribool down, bool previousState);
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
		bool fixedHero;
		
		PlayerOptionsEntry(OptionsTab *owner, PlayerSettings &S);
		void selectButtons(bool onlyHero = true); //hides unavailable buttons
		void showAll(SDL_Surface * to);
	};
	CMenuScreen::EState type;
	CPicture *bg;
	CSlider *turnDuration;

	std::set<int> usedHeroes;

	std::vector<PlayerOptionsEntry *> entries;

	void nextCastle(int player, int dir); //dir == -1 or +1
	void nextHero(int player, int dir); //dir == -1 or +1
	void nextBonus(int player, int dir); //dir == -1 or +1
	void setTurnLength(int npos);
	void flagPressed(int player);

	void changeSelection(const CMapHeader *to);
	OptionsTab(CMenuScreen::EState Type/*, StartInfo &Opts*/);
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

	CMenuScreen::EState type; //new/save/load#Game
	const CMapInfo *current;
	StartInfo sInfo;
	CIntObject *curTab;
	std::vector<std::string> playerNames;

	CSelectionScreen(CMenuScreen::EState Type, const std::vector<std::string> &PlayerNames = std::vector<std::string>());
	~CSelectionScreen();
	void toggleTab(CIntObject *tab);
	void changeSelection(const CMapInfo *to);
	void updateStartInfo(const CMapInfo * to);
	void startGame();
	void difficultyChange(int to);
};

class CScenarioInfo : public CIntObject
{
public:
	AdventureMapButton *back;
	InfoCard *card;
	OptionsTab *opt;

	CScenarioInfo(const CMapHeader *mapHeader, const StartInfo *startInfo, const CMapInfo * makeItCurrent=NULL);
	~CScenarioInfo();
};

class CMultiMode : public CIntObject
{
public:
	CPicture *bg;
	CTextInput *txt;
	AdventureMapButton *btns[7]; //0 - hotseat, 6 - cancel
	CGStatusBar *bar;

	CMultiMode();
	void openHotseat();
};

class CHotSeatPlayers : public CIntObject
{
public:
	CPicture *bg;
	CTextInput *txt[8];
	AdventureMapButton *ok, *cancel;
	CGStatusBar *bar;

	CHotSeatPlayers(const std::string &firstPlayer);
	void enterSelectionScreen();
};

class CGPreGame : public CIntObject, public IUpdateable
{
public:
	SDL_Surface *mainbg;
	CMenuScreen *scrs[4];

	SDL_Surface *nHero, *rHero, *nTown, *rTown; // none/random hero/town imgs
	CDefHandler *bonuses;
	CDefHandler *victory, *loss;

	CGPreGame();
	~CGPreGame();
	void update();
	void run();
	void openSel(CMenuScreen::EState type);
	void loadGraphics();
	void disposeGraphics();
};

extern CGPreGame *CGP;

#endif // __CPREGAME_H__

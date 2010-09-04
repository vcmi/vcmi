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
class CCampaign;
class CGStatusBar;
class CTextBox;
class CCampaignState;
class CConnection;

class CMapInfo
{
public:
	CMapHeader * mapHeader; //may be NULL if campaign
	CCampaignHeader * campaignHeader; //may be NULL if scenario
	StartInfo *scenarioOpts; //options with which scenario has been started (used only with saved games)
	std::string filename;
	bool lodCmpgn; //tells if this campaign is located in Lod file
	std::string date;
	int playerAmnt, //players in map
		humenPlayers; //players ALLOWED to be controlled by human
	int actualHumanPlayers; // >1 if multiplayer game
	CMapInfo(bool map = true);
	~CMapInfo();
	//CMapInfo(const std::string &fname, const unsigned char *map);
	void setHeader(CMapHeader *header);
	void mapInit(const std::string &fname, const unsigned char *map);
	void campaignInit();
	void countPlayers();
};

enum ESortBy{_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps}; //_numOfMaps is for campaigns

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
		mainMenu, newGame, loadGame, campaignMain, saveGame, scenarioInfo, campaignList
	};

	enum EMultiMode {
		SINGLE_PLAYER = 0, HOT_SEAT, MULTI_PLAYER
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
	bool inLod; //tells if this file is located in Lod
};

class CChatBox : public CIntObject
{
public:
	CTextBox *chatHistory;
	CTextInput *inputBox;

	CChatBox(const Rect &rect);
};

class InfoCard : public CIntObject
{
	CPicture *bg; 
public:
	CMenuScreen::EState type;

	bool chatOn;  //if chat is shown, then description is hidden
	CTextBox *mapDescription;
	CChatBox *chat;

	CHighlightableButtonsGroup *difficulty;
	CDefHandler *sizes, *sFlags;;

	void changeSelection(const CMapInfo *to);
	void showAll(SDL_Surface * to);
	void clickRight(tribool down, bool previousState);
	void showTeamsPopup();
	void toggleChat();
	void setChat(bool activateChat);
	InfoCard(CMenuScreen::EState Type, bool network = false);
	~InfoCard();
};

class SelectionTab : public CIntObject
{
private:
	CDefHandler *format; //map size

	void parseMaps(std::vector<FileInfo> &files, int start = 0, int threads = 1);
	void parseGames(std::vector<FileInfo> &files, bool multi);
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
	void keyPressed(const SDL_KeyboardEvent & key);
	void onDoubleClick();
	SelectionTab(CMenuScreen::EState Type, const boost::function<void(CMapInfo *)> &OnSelect, bool MultiPlayer=false);
	~SelectionTab();
};

class OptionsTab : public CIntObject
{
	CPicture *bg;
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
		enum {HUMAN_OR_CPU, HUMAN, CPU} whoCanPlay;
		
		PlayerOptionsEntry(OptionsTab *owner, PlayerSettings &S);
		void selectButtons(bool onlyHero = true); //hides unavailable buttons
		void showAll(SDL_Surface * to);
	};
	CMenuScreen::EState type;
	CSlider *turnDuration;

	std::set<int> usedHeroes;

	std::map<int, PlayerOptionsEntry *> entries; //indexed by color

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
	InfoCard *card;
	OptionsTab *opt;
	AdventureMapButton *start, *back;

	SelectionTab *sel;
	CMenuScreen::EState type; //new/save/load#Game
	const CMapInfo *current;
	StartInfo sInfo;
	CIntObject *curTab;
	CMenuScreen::EMultiMode multiPlayer;

	CConnection *serv; //connection to server, used in MP mode

	CSelectionScreen(CMenuScreen::EState Type, CMenuScreen::EMultiMode MultiPlayer = CMenuScreen::SINGLE_PLAYER);
	~CSelectionScreen();
	void toggleTab(CIntObject *tab);
	void changeSelection(const CMapInfo *to);
	static void updateStartInfo( const CMapInfo * to, StartInfo & sInfo, const CMapHeader * mapHeader );
	void startCampaign();
	void startGame();
	void difficultyChange(int to);

	void toggleChat();
	void handleConnection();
};

class CSavingScreen : public CSelectionScreen
{
public:
	const CMapInfo *ourGame; 


	CSavingScreen(bool hotseat = false);
	~CSavingScreen();
};

class CScenarioInfo : public CIntObject
{
public:
	AdventureMapButton *back;
	InfoCard *card;
	OptionsTab *opt;

	CScenarioInfo(const CMapHeader *mapHeader, const StartInfo *startInfo);
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
	void hostTCP();
	void joinTCP();
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

class CBonusSelection : public CIntObject
{
	SDL_Surface * background;
	AdventureMapButton * startB, * backB;

	//campaign & map descriptions:
	CTextBox * cmpgDesc, * mapDesc;

	struct SCampPositions
	{
		std::string campPrefix;
		int colorSuffixLength;

		struct SRegionDesc
		{
			std::string infix;
			int xpos, ypos;
		};

		std::vector<SRegionDesc> regions;

	};

	std::vector<SCampPositions> campDescriptions;

	class CRegion : public CIntObject
	{
		CBonusSelection * owner;
		SDL_Surface * graphics[3]; //[0] - not selected, [1] - selected, [2] - striped
		bool accessible; //false if region should be striped
		bool selectable; //true if region should be selectable
		int myNumber; //number of region
	public:
		std::string rclickText;
		CRegion(CBonusSelection * _owner, bool _accessible, bool _selectable, int _myNumber);
		~CRegion();

		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void show(SDL_Surface * to);
	};

	std::vector<CRegion *> regions;
	CRegion * highlightedRegion;

	void loadPositionsOfGraphics();
	CCampaignState * ourCampaign;
	CMapHeader *ourHeader;
	CDefHandler *sizes; //icons of map sizes
	SDL_Surface * diffPics[5]; //pictures of difficulties, user-selectable (or not if campaign locks this)
	AdventureMapButton * diffLb, * diffRb; //buttons for changing difficulty
	void changeDiff(bool increase); //if false, then decrease

	//bonus selection
	void updateBonusSelection();
	CHighlightableButtonsGroup * bonuses;

public:
	void bonusSelectionChanges(int choosenBonus);

	StartInfo sInfo;
	CDefHandler *sFlags;

	void selectMap(int whichOne);
	void selectBonus(int id);

	CBonusSelection(CCampaignState * _ourCampaign);
	~CBonusSelection();

	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);

	void goBack();
	void startMap();
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
	void openSel(CMenuScreen::EState type, CMenuScreen::EMultiMode multi = CMenuScreen::SINGLE_PLAYER);

	void resetPlayerNames();
	void loadGraphics();
	void disposeGraphics();
};

extern CGPreGame *CGP;

#endif // __CPREGAME_H__

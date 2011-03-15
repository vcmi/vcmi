#ifndef __CPREGAME_H__
#define __CPREGAME_H__
#include "../global.h"
#include <set>
#include <SDL.h>
#include "../StartInfo.h"
#include "GUIBase.h"
#include "GUIClasses.h"
#include "FunctionList.h"
#include "../lib/CMapInfo.h"

/*
 * CPreGame.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CMusicHandler;
class CMapHeader;
class CCampaignHeader;
class CTextInput;
class CCampaign;
class CGStatusBar;
class CTextBox;
class CCampaignState;
class CConnection;
struct CPackForSelectionScreen;
struct PlayerInfo;

namespace boost{ class thread; class recursive_mutex;}

enum ESortBy{_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps}; //_numOfMaps is for campaigns

/// Class which handles map sorting by different criteria
class mapSorter
{
public:
	ESortBy sortBy;
	bool operator()(const CMapInfo *aaa, const CMapInfo *bbb);
	mapSorter(ESortBy es):sortBy(es){};
};

/// The main menu screens listed in the EState enum
class CMenuScreen : public CIntObject
{
public:
	enum EState { //where are we?
		mainMenu, newGame, loadGame, campaignMain, saveGame, scenarioInfo, campaignList
	};

	enum EMultiMode {
		SINGLE_PLAYER = 0, MULTI_HOT_SEAT, MULTI_NETWORK_HOST, MULTI_NETWORK_GUEST
	};

	CPicture *bgAd;
	AdventureMapButton *buttons[5];

	CMenuScreen(EState which);
	~CMenuScreen();
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void moveTo(CMenuScreen *next);
};

/// Struct which stores name, date and a value which says if the file is located in LOD
struct FileInfo
{
	std::string name; // file name with full path and extension
	std::time_t date;
	bool inLod; //tells if this file is located in Lod
};

/// Implementation of the chat box
class CChatBox : public CIntObject
{
public:
	CTextBox *chatHistory;
	CTextInput *inputBox;

	CChatBox(const Rect &rect);

	void keyPressed(const SDL_KeyboardEvent & key);

	void addNewMessage(const std::string &text);
};

class InfoCard : public CIntObject
{
	CPicture *bg; 
public:
	CMenuScreen::EState type;

	bool network;
	bool chatOn;  //if chat is shown, then description is hidden
	CTextBox *mapDescription;
	CChatBox *chat;
	CPicture *playerListBg;

	CHighlightableButtonsGroup *difficulty;
	CDefHandler *sizes, *sFlags;;

	void changeSelection(const CMapInfo *to);
	void showAll(SDL_Surface * to);
	void clickRight(tribool down, bool previousState);
	void showTeamsPopup();
	void toggleChat();
	void setChat(bool activateChat);
	InfoCard(bool Network = false);
	~InfoCard();
};

/// The selection tab which is shown at the map selection screen
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
	SelectionTab(CMenuScreen::EState Type, const boost::function<void(CMapInfo *)> &OnSelect, CMenuScreen::EMultiMode MultiPlayer = CMenuScreen::SINGLE_PLAYER);
	~SelectionTab();
};

/// The options tab which is shown at the map selection phase.
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
		PlayerInfo &pi;
		PlayerSettings &s;
		CPicture *bg;
		AdventureMapButton *btns[6]; //left and right for town, hero, bonus
		AdventureMapButton *flag;
		SelectedBox *town;
		SelectedBox *hero;
		SelectedBox *bonus;
		enum {HUMAN_OR_CPU, HUMAN, CPU} whoCanPlay;
		
		PlayerOptionsEntry(OptionsTab *owner, PlayerSettings &S);
		void selectButtons(bool onlyHero = true); //hides unavailable buttons
		void showAll(SDL_Surface * to);
	};

	CSlider *turnDuration;

	std::set<int> usedHeroes;

	struct PlayerToRestore
	{
		int color, id; 
		void reset() { color = id = -1; }
		PlayerToRestore(){ reset(); }
	} playerToRestore;


	std::map<int, PlayerOptionsEntry *> entries; //indexed by color

	void nextCastle(int player, int dir); //dir == -1 or +1
	void nextHero(int player, int dir); //dir == -1 or +1
	void nextBonus(int player, int dir); //dir == -1 or +1
	void setTurnLength(int npos);
	void flagPressed(int player);

	void recreate();
	OptionsTab();
	~OptionsTab();
	void showAll(SDL_Surface * to);

	int nextAllowedHero( int min, int max, int incl, int dir );

	bool canUseThisHero( int ID );
};

/// Interface for selecting a map.
class ISelectionScreenInfo
{
public:
	CMenuScreen::EMultiMode multiPlayer;
	CMenuScreen::EState screenType; //new/save/load#Game
	const CMapInfo *current;
	StartInfo sInfo;
	std::map<ui32, std::string> playerNames; // id of player <-> player name; 0 is reserved as ID of AI "players"

	ISelectionScreenInfo(const std::map<ui32, std::string> *Names = NULL);
	virtual ~ISelectionScreenInfo();
	virtual void update(){};
	virtual void propagateOptions() {};
	virtual void postRequest(ui8 what, ui8 dir) {};
	virtual void postChatMessage(const std::string &txt){};

	void setPlayer(PlayerSettings &pset, unsigned player);
	void updateStartInfo( std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader );

	int getIdOfFirstUnallocatedPlayer(); //returns 0 if none
	bool isGuest() const;
	bool isHost() const;

};

/// The actual map selection screen which consists of the options and selection tab
class CSelectionScreen : public CIntObject, public ISelectionScreenInfo
{
public:
	CPicture *bg; //general bg image
	InfoCard *card;
	OptionsTab *opt;
	AdventureMapButton *start, *back;

	SelectionTab *sel;
	CIntObject *curTab;

	boost::thread *serverHandlingThread;
	boost::recursive_mutex *mx;
	std::list<CPackForSelectionScreen *> upcomingPacks; //protected by mx

	CConnection *serv; //connection to server, used in MP mode
	bool ongoingClosing;
	ui8 myNameID; //used when networking - otherwise all player are "mine"

	CSelectionScreen(CMenuScreen::EState Type, CMenuScreen::EMultiMode MultiPlayer = CMenuScreen::SINGLE_PLAYER, const std::map<ui32, std::string> *Names = NULL);
	~CSelectionScreen();
	void toggleTab(CIntObject *tab);
	void changeSelection(const CMapInfo *to);
	void startCampaign();
	void startGame();
	void difficultyChange(int to);

	void handleConnection();

	void processPacks();
	void setSInfo(const StartInfo &si);
	void update() OVERRIDE;
	void propagateOptions() OVERRIDE;
	void postRequest(ui8 what, ui8 dir) OVERRIDE;
	void postChatMessage(const std::string &txt) OVERRIDE;
	void propagateNames();
};

/// Save game screen
class CSavingScreen : public CSelectionScreen
{
public:
	const CMapInfo *ourGame; 


	CSavingScreen(bool hotseat = false);
	~CSavingScreen();
};

/// Scenario information screen shown during the game (thus not really a "pre-game" but fits here anyway)
class CScenarioInfo : public CIntObject, public ISelectionScreenInfo
{
public:
	AdventureMapButton *back;
	InfoCard *card;
	OptionsTab *opt;

	CScenarioInfo(const CMapHeader *mapHeader, const StartInfo *startInfo);
	~CScenarioInfo();
};

/// Multiplayer mode
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

/// Hot seat player window
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

/// Campaign screen where you can choose one out of three starting bonuses
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

/// Campaign selection screen
class CCampaignScreen : public CIntObject
{
	public:
		enum CampaignStatus {DEFAULT = 0, ENABLED, DISABLED, COMPLETED}; // the status of the campaign
	
	private:
		SDL_Surface *bg; // background image
		SDL_Surface *noCamp; // no campaign placeholder
		AdventureMapButton *back; // back button

		/// A button which plays a video when you move the mouse cursor over it
		class CCampaignButton : public CIntObject
		{
		private:
			std::string image;
			SDL_Surface *bg;
			SDL_Surface *button;
			SDL_Surface *checked;
			CLabel *hoverLabel; 
			CampaignStatus status;

			void clickLeft(tribool down, bool previousState);
			void hover(bool on);

		public:
			std::string campFile; // the filename/resourcename of the campaign
			std::string video; // the resource name of the video
			std::string hoverText; // the text which gets shown when you move the mouse cursor over the button

			CCampaignButton(SDL_Surface *bg, const std::string image, const int x, const int y, CampaignStatus status); // c-tor
			~CCampaignButton(); // d-tor
			void show(SDL_Surface *to);
		};

		std::vector<CCampaignButton*> campButtons; // a container which holds all buttons where you can start a campaign
		
		void drawCampaignPlaceholder(); // draws the no campaign placeholder at the lower right position
public:
	enum CampaignSet {ROE, SOD, WOG};

	CCampaignScreen(CampaignSet campaigns, std::map<std::string, CampaignStatus>& camps);
	~CCampaignScreen();
	void showAll(SDL_Surface *to);
};

/// Handles background screen, loads graphics for victory/loss condition and random town or hero selection
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
	void openSel(CMenuScreen::EState type, CMenuScreen::EMultiMode multi = CMenuScreen::SINGLE_PLAYER);
	void openCampaignScreen(CCampaignScreen::CampaignSet campaigns);

	void loadGraphics();
	void disposeGraphics();
};

extern CGPreGame *CGP;

#endif // __CPREGAME_H__

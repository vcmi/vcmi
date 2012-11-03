#pragma once

#include "../lib/Filesystem/CResourceLoader.h"
#include <SDL.h>
#include "../lib/StartInfo.h"
#include "GUIClasses.h"
#include "FunctionList.h"
#include "../lib/Map/CMapInfo.h"

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
class JsonNode;
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
	const JsonNode& config;

	CTabbedInt *tabs;

	CPicture * background;
	std::vector<CPicture*> images;

	CIntObject *createTab(size_t index);
public:
	std::vector<std::string> menuNameToEntry;

	enum EState { //where are we?
		mainMenu, newGame, loadGame, campaignMain, saveGame, scenarioInfo, campaignList
	};

	enum EMultiMode {
		SINGLE_PLAYER = 0, MULTI_HOT_SEAT, MULTI_NETWORK_HOST, MULTI_NETWORK_GUEST
	};
	CMenuScreen(const JsonNode& configNode);

	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void activate();
	void deactivate();

	void switchToTab(size_t index);
};

class CMenuEntry : public CIntObject
{
	std::vector<CPicture*> images;
	std::vector<CAdventureMapButton*> buttons;

	CAdventureMapButton* createButton(CMenuScreen* parent, const JsonNode& button);
public:
	CMenuEntry(CMenuScreen* parent, const JsonNode &config);
};

class CreditsScreen : public CIntObject
{
	CTextBox* credits;
public:
	CreditsScreen();

	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);

	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
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
public:
	CPicture *bg;
	CMenuScreen::EState type;

	bool network;
	bool chatOn;  //if chat is shown, then description is hidden
	CTextBox *mapDescription;
	CChatBox *chat;
	CPicture *playerListBg;

	CHighlightableButtonsGroup *difficulty;
	CDefHandler *sizes, *sFlags;

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

    void parseMaps(const std::vector<ResourceID> &files);
	void parseGames(const std::vector<ResourceID> &files, bool multi);
	void parseCampaigns(const std::vector<ResourceID> & files );
	std::vector<ResourceID> getFiles(std::string dirURI, int resType);
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
	void selectFName(std::string fname);

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

		size_t getBonusImageIndex() const;
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
		CAdventureMapButton *btns[6]; //left and right for town, hero, bonus
		CAdventureMapButton *flag;
		SelectedBox *town;
		SelectedBox *hero;
		SelectedBox *bonus;
		enum {HUMAN_OR_CPU, HUMAN, CPU} whoCanPlay;

		PlayerOptionsEntry(OptionsTab *owner, PlayerSettings &S);
		void selectButtons(); //hides unavailable buttons
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
	std::map<TPlayerColor, std::string> playerNames; // id of player <-> player name; 0 is reserved as ID of AI "players"

	ISelectionScreenInfo(const std::map<TPlayerColor, std::string> *Names = NULL);
	virtual ~ISelectionScreenInfo();
	virtual void update(){};
	virtual void propagateOptions() {};
	virtual void postRequest(ui8 what, ui8 dir) {};
	virtual void postChatMessage(const std::string &txt){};

	void setPlayer(PlayerSettings &pset, TPlayerColor player);
	void updateStartInfo( std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader );

	int getIdOfFirstUnallocatedPlayer(); //returns 0 if none
	bool isGuest() const;
	bool isHost() const;

};

/// The actual map selection screen which consists of the options and selection tab
class CSelectionScreen : public CIntObject, public ISelectionScreenInfo
{
	bool bordered;
public:
	CPicture *bg; //general bg image
	InfoCard *card;
	OptionsTab *opt;
	CAdventureMapButton *start, *back;

	SelectionTab *sel;
	CIntObject *curTab;

	boost::thread *serverHandlingThread;
	boost::recursive_mutex *mx;
	std::list<CPackForSelectionScreen *> upcomingPacks; //protected by mx

	CConnection *serv; //connection to server, used in MP mode
	bool ongoingClosing;
	ui8 myNameID; //used when networking - otherwise all player are "mine"

	CSelectionScreen(CMenuScreen::EState Type, CMenuScreen::EMultiMode MultiPlayer = CMenuScreen::SINGLE_PLAYER, const std::map<TPlayerColor, std::string> *Names = NULL);
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
	void showAll(SDL_Surface *to);
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
	CAdventureMapButton *back;
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
	CAdventureMapButton *btns[7]; //0 - hotseat, 6 - cancel
	CGStatusBar *bar;

	CMultiMode();
	void openHotseat();
	void hostTCP();
	void joinTCP();
};

/// Hot seat player window
class CHotSeatPlayers : public CIntObject
{
	CPicture *bg;
	CTextBox *title;
	CTextInput* txt[8];
	CAdventureMapButton *ok, *cancel;
	CGStatusBar *bar;

	void onChange(std::string newText);
	void enterSelectionScreen();

public:
	CHotSeatPlayers(const std::string &firstPlayer);
};

/// Campaign screen where you can choose one out of three starting bonuses
class CBonusSelection : public CIntObject
{
	SDL_Surface * background;
	CAdventureMapButton * startB, * backB;

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
		SDL_Surface* graphics[3]; //[0] - not selected, [1] - selected, [2] - striped
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
	shared_ptr<CCampaignState> ourCampaign;
	CMapHeader *ourHeader;
	CDefHandler *sizes; //icons of map sizes
	SDL_Surface* diffPics[5]; //pictures of difficulties, user-selectable (or not if campaign locks this)
	CAdventureMapButton * diffLb, * diffRb; //buttons for changing difficulty
	void changeDiff(bool increase); //if false, then decrease


	void updateStartButtonState(int selected = -1); //-1 -- no bonus is selected
	//bonus selection
	void updateBonusSelection();
	CHighlightableButtonsGroup * bonuses;

public:
	StartInfo sInfo;
	CDefHandler *sFlags;

	void selectMap(int whichOne);
	void selectBonus(int id);
	void init();

	CBonusSelection(std::string campaignFName);
	CBonusSelection(shared_ptr<CCampaignState> _ourCampaign);
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
	/// A button which plays a video when you move the mouse cursor over it
	class CCampaignButton : public CIntObject
	{
	private:
		CPicture *image;
		CPicture *checkMark;

		CLabel *hoverLabel;
		CampaignStatus status;

		std::string campFile; // the filename/resourcename of the campaign
		std::string video; // the resource name of the video
		std::string hoverText;

		void clickLeft(tribool down, bool previousState);
		void hover(bool on);

	public:
		CCampaignButton(const JsonNode &config );
		void show(SDL_Surface * to);
	};

	CAdventureMapButton *back;
	std::vector<CCampaignButton*> campButtons;
	std::vector<CPicture*> images;

	CAdventureMapButton* createExitButton(const JsonNode& button);

public:
	enum CampaignSet {ROE, AB, SOD, WOG};

	CCampaignScreen(const JsonNode &config);
	void showAll(SDL_Surface *to);
};

/// Handles background screen, loads graphics for victory/loss condition and random town or hero selection
class CGPreGame : public CIntObject, public IUpdateable
{
	void loadGraphics();
	void disposeGraphics();

	CGPreGame(); //Use createIfNotPresent

public:
	const JsonNode * const pregameConfig;

	CMenuScreen* menu;

	SDL_Surface *nHero, *rHero, *nTown, *rTown; // none/random hero/town imgs
	CDefHandler *bonuses;
	CDefHandler *victory, *loss;

	~CGPreGame();
	void update();
	void openSel(CMenuScreen::EState type, CMenuScreen::EMultiMode multi = CMenuScreen::SINGLE_PLAYER);

	void openCampaignScreen(std::string name);

	static CGPreGame * create();
	void removeFromGui();
};

extern ISelectionScreenInfo *SEL;
extern CGPreGame *CGP;

#pragma once

#include "../lib/filesystem/Filesystem.h"
#include <SDL.h>
#include "../lib/StartInfo.h"
#include "GUIClasses.h"
#include "../lib/FunctionList.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/rmg/CMapGenerator.h"

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
class CMapGenOptions;
class CRandomMapTab;
struct CPackForSelectionScreen;
struct PlayerInfo;

namespace boost{ class thread; class recursive_mutex;}

enum ESortBy{_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps, _fileName}; //_numOfMaps is for campaigns

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
	int positionCounter;
	CMultiLineLabel* credits;
public:
	CreditsScreen();

	void show(SDL_Surface * to);

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

    void parseMaps(const std::unordered_set<ResourceID> &files);
	void parseGames(const std::unordered_set<ResourceID> &files, bool multi);
	void parseCampaigns(const std::unordered_set<ResourceID> & files );
	std::unordered_set<ResourceID> getFiles(std::string dirURI, int resType);
	CMenuScreen::EState tabType;
public:
	int positions; //how many entries (games/maps) can be shown
	CPicture *bg; //general bg image
	CSlider *slider;
	std::vector<CMapInfo> allItems;
	std::vector<CMapInfo*> curItems;
	size_t selectionPos;
	std::function<void(CMapInfo *)> onSelect;

	ESortBy sortingBy;
	ESortBy generalSortingBy;
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
	const CMapInfo * getSelectedMapInfo() const;

	void showAll(SDL_Surface * to);
	void clickLeft(tribool down, bool previousState);
	void keyPressed(const SDL_KeyboardEvent & key);
	void onDoubleClick();
	SelectionTab(CMenuScreen::EState Type, const std::function<void(CMapInfo *)> &OnSelect, CMenuScreen::EMultiMode MultiPlayer = CMenuScreen::SINGLE_PLAYER);
    ~SelectionTab();
};

/// The options tab which is shown at the map selection phase.
class OptionsTab : public CIntObject
{
	CPicture *bg;
public:
	enum SelType {TOWN, HERO, BONUS};

	struct CPlayerSettingsHelper
	{
		const PlayerSettings & settings;
		const SelType type;

		CPlayerSettingsHelper(const PlayerSettings & settings, SelType type):
		    settings(settings),
		    type(type)
		{}

		/// visible image settings
		size_t getImageIndex();
		std::string getImageName();

		std::string getName();       /// name visible in options dialog
		std::string getTitle();      /// title in popup box
		std::string getSubtitle();   /// popup box subtitle
		std::string getDescription();/// popup box description, not always present
	};

	class CPregameTooltipBox : public CWindowObject, public CPlayerSettingsHelper
	{
		void genHeader();
		void genTownWindow();
		void genHeroWindow();
		void genBonusWindow();
	public:
		CPregameTooltipBox(CPlayerSettingsHelper & helper);
	};

	struct SelectedBox : public CIntObject, public CPlayerSettingsHelper //img with current town/hero/bonus
	{
		CAnimImage * image;
		CLabel *subtitle;

		SelectedBox(Point position, PlayerSettings & settings, SelType type);
		void clickRight(tribool down, bool previousState);

		void update();
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
		void update();
	};

	CSlider *turnDuration;

	std::set<int> usedHeroes;

	struct PlayerToRestore
	{
		PlayerColor color;
		int id;
		void reset() { id = -1; color = PlayerColor::CANNOT_DETERMINE; }
		PlayerToRestore(){ reset(); }
	} playerToRestore;


	std::map<PlayerColor, PlayerOptionsEntry *> entries; //indexed by color

	void nextCastle(PlayerColor player, int dir); //dir == -1 or +1
	void nextHero(PlayerColor player, int dir); //dir == -1 or +1
	void nextBonus(PlayerColor player, int dir); //dir == -1 or +1
	void setTurnLength(int npos);
	void flagPressed(PlayerColor player);

	void recreate();
	OptionsTab();
	~OptionsTab();
	void showAll(SDL_Surface * to);

	int nextAllowedHero(PlayerColor player, int min, int max, int incl, int dir );

	bool canUseThisHero(PlayerColor player, int ID );
};

/// The random map tab shows options for generating a random map.
class CRandomMapTab : public CIntObject
{
public:
	CRandomMapTab();

    void showAll(SDL_Surface * to);
	void updateMapInfo();
	CFunctionList<void (const CMapInfo *)> & getMapInfoChanged();
	const CMapInfo * getMapInfo() const;
	const CMapGenOptions & getMapGenOptions() const;

private:
    void addButtonsToGroup(CHighlightableButtonsGroup * group, const std::vector<std::string> & defs, int startIndex, int endIndex, int btnWidth, int helpStartIndex) const;
    void addButtonsWithRandToGroup(CHighlightableButtonsGroup * group, const std::vector<std::string> & defs, int startIndex, int endIndex, int btnWidth, int helpStartIndex, int helpRandIndex) const;
    void deactivateButtonsFrom(CHighlightableButtonsGroup * group, int startId);
    void validatePlayersCnt(int playersCnt);
    void validateCompOnlyPlayersCnt(int compOnlyPlayersCnt);

    CPicture * bg;
	CHighlightableButton * twoLevelsBtn;
	CHighlightableButtonsGroup * mapSizeBtnGroup, * playersCntGroup, * teamsCntGroup, * compOnlyPlayersCntGroup,
		* compOnlyTeamsCntGroup, * waterContentGroup, * monsterStrengthGroup;
    CAdventureMapButton * showRandMaps;
	CMapGenOptions mapGenOptions;
	unique_ptr<CMapInfo> mapInfo;
	CFunctionList<void(const CMapInfo *)> mapInfoChanged;
};

/// Interface for selecting a map.
class ISelectionScreenInfo
{
public:
	CMenuScreen::EMultiMode multiPlayer;
	CMenuScreen::EState screenType; //new/save/load#Game
	const CMapInfo *current;
	StartInfo sInfo;
	std::map<ui8, std::string> playerNames; // id of player <-> player name; 0 is reserved as ID of AI "players"

	ISelectionScreenInfo(const std::map<ui8, std::string> *Names = nullptr);
	virtual ~ISelectionScreenInfo();
	virtual void update(){};
	virtual void propagateOptions() {};
	virtual void postRequest(ui8 what, ui8 dir) {};
	virtual void postChatMessage(const std::string &txt){};

	void setPlayer(PlayerSettings &pset, ui8 player);
	void updateStartInfo( std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader );

	ui8 getIdOfFirstUnallocatedPlayer(); //returns 0 if none
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
	CRandomMapTab * randMapTab;
	CAdventureMapButton *start, *back;

	SelectionTab *sel;
	CIntObject *curTab;

	boost::thread *serverHandlingThread;
	boost::recursive_mutex *mx;
	std::list<CPackForSelectionScreen *> upcomingPacks; //protected by mx

	CConnection *serv; //connection to server, used in MP mode
	bool ongoingClosing;
	ui8 myNameID; //used when networking - otherwise all player are "mine"

	CSelectionScreen(CMenuScreen::EState Type, CMenuScreen::EMultiMode MultiPlayer = CMenuScreen::SINGLE_PLAYER, const std::map<ui8, std::string> * Names = nullptr, const std::string & Address = "", const std::string & Port = "");
	~CSelectionScreen();
	void toggleTab(CIntObject *tab);
	void changeSelection(const CMapInfo *to);
	void startCampaign();
	void startScenario();
	void difficultyChange(int to);

	void handleConnection();

	void processPacks();
	void setSInfo(const StartInfo &si);
	void update() override;
	void propagateOptions() override;
	void postRequest(ui8 what, ui8 dir) override;
	void postChatMessage(const std::string &txt) override;
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


class CPrologEpilogVideo : public CWindowObject
{
	CCampaignScenario::SScenarioPrologEpilog spe;
	int positionCounter;
	int voiceSoundHandle;
	std::function<void()> exitCb;

	CMultiLineLabel * text;
public:
	CPrologEpilogVideo(CCampaignScenario::SScenarioPrologEpilog _spe, std::function<void()> callback);

	void clickLeft(tribool down, bool previousState);
	void show(SDL_Surface * to);
};

/// Campaign screen where you can choose one out of three starting bonuses
class CBonusSelection : public CIntObject
{
public:
	CBonusSelection(const std::string & campaignFName);
	CBonusSelection(shared_ptr<CCampaignState> _ourCampaign);
	~CBonusSelection();

	void showAll(SDL_Surface * to) override;
	void show(SDL_Surface * to) override;

private:
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

	void init();
	void loadPositionsOfGraphics();
	void updateStartButtonState(int selected = -1); //-1 -- no bonus is selected
	void updateBonusSelection();
	void updateCampaignState();

	// Event handlers
	void goBack();
	void startMap();
	void restartMap();
	void selectMap(int mapNr, bool initialSelect);
	void selectBonus(int id);
	void increaseDifficulty();
	void decreaseDifficulty();

	// GUI components
	SDL_Surface * background;
	CAdventureMapButton * startB, * restartB, * backB;
	CTextBox * campaignDescription, * mapDescription;
	std::vector<SCampPositions> campDescriptions;
	std::vector<CRegion *> regions;
	CRegion * highlightedRegion;
	CHighlightableButtonsGroup * bonuses;
	SDL_Surface * diffPics[5]; //pictures of difficulties, user-selectable (or not if campaign locks this)
	CAdventureMapButton * diffLb, * diffRb; //buttons for changing difficulty
	CDefHandler * sizes; //icons of map sizes
	CDefHandler * sFlags;

	// Data
	shared_ptr<CCampaignState> ourCampaign;
	int selectedMap;
	boost::optional<int> selectedBonus;
	StartInfo startInfo;
	CMapHeader * ourHeader;
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

/// Manages the configuration of pregame GUI elements like campaign screen, main menu, loading screen,...
class CGPreGameConfig
{
public:
	static CGPreGameConfig & get();
	const JsonNode & getConfig() const;
	const JsonNode & getCampaigns() const;

private:
	CGPreGameConfig();

	const JsonNode campaignSets;
	const JsonNode config;
};

/// Handles background screen, loads graphics for victory/loss condition and random town or hero selection
class CGPreGame : public CIntObject, public ILockedUpdatable
{
	void loadGraphics();
	void disposeGraphics();

	CGPreGame(); //Use createIfNotPresent

public:
	CMenuScreen * menu;

	CDefHandler *victory, *loss;

	~CGPreGame();
	void update() override;
	void runLocked(std::function<void(IUpdateable * )> cb) override;
	void openSel(CMenuScreen::EState type, CMenuScreen::EMultiMode multi = CMenuScreen::SINGLE_PLAYER);

	void openCampaignScreen(std::string name);

	static CGPreGame * create();
	void removeFromGui();
	static void showLoadingScreen(std::function<void()> loader);
};

class CLoadingScreen : public CWindowObject
{
	boost::thread loadingThread;

	std::string getBackground();
public:
	CLoadingScreen(std::function<void()> loader);
	~CLoadingScreen();

	void showAll(SDL_Surface *to);
};

/// Simple window to enter the server's address.
class CSimpleJoinScreen : public CIntObject
{
	CPicture * bg;
	CTextBox * title;
	CAdventureMapButton * ok, * cancel;
	CGStatusBar * bar;
	CTextInput * address;
	CTextInput * port;

	void enterSelectionScreen();
	void onChange(const std::string & newText);
public:
	CSimpleJoinScreen();
};

extern ISelectionScreenInfo *SEL;
extern CGPreGame *CGP;

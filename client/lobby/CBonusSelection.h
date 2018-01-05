/*
 * CBonusSelection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../gui/CIntObject.h"

class SDL_Surface;
class CCampaignState;
class CButton;
class CTextBox;
class CToggleGroup;
class CAnimImage;
class CMapHeader;
struct StartInfo;

/// Campaign screen where you can choose one out of three starting bonuses
class CBonusSelection : public CIntObject
{
public:
	std::shared_ptr<CCampaignState> getCampaign();
	CBonusSelection();
	~CBonusSelection();

	void showAll(SDL_Surface * to) override;
	void show(SDL_Surface * to) override;

//private: MPTODO: for now all public
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
		SDL_Surface * graphics[3]; //[0] - not selected, [1] - selected, [2] - striped
		bool accessible; //false if region should be striped
		bool selectable; //true if region should be selectable
		int myNumber; //number of region

	public:
		std::string rclickText;

		CRegion(CBonusSelection * _owner, bool _accessible, bool _selectable, int _myNumber);
		~CRegion();
		void show(SDL_Surface * to) override;
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
	};

	void loadPositionsOfGraphics();
	void updateStartButtonState(int selected = -1); //-1 -- no bonus is selected
	void updateBonusSelection();
	void updateAfterStateChange();

	// Event handlers
	void goBack();
	void startMap();
	void restartMap();
	void selectMap(int mapNr);
	void selectBonus(int id);
	void increaseDifficulty();
	void decreaseDifficulty();

	// GUI components
	SDL_Surface * background;
	CButton * buttonStart;
	CButton * buttonRestart;
	CButton * buttonBack;
	CTextBox * campaignDescription, * mapDescription;
	std::vector<SCampPositions> campDescriptions;
	std::vector<CRegion *> regions;
	CRegion * highlightedRegion;
	CToggleGroup * bonuses;
	std::array<CAnimImage *, 5> difficultyIcons;
	CButton * buttonDifficultyLeft;
	CButton * buttonDifficultyRight;
	CAnimImage * mapSizeIcons;
	std::shared_ptr<CAnimation> sFlags;
};

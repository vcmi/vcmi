/*
 * OptionsTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMap.h"

class CSlider;
class CLabel;
class CMultiLineLabel;
class CFilledTexture;
class CAnimImage;
class CComponentBox;
/// The options tab which is shown at the map selection phase.
class OptionsTab : public View
{
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CMultiLineLabel> labelSubTitle;
	std::shared_ptr<CMultiLineLabel> labelPlayerNameAndHandicap;
	std::shared_ptr<CMultiLineLabel> labelStartingTown;
	std::shared_ptr<CMultiLineLabel> labelStartingHero;
	std::shared_ptr<CMultiLineLabel> labelStartingBonus;

	std::shared_ptr<CLabel> labelPlayerTurnDuration;
	std::shared_ptr<CLabel> labelTurnDurationValue;

public:
	enum SelType
	{
		TOWN,
		HERO,
		BONUS
	};

	struct CPlayerSettingsHelper
	{
		const PlayerSettings & settings;
		const SelType type;

		CPlayerSettingsHelper(const PlayerSettings & settings, SelType type)
			: settings(settings), type(type)
		{}

		/// visible image settings
		size_t getImageIndex();
		std::string getImageName();

		std::string getName(); /// name visible in options dialog
		std::string getTitle(); /// title in popup box
		std::string getSubtitle(); /// popup box subtitle
		std::string getDescription(); /// popup box description, not always present
	};

	class CPlayerOptionTooltipBox : public CWindowObject, public CPlayerSettingsHelper
	{
		std::shared_ptr<CFilledTexture> backgroundTexture;
		std::shared_ptr<CLabel> labelTitle;
		std::shared_ptr<CLabel> labelSubTitle;
		std::shared_ptr<CAnimImage> image;

		std::shared_ptr<CLabel> labelAssociatedCreatures;
		std::shared_ptr<CComponentBox> boxAssociatedCreatures;

		std::shared_ptr<CLabel> labelHeroSpeciality;
		std::shared_ptr<CAnimImage> imageSpeciality;
		std::shared_ptr<CLabel> labelSpecialityName;

		std::shared_ptr<CTextBox> textBonusDescription;

		void genHeader();
		void genTownWindow();
		void genHeroWindow();
		void genBonusWindow();

	public:
		CPlayerOptionTooltipBox(CPlayerSettingsHelper & helper);
	};

	/// Image with current town/hero/bonus
	struct SelectedBox : public View, public CPlayerSettingsHelper
	{
		std::shared_ptr<CAnimImage> image;
		std::shared_ptr<CLabel> subtitle;

		SelectedBox(Point position, PlayerSettings & settings, SelType type);
		void clickRight(const SDL_Event &event, tribool down) override;

		void update();
	};

	struct PlayerOptionsEntry : public View
	{
		PlayerInfo pi;
		PlayerSettings s;
		std::shared_ptr<CLabel> labelPlayerName;
		std::shared_ptr<CMultiLineLabel> labelWhoCanPlay;
		std::shared_ptr<CPicture> background;
		std::shared_ptr<CButton> buttonTownLeft;
		std::shared_ptr<CButton> buttonTownRight;
		std::shared_ptr<CButton> buttonHeroLeft;
		std::shared_ptr<CButton> buttonHeroRight;
		std::shared_ptr<CButton> buttonBonusLeft;
		std::shared_ptr<CButton> buttonBonusRight;
		std::shared_ptr<CButton> flag;
		std::shared_ptr<SelectedBox> town;
		std::shared_ptr<SelectedBox> hero;
		std::shared_ptr<SelectedBox> bonus;
		enum {HUMAN_OR_CPU, HUMAN, CPU} whoCanPlay;

		PlayerOptionsEntry(const PlayerSettings & S);
		void hideUnavailableButtons();
	};

	std::shared_ptr<CSlider> sliderTurnDuration;
	std::map<PlayerColor, std::shared_ptr<PlayerOptionsEntry>> entries;

	OptionsTab();
	void recreate();
};

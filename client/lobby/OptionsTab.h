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

#include "../windows/CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN
struct PlayerSettings;
struct PlayerInfo;
VCMI_LIB_NAMESPACE_END

#include "../widgets/Scrollable.h"
#include "../../lib/mapping/CMapHeader.h"

class CSlider;
class CLabel;
class CMultiLineLabel;
class CFilledTexture;
class CAnimImage;
class CComponentBox;
class CTextBox;
class CButton;

/// The options tab which is shown at the map selection phase.
class OptionsTab : public CIntObject
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
	ui8 humanPlayers;

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
		size_t getImageIndex(bool big = false);
		std::string getImageName(bool big = false);

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

	class SelectionWindow : public CWindowObject
	{
		const int ELEMENTS_PER_LINE = 5;

		std::shared_ptr<CFilledTexture> backgroundTexture;
		std::vector<std::shared_ptr<CIntObject>> components;

		std::vector<FactionID> factions;
		std::vector<HeroTypeID> heroes;

		void genContentCastles(PlayerSettings settings, PlayerInfo playerInfo);
		void genContentHeroes(PlayerSettings settings, PlayerInfo playerInfo);

		void apply();
		FactionID getElementCastle(const Point & cursorPosition);
		SHeroName getElementHero(const Point & cursorPosition);
		int getElementBonus(const Point & cursorPosition);
		Point getElement(const Point & cursorPosition);

		void clickReleased(const Point & cursorPosition) override;
		void showPopupWindow(const Point & cursorPosition) override;

	public:
		SelectionWindow(PlayerSettings settings, PlayerInfo playerInfo);
	};

	/// Image with current town/hero/bonus
	struct SelectedBox : public Scrollable, public CPlayerSettingsHelper
	{
		const PlayerInfo & playerInfo;

		std::shared_ptr<CAnimImage> image;
		std::shared_ptr<CLabel> subtitle;

		SelectedBox(Point position, PlayerSettings & settings, PlayerInfo & playerInfo, SelType type);
		void showPopupWindow(const Point & cursorPosition) override;
		void clickReleased(const Point & cursorPosition) override;
		void scrollBy(int distance) override;

		void update();
	};

	struct PlayerOptionsEntry : public CIntObject
	{
		std::unique_ptr<PlayerInfo> pi;
		std::unique_ptr<PlayerSettings> s;
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

		PlayerOptionsEntry(const PlayerSettings & S, const OptionsTab & parentTab);
		void hideUnavailableButtons();

	private:
		const OptionsTab & parentTab;
	};

	std::shared_ptr<CSlider> sliderTurnDuration;
	std::map<PlayerColor, std::shared_ptr<PlayerOptionsEntry>> entries;

	OptionsTab();
	void recreate();
	void onSetPlayerClicked(const PlayerSettings & ps) const;
};

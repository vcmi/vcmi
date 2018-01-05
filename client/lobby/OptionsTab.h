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
#include "CSelectionBase.h"

/// The options tab which is shown at the map selection phase.
class OptionsTab : public CIntObject
{
	CPicture * bg;

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
		void genHeader();
		void genTownWindow();
		void genHeroWindow();
		void genBonusWindow();

	public:
		CPlayerOptionTooltipBox(CPlayerSettingsHelper & helper);
	};

	struct SelectedBox : public CIntObject, public CPlayerSettingsHelper //img with current town/hero/bonus
	{
		CAnimImage * image;
		CLabel * subtitle;

		SelectedBox(Point position, PlayerSettings & settings, SelType type);
		void clickRight(tribool down, bool previousState) override;

		void update();
	};

	struct PlayerOptionsEntry : public CIntObject
	{
		PlayerInfo pi;
		PlayerSettings s;
		CPicture * bg;
		CButton * btns[6]; //left and right for town, hero, bonus
		CButton * flag;
		SelectedBox * town;
		SelectedBox * hero;
		SelectedBox * bonus;
		enum {HUMAN_OR_CPU, HUMAN, CPU} whoCanPlay;

		PlayerOptionsEntry(OptionsTab * owner, const PlayerSettings & S);
		void showAll(SDL_Surface * to) override;
		void hideUnavailableButtons();
	};

	CSlider * turnDuration;
	std::map<PlayerColor, PlayerOptionsEntry *> entries; //indexed by color

	OptionsTab();

	void showAll(SDL_Surface * to) override;
	void recreate();
};

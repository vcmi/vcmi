/*
 * GlobalLobbyServerSetup.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class CLabel;
class CTextBox;
class FilledTexturePlayerColored;
class CButton;
class CToggleGroup;

class GlobalLobbyServerSetup : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::shared_ptr<CLabel> labelTitle;

	std::shared_ptr<CLabel> labelPlayerLimit;
	std::shared_ptr<CLabel> labelRoomType;
	std::shared_ptr<CLabel> labelGameMode;

	std::shared_ptr<CToggleGroup> togglePlayerLimit; // 2-8
	std::shared_ptr<CToggleGroup> toggleRoomType; // public or private
	std::shared_ptr<CToggleGroup> toggleGameMode; // new game or load game

	std::shared_ptr<CTextBox> labelDescription;
	std::shared_ptr<CTextBox> labelStatus;

	std::shared_ptr<CButton> buttonCreate;
	std::shared_ptr<CButton> buttonClose;

	void updateDescription();
	void onPlayerLimitChanged(int value);
	void onRoomTypeChanged(int value);
	void onGameModeChanged(int value);

	void onCreate();
	void onClose();

public:
	GlobalLobbyServerSetup();
};

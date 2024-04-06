/*
 * GlobalLobbyLoginWindow.h, part of VCMI engine
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
class CTextInput;
class CToggleGroup;
class FilledTexturePlayerColored;
class TransparentFilledRectangle;
class CButton;

class GlobalLobbyLoginWindow : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CLabel> labelUsernameTitle;
	std::shared_ptr<CLabel> labelUsername;
	std::shared_ptr<CTextBox> labelStatus;
	std::shared_ptr<TransparentFilledRectangle> backgroundUsername;
	std::shared_ptr<CTextInput> inputUsername;

	std::shared_ptr<CButton> buttonLogin;
	std::shared_ptr<CButton> buttonClose;
	std::shared_ptr<CToggleGroup> toggleMode; // create account or use existing

	void onLoginModeChanged(int value);
	void onClose();
	void onLogin();

public:
	GlobalLobbyLoginWindow();

	void onConnectionSuccess();
	void onLoginSuccess();
	void onConnectionFailed(const std::string & reason);
};

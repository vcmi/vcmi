/*
 * GlobalLobbyInviteWindow.h, part of VCMI engine
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
class FilledTexturePlayerColored;
class TransparentFilledRectangle;
class CListBox;
class CButton;
struct GlobalLobbyAccount;

class GlobalLobbyInviteWindow : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CListBox> accountList;
	std::shared_ptr<TransparentFilledRectangle> listBackground;
	std::shared_ptr<CButton> buttonClose;

public:
	GlobalLobbyInviteWindow();
};

class GlobalLobbyInviteAccountCard : public CIntObject
{
	std::string accountID;

	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelStatus;
	std::shared_ptr<CLabel> labelInviteStatus;
	std::shared_ptr<CButton> buttonInvite;

	void clickPressed(const Point & cursorPosition) override;
public:
	GlobalLobbyInviteAccountCard(const GlobalLobbyAccount & accountDescription);
};

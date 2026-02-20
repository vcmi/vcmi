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

#include "GlobalLobbyObserver.h"

#include "../windows/CWindowObject.h"

class CLabel;
class FilledTexturePlayerColored;
class TransparentFilledRectangle;
class CListBox;
class CButton;
struct GlobalLobbyAccount;

class GlobalLobbyAddChannelWindow final : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CListBox> languageList;
	std::shared_ptr<TransparentFilledRectangle> listBackground;
	std::shared_ptr<CButton> buttonClose;

public:
	GlobalLobbyAddChannelWindow();
};

class GlobalLobbyAddChannelWindowCard : public CIntObject
{
	std::string languageID;

	std::shared_ptr<TransparentFilledRectangle> backgroundOverlay;
	std::shared_ptr<CLabel> labelNameNative;
	std::shared_ptr<CLabel> labelNameTranslated;

	void clickPressed(const Point & cursorPosition) override;
public:
	GlobalLobbyAddChannelWindowCard(const std::string & languageID);
};

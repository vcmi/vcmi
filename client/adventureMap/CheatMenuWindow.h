/*
 * CheatMenuWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class CFilledTexture;
class CButton;
class CLabel;

/// Lists existing chat-based cheat codes as clickable buttons
class CheatMenuWindow : public CWindowObject
{
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CButton> buttonClose;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CButton>> buttons;

	void castCheat(const std::string & cheatCode);

public:
	CheatMenuWindow();
};

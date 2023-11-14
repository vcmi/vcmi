/*
 * CTutorialWindow.h, part of VCMI engine
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

enum TutorialMode
{
    TOUCH_ADVENTUREMAP,
    TOUCH_BATTLE
};

class CTutorialWindow : public CWindowObject
{
    TutorialMode mode;
    std::shared_ptr<CFilledTexture> background;

public:
    CTutorialWindow(const TutorialMode & m);
    static void openWindowFirstTime(const TutorialMode & m);
    
    
    void clickPressed(const Point & cursorPosition) override;
};
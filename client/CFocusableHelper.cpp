/*
 * CFocusableHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "CFocusableHelper.h"
#include "../Global.h"
#include "widgets/TextControls.h"

void removeFocusFromActiveInput()
{
    if(CFocusable::inputWithFocus == nullptr)
        return;
    CFocusable::inputWithFocus->focus = false;
    CFocusable::inputWithFocus->redraw();
    CFocusable::inputWithFocus = nullptr;
}

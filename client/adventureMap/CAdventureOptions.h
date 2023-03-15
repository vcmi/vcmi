/*
 * CAdventureOptions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class CButton;

/// Adventure options dialog where you can view the world, dig, play the replay of the last turn,...
class CAdventureOptions : public CWindowObject
{
	std::shared_ptr<CButton> exit;
	std::shared_ptr<CButton> viewWorld;
	std::shared_ptr<CButton> puzzle;
	std::shared_ptr<CButton> dig;
	std::shared_ptr<CButton> scenInfo;
	/*std::shared_ptr<CButton> replay*/

public:
	CAdventureOptions();

	static void showScenarioInfo();
};


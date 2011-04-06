#pragma once
#include "../global.h"
#include "ERMParser.h"

/*
 * ERMInterpreter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class ERMInterpreter
{
	std::map<std::string, std::vector<ERM::TLine> > scripts;
public:
	void scanForScripts();
	enum EPrintMode{ALL, ERM_ONLY, VERM_ONLY};
	void printScripts(EPrintMode mode = ALL);
	void startExecution();
};
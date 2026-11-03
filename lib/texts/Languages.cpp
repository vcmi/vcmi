/*
 * Languages.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Languages.h"

#include "CConfigHandler.h"

namespace Languages
{

std::string getPreferredLanguage()
{
	assert(!settings["general"]["language"].String().empty());
	return settings["general"]["language"].String();
}

}

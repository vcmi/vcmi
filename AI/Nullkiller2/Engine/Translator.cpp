/*
* Translator.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Translator.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

namespace NK2AI
{

const std::string & Translator::translateString(const TextIdentifier & identifier) const
{
	return LIBRARY->generaltexth->translateString(identifier);
}

}

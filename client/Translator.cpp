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

#include "../lib/GameLibrary.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/texts/TextLocalizationContainer.h"

void Translator::install(const TextLocalizationContainer & source)
{
	if(!vstd::contains(overlays, &source))
		overlays.push_back(&source);
}

void Translator::uninstall(const TextLocalizationContainer & source)
{
	vstd::erase(overlays, &source);
}

void Translator::uninstallAll()
{
	overlays.clear();
}

const std::string & Translator::translateString(const TextIdentifier & identifier) const
{
	for(auto overlay = overlays.rbegin(); overlay != overlays.rend(); ++overlay)
		if((*overlay)->identifierExists(identifier))
			return (*overlay)->translateString(identifier);

	return LIBRARY->generaltexth->translateString(identifier);
}

/*
 * CompositeTranslator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CompositeTranslator.h"

#include "../GameLibrary.h"
#include "CGeneralTextHandler.h"
#include "TextLocalizationContainer.h"

void CompositeTranslator::install(const std::shared_ptr<const TextLocalizationContainer> & source)
{
	overlays.push_back(source);
}

void CompositeTranslator::uninstall(const TextLocalizationContainer & source)
{
	auto position = std::find_if(overlays.rbegin(), overlays.rend(), [&source](const auto & overlay){ return overlay.get() == &source; });
	if(position != overlays.rend())
		overlays.erase(std::next(position).base());
}

void CompositeTranslator::clear()
{
	overlays.clear();
}

const std::string & CompositeTranslator::translateString(const TextIdentifier & identifier) const
{
	// a translation mod stores its overrides of map and campaign strings in the static store, so
	// those have to win over the text the map carries itself - that one is in its original language
	if(!LIBRARY->generaltexth->identifierExists(identifier))
	{
		for(auto overlay = overlays.rbegin(); overlay != overlays.rend(); ++overlay)
			if((*overlay)->identifierExists(identifier))
				return (*overlay)->translateString(identifier);
	}

	return LIBRARY->generaltexth->translateString(identifier);
}

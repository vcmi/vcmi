/*
 * OverlayTranslator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "OverlayTranslator.h"

#include "../GameLibrary.h"
#include "CGeneralTextHandler.h"
#include "TextLocalizationContainer.h"

const std::string & OverlayTranslator::translateString(const TextIdentifier & identifier) const
{
	// same order as the client translator - a translation mod's override outranks the map's own text
	if(LIBRARY->generaltexth->identifierExists(identifier))
		return LIBRARY->generaltexth->translateString(identifier);

	if(overlay.identifierExists(identifier))
		return overlay.translateString(identifier);

	return LIBRARY->generaltexth->translateString(identifier);
}

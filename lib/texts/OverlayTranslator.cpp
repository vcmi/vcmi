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
	if(overlay.identifierExists(identifier))
		return overlay.translateString(identifier);

	return LIBRARY->generaltexth->translateString(identifier);
}

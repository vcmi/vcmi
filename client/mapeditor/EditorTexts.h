/*
 * EditorTexts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/texts/TextIdentifier.h"

class MetaString;

/// Text lookup of the map editor
namespace EditorTexts
{
	/// Text of the editor interface. The key is relative to "vcmi.editor."
	std::string get(const std::string & key);

	/// Same as get(), the text has positional %s placeholders that are replaced by the arguments
	std::string format(const std::string & key, const std::vector<std::string> & arguments);

	/// Text that belongs to the game or to the opened map, including the texts defined by the map
	std::string translate(const TextIdentifier & identifier);
	std::string translate(const MetaString & text);

	/// Name of the player, or of the neutral side
	std::string playerName(const PlayerColor & player);
}

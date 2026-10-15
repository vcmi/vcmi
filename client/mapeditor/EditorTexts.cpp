/*
 * EditorTexts.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorTexts.h"

#include "../GameInstance.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"

namespace EditorTexts
{

std::string get(const std::string & key)
{
	return LIBRARY->generaltexth->translate("vcmi.editor." + key);
}

std::string format(const std::string & key, const std::vector<std::string> & arguments)
{
	std::string result = get(key);

	for(const auto & argument : arguments)
	{
		const size_t placeholder = result.find("%s");
		if(placeholder == std::string::npos)
			break;

		result.replace(placeholder, 2, argument);
	}
	return result;
}

std::string translate(const TextIdentifier & identifier)
{
	return GAME->translator().translateString(identifier);
}

std::string translate(const MetaString & text)
{
	return text.toString(&GAME->translator());
}

std::string playerName(const PlayerColor & player)
{
	if(player == PlayerColor::NEUTRAL)
		return get("player.neutral");

	if(!player.isValidPlayer())
		return get("player.unflaggable");

	MetaString text;
	text.appendName(player);
	return translate(text);
}

}

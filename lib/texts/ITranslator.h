/*
 * ITranslator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "TextIdentifier.h"

/// Resolves text identifiers into human-readable text in the local player's language.
/// Implemented on the rendering side only (client, map editor) - lib and server code
/// never owns one, so they can not accidentally resolve text into a specific language.
class DLL_LINKAGE ITranslator
{
public:
	virtual ~ITranslator() = default;

	/// converts identifier into user-readable string
	virtual const std::string & translateString(const TextIdentifier & identifier) const = 0;

	template<typename ... Args>
	std::string translate(std::string arg1, Args ... args) const
	{
		return translateString(TextIdentifier(arg1, args ...));
	}
};

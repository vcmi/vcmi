/*
 * Translator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/texts/ITranslator.h"

/// Client-side resolver of text identifiers. Composes the immutable static text store
/// with the map and campaign text overlays that are installed while a game is loaded.
class Translator final : public ITranslator
{
public:
	const std::string & translateString(const TextIdentifier & identifier) const override;
};

/*
 * translator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/texts/ITranslator.h"

/// Map editor resolver of text identifiers. The editor keeps map texts in the static
/// store (see mapsettings/translations), so this only needs to forward to it.
class Translator final : public ITranslator
{
public:
	/// The editor has no game instance to own a translator, so it lives here
	static const Translator & instance();

	const std::string & translateString(const TextIdentifier & identifier) const override;
};

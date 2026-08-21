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

class TextLocalizationContainer;

/// Map editor resolver of text identifiers. Composes the static text store with the
/// text overlay of the map currently being edited.
class Translator final : public ITranslator
{
	/// Searched back to front, so a later install shadows an earlier one
	std::vector<const TextLocalizationContainer *> overlays;

public:
	/// The editor has no game instance to own a translator, so it lives here
	static Translator & instance();

	/// Installing the same source twice is a no-op, so callers need not track what is already up
	void install(const TextLocalizationContainer & source);
	void uninstall(const TextLocalizationContainer & source);

	const std::string & translateString(const TextIdentifier & identifier) const override;
};

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

class TextLocalizationContainer;

/// Client-side resolver of text identifiers. Composes the immutable static text store
/// with the map and campaign text overlays installed while a game or the picker is up.
class Translator final : public ITranslator
{
	/// Searched back to front, so a later install shadows an earlier one
	std::vector<const TextLocalizationContainer *> overlays;

public:
	/// Installing the same source twice is a no-op, so callers need not track what is already up
	void install(const TextLocalizationContainer & source);
	void uninstall(const TextLocalizationContainer & source);
	void uninstallAll();

	const std::string & translateString(const TextIdentifier & identifier) const override;
};

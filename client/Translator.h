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
	/// Prefer TranslatorOverlay - an overlay left installed past the life of its container dangles
	void install(const TextLocalizationContainer & source);
	void uninstall(const TextLocalizationContainer & source);

	const std::string & translateString(const TextIdentifier & identifier) const override;
};

/// Keeps a text container installed in the client translator for exactly as long as this object
/// lives. Whoever owns the container owns the overlay, so the two can never drift apart.
class TranslatorOverlay final
{
	const TextLocalizationContainer * source = nullptr;

public:
	TranslatorOverlay() = default;
	explicit TranslatorOverlay(const TextLocalizationContainer & source);
	TranslatorOverlay(TranslatorOverlay && other) noexcept;
	TranslatorOverlay & operator=(TranslatorOverlay && other) noexcept;
	~TranslatorOverlay();
};

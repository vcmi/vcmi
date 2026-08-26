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

class TextLocalizationContainer;

/// Keeps a text container installed in the client translator for exactly as long as this object
/// lives, sharing ownership of it in the meantime.
class TranslatorOverlay final
{
	std::shared_ptr<const TextLocalizationContainer> source;

public:
	TranslatorOverlay() = default;
	explicit TranslatorOverlay(std::shared_ptr<const TextLocalizationContainer> source);
	/// these are kept in vectors, so growing one must hand the install over instead of copying it
	TranslatorOverlay(TranslatorOverlay && other) noexcept = default;
	~TranslatorOverlay();
};

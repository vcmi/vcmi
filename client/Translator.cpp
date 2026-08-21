/*
 * Translator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Translator.h"

#include "GameInstance.h"

#include "../lib/GameLibrary.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/texts/TextLocalizationContainer.h"

void Translator::install(const TextLocalizationContainer & source)
{
	overlays.push_back(&source);
}

void Translator::uninstall(const TextLocalizationContainer & source)
{
	// only the most recent install is dropped, so overlapping installs of one container nest
	auto position = std::find(overlays.rbegin(), overlays.rend(), &source);
	if(position != overlays.rend())
		overlays.erase(std::next(position).base());
}

const std::string & Translator::translateString(const TextIdentifier & identifier) const
{
	// a translation mod stores its overrides of map and campaign strings in the static store, so
	// those have to win over the text the map carries itself - that one is in its original language
	if(LIBRARY->generaltexth->identifierExists(identifier))
		return LIBRARY->generaltexth->translateString(identifier);

	for(auto overlay = overlays.rbegin(); overlay != overlays.rend(); ++overlay)
		if((*overlay)->identifierExists(identifier))
			return (*overlay)->translateString(identifier);

	return LIBRARY->generaltexth->translateString(identifier);
}

TranslatorOverlay::TranslatorOverlay(const TextLocalizationContainer & source)
	: source(&source)
{
	GAME->translator().install(source);
}

TranslatorOverlay::TranslatorOverlay(TranslatorOverlay && other) noexcept
	: source(other.source)
{
	other.source = nullptr;
}

TranslatorOverlay & TranslatorOverlay::operator=(TranslatorOverlay && other) noexcept
{
	std::swap(source, other.source);
	return *this;
}

TranslatorOverlay::~TranslatorOverlay()
{
	// GAME is already null while its own members are being torn down - the translator is going too
	if(source && GAME)
		GAME->translator().uninstall(*source);
}

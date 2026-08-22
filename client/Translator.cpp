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

#include "../lib/texts/CompositeTranslator.h"

TranslatorOverlay::TranslatorOverlay(std::shared_ptr<const TextLocalizationContainer> source)
	: source(std::move(source))
{
	GAME->translatorInstance->install(this->source);
}

TranslatorOverlay::~TranslatorOverlay()
{
	// GAME is already null while its own members are being torn down - the translator is going too
	if(source && GAME)
		GAME->translatorInstance->uninstall(*source);
}

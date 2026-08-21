/*
 * OverlayTranslator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ITranslator.h"

class TextLocalizationContainer;

/// Resolves against one explicitly named text container, falling back to the static store.
/// Only for rendering a map's own embedded text where no player-facing translator exists -
/// local file paths and lobby metadata. Never use it to build text sent to a player: the
/// caller must name the container, but it still cannot know the recipient's language.
class DLL_LINKAGE OverlayTranslator final : public ITranslator
{
	const TextLocalizationContainer & overlay;

public:
	explicit OverlayTranslator(const TextLocalizationContainer & overlay)
		: overlay(overlay)
	{}

	const std::string & translateString(const TextIdentifier & identifier) const override;
};

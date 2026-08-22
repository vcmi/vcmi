/*
 * CompositeTranslator.h, part of VCMI engine
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

/// Resolves text identifiers against the static text store plus a stack of installed overlays.
/// Overlays hold the text a map or campaign carries itself, which is inert data until something
/// installs it - only the rendering side can do that, since only it knows the player's language.
class DLL_LINKAGE CompositeTranslator : public ITranslator
{
	/// Searched back to front, so a later install shadows an earlier one. Ownership is shared
	/// with whoever supplied the container, so an overlay can never outlive what it points at
	std::vector<std::shared_ptr<const TextLocalizationContainer>> overlays;

public:
	void install(const std::shared_ptr<const TextLocalizationContainer> & source);
	/// Drops the most recent install of this container, so overlapping installs of one nest
	void uninstall(const TextLocalizationContainer & source);
	void clear();

	const std::string & translateString(const TextIdentifier & identifier) const override;
};

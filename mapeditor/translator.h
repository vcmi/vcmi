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

#include "../lib/texts/CompositeTranslator.h"

/// Map editor resolver of text identifiers. Composes the static text store with the
/// text overlays of whatever map or campaign is being edited.
class Translator final : public CompositeTranslator
{
public:
	/// The editor has no game instance to own a translator, so it lives here
	static Translator & instance();
};

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

#include "../../../lib/texts/ITranslator.h"

namespace NK2AI
{

/// Resolves text identifiers for AI logging. The AI is not linked against the client,
/// so it can not reach the client translator and needs its own view of the text store.
/// Sees static strings only - map-specific texts render as their identifier.
class Translator final : public ITranslator
{
public:
	const std::string & translateString(const TextIdentifier & identifier) const override;
};

}

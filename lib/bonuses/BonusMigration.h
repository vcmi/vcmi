/*
 * BonusMigration.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../texts/TextIdentifier.h"

class JsonNode;
struct Bonus;

/// Conversion of combat abilities that used to be hardcoded bonus types into the
/// COMBAT_EVENT_TRIGGER bonus that runs the combat script replacing them. Keeps existing content
/// working without every mod having to be updated.
namespace BonusMigration
{
/// Rewrites a bonus config declaring such an ability into its script form. Returns false and
/// leaves 'migrated' untouched when the config needs no conversion.
DLL_LINKAGE bool migrateCombatAbility(const JsonNode & ability, JsonNode & migrated);

/// Same conversion, for a bonus restored from a save predating the ability becoming a script.
DLL_LINKAGE bool migrateCombatAbility(Bonus & bonus);

/// Logs a warning for abilities that were retired without a conversion, so that content still
/// declaring them fails loudly rather than by quietly doing nothing.
DLL_LINKAGE void warnIfRetired(const JsonNode & ability, const TextIdentifier & descriptionID);
}

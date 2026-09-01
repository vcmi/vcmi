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

/// Conversion of bonus types that no longer exist into whatever replaced them - most of them
/// combat abilities that became the COMBAT_EVENT_TRIGGER bonus running a combat script. Keeps
/// existing content working without every mod having to be updated.
namespace BonusMigration
{
/// Rewrites a bonus config declaring a retired bonus into its replacement. Returns false and
/// leaves 'migrated' untouched when the config needs no conversion.
DLL_LINKAGE bool migrateBonus(const JsonNode & ability, JsonNode & migrated);

/// Same conversion, for a bonus restored from a save predating the ability becoming a script.
DLL_LINKAGE bool migrateCombatAbility(Bonus & bonus);

/// Logs a warning for abilities that were retired without a conversion, so that content still
/// declaring them fails loudly rather than by quietly doing nothing.
DLL_LINKAGE void warnIfRetired(const JsonNode & ability, const TextIdentifier & descriptionID);
}

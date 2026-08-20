/*
 * BonusFilter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BonusCustomTypes.h"
#include "BonusSelector.h"

#include <vcmi/scripting/ApiTags.h>

/// Which bonuses of a bearer a script is asking about. Every field left out widens the answer, so
/// an empty filter asks for all of them. Anything the fields below cannot express is for the script
/// to sort out over the bonuses this returns.
struct DLL_LINKAGE BonusFilter final : public scripting::ApiSerializable<BonusFilter>
{
	static constexpr std::string_view luaName = "BonusFilter";
	static constexpr std::string_view luaDescription =
		"Which bonuses of a bearer a query is about, handed to `getBonuses`, `getBonusesValue` and "
		"`hasBonuses` as a plain table. Every field left out widens the answer, so `{}` asks for all "
		"of them.";

	/// json key of the bonus type, "SLAYER" and the like
	std::optional<std::string> type;
	/// json key of the subtype - a creature, a spell, a bonus type, ... whichever the bonus type
	/// names. Meaningless without a type, since the type is what decides what a subtype is.
	std::optional<std::string> subtype;
	std::optional<BonusSource> sourceType;
	/// Kind of blow the bonus has to count for, rather than the effect range it carries: a bonus
	/// limited to the other kind is left out, one limited to neither always counts. Asked this way
	/// because "counts in melee" is two effect ranges at once, and splitting it into two queries
	/// would add their values up instead of combining them as the engine does.
	std::optional<bool> shooting;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("type", type, "Bonus type to look for, by its json key.");
		s("subtype", subtype, "Subtype to look for, by its json key. Requires a type.");
		s("sourceType", sourceType, "Where the bonus has to come from - an artifact, a spell effect, ...");
		s("shooting", shooting, "Kind of blow the bonus has to count for - pass the `shooting` flag of the attack. Bonuses limited to the other kind are left out, those limited to neither always count.");
	}

	/// Selector matching what this filter describes, and the string the bonus system caches its
	/// answer under. The string has to tell every filter apart, since it is the cache key.
	std::pair<CSelector, std::string> compile() const;
};

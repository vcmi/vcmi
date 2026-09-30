/*
 * DamageEstimationTexts.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "DamageEstimationTexts.h"

#include "Unit.h"

#include "../GameLibrary.h"

#include <vcmi/Creature.h>

namespace
{
// the text of the token is complete before the next token is replaced, so its "%d" is the first one in the text
void replacePlural(MetaString & text, const std::string & token, DamageRange amount, const std::string & baseTextID)
{
	text.replaceTokenTextID(token, amount.max == 1 ? baseTextID + ".1" : baseTextID);

	if(amount.min == amount.max)
		text.replaceTokenNumber("%d", amount.max);
	else
		text.replaceTokenRawString("%d", std::to_string(amount.min) + " - " + std::to_string(amount.max));
}

std::string unitNameTextID(const battle::Unit & unit)
{
	const auto * creature = unit.creatureId().toEntity(LIBRARY);
	return unit.getCount() == 1 ? creature->getNameSingularTextID() : creature->getNamePluralTextID();
}

MetaString attack(const DamageEstimation & estimation, const std::string & baseTextID, const std::string & targetNameTextID, int shotsLeft)
{
	MetaString text = MetaString::createFromTextID(baseTextID);
	text.replaceTokenTextID("%CREATURE", targetNameTextID);
	replacePlural(text, "%DAMAGE", estimation.damage, "vcmi.battleWindow.damageEstimation.damage");
	replacePlural(text, "%SHOTS", {shotsLeft, shotsLeft}, "vcmi.battleWindow.damageEstimation.shots");
	replacePlural(text, "%KILLS", estimation.kills, "vcmi.battleWindow.damageEstimation.kills");
	return text;
}
}

MetaString DamageEstimationTexts::plural(DamageRange amount, const std::string & baseTextID)
{
	MetaString text = MetaString::createFromRawString("%AMOUNT");
	replacePlural(text, "%AMOUNT", amount, baseTextID);
	return text;
}

MetaString DamageEstimationTexts::meleeAttack(const DamageEstimation & estimation, const battle::Unit & target)
{
	const std::string baseTextID = estimation.kills.max == 0 ? "vcmi.battleWindow.damageEstimation.melee" : "vcmi.battleWindow.damageEstimation.meleeKills";
	return attack(estimation, baseTextID, unitNameTextID(target), 0);
}

MetaString DamageEstimationTexts::rangedAttack(const DamageEstimation & estimation, const std::string & targetNameTextID, int shotsLeft)
{
	const std::string baseTextID = estimation.kills.max == 0 ? "vcmi.battleWindow.damageEstimation.ranged" : "vcmi.battleWindow.damageEstimation.rangedKills";
	return attack(estimation, baseTextID, targetNameTextID, shotsLeft);
}

MetaString DamageEstimationTexts::rangedAttack(const DamageEstimation & estimation, const battle::Unit & target, int shotsLeft)
{
	return rangedAttack(estimation, unitNameTextID(target), shotsLeft);
}

MetaString DamageEstimationTexts::retaliation(const DamageEstimation & estimation, bool mayBeKilled)
{
	if(estimation.damage.max == 0)
		return MetaString::createFromTextID("vcmi.battleWindow.damageRetaliation.never");

	MetaString text = MetaString::createFromTextID(mayBeKilled ? "vcmi.battleWindow.damageRetaliation.may" : "vcmi.battleWindow.damageRetaliation.will");
	text.appendTextID(estimation.kills.max == 0 ? "vcmi.battleWindow.damageRetaliation.damage" : "vcmi.battleWindow.damageRetaliation.damageKills");
	replacePlural(text, "%DAMAGE", estimation.damage, "vcmi.battleWindow.damageEstimation.damage");
	replacePlural(text, "%KILLS", estimation.kills, "vcmi.battleWindow.damageEstimation.kills");
	return text;
}

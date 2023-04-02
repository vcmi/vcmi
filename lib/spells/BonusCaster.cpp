/*
 * BonusCaster.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BonusCaster.h"

#include <vcmi/spells/Spell.h>

#include "../NetPacksBase.h"
#include "../HeroBonus.h"
#include "../battle/Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

BonusCaster::BonusCaster(const Caster * actualCaster_, std::shared_ptr<Bonus> bonus_):
	ProxyCaster(actualCaster_),
	bonus(std::move(bonus_))
{
}

BonusCaster::~BonusCaster() = default;

void BonusCaster::getCasterName(MetaString & text) const
{
	if(!bonus->description.empty())
		text.addReplacement(bonus->description);
	else
		actualCaster->getCasterName(text);
}

void BonusCaster::getCastDescription(const Spell * spell, const std::vector<const battle::Unit*> & attacked, MetaString & text) const
{
	const bool singleTarget = attacked.size() == 1;
	const int textIndex = singleTarget ? 195 : 196;

	text.addTxt(MetaString::GENERAL_TXT, textIndex);
	getCasterName(text);
	text.addReplacement(MetaString::SPELL_NAME, spell->getIndex());
	if(singleTarget)
		attacked.at(0)->addNameReplacement(text, true);
}

void BonusCaster::spendMana(ServerCallback * server, const int spellCost) const
{
	logGlobal->error("Unexpected call to BonusCaster::spendMana");
}


} // namespace spells

VCMI_LIB_NAMESPACE_END

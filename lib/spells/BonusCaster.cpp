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
#include <vcmi/HeroType.h>
#include <vcmi/Artifact.h>

#include "../battle/Unit.h"
#include "../bonuses/Bonus.h"
#include "../GameLibrary.h"
#include "../CSkillHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

BonusCaster::BonusCaster(const Caster * actualCaster_, std::shared_ptr<Bonus> bonus_):
	ProxyCaster(actualCaster_),
	bonus(std::move(bonus_))
{
}

BonusCaster::~BonusCaster() = default;

std::string BonusCaster::getCasterNameTextID() const
{
	switch(bonus->source)
	{
		case BonusSource::ARTIFACT:
			return bonus->sid.as<ArtifactID>().toEntity(LIBRARY)->getNameTextID();
		case BonusSource::SPELL_EFFECT:
			return bonus->sid.as<SpellID>().toEntity(LIBRARY)->getNameTextID();
		case BonusSource::CREATURE_ABILITY:
			return bonus->sid.as<CreatureID>().toEntity(LIBRARY)->getNamePluralTextID();
		case BonusSource::SECONDARY_SKILL:
			return bonus->sid.as<SecondarySkill>().toEntity(LIBRARY)->getNameTextID();
		case BonusSource::HERO_SPECIAL:
			return bonus->sid.as<HeroTypeID>().toEntity(LIBRARY)->getNameTextID();
		default:
			return actualCaster->getCasterNameTextID();
	}
}

void BonusCaster::getCastDescription(const Spell * spell, const battle::Units & attacked, MetaString & text) const
{
	const bool singleTarget = attacked.size() == 1;
	const int textIndex = singleTarget ? 195 : 196;

	text.appendLocalString(EMetaText::GENERAL_TXT, textIndex);
	text.replaceTextID(getCasterNameTextID());
	text.replaceName(spell->getId());
	if(singleTarget)
		attacked.at(0)->addNameReplacement(text, true);
}

void BonusCaster::spendMana(ServerCallback * server, const int spellCost) const
{
	logGlobal->error("Unexpected call to BonusCaster::spendMana");
}


} // namespace spells

VCMI_LIB_NAMESPACE_END

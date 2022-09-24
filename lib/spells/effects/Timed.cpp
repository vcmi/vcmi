/*
 * Timed.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Timed.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::string EFFECT_NAME = "core:timed";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Timed, EFFECT_NAME);

Timed::Timed()
	: UnitEffect(),
	cumulative(false),
	bonus()
{
}

Timed::~Timed() = default;

void Timed::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	const bool describe = server->describeChanges();
	int32_t duration = m->getEffectDuration();

	std::vector<Bonus> converted;
	convertBonus(m, duration, converted);

	std::shared_ptr<const Bonus> peculiarBonus = nullptr;
	std::shared_ptr<const Bonus> addedValueBonus = nullptr;
	std::shared_ptr<const Bonus> fixedValueBonus = nullptr; 

	auto casterHero = dynamic_cast<const CGHeroInstance *>(m->caster);
	if(casterHero)
	{ 
		peculiarBonus = casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_PECULIAR_ENCHANT, m->getSpellIndex()));
		addedValueBonus = casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_ADD_VALUE_ENCHANT, m->getSpellIndex()));
		fixedValueBonus = casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_FIXED_VALUE_ENCHANT, m->getSpellIndex()));
	}	
	//TODO: does hero specialty should affects his stack casting spells?

	SetStackEffect sse;
	BattleLogMessage blm;

	for(auto & t : target)
	{
		std::vector<Bonus> buffer;
		std::copy(converted.begin(), converted.end(), std::back_inserter(buffer));

		const battle::Unit * affected = t.unitValue;
		if(!affected)
		{
			logGlobal->error("[Internal error] Invalid target for timed effect");
			continue;
		}

		if(!affected->alive())
			continue;

		if(describe)
			describeEffect(blm.lines, m, converted, affected);


		//Apply hero specials - peculiar enchants
		const auto tier = std::max(affected->creatureLevel(), 1); //don't divide by 0 for certain creatures (commanders, war machines)
		if(peculiarBonus)
		{
			si32 power = 0;
			switch (peculiarBonus->additionalInfo[0])
			{
			case 0: //normal
				switch (tier)
				{
				case 1:
				case 2:
					power = 3;
					break;
				case 3:
				case 4:
					power = 2;
					break;
				case 5:
				case 6:
					power = 1;
					break;
				}
				break;
			case 1: 
				//Coronius style specialty bonus.
				//Please note that actual Coronius isnt here, because Slayer is a spell that doesnt affect monster stats and is used only in calculateDmgRange
				power = std::max(5 - tier, 0);
				break;
			}
			if(m->isNegativeSpell())
			{
				//negative spells like weakness are defined in json with negative numbers, so we need do same here
				power = -1 * power;
			}
			for(Bonus& b : buffer)
			{
				b.val += power;
			}

		}

		if(addedValueBonus)
		{
			for(Bonus & b : buffer)
			{
				b.val += addedValueBonus->additionalInfo[0];
			}
		}
		if(fixedValueBonus)
		{
			for(Bonus & b : buffer)
			{
				b.val = fixedValueBonus->additionalInfo[0];
			}
		}

		if(casterHero && casterHero->hasBonusOfType(Bonus::SPECIAL_BLESS_DAMAGE, m->getSpellIndex())) //TODO: better handling of bonus percentages
		{
			int damagePercent = casterHero->valOfBonuses(Bonus::SPECIAL_BLESS_DAMAGE, m->getSpellIndex()) / tier;
			Bonus specialBonus(Bonus::N_TURNS, Bonus::CREATURE_DAMAGE, Bonus::SPELL_EFFECT, damagePercent, m->getSpellIndex(), 0, Bonus::PERCENT_TO_ALL);
			specialBonus.turnsRemain = duration;
			buffer.push_back(specialBonus);
		}

		if(cumulative)
			sse.toAdd.push_back(std::make_pair(affected->unitId(), buffer));
		else
			sse.toUpdate.push_back(std::make_pair(affected->unitId(), buffer));
	}

	if(!(sse.toAdd.empty() && sse.toUpdate.empty()))
		server->apply(&sse);

	if(describe && !blm.lines.empty())
		server->apply(&blm);
}

void Timed::convertBonus(const Mechanics * m, int32_t & duration, std::vector<Bonus> & converted) const
{
	int32_t maxDuration = 0;

	for(const std::shared_ptr<Bonus> & b : bonus)
	{
		Bonus nb(*b);

		//use configured duration if present
		if(nb.turnsRemain == 0)
			nb.turnsRemain = duration;
		vstd::amax(maxDuration, nb.turnsRemain);

		nb.sid = m->getSpellIndex(); //for all
		nb.source = Bonus::SPELL_EFFECT;//for all

		//fix to original config: shield should display damage reduction
		if((nb.sid == SpellID::SHIELD || nb.sid == SpellID::AIR_SHIELD) && (nb.type == Bonus::GENERAL_DAMAGE_REDUCTION))
			nb.val = 100 - nb.val;
		//we need to know who cast Bind
		else if(nb.sid == SpellID::BIND && nb.type == Bonus::BIND_EFFECT && m->caster->getCasterUnitId() >= 0)
			nb.additionalInfo = m->caster->getCasterUnitId();

		converted.push_back(nb);
	}

	//if all spell effects have special duration, use it later for special bonuses
	duration = maxDuration;
}

void Timed::describeEffect(std::vector<MetaString> & log, const Mechanics * m, const std::vector<Bonus> & bonuses, const battle::Unit * target) const
{
	auto addLogLine = [&](const int32_t baseTextID, const boost::logic::tribool & plural)
	{
		MetaString line;
		target->addText(line, MetaString::GENERAL_TXT, baseTextID, plural);
		target->addNameReplacement(line, plural);
		log.push_back(std::move(line));
	};

	if(m->getSpellIndex() == SpellID::DISEASE)
	{
		addLogLine(553, boost::logic::indeterminate);
		return;
	}

	for(const auto & bonus : bonuses)
	{
		switch(bonus.type)
		{
		case Bonus::NOT_ACTIVE:
			{
				switch(bonus.subtype)
				{
				case SpellID::STONE_GAZE:
					addLogLine(558, boost::logic::indeterminate);
					return;
				case SpellID::PARALYZE:
					addLogLine(563, boost::logic::indeterminate);
					return;
				default:
					break;
				}
			}
			break;
		case Bonus::POISON:
			addLogLine(561, boost::logic::indeterminate);
			return;
		case Bonus::BIND_EFFECT:
			addLogLine(-560, true);
			return;
		case Bonus::STACK_HEALTH:
			{
				if(bonus.val < 0)
				{
					BonusList unitHealth = *target->getBonuses(Selector::type()(Bonus::STACK_HEALTH));

					auto oldHealth = unitHealth.totalValue();
					unitHealth.push_back(std::make_shared<Bonus>(bonus));
					auto newHealth = unitHealth.totalValue();

					//"The %s shrivel with age, and lose %d hit points."
					MetaString line;
					target->addText(line, MetaString::GENERAL_TXT, 551);
					target->addNameReplacement(line);

					line.addReplacement(oldHealth - newHealth);
					log.push_back(std::move(line));
					return;
				}
			}
			break;
		default:
			break;
		}
	}
}

void Timed::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	assert(!handler.saving);
	handler.serializeBool("cumulative", cumulative, false);
	{
		auto guard = handler.enterStruct("bonus");
		const JsonNode & data = handler.getCurrent();

		for(const auto & p : data.Struct())
		{
			//TODO: support JsonSerializeFormat in Bonus
			auto guard = handler.enterStruct(p.first);
			const JsonNode & bonusNode = handler.getCurrent();
			auto b = JsonUtils::parseBonus(bonusNode);
			bonus.push_back(b);
		}
	}
}

}
}

VCMI_LIB_NAMESPACE_END

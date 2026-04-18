/*
 * BattleBonusEffectProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "BattleBonusEffectsProcessor.h"

#include <CStack.h>

#include <CGameHandler.h>
#include <battle/IBattleInfoCallback.h>
#include <battle/IBattleState.h>
#include <bonuses/BonusEffects.h>
#include <bonuses/BonusParameters.h>
#include <networkPacks/SetStackEffect.h>
#include <spells/AbilityCaster.h>
#include <spells/CSpellHandler.h>
#include <spells/ISpellMechanics.h>

void BattleBonusEffectsProcessor::processBattleEventTriggers(
	const CBattleInfoCallback & battle,
	CGameHandler * gameHandler,
	CombatEventType event,
	const CStack * target,
	const CStack * secondary
)
{
	const auto & bonuses = target->getAllBonuses(Selector::all);
	for(auto & bonus : *bonuses)
	{
		TBattleEffects battleEffects = bonus->triggerBattleEffects(event);
		for(auto & effect : battleEffects)
		{
			auto * bonusEffect = std::get_if<BBGrantBonus>(&effect);
			auto * spellEffect = std::get_if<BBCastSpell>(&effect);
			auto * terminateBonus = std::get_if<BBTerminate>(&effect);
			auto * changeBonusDuration = std::get_if<BBChangeDuration>(&effect);

			if(bonusEffect)
			{
				SetStackEffect sse;
				sse.battleID = battle.getBattle()->getBattleID();
				std::vector<Bonus> bonuses{*bonusEffect->bonus};
				if(bonusEffect->targetEnemy && secondary)
					sse.toAdd.emplace_back(secondary->unitId(), bonuses);
				if(!bonusEffect->targetEnemy)
					sse.toAdd.emplace_back(target->unitId(), bonuses);
				gameHandler->sendAndApply(sse);
			}
			if(spellEffect)
			{
				const CSpell * spell = spellEffect->spell.toSpell();
				spells::AbilityCaster spellCaster(target, spellEffect->masteryLevel);

				spells::Target spellTarget;
				if(spellEffect->targetEnemy && secondary)
					spellTarget.emplace_back(secondary);
				if(!spellEffect->targetEnemy)
					spellTarget.emplace_back(target);

				spells::BattleCast parameters(&battle, &spellCaster, spells::Mode::PASSIVE, spell);

				auto m = spell->battleMechanics(&parameters);

				if(m->canBeCastAt(spellTarget))
					parameters.cast(gameHandler->spellcastEnvironment(), spellTarget);
			}
			if(terminateBonus)
			{
				SetStackEffect sse;
				sse.battleID = battle.getBattle()->getBattleID();
				std::vector<Bonus> buffer;
				buffer.push_back(*bonus);
				sse.toRemove.emplace_back(target->unitId(), buffer);
				gameHandler->sendAndApply(sse);
			}
			if(changeBonusDuration) // The only way I found for UI registering bonus duration being changed is to remove it and apply anew
			{
				bonus->turnsRemain += changeBonusDuration->value;
				SetStackEffect rs;
				rs.battleID = battle.getBattle()->getBattleID();
				std::vector<Bonus> buffer;
				buffer.push_back(*bonus);
				rs.toRemove.emplace_back(target->unitId(), buffer);
				gameHandler->sendAndApply(rs);
				if(bonus->turnsRemain > 0)
				{
					SetStackEffect readd;
					readd.battleID = battle.getBattle()->getBattleID();
					readd.toAdd.emplace_back(target->unitId(), buffer);
					gameHandler->sendAndApply(readd);
				}
			}
		}
	}
}

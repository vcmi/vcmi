# SpellCastProblem

Error codes returned from spell-cast validation.

| Key | Value | Description |
| --- | ----- | ----------- |
| `noHeroToCastSpell` | 1 | There is no hero available to cast the spell. |
| `castsPerTurnLimit` | 2 | The caster has already used their per-turn cast allowance. |
| `noSpellbook` | 3 | The caster does not carry a spellbook. |
| `heroDoesntKnowSpell` | 4 | The spell is not present in the caster's spellbook. |
| `notEnoughMana` | 5 | The caster does not have enough mana for this spell. |
| `advmapSpellInsteadOfBattleSpell` | 6 | An adventure-map spell was attempted in battle context. |
| `spellLevelLimitExceeded` | 7 | The caster's wisdom is too low for the spell's level. |
| `noSpellsToDispel` | 8 | Dispel was cast but there are no eligible spells to remove. |
| `noAppropriateTarget` | 9 | No legal target was selected for the spell. |
| `stackImmuneToSpell` | 10 | The chosen target unit is immune to this spell. |
| `wrongSpellTarget` | 11 | The selected target is not valid for this spell's AimType. |
| `ongoinTacticPhase` | 12 | Spells may not be cast during the tactic phase. |
| `magicIsBlocked` | 13 | Magic is suppressed on this battlefield (anti-magic terrain or effect). |

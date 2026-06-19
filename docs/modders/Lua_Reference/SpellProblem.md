# SpellProblem

Output accumulator passed to a spell's `canBeCast` / `canBeCastAt` check. Scripts append reasons the cast cannot proceed — generic, standard (picked from the ESpellCastProblem enum), or fully custom via MetaString. An empty SpellProblem means the cast is allowed.

### addCustom

Adds a custom-message problem entry built from the given MetaString config.

- param `config`: [`MetaString`](MetaString.md) — MetaString that describes the custom problem message.

### addGeneric

Adds the generic 'cannot cast' problem entry derived from the given mechanics.

- param `mechanics`: [`SpellMechanics`](SpellMechanics.md) — Mechanics of the spell being cast.

### addStandard

Adds a standard problem entry with the requested SpellCastProblem value.

- param `mechanics`: [`SpellMechanics`](SpellMechanics.md) — Mechanics of the spell being cast.
- param `spellProblem`: [`SpellCastProblem`](SpellCastProblem.md) — Standard problem code to surface.

# Bonus Propagators

Propagators allow to propagate bonus effect "upwards". For example, they can be used to make unit ability battle-wide, or to provide some bonuses to hero. See [Bonus System Guide](../Guides/Bonus_System.md) for more information.

## Available propagators

- `ARMY`: Propagators that allow bonuses to be transferred to an army. It is typically used by creature abilities to affect the army that the creature is part of.
- `HERO`: Similar to `ARMY`, but works only with armies led by heroes.
- `TOWN`: Similar to `ARMY`, but only affects units that are part of the town garrison.
- `TOWN_AND_VISITOR`: Propagator that allows the town and the visiting hero to interact. It can be used to propagate the effects of town buildings to the visiting hero outside of combat or the effects of the hero to the town (e.g. Legion artifacts)
- `BATTLE_WIDE` - Propagator that allows bonuses to affect all entities in battles. It is typically used for creature abilities or artifacts that need to affect either both sides or only the enemy side in combat.
- `PLAYER`: The bonus affects all objects owned by the player. Used by the Statue of Legion.
- `TEAM`: The bonus affects all objects owned by the player and their allies.
- `GLOBAL_EFFECT`: This effect influences all creatures, towns and recruited heroes on the map.

## Deprecated propagators

These propagators are still supported, but in future they may be removed.

- `VISITED_TOWN_AND_VISITOR`: Replaced by `TOWN_AND_VISITOR`
- `PLAYER_PROPAGATOR`: Replaced by `PLAYER`
- `TEAM_PROPAGATOR`: Replaced by `TEAM`

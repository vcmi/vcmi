# BonusDuration

Lifetime selectors used by Bonus / BonusDescriptor `duration`.

| Key | Value | Description |
| --- | ----- | ----------- |
| `permanent` | 1 | Lasts forever (until explicitly removed). |
| `oneBattle` | 2 | Expires at the end of the current battle. |
| `oneDay` | 4 | Expires after end of current in-game day. |
| `oneWeek` | 8 | Expires after end of current in-game week. |
| `nTurns` | 16 | Expires after N battle turns. |
| `nDays` | 32 | Expires after N in-game days. |
| `untilBeingAttacked` | 64 | Expires the first time the bearer is attacked. |
| `untilAttack` | 128 | Expires the first time the bearer makes any attack. |
| `stackGetsTurn` | 256 | Expires when the bearer's unit gets to act. |
| `commanderKilled` | 512 | Tied to a commander — expires when the commander dies. |
| `untilOwnAttack` | 1024 | Expires when the bearer initiates an attack (not counter-attack). |
| `untilTakingIndirectDamage` | 2048 | Expires when the bearer takes spell or environmental damage. |
| `untilAfterAttackSequence` | 4096 | Expires after the current attack-and-counter sequence resolves. |

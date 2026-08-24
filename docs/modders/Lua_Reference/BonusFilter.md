# BonusFilter

Which bonuses of a bearer a query is about, handed to `getBonuses`, `getBonusesValue` and `hasBonuses` as a plain table. Every field left out widens the answer, so `{}` asks for all of them.

### type

Bonus type to look for, by its json key.

- type: `string?`

### subtype

Subtype to look for, by its json key. Requires a type.

- type: `string?`

### sourceType

Where the bonus has to come from - an artifact, a spell effect, ...

- type: [`BonusSource?`](BonusSource.md)

### shooting

Kind of blow the bonus has to count for - pass the `shooting` flag of the attack. Bonuses limited to the other kind are left out, those limited to neither always count.

- type: `boolean?`

# BonusValueType

Combination rules used by Bonus / BonusDescriptor `valueType`.

| Key | Value | Description |
| --- | ----- | ----------- |
| `additiveValue` | 0 | Adds to the base value, but after `percent to base`. |
| `baseNumber` | 1 | Adds the base value before any additive/percent steps. |
| `percentToAll` | 2 | Percentage applied to the running total of all sources combined. |
| `percentToBase` | 3 | Percentage applied to the base value only. |
| `percentToSource` | 4 | Percentage applied to the same-source subtotal. |
| `percentToTargetType` | 5 | Percentage applied only to bonuses from a matching targetSourceType. |
| `independentMax` | 6 | Lower bound, AT_LEAST in json — total value is at least the highest of such bonuses. |
| `independentMin` | 7 | Upper bound, AT_MOST in json — total value is at most the lowest of such bonuses. |

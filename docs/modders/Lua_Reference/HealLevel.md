# HealLevel

Outcome categories for the heal action: plain heal / resurrect / overheal.

| Key | Value | Description |
| --- | ----- | ----------- |
| `heal` | 0 | Standard healing — capped at the stack's current size, does not revive dead. |
| `resurrect` | 1 | Brings dead creatures in the stack back, up to the starting count. |
| `overheal` | 2 | Allows increasing size of stack past the starting count. |

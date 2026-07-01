# WallPart

Town-wall sections referenced by siege APIs and `catapultAttack`.

| Key | Value | Description |
| --- | ----- | ----------- |
| `invalid` | -1 | No wall part. Returned for hexes outside the fortifications. |
| `indestructiblePart` | -2 | Sections of walls that always exist and always block ranged attacks |
| `indestructibleGate` | -3 | Section of gate that always exists, but does not blocks ranged attacks |
| `keep` | 0 | Central keep building, usually built by Citadel. |
| `bottomTower` | 1 | Bottom tower of the town walls, usually built by Castle. |
| `bottomWall` | 2 | Destructible wall segment adjacent to bottom tower. |
| `belowGate` | 3 | Destructible wall segment immediately below the gatehouse. |
| `overGate` | 4 | Destructible wall segment immediately above the gatehouse. |
| `upperWall` | 5 | Destructible wall segment adjacent to the upper tower. |
| `upperTower` | 6 | Upper tower of the town walls, usually built by Castle. |
| `gate` | 7 | Destructible town gate that can only be opened by defenders. |

< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Bonus Format](../Bonus_Format.md) / Bonus Range Types

# List of all Bonus range types

-   NO_LIMIT
-   ONLY_DISTANCE_FIGHT
-   ONLY_MELEE_FIGHT
-   ONLY_ENEMY_ARMY (before 1.2)

TODO: in 0.91 ONLY_MELEE_FIGHT / ONLY_DISTANCE_FIGHT range types work
ONLY with creature attack, should be extended to all battle abilities.

(1.2+) For replacing ONLY_ENEMY_ARMY alias, you should use the following
parameters of bonus:

    	"propagator": "BATTLE_WIDE",
    	"propagationUpdater" : "BONUS_OWNER_UPDATER",
    	"limiters" : [ "OPPOSITE_SIDE" ]

If some propagators was set before, it was actually ignored and should
be replaced to code above. And OPPOSITE_SIDE limiter should be first, if
any other limiters exists.
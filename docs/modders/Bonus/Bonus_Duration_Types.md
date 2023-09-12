< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Bonus Format](../Bonus_Format.md) / Bonus Duration Types


Bonus may have any of these durations. They acts in disjunction.

## List of all bonus duration types

-   PERMANENT
-   ONE_BATTLE //at the end of battle
-   ONE_DAY //at the end of day
-   ONE_WEEK //at the end of week (bonus lasts till the end of week,
    thats NOT 7 days
-   N_TURNS //used during battles, after battle bonus is always removed
-   N_DAYS
-   UNTIL_BEING_ATTACKED //removed after any damage-inflicting attack
-   UNTIL_ATTACK //removed after attack and counterattacks are performed
-   STACK_GETS_TURN //removed when stack gets its turn - used for
    defensive stance
-   COMMANDER_KILLED
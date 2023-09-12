< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Bonus Format](../Bonus_Format.md) / Bonus Value Types

Total value of Bonus is calculated using the following:

-   For each bonus source type we calculate new source value (for all
    bonus value types except PERCENT_TO_SOURCE and
    PERCENT_TO_TARGET_TYPE) using the following:

` newVal = (val * (100 + PERCENT_TO_SOURCE) / 100))`

PERCENT_TO_TARGET_TYPE applies as PERCENT_TO_SOURCE to targetSourceType
of bonus.

-   All bonus value types summarized and then used as subject of the
    following formula:

` clamp(((BASE_NUMBER * (100 + PERCENT_TO_BASE) / 100) + ADDITIVE_VALUE) * (100 + PERCENT_TO_ALL) / 100), INDEPENDENT_MAX, INDEPENDENT_MIN)`

As for 1.2, semantics of INDEPENDENT_MAX and INDEPENDENT_MIN are
wrapped, and first means than bonus total value will be at least
INDEPENDENT_MAX, and second means than bonus value will be at most
INDEPENDENT_MIN.

# List of all bonus value types

-   ADDITIVE_VALUE
-   BASE_NUMBER
-   PERCENT_TO_ALL
-   PERCENT_TO_BASE
-   INDEPENDENT_MAX
-   INDEPENDENT_MIN
-   PERCENT_TO_SOURCE (since 1.2)
-   PERCENT_TO_TARGET_TYPE (since 1.2)
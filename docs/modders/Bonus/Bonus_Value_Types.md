# Bonus Value Types

Total value of Bonus is calculated using the following:

- For each bonus source type we calculate new source value (for all bonus value types except PERCENT_TO_SOURCE and PERCENT_TO_TARGET_TYPE) using the following:

```text
newVal = (val * (100 + PERCENT_TO_SOURCE) / 100))
```

- PERCENT_TO_TARGET_TYPE applies as PERCENT_TO_SOURCE to targetSourceType of bonus. If bonus also has targetSourceID, only bonuses granted by that specific source object are affected.

- All bonus value types summarized and then used as subject of the following formula:

```text
clamp(((BASE_NUMBER * (100 + PERCENT_TO_BASE) / 100) + ADDITIVE_VALUE) * (100 + PERCENT_TO_ALL) / 100), AT_LEAST, AT_MOST)
```

AT_LEAST means that total value will be at least the highest of such bonuses. AT_MOST means that total value will be at most the lowest of such bonuses. If these bounds conflict, AT_MOST takes priority.

INDEPENDENT_MAX and INDEPENDENT_MIN are older names of AT_LEAST and AT_MOST respectively. They are still supported, but new content should use AT_LEAST and AT_MOST instead.

If there are no other bonuses to clamp:

- with only AT_LEAST or only AT_MOST bonuses, total value is equal to that bound
- with both AT_LEAST and AT_MOST bonuses, total value is the middle of the range between them, rounded towards AT_LEAST

## List of all bonus value types

- ADDITIVE_VALUE
- BASE_NUMBER
- PERCENT_TO_ALL
- PERCENT_TO_BASE
- AT_LEAST (older name: INDEPENDENT_MAX, should be avoided)
- AT_MOST (older name: INDEPENDENT_MIN, should be avoided)
- PERCENT_TO_SOURCE
- PERCENT_TO_TARGET_TYPE

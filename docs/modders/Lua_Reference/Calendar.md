# Calendar

Interprets an in-game day count using the map's calendar settings (days per week, weeks per month). Obtained from Game:getCalendar().

### getCurrentDay

Returns the current day count.

- returns `integer` — Total number of days since the start of the game (1..).

### getDayOfWeek

Returns the day within the current week.

- returns `integer` — Day within the current week (1..days-per-week).

### getDayOfMonth

Returns the day within the current month.

- returns `integer` — Day within the current month (1..days-per-month).

### getWeek

Returns the week within the current month.

- returns `integer` — Week within the current month (1..weeks-per-month).

### getMonth

Returns the current month.

- returns `integer` — Current month (1..).

### getDaysInWeek

Returns the number of days in a week.

- returns `integer` — Configured number of days per week.

### getDaysInMonth

Returns the number of days in a month.

- returns `integer` — Configured number of days per month.

### getWeeksInMonth

Returns the number of weeks in a month.

- returns `integer` — Configured number of weeks per month.
